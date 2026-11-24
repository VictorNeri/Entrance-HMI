#!/usr/bin/env bash
# Compiles the firmware with the board options from the README and
# stamps the build with its git revision and build time (see
# entrance_hmi/src/app/fw_version.h).
#
#   scripts/build.sh [output-dir]      # default: build/
#
# Extra arduino-cli options can go in ARDUINO_CLI_FLAGS, e.g.
# ARDUINO_CLI_FLAGS="--warnings default".
set -euo pipefail

repo=$(cd "$(dirname "$0")/.." && pwd)
out=${1:-"$repo/build"}
# FQBN can be overridden from the environment (CI sets the same value).
fqbn=${FQBN:-"esp32:esp32:esp32s3:FlashSize=8M,PartitionScheme=default_8MB,PSRAM=enabled"}

rev=$(git -C "$repo" describe --always --dirty --abbrev=7 2>/dev/null || echo unknown)
built=$(date -u +%Y-%m-%dT%H:%MZ)
# Both end up as bare preprocessor tokens; anything outside this set
# would break the build or the stringified value.
[[ $rev =~ ^[0-9A-Za-z.-]+$ ]] || rev=unknown

# shellcheck disable=SC2086  # ARDUINO_CLI_FLAGS is deliberately word-split
arduino-cli compile --fqbn "$fqbn" ${ARDUINO_CLI_FLAGS:-} \
  --build-property "compiler.cpp.extra_flags=-DFW_GIT_REV=$rev -DFW_BUILD_TIME=$built" \
  --output-dir "$out" "$repo/entrance_hmi"

echo "firmware $rev built $built -> $out/entrance_hmi.ino.bin"
