#pragma once
#include <Arduino.h>
#include "../storage/button_config_store.h"

// Feedback for the HA_CONTROL toggle. The displayed state still only
// changes when HA's statestream confirms it (never optimistically);
// this tracks the one toggle in flight so the screen can show that it
// was sent, and what went wrong if it didn't take.
//
// Kept by entity id rather than on HaEntity itself, since a config
// update replaces ha_entity_list wholesale.

enum class HaPendingStatus : uint8_t {
  NONE,
  WAITING,    // sent, no state change seen yet
  TIMED_OUT,  // sent, but no state change within HA_PENDING_TIMEOUT_MS
  NOT_SENT,   // MQTT was down, or the publish failed
};

constexpr unsigned long HA_PENDING_TIMEOUT_MS = 10UL * 1000;
// How long a TIMED_OUT/NOT_SENT note stays up before clearing itself.
constexpr unsigned long HA_PENDING_NOTE_MS = 30UL * 1000;

// Records a toggle just published (`sent`) or dropped (!sent) for
// `entity`, remembering its state at the time of the press.
void ha_pending_start(const HaEntity &entity, bool sent);

// Status for `entity` (NONE unless it's the one in flight). Clears the
// pending toggle once the entity's state differs from what it was at
// the press. Main thread only.
HaPendingStatus ha_pending_status(const HaEntity &entity);

// Call every loop(). Times WAITING out and clears expired notes;
// returns true when the status changed this way (HA_CONTROL should
// redraw).
bool ha_pending_tick();
