#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$(dirname -- "$SCRIPT_DIR")

if [ -n "${EMSDK_DIR:-}" ]; then
    if [ ! -f "$EMSDK_DIR/emsdk_env.sh" ]; then
        echo "emsdk_env.sh not found: $EMSDK_DIR" >&2
        exit 1
    fi
    . "$EMSDK_DIR/emsdk_env.sh" >/dev/null
elif ! command -v emcmake >/dev/null 2>&1; then
    echo "emcmake not found. Activate emsdk or set EMSDK_DIR." >&2
    exit 1
fi

emcmake cmake -S "$PROJECT_DIR" -B "$PROJECT_DIR/build-wasm" \
    -DCJ4_WEB_BUILD_TESTS=OFF
cmake --build "$PROJECT_DIR/build-wasm" --parallel

echo "Built browser files: $PROJECT_DIR/build-wasm/site"
