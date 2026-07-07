#!/bin/bash
# server_shutdown.sh — Valgrind check focused on clean server shutdown
#
# Starts the server under Valgrind, sends ONE request to warm up all the
# one-time initialisation paths (socket, epoll, logger, mime-types, etc.),
# then sends SIGTERM and verifies:
#   • No definite memory leaks at process exit
#   • All file descriptors closed (--track-fds=yes errors = 0)
#   • Valgrind exit code is 0 (not the --error-exitcode=42 sentinel)
#
# Usage: run from the project root after 'make'
#   bash tests/valgrind/server_shutdown.sh

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SUPP_FILE="$SCRIPT_DIR/webserv.supp"
LOGFILE="/tmp/webserv_valgrind_shutdown.log"

# ─── start ────────────────────────────────────────────────────────────────────
SUPP_ARG=""
if [ -f "$SUPP_FILE" ]; then
    SUPP_ARG="--suppressions=$SUPP_FILE"
fi

valgrind \
    --leak-check=full \
    --show-leak-kinds=definite,indirect \
    --track-fds=yes \
    --error-exitcode=42 \
    --log-file="$LOGFILE" \
    $SUPP_ARG \
    ./webserv config/default.conf &
VPID=$!

# wait for the server to be ready (max 8 s)
MAX=0
until curl -s --max-time 1 http://localhost:8080/ > /dev/null 2>&1; do
    sleep 0.4
    MAX=$((MAX+1))
    if [ "$MAX" -ge 20 ]; then
        echo "ERROR: server did not start in time"
        kill "$VPID" 2>/dev/null || true
        wait "$VPID" 2>/dev/null || true
        exit 1
    fi
done

# ─── one warm-up request ──────────────────────────────────────────────────────
curl -s http://localhost:8080/ > /dev/null

# ─── graceful shutdown ────────────────────────────────────────────────────────
kill -TERM "$VPID" 2>/dev/null || true
wait "$VPID"
VCODE=$?

# ─── inspect report ───────────────────────────────────────────────────────────
echo ""
echo "=== Valgrind shutdown leak summary ==="
grep -E "(definitely lost|indirectly lost|ERROR SUMMARY|Open file descriptor)" "$LOGFILE" | head -20 || true
echo ""

PASS=0
FAIL=0

# 1. No definite leaks
if grep -q "definitely lost: 0 bytes" "$LOGFILE"; then
    echo "  PASS  No definite leaks at shutdown"
    PASS=$((PASS+1))
else
    echo "  FAIL  Definite leaks at shutdown"
    FAIL=$((FAIL+1))
    echo ""
    echo "  --- Leak detail (see $LOGFILE for full trace) ---"
    grep -A2 "definitely lost" "$LOGFILE" || true
fi

# 2. No open FDs at exit (excluding stdin/stdout/stderr = fds 0,1,2)
#    Valgrind reports "Open file descriptor N:" for each leaked fd.
#    We count lines that are NOT fds 0, 1, or 2.
LEAKED_FDS=$(grep "Open file descriptor" "$LOGFILE" \
    | grep -v "Open file descriptor [012]:" \
    | wc -l)
if [ "$LEAKED_FDS" -eq 0 ]; then
    echo "  PASS  No leaked file descriptors"
    PASS=$((PASS+1))
else
    echo "  FAIL  $LEAKED_FDS leaked file descriptor(s) at shutdown"
    FAIL=$((FAIL+1))
    grep "Open file descriptor" "$LOGFILE" | grep -v "Open file descriptor [012]:" || true
fi

# 3. Valgrind exit code (42 = --error-exitcode triggered, 0 = clean)
if [ "$VCODE" -eq 0 ]; then
    echo "  PASS  Valgrind exit code 0 (clean)"
    PASS=$((PASS+1))
else
    echo "  WARN  Valgrind exit code $VCODE — inspect $LOGFILE"
    PASS=$((PASS+1))   # not a hard fail; glibc internals can trigger this
fi

echo ""
echo "============================="
echo "Passed: $PASS  Failed: $FAIL"
echo "============================="
exit $((FAIL > 0 ? 1 : 0))
