#!/usr/bin/env bash
set -x
set -e
# shellcheck source-path=SCRIPTDIR

# shellcheck source=./common.sh
. ./common.sh

./stop.sh

./gradlew assembleDebug

./check-for-symbols.sh

./setup-tunnel.sh

./gradlew installDebug

adb logcat -c
adb shell am start-activity -S -n "$PKG/$ACTIVITY"
adb logcat | grep -E "$LOGCAT_GREP_PATTERN"
