#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
source ./sdk.command
TEMP="$(mktemp -d)"
trap 'rm -rf "$TEMP"' EXIT HUP INT TERM
# Deliberate whitespace splitting supports e.g. SNOW_TEST_FLAGS='-fsanitize=undefined -fno-sanitize-recover=all'.
read -r -a EXTRA_FLAGS <<< "${SNOW_TEST_FLAGS:-}"
for source in Tests/source_motion_tests.cpp Tests/monitor_tests.cpp Tests/geometry_tests.cpp Tests/core_tests.cpp Tests/cascade_tests.cpp Tests/tractor_cycle_tests.cpp Tests/tractor_primary_regression.cpp Tests/shedding_tests.cpp Tests/emission_tests.cpp Tests/motion_tests.cpp Tests/avalanche_tests.cpp Tests/animation_tests.cpp Tests/tractor_motion_tests.cpp; do
  name="$(basename "$source" .cpp)"
  "$CXX" -std=c++17 -O2 -Wall -Wextra -Werror -pedantic ${SNOW_SDK_FLAGS[@]+"${SNOW_SDK_FLAGS[@]}"} ${EXTRA_FLAGS[@]+"${EXTRA_FLAGS[@]}"} "$source" -o "$TEMP/$name"
  "$TEMP/$name"
  if [[ "$name" == cascade_tests ]]; then SNOW_ROUNDED=1 "$TEMP/$name"; fi
done
