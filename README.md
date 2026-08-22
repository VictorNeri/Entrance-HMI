# Entrance HMI

Custom firmware for a home entrance display built on the Elecrow CrowPanel ESP32-S3 5.79" e-paper HMI panel: current weather, nearest SL (Stockholm) transit departures, and configurable Home Assistant control buttons via MQTT.

The active project lives in [`entrance_hmi/`](entrance_hmi/).

## `vendor-reference/`

Everything under [`vendor-reference/`](vendor-reference/) is the original, unmodified vendor package this repo was forked from (Elecrow's factory firmware, schematics, datasheets, and Arduino examples). It's kept only as a lookup reference during development — `entrance_hmi/` takes its own copy of the pieces it needs (see `entrance_hmi/src/epd_driver/`) and does not depend on this directory at build time. `vendor-reference/` can be deleted entirely once it's no longer needed.

## Hardware

- ESP32-S3-WROOM-1-N8R8, 8MB Flash, 8MB PSRAM
- 5.79" black/white e-paper, dual SSD1683 driver, 792×272 visible resolution
- 5 physical buttons (HOME/EXIT/PRV/NEXT/OK), no touch input

## Build

Board: `esp32:esp32:esp32s3`. **Set these board options before compiling/flashing** — the ESP32 Arduino core's default board options target a 4MB-flash variant, not this board's real 8MB flash + 8MB PSRAM, and building against the default 1.2MB APP partition runs out of headroom well before this firmware is done growing:

- Flash Size: `8MB (64Mb)`
- Partition Scheme: `8M with spiffs (3MB APP/1.5MB SPIFFS)`
- PSRAM: `QSPI PSRAM` (Enabled)

Arduino IDE: set these in Tools menu after selecting the board. `arduino-cli`:

```bash
arduino-cli compile --fqbn "esp32:esp32:esp32s3:FlashSize=8M,PartitionScheme=default_8MB,PSRAM=enabled" entrance_hmi
```

## Configuration

Two layers, deliberately split by how often each one changes:

**`entrance_hmi/config.h`** (gitignored, compile-time — reflash to change): OpenWeatherMap API key/coordinates, MQTT broker/credentials, NTP servers/timezone. Copy `config.example.h` to `config.h` and fill in real values.

**SD card `/config.json`** (edit on any computer, no reflash needed): WiFi credentials, the SL station ID, and poll/rotation intervals — the settings you're most likely to want to tune in the field.

```json
{
  "wifi_ssid": "your-wifi-ssid",
  "wifi_password": "your-wifi-password",
  "sl_site_id": "9001",
  "weather_poll_interval_sec": 600,
  "transit_poll_active_sec": 90,
  "transit_poll_background_sec": 300,
  "screen_rotation_interval_sec": 300
}
```

- `sl_site_id`: your nearest SL stop's site ID — look it up via `https://transport.integration.sl.se/v1/sites` (search the JSON by stop name) or trafiklab.se's stop lookup. `9001` is T-Centralen, a placeholder.
- `weather_poll_interval_sec` / `transit_poll_active_sec` / `transit_poll_background_sec`: how often the WEATHER screen refetches, and how often TRANSIT refetches while visible vs. in the background.
- `screen_rotation_interval_sec`: how often HOME/WEATHER/TRANSIT auto-advance when idle, and also how long a button press pauses auto-rotation before it resumes (one interval serves both).

All fields are optional except `wifi_ssid`/`wifi_password`/`sl_site_id` — omitted intervals fall back to the defaults shown above. If the SD card is missing, unreadable, or the file is invalid JSON, the device still boots (interval defaults apply, WiFi simply stays disconnected until a valid card is inserted and it's rebooted) — check serial output for the specific reason.

SD card wiring (separate SPI bus from the e-paper panel, no pin conflicts):

| Signal | GPIO |
|---|---|
| SD SCK / MISO / MOSI / CS | 39 / 13 / 40 / 10 |
| SD card power enable | 42 |
