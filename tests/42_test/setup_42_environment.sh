#!/bin/bash

set -e

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"

echo "[42 TEST] Creating YoupiBanane structure..."

rm -rf "$ROOT_DIR/YoupiBanane"

mkdir -p "$ROOT_DIR/YoupiBanane/nop"
mkdir -p "$ROOT_DIR/YoupiBanane/Yeah"

touch "$ROOT_DIR/YoupiBanane/youpi.bad_extension"
touch "$ROOT_DIR/YoupiBanane/youpi.bla"

touch "$ROOT_DIR/YoupiBanane/nop/youpi.bad_extension"
touch "$ROOT_DIR/YoupiBanane/nop/other.pouic"

touch "$ROOT_DIR/YoupiBanane/Yeah/not_happy.bad_extension"

echo "[42 TEST] Done"
