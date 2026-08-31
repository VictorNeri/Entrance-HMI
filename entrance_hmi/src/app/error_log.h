#pragma once
#include <Arduino.h>

// Tiny last-error recorder, fixed-size (no heap churn) — feeds the
// STATUS screen's error line. Purely a state store: callers keep doing
// their own Serial logging at the actual failure site; this just
// remembers the most recent one for on-screen display.
void error_log_set(const char *message);

// "" if nothing has been recorded yet.
const char *error_log_get();

// millis() timestamp of the last error_log_set() call, 0 if none yet.
unsigned long error_log_get_ms();
