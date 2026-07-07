#!/bin/bash
# stress_extra.sh — supplementary stress scenarios
# Run AFTER: ./webserv config/default.conf &
#
# Covers:
#   1. Large POST body (near the 10 MB limit) — repeated 50 times
#   2. DELETE storm — upload then delete 30 files in rapid sequence
#   3. Mixed GET/POST/DELETE concurrency via siege (if available)
#   4. HTTP pipelining resilience (raw socket)
#   5. Server liveness probe after all load

set -uo pipefail

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

# ─────────────────────────────────────────────────────────────
# 1. Large POST (9 MB payload × 50 sequential requests)
# ─────────────────────────────────────────────────────────────
echo "=== Large POST body (9 MB × 50) ==="
LARGE_FAIL=0
for i in $(seq 1 50); do
    CODE=$(dd if=/dev/urandom bs=9M count=1 2>/dev/null \
        | curl -s -o /dev/null -w "%{http_code}" \
               --max-time 30 \
               -X POST \
               -H "Content-Type: application/octet-stream" \
               -H "Content-Disposition: attachment; filename=\"stress_large_${i}.bin\"" \
               --data-binary @- \
               "$BASE/uploads")
    if [ "$CODE" != "201" ]; then
        LARGE_FAIL=$((LARGE_FAIL+1))
    fi
done
check "50 × 9 MB POST → all 201 (failures=$LARGE_FAIL)" \
    "$([ $LARGE_FAIL -eq 0 ] && echo 0 || echo 1)"

# ─────────────────────────────────────────────────────────────
# 2. Upload-then-delete storm (30 files in rapid sequence)
# ─────────────────────────────────────────────────────────────
echo ""
echo "=== Upload-then-delete storm (30 files) ==="
STORM_FAIL=0
for i in $(seq 1 30); do
    FNAME="storm_${i}_$(date +%s%N).dat"
    # Upload
    LOC=$(curl -s -o /dev/null -D - \
        -X POST \
        -H "Content-Type: application/octet-stream" \
        -H "Content-Disposition: attachment; filename=\"$FNAME\"" \
        --data-binary "storm payload $i" \
        "$BASE/uploads" | grep -i "^location:" | tr -d '\r' | awk '{print $2}')
    if [ -z "$LOC" ]; then
        STORM_FAIL=$((STORM_FAIL+1))
        continue
    fi
    # Delete
    DEL_CODE=$(curl -s -o /dev/null -w "%{http_code}" \
        -X DELETE "$BASE/${LOC#/}")
    if [ "$DEL_CODE" != "204" ]; then
        STORM_FAIL=$((STORM_FAIL+1))
    fi
done
check "30 upload+delete cycles (failures=$STORM_FAIL)" \
    "$([ $STORM_FAIL -eq 0 ] && echo 0 || echo 1)"

# ─────────────────────────────────────────────────────────────
# 3. Mixed workload via siege (if available)
# ─────────────────────────────────────────────────────────────
echo ""
echo "=== Mixed GET+CGI concurrency (siege) ==="
if require_tool siege; then
    # Siege URL file: mix static and CGI
    SIFILE=$(mktemp /tmp/siege_urls_XXXXXX.txt)
    cat > "$SIFILE" <<'URLS'
http://localhost:8080/index.html
http://localhost:8080/about.html
http://localhost:8080/cgi-bin/test.py
http://localhost:8080/index.html
URLS
    siege -q -c 30 -t 20s -f "$SIFILE" 2>&1 | tee /tmp/siege_mixed.txt
    AVAIL=$(grep "availability" /tmp/siege_mixed.txt | awk -F: '{print $2}' | tr -d ' ,')
    AVAIL_INT=$(echo "$AVAIL" | cut -d'.' -f1)
    check "Mixed siege availability >= 95%" \
        "$([ "${AVAIL_INT:-0}" -ge 95 ] && echo 0 || echo 1)"
    rm -f "$SIFILE"
else
    echo "SKIP: siege not installed — skipping mixed workload test"
fi

# ─────────────────────────────────────────────────────────────
# 4. HTTP pipelining resilience
#    Send two back-to-back requests in a single TCP write and
#    expect two well-formed HTTP responses without the server closing early.
# ─────────────────────────────────────────────────────────────
echo ""
echo "=== HTTP pipelining (raw socket) ==="
python3 - <<'PYEOF'
import socket, sys, time

req1 = b"GET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n"
req2 = b"GET /about.html HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"

s = socket.socket()
s.settimeout(10)
try:
    s.connect(('localhost', 8080))
    s.sendall(req1 + req2)          # single write — pipelined

    buf = b""
    deadline = time.time() + 10
    while time.time() < deadline:
        try:
            chunk = s.recv(4096)
        except socket.timeout:
            break
        if not chunk:
            break
        buf += chunk
        # Both responses received when we see two "HTTP/1.1"
        if buf.count(b"HTTP/1.1") >= 2:
            break

    count = buf.count(b"HTTP/1.1")
    if count >= 2:
        print("  PASS  Pipelining: received 2 HTTP responses")
        sys.exit(0)
    else:
        print(f"  FAIL  Pipelining: only {count} HTTP response(s) received")
        sys.exit(1)
except Exception as e:
    print(f"  FAIL  Pipelining socket error: {e}")
    sys.exit(1)
finally:
    s.close()
PYEOF
[ $? -eq 0 ] && PASS=$((PASS+1)) || FAIL=$((FAIL+1))

# ─────────────────────────────────────────────────────────────
# 5. Liveness probe — server must still respond normally
# ─────────────────────────────────────────────────────────────
echo ""
echo "=== Liveness probe after stress ==="
RESP=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 "$BASE/index.html")
check "Server alive after all stress (→ 200)" \
    "$([ "$RESP" = "200" ] && echo 0 || echo 1)"

# ─────────────────────────────────────────────────────────────
# Summary
# ─────────────────────────────────────────────────────────────
echo ""
echo "============================="
echo "Passed: $PASS  Failed: $FAIL"
echo "============================="
exit $((FAIL > 0 ? 1 : 0))
