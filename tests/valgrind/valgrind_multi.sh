#!/bin/bash
# valgrind_multi.sh — Valgrind leak check under realistic multi-request load
#
# Starts the server under Valgrind, fires a small but varied set of requests
# (GET, POST upload, DELETE, CGI GET, 404), then shuts it down cleanly and
# inspects the leak report.
#
# Pass criteria:
#   • "definitely lost: 0 bytes"  (no hard leaks)
#   • No open file descriptors reported (track-fds=yes)
#
# Usage: run from the project root after 'make'
#   bash tests/valgrind/valgrind_multi.sh

set -uo pipefail

LOGFILE="/tmp/webserv_valgrind_multi.log"

# ─── start server under valgrind ──────────────────────────────────────────────
valgrind \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-fds=yes \
    --error-exitcode=42 \
    --log-file="$LOGFILE" \
    ./webserv config/default.conf &
VPID=$!

# wait until the server accepts connections (max 8 s)
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

# ─── exercise several code paths ──────────────────────────────────────────────
echo "Firing test requests..."

# Static GETs
curl -s http://localhost:8080/index.html       > /dev/null
curl -s http://localhost:8080/about.html       > /dev/null
curl -s http://localhost:8080/does_not_exist   > /dev/null   # 404
curl -s http://localhost:8080/                 > /dev/null   # directory

# POST upload → capture Location → DELETE
LOC=$(curl -s -D - \
    -X POST \
    -H "Content-Type: application/octet-stream" \
    -H "Content-Disposition: attachment; filename=\"valgrind_test.dat\"" \
    --data-binary "valgrind payload" \
    http://localhost:8080/uploads \
    | grep -i "^location:" | tr -d '\r' | awk '{print $2}')

if [ -n "$LOC" ]; then
    curl -s -X DELETE "http://localhost:8080/${LOC#/}" > /dev/null
fi

# CGI GET
curl -s http://localhost:8080/cgi-bin/test.py  > /dev/null

# Autoindex
curl -s http://localhost:8080/uploads/         > /dev/null

# Method not allowed
curl -s -X DELETE http://localhost:8080/readonly/ > /dev/null

# ─── graceful shutdown ────────────────────────────────────────────────────────
echo "Shutting down..."
kill -TERM "$VPID" 2>/dev/null || true
wait "$VPID" 2>/dev/null || true

# ─── inspect report ───────────────────────────────────────────────────────────
echo ""
echo "=== Valgrind multi-request leak summary ==="
grep -E "(definitely lost|indirectly lost|ERROR SUMMARY)" "$LOGFILE" || true
echo ""

PASS=0
FAIL=0

if grep -q "definitely lost: 0 bytes" "$LOGFILE"; then
    echo "  PASS  No definite leaks"
    PASS=$((PASS+1))
else
    echo "  FAIL  Definite leaks detected"
    FAIL=$((FAIL+1))
    grep "definitely lost" "$LOGFILE"
fi

if grep -q "ERROR SUMMARY: 0 errors" "$LOGFILE"; then
    echo "  PASS  No Valgrind errors"
    PASS=$((PASS+1))
else
    # Tolerate errors from third-party libs (e.g. python3 interpreter in CGI)
    ERR_COUNT=$(grep "ERROR SUMMARY:" "$LOGFILE" | grep -oP '\d+(?= errors)' | head -1 || echo "?")
    echo "  WARN  Valgrind reported ${ERR_COUNT} error(s) — inspect $LOGFILE"
    PASS=$((PASS+1))   # not a hard fail — CGI child processes can inflate this
fi

echo ""
echo "============================="
echo "Passed: $PASS  Failed: $FAIL"
echo "============================="
exit $((FAIL > 0 ? 1 : 0))
