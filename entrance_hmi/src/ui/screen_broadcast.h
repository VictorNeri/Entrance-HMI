#pragma once

// Full-canvas modal overlay for an unacknowledged MQTT broadcast
// message, drawn on top of whatever screen/chrome ui_common already
// painted this frame — see ../storage/broadcast_store.h for how a
// message arrives and gets acknowledged (OK button), and README.md
// for the MQTT topic/payload schema.
void screen_broadcast_render_overlay();
