#include "fetch_worker.h"
#include <atomic>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "../storage/sd_config.h"
#include "http_client_helper.h"
#include "transit_client.h"
#include "weather_client.h"
#include "weather_forecast_client.h"
#include "wifi_manager.h"
#include "net_log_shadow.h"

namespace {

// TLS handshakes + ArduinoJson parsing ran fine in the 8KB Arduino
// loop task before; 12KB leaves margin now that it's a separate task.
constexpr uint32_t TASK_STACK_BYTES = 12288;
constexpr UBaseType_t TASK_PRIORITY = 1;
constexpr BaseType_t TASK_CORE = 0;
// How long the worker sleeps between schedule checks when nothing
// wakes it early (a request notifies it immediately).
constexpr TickType_t IDLE_WAIT = pdMS_TO_TICKS(500);


// Rate-limit backoff: after an HTTP 429, that endpoint waits at least
// this long before its next request (scheduled or user-requested),
// doubling on each consecutive 429 up to the cap, and clearing on the
// next success. Starts above the fastest normal poll (TRANSIT's 90s
// default active interval) so the first 429 actually slows things down.
constexpr unsigned long RATE_LIMIT_BACKOFF_MIN_MS = 2UL * 60 * 1000;
constexpr unsigned long RATE_LIMIT_BACKOFF_MAX_MS = 16UL * 60 * 1000;

TaskHandle_t task_handle = nullptr;
SemaphoreHandle_t staged_lock = nullptr;

std::atomic<bool> transit_active{false};
std::atomic<bool> transit_paused{false};
std::atomic<bool> want_weather{false};
std::atomic<bool> want_transit{false};

// --- Staged results: guarded by staged_lock ---
WeatherData staged_weather;
bool staged_weather_ready = false;
bool staged_weather_forced = false;
ForecastData staged_forecast;
bool staged_forecast_ready = false;
DepartureList staged_transit;
bool staged_transit_ready = false;

// --- Worker-task-only schedule state ---
struct Schedule {
  const char *name;
  bool fetched_once = false;  // fetch on the first chance after boot, not one interval later
  unsigned long last_ms = 0;
  unsigned long backoff_ms = 0;  // 0 = not rate-limited

  explicit Schedule(const char *n) : name(n) {}

  bool backing_off(unsigned long now) const {
    return backoff_ms > 0 && now - last_ms < backoff_ms;
  }
  // `forced` (a user request) skips the normal interval, but never a
  // rate-limit backoff.
  bool should_fetch(unsigned long interval_ms, unsigned long now, bool forced) const {
    if (backing_off(now)) return false;
    return forced || !fetched_once || now - last_ms >= interval_ms;
  }
  void mark(unsigned long now) {
    fetched_once = true;
    last_ms = now;
  }
  void record_result(bool ok, int http_status) {
    if (http_status == HTTP_STATUS_TOO_MANY_REQUESTS) {
      backoff_ms = backoff_ms == 0 ? RATE_LIMIT_BACKOFF_MIN_MS : backoff_ms * 2;
      if (backoff_ms > RATE_LIMIT_BACKOFF_MAX_MS) backoff_ms = RATE_LIMIT_BACKOFF_MAX_MS;
      Serial.printf("[fetch] %s rate-limited (429), backing off %lus\n", name, backoff_ms / 1000);
    } else if (ok) {
      backoff_ms = 0;
    }
  }
};

Schedule weather_schedule("weather");
Schedule forecast_schedule("forecast");
Schedule transit_schedule("transit");

void run_due_fetches() {
  bool forced_weather = want_weather.exchange(false);
  bool forced_transit = want_transit.exchange(false);
  unsigned long now = millis();

  if (weather_schedule.should_fetch(sd_config.weather_poll_interval_ms, now, forced_weather)) {
    weather_schedule.mark(now);
    WeatherData next;
    int status = 0;
    bool ok = weather_client_fetch(next, &status);
    weather_schedule.record_result(ok, status);
    if (ok) {
      xSemaphoreTake(staged_lock, portMAX_DELAY);
      staged_weather = next;
      staged_weather_ready = true;
      staged_weather_forced = staged_weather_forced || forced_weather;
      xSemaphoreGive(staged_lock);
    }
    esp_task_wdt_reset();
  }

  now = millis();
  if (forecast_schedule.should_fetch(FORECAST_POLL_INTERVAL_MS, now, forced_weather)) {
    forecast_schedule.mark(now);
    ForecastData next;
    int status = 0;
    bool ok = weather_forecast_client_fetch(next, &status);
    forecast_schedule.record_result(ok, status);
    if (ok) {
      xSemaphoreTake(staged_lock, portMAX_DELAY);
      staged_forecast = next;
      staged_forecast_ready = true;
      xSemaphoreGive(staged_lock);
    }
    esp_task_wdt_reset();
  }

  now = millis();
  unsigned long transit_interval = transit_active.load() ? sd_config.transit_poll_active_ms
                                                         : sd_config.transit_poll_background_ms;
  bool transit_due = forced_transit || !transit_paused.load();
  if (transit_due && transit_schedule.should_fetch(transit_interval, now, forced_transit)) {
    transit_schedule.mark(now);
    DepartureList next;
    int status = 0;
    bool ok = transit_client_fetch(next, &status);
    transit_schedule.record_result(ok, status);
    if (ok) {
      xSemaphoreTake(staged_lock, portMAX_DELAY);
      staged_transit = next;
      staged_transit_ready = true;
      xSemaphoreGive(staged_lock);
    }
    esp_task_wdt_reset();
  }
}

void worker_main(void *) {
  // Subscribed separately from the loop task: a TLS stack or socket
  // that wedges in here reboots the device just like a hung loop().
  esp_task_wdt_add(nullptr);

  for (;;) {
    esp_task_wdt_reset();
    // Requests stay pending while WiFi is down (not consumed), so an
    // OK press during an outage still takes effect once it's back.
    if (wifi_is_connected()) {
      run_due_fetches();
    }
    ulTaskNotifyTake(pdTRUE, IDLE_WAIT);
  }
}

void wake_worker() {
  if (task_handle != nullptr) xTaskNotifyGive(task_handle);
}

}  // namespace

void fetch_worker_begin() {
  if (task_handle != nullptr) return;
  staged_lock = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(worker_main, "fetch", TASK_STACK_BYTES, nullptr, TASK_PRIORITY,
                          &task_handle, TASK_CORE);
}

void fetch_worker_set_transit_active(bool active) {
  transit_active.store(active);
}

void fetch_worker_request_weather() {
  want_weather.store(true);
  wake_worker();
}

void fetch_worker_set_transit_paused(bool paused) {
  transit_paused.store(paused);
}

void fetch_worker_request_transit() {
  want_transit.store(true);
  wake_worker();
}

FetchUpdates fetch_worker_collect() {
  FetchUpdates updates;
  if (staged_lock == nullptr) return updates;

  xSemaphoreTake(staged_lock, portMAX_DELAY);
  if (staged_weather_ready) {
    updates.weather = staged_weather_forced || weather_data_differs(staged_weather, weather_data);
    weather_data = staged_weather;
    staged_weather_ready = false;
    staged_weather_forced = false;
  }
  if (staged_forecast_ready) {
    forecast_data = staged_forecast;
    staged_forecast_ready = false;
    updates.forecast = true;
  }
  if (staged_transit_ready) {
    departure_list = staged_transit;
    staged_transit_ready = false;
    updates.transit = true;
  }
  xSemaphoreGive(staged_lock);
  return updates;
}
