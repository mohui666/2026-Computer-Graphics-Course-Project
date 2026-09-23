#!/bin/sh
set -eu
cd "$(dirname "$0")"
python3 -m venv .venv-build
.venv-build/bin/python -m pip install -r requirements-build.txt
cmake -S . -B build-macos -DCMAKE_BUILD_TYPE=Release \
  -DPython_EXECUTABLE="$PWD/.venv-build/bin/python" "$@"
cmake --build build-macos --config Release --parallel 2
ctest --test-dir build-macos -C Release --output-on-failure
