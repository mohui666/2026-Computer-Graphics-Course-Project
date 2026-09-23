#!/bin/sh
set -eu
cd "$(dirname "$0")"
exec ./build-macos/bin/Release/MineLongwallSimulation.app/Contents/MacOS/MineLongwallSimulation "$@"
