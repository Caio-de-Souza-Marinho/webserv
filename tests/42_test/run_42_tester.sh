#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"

echo "[42 TEST] ROOT_DIR = $ROOT_DIR"

"$ROOT_DIR/tests/42_test/setup_42_environment.sh"

echo "[42 TEST] Checking executable..."

if [ ! -f "$ROOT_DIR/webserv" ]; then
    echo "[42 TEST] ERROR: $ROOT_DIR/webserv does not exist"
    exit 1
fi

if [ ! -x "$ROOT_DIR/webserv" ]; then
    echo "[42 TEST] ERROR: $ROOT_DIR/webserv is not executable"
    ls -l "$ROOT_DIR/webserv"
    exit 1
fi

echo "[42 TEST] Starting webserv..."

"$ROOT_DIR/webserv" "$ROOT_DIR/config/42.conf" &
WEBSERV_PID=$!

sleep 2

if ! kill -0 "$WEBSERV_PID" 2>/dev/null; then
    echo "[42 TEST] ERROR: webserv exited immediately"
    wait "$WEBSERV_PID" || true
    exit 1
fi

cleanup()
{
    echo "[42 TEST] Stopping webserv..."

    if kill -0 "$WEBSERV_PID" 2>/dev/null; then
        kill "$WEBSERV_PID" 2>/dev/null || true
        wait "$WEBSERV_PID" 2>/dev/null || true
    fi
}

trap cleanup EXIT

echo "[42 TEST] Launching tester..."

(
    sleep 1; echo
    sleep 1; echo
    sleep 1; echo
    sleep 1; echo
    sleep 1; echo
) | "$ROOT_DIR/tests/42_test/tester" "http://localhost:8080"

echo "[42 TEST] Tester finished"
