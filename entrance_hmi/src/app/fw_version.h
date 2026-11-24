#pragma once

// Which build is running: the git commit it was built from (with
// "-dirty" if the tree had uncommitted changes) and when it was built.
// Shown on STATUS, in the net-log heartbeat, and sent to Home
// Assistant, so "is the panel on the latest firmware?" has an answer
// without guessing from log formats.
//
// scripts/build.sh (and CI) pass both in as -D flags. They go in as
// bare tokens and get stringified here, because arduino-cli strips the
// quotes from a -DNAME="value" build property. So the values must be
// plain tokens with no spaces: a git short hash and an ISO timestamp
// both qualify. A build without the flags (Arduino IDE, or arduino-cli
// called directly) still compiles and reports "dev" plus the compile
// date of this translation unit.

#define FW_STR_(x) #x
#define FW_STR(x) FW_STR_(x)

#ifdef FW_GIT_REV
#define FW_VERSION FW_STR(FW_GIT_REV)
#else
#define FW_VERSION "dev"
#endif

#ifdef FW_BUILD_TIME
#define FW_BUILT FW_STR(FW_BUILD_TIME)
#else
#define FW_BUILT __DATE__ " " __TIME__
#endif
