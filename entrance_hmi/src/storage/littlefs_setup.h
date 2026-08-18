#pragma once

// Mounts LittleFS, formatting on first boot if no filesystem exists
// yet. Returns false only if formatting itself fails (storage is
// effectively unusable) — the HA button config cache is then simply
// unavailable until MQTT delivers a fresh config.
bool littlefs_setup_begin();
