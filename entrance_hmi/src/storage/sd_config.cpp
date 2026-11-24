#include "sd_config.h"
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include "../net/net_log_shadow.h"

SdConfig sd_config;

namespace {

constexpr uint8_t SD_POWER_PIN = 42;
constexpr uint8_t SD_SCK = 39;
constexpr uint8_t SD_MISO = 13;
constexpr uint8_t SD_MOSI = 40;
constexpr uint8_t SD_CS = 10;
constexpr uint32_t SD_SPI_HZ = 80000000;
constexpr const char *CONFIG_PATH = "/config.json";
constexpr const char *MQTT_CA_PATH = "/mqtt_ca.pem";
// A broker CA (or a short chain) is a few KB; anything far larger isn't
// a certificate file and would only waste heap.
constexpr size_t MQTT_CA_MAX_BYTES = 8192;

void load_mqtt_ca() {
  File f = SD.open(MQTT_CA_PATH, FILE_READ);
  if (!f) {
    Serial.println("[sd-config] mqtt_tls is on but /mqtt_ca.pem not found, MQTT stays off");
    return;
  }
  size_t size = f.size();
  if (size == 0 || size > MQTT_CA_MAX_BYTES) {
    Serial.printf("[sd-config] /mqtt_ca.pem has bad size (%u bytes), MQTT stays off\n", (unsigned)size);
    f.close();
    return;
  }
  String pem = f.readString();
  f.close();
  if (pem.indexOf("-----BEGIN CERTIFICATE-----") < 0) {
    Serial.println("[sd-config] /mqtt_ca.pem is not a PEM certificate, MQTT stays off");
    return;
  }
  sd_config.mqtt_ca_pem = pem;
}

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
  sd_config.mqtt_tls = doc["mqtt_tls"] | false;
  sd_config.mqtt_port = (int)(doc["mqtt_port"] | (sd_config.mqtt_tls ? 8883 : 1883));
  sd_config.mqtt_username = (const char *)(doc["mqtt_username"] | "");
  sd_config.mqtt_password = (const char *)(doc["mqtt_password"] | "");
  sd_config.mqtt_topic_prefix = (const char *)(doc["mqtt_topic_prefix"] | "entrance-hmi");
  sd_config.ha_discovery = doc["ha_discovery"] | true;
  sd_config.ha_discovery_prefix = (const char *)(doc["ha_discovery_prefix"] | "homeassistant");
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
      static_cast<unsigned long>((int)(doc["screen_rotation_interval_sec"] | 12)) * 1000UL;
  int night_start = doc["night_start_hour"] | 23;
  int night_end = doc["night_end_hour"] | 6;
  sd_config.night_start_hour = (night_start >= 0 && night_start <= 23) ? night_start : 23;
  sd_config.night_end_hour = (night_end >= 0 && night_end <= 23) ? night_end : 6;
  if (sd_config.mqtt_tls) load_mqtt_ca();
  sd_config.loaded_from_sd = true;

  Serial.printf("[sd-config] loaded: ssid=%s site=%s\n", sd_config.wifi_ssid.c_str(),
                sd_config.sl_site_id.c_str());
}
