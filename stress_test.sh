#!/bin/bash
# stress_test.sh — run after: ./webserv config/default.conf &

BASE="http://localhost:8080"
PASS=0
FAIL=0

check() {
    local name="$1"
    local ok="$2"
    if [ "$ok" = "0" ]; then
        echo "  PASS  $name"
        PASS=$((PASS+1))
    else
        echo "  FAIL  $name"
        FAIL=$((FAIL+1))
    fi
}

require_tool() {
    command -v "$1" >/dev/null 2>&1 || { echo "SKIP: $1 not installed"; return 1; }
}

echo "=== Concurrency: static file (siege) ==="
if require_tool siege; then
    siege -q -c 25 -t 15s "$BASE/index.html" 2>&1 | tee /tmp/siege_static.txt
    AVAIL=$(grep "availability" /tmp/siege_static.txt | awk -F: '{print $2}' | tr -d ' ,')
    AVAIL_INT=$(echo "$AVAIL" | cut -d'.' -f1)
    check "Static availability >= 99%" "$([ "${AVAIL_INT:-0}" -ge 99 ] && echo 0 || echo 1)"
fi

echo ""
echo "=== Concurrency: CGI (siege) ==="
if require_tool siege; then
    siege -q -c 10 -t 15s "$BASE/cgi-bin/test.py" 2>&1 | tee /tmp/siege_cgi.txt
    AVAIL=$(grep "availability" /tmp/siege_cgi.txt | awk -F: '{print $2}' | tr -d ' ,')
    AVAIL_INT=$(echo "$AVAIL" | cut -d'.' -f1)
    check "CGI availability >= 95%" "$([ "${AVAIL_INT:-0}" -ge 95 ] && echo 0 || echo 1)"
fi

echo ""
echo "=== Keep-Alive pipeline (ab) ==="
if require_tool ab; then
    RESULT=$(ab -q -n 2000 -c 50 -k "$BASE/index.html" 2>&1)
    FAILED=$(echo "$RESULT" | grep "Failed requests" | awk '{print $3}')
    check "Keep-Alive: 0 failed requests" "$([ "$FAILED" = "0" ] && echo 0 || echo 1)"
fi

echo ""
echo "=== Slow client timeout ==="
python3 - <<'EOF'
import socket, time, sys
s = socket.socket()
s.connect(('localhost', 8080))
s.send(b'GET /index.html HTTP/1.1\r\nHost: loc')
time.sleep(35)
try:
    data = s.recv(1024)
    if b'408' in data or data == b'':
        print("  PASS  Slow client got 408 or connection closed")
        sys.exit(0)
    else:
        print(f"  FAIL  Unexpected response: {data[:80]}")
        sys.exit(1)
except:
    print("  PASS  Connection closed by server (timeout)")
    sys.exit(0)
finally:
    s.close()
EOF
[ $? -eq 0 ] && PASS=$((PASS+1)) || FAIL=$((FAIL+1))

echo ""
echo "=== Server still alive after all tests ==="
RESP=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/index.html")
check "Server responds after stress" "$([ "$RESP" = "200" ] && echo 0 || echo 1)"

echo ""
echo "============================="
echo "Passed: $PASS  Failed: $FAIL"
echo "============================="
exit $((FAIL > 0 ? 1 : 0))
