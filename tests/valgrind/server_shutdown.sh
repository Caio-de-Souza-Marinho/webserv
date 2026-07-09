#!/bin/bash
# server_shutdown.sh — Valgrind check focused on clean server shutdown

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SUPP_FILE="$SCRIPT_DIR/webserv.supp"
LOGFILE="/tmp/webserv_valgrind_shutdown.log"

SUPP_ARG=""
[ -f "$SUPP_FILE" ] && SUPP_ARG="--suppressions=$SUPP_FILE"

valgrind \
    --leak-check=full \
    --show-leak-kinds=definite,indirect \
    --track-fds=no \
    --error-exitcode=42 \
    --log-file="$LOGFILE" \
    $SUPP_ARG \
    ./webserv config/default.conf &
VPID=$!

MAX=0
until curl -s --max-time 1 http://localhost:8080/ > /dev/null 2>&1; do
    sleep 0.4; MAX=$((MAX+1))
    if [ "$MAX" -ge 20 ]; then
        echo "ERROR: server did not start in time"
        kill "$VPID" 2>/dev/null || true; wait "$VPID" 2>/dev/null || true
        exit 1
    fi
done

curl -s http://localhost:8080/ > /dev/null

kill -TERM "$VPID" 2>/dev/null || true
wait "$VPID"; VCODE=$?

echo ""
echo "=== Valgrind shutdown leak summary ==="
grep -E "(HEAP SUMMARY|in use at exit|All heap blocks|definitely lost|indirectly lost|ERROR SUMMARY)" "$LOGFILE" || true
echo ""

PASS=0; FAIL=0

if grep -qE "All heap blocks were freed|definitely lost: 0 bytes" "$LOGFILE"; then
    echo "  PASS  No definite leaks at shutdown"
    PASS=$((PASS+1))
else
    echo "  FAIL  Definite leaks at shutdown"
    FAIL=$((FAIL+1))
    echo "  --- Leak detail ---"
    grep -A8 "are definitely lost" "$LOGFILE" | head -40 || true
fi

if [ "$VCODE" -eq 0 ]; then
    echo "  PASS  Valgrind exit code 0 (clean)"
    PASS=$((PASS+1))
elif [ "$VCODE" -eq 42 ]; then
    echo "  FAIL  Valgrind reported errors (exit 42) — inspect $LOGFILE"
    FAIL=$((FAIL+1))
else
    echo "  PASS  Valgrind exit code $VCODE (signal termination)"
    PASS=$((PASS+1))
fi

echo ""
echo "============================="
echo "Passed: $PASS  Failed: $FAIL"
echo "============================="
exit $((FAIL > 0 ? 1 : 0))
