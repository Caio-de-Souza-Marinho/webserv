#!/bin/bash
# valgrind_multi.sh — Valgrind leak check under realistic multi-request load

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SUPP_FILE="$SCRIPT_DIR/webserv.supp"
LOGFILE="/tmp/webserv_valgrind_multi.log"

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

echo "Firing test requests..."
curl -s http://localhost:8080/index.html     > /dev/null
curl -s http://localhost:8080/about.html     > /dev/null
curl -s http://localhost:8080/does_not_exist > /dev/null
curl -s http://localhost:8080/               > /dev/null

LOC=$(curl -s -D - \
    -X POST \
    -H "Content-Type: application/octet-stream" \
    -H "Content-Disposition: attachment; filename=\"valgrind_test.dat\"" \
    --data-binary "valgrind payload" \
    http://localhost:8080/uploads \
    | grep -i "^location:" | tr -d '\r' | awk '{print $2}')
[ -n "$LOC" ] && curl -s -X DELETE "http://localhost:8080/${LOC#/}" > /dev/null

curl -s http://localhost:8080/cgi-bin/test.py > /dev/null
curl -s http://localhost:8080/uploads/        > /dev/null
curl -s -X DELETE http://localhost:8080/readonly/ > /dev/null

echo "Shutting down..."
kill -TERM "$VPID" 2>/dev/null || true
wait "$VPID"; VCODE=$?

echo ""
echo "=== Valgrind multi-request leak summary ==="
grep -E "(HEAP SUMMARY|in use at exit|All heap blocks|definitely lost|indirectly lost|ERROR SUMMARY)" "$LOGFILE" || true
echo ""

PASS=0; FAIL=0

# Pass if zero leaks: either "All heap blocks were freed" OR "definitely lost: 0 bytes"
if grep -qE "All heap blocks were freed|definitely lost: 0 bytes" "$LOGFILE"; then
    echo "  PASS  No definite leaks"
    PASS=$((PASS+1))
else
    echo "  FAIL  Definite leaks detected"
    FAIL=$((FAIL+1))
    echo "  --- Leak detail ---"
    grep -A8 "are definitely lost" "$LOGFILE" | head -40 || true
fi

if [ "$VCODE" -eq 0 ]; then
    echo "  PASS  No Valgrind errors"
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
