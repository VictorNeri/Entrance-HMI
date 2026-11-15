#include "ha_pending.h"

namespace {

String pending_id;  // empty = nothing in flight
HaPendingStatus status = HaPendingStatus::NONE;
bool from_known = false;
bool from_on = false;
unsigned long status_since_ms = 0;

void clear() {
  pending_id = "";
  status = HaPendingStatus::NONE;
}

const HaEntity *find_pending_entity() {
  for (uint8_t i = 0; i < ha_entity_list.count; i++) {
    if (ha_entity_list.items[i].id == pending_id) return &ha_entity_list.items[i];
  }
  return nullptr;
}

// True once HA has reported a state that differs from the one at the
// press (or reported one at all, if it was unknown then).
bool confirmed(const HaEntity &entity) {
  if (!entity.state_known) return false;
  return !from_known || entity.state_on != from_on;
}

}  // namespace

void ha_pending_start(const HaEntity &entity, bool sent) {
  pending_id = entity.id;
  status = sent ? HaPendingStatus::WAITING : HaPendingStatus::NOT_SENT;
  from_known = entity.state_known;
  from_on = entity.state_on;
  status_since_ms = millis();
}

HaPendingStatus ha_pending_status(const HaEntity &entity) {
  if (status == HaPendingStatus::NONE || entity.id != pending_id) return HaPendingStatus::NONE;
  if (status != HaPendingStatus::NOT_SENT && confirmed(entity)) {
    clear();  // a late confirmation also clears a TIMED_OUT note
    return HaPendingStatus::NONE;
  }
  return status;
}

bool ha_pending_tick() {
  if (status == HaPendingStatus::NONE) return false;

  const HaEntity *entity = find_pending_entity();
  if (entity == nullptr) {  // config update dropped it
    clear();
    return true;
  }
  if (ha_pending_status(*entity) == HaPendingStatus::NONE) return false;  // confirmed; that state message already redraws

  unsigned long elapsed = millis() - status_since_ms;
  if (status == HaPendingStatus::WAITING && elapsed >= HA_PENDING_TIMEOUT_MS) {
    status = HaPendingStatus::TIMED_OUT;
    status_since_ms = millis();
    Serial.printf("[ha] toggle %s: no state change from HA\n", pending_id.c_str());
    return true;
  }
  if (status != HaPendingStatus::WAITING && elapsed >= HA_PENDING_NOTE_MS) {
    clear();
    return true;
  }
  return false;
}
