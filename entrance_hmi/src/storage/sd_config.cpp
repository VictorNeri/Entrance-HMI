#include "sd_config.h"
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>

SdConfig sd_config;

namespace {

constexpr uint8_t SD_POWER_PIN = 42;
constexpr uint8_t SD_SCK = 39;
constexpr uint8_t SD_MISO = 13;
constexpr uint8_t SD_MOSI = 40;
constexpr uint8_t SD_CS = 10;
constexpr uint32_t SD_SPI_HZ = 80000000;
constexpr const char *CONFIG_PATH = "/config.json";

SPIClass sd_spi(HSPI);

}  // namespace

void sd_config_load() {
  pinMode(SD_POWER_PIN, OUTPUT);
  digitalWrite(SD_POWER_PIN, HIGH);

  sd_spi.begin(SD_SCK, SD_MISO, SD_MOSI);
  if (!SD.begin(SD_CS, sd_spi, SD_SPI_HZ)) {
    Serial.println("[sd-config] card mount failed, using defaults (no WiFi credentials)");
    return;
  }

  File f = SD.open(CONFIG_PATH, FILE_READ);
  if (!f) {
    Serial.println("[sd-config] /config.json not found, using defaults (no WiFi credentials)");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err != DeserializationError::Ok) {
    Serial.println("[sd-config] /config.json invalid, using defaults (no WiFi credentials)");
    return;
  }

  sd_config.wifi_ssid = (const char *)(doc["wifi_ssid"] | "");
  sd_config.wifi_password = (const char *)(doc["wifi_password"] | "");
  sd_config.sl_site_id = (const char *)(doc["sl_site_id"] | "");
  sd_config.mqtt_host = (const char *)(doc["mqtt_host"] | "");
  sd_config.mqtt_port = (int)(doc["mqtt_port"] | 1883);
  sd_config.mqtt_username = (const char *)(doc["mqtt_username"] | "");
  sd_config.mqtt_password = (const char *)(doc["mqtt_password"] | "");
  sd_config.mqtt_topic_prefix = (const char *)(doc["mqtt_topic_prefix"] | "entrance-hmi");
  sd_config.owm_api_key = (const char *)(doc["owm_api_key"] | "");
  sd_config.owm_lat = (const char *)(doc["owm_lat"] | "");
  sd_config.owm_lon = (const char *)(doc["owm_lon"] | "");
  sd_config.walk_time_min = (unsigned long)(int)(doc["walk_time_min"] | 0);
  sd_config.weather_poll_interval_ms =
      static_cast<unsigned long>((int)(doc["weather_poll_interval_sec"] | 600)) * 1000UL;
  sd_config.transit_poll_active_ms =
      static_cast<unsigned long>((int)(doc["transit_poll_active_sec"] | 90)) * 1000UL;
  sd_config.transit_poll_background_ms =
      static_cast<unsigned long>((int)(doc["transit_poll_background_sec"] | 300)) * 1000UL;
  sd_config.screen_rotation_interval_ms =
      static_cast<unsigned long>((int)(doc["screen_rotation_interval_sec"] | 300)) * 1000UL;
  sd_config.loaded_from_sd = true;

  Serial.printf("[sd-config] loaded: ssid=%s site=%s\n", sd_config.wifi_ssid.c_str(),
                sd_config.sl_site_id.c_str());
}
