# Entrance HMI

Custom firmware for a home entrance display built on the Elecrow CrowPanel ESP32-S3 5.79" e-paper HMI panel: current weather, nearest SL (Stockholm) transit departures, and configurable Home Assistant control buttons via MQTT.

The active project lives in [`entrance_hmi/`](entrance_hmi/).

## `vendor-reference/`

Everything under [`vendor-reference/`](vendor-reference/) is the original, unmodified vendor package this repo was forked from (Elecrow's factory firmware, schematics, datasheets, and Arduino examples). It's kept only as a lookup reference during development — `entrance_hmi/` takes its own copy of the pieces it needs (see `entrance_hmi/src/epd_driver/`) and does not depend on this directory at build time. `vendor-reference/` can be deleted entirely once it's no longer needed.

## Hardware

- ESP32-S3-WROOM-1-N8R8, 8MB Flash, 8MB PSRAM
- 5.79" black/white e-paper, dual SSD1683 driver, 792×272 visible resolution
- 5 physical buttons (HOME/EXIT/PRV/NEXT/OK), no touch input
