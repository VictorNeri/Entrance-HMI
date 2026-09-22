#pragma once
#include "net_log.h"

// Include this (never net_log.h directly) as the LAST #include in a
// file that calls Serial.begin/println/printf, to also mirror that
// file's output over the net_log TCP bridge — see net_log.h for why.
// Must come last: the #define below is a straight text substitution
// for every remaining `Serial` in this translation unit, so anything
// included after this point that references the real Serial by name
// would be redirected too.
#define Serial net_log
