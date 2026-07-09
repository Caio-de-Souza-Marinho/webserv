#!/bin/bash
# run_all_tests.sh — full webserv test suite
#
# Usage: ./run_all_tests.sh [options]
#
# Options:
#   --no-42           skip the 42 tester     (run_42_tester.sh)
#   --no-unit         skip unit tests        (make test)
#   --no-integration  skip integration tests (integration_test.py)
#   --no-stress       skip stress tests      (stress_test.sh)
#   --no-valgrind     skip Valgrind checks   (valgrind_multi.sh + server_shutdown.sh)
#   --help / -h       show this message
#
# Exit code: 0 if everything passed, 1 if anything failed.

set -uo pipefail

# ── colours ───────────────────────────────────────────────────────────────────
R="\033[0m"
BOLD="\033[1m"
DIM="\033[2m"
GREEN="\033[1;32m"
RED="\033[1;31m"
YELLOW="\033[1;33m"
CYAN="\033[1;36m"
WHITE="\033[1;37m"

# ── flags ─────────────────────────────────────────────────────────────────────
RUN_42=true
RUN_UNIT=true
RUN_INTEGRATION=true
RUN_STRESS=true
RUN_VALGRIND=true

for arg in "$@"; do
    case "$arg" in
        --no-unit)        RUN_UNIT=false ;;
        --no-42)          RUN_42=false ;;
        --no-integration) RUN_INTEGRATION=false ;;
        --no-stress)      RUN_STRESS=false ;;
        --no-valgrind)    RUN_VALGRIND=false ;;
        --help|-h)
            sed -n '3,12p' "$0" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *)
            echo "Unknown option: $arg  (try --help)"
            exit 1 ;;
    esac
done

# ── counters & state ──────────────────────────────────────────────────────────
TOTAL_PASS=0
TOTAL_FAIL=0
SERVER_PID=""

# ── helpers ───────────────────────────────────────────────────────────────────
banner() {   # banner <number> <title>
    local num="$1" title="$2"
    echo ""
    echo -e "${CYAN}${BOLD}┌──────────────────────────────────────────────────┐${R}"
    echo -e "${CYAN}${BOLD}│  ${WHITE}${num}. ${title}${CYAN}${R}"
    echo -e "${CYAN}${BOLD}└──────────────────────────────────────────────────┘${R}"
}

pass()  { echo -e "  ${GREEN}✔${R}  $1"; TOTAL_PASS=$((TOTAL_PASS + 1)); }
fail()  { echo -e "  ${RED}✘${R}  $1"; TOTAL_FAIL=$((TOTAL_FAIL + 1)); }
warn()  { echo -e "  ${YELLOW}!${R}  $1"; }
skip()  { echo -e "  ${DIM}–  $1  (skipped)${R}"; }
info()  { echo -e "  ${DIM}→${R}  $1"; }
divider() { echo -e "  ${DIM}··············································${R}"; }

start_server() {
    info "starting webserv on ports 8080 / 8081 …"
    ./webserv config/default.conf > /tmp/webserv_test.log 2>&1 &
    SERVER_PID=$!
    local tries=0
    while ! curl -s --max-time 1 http://localhost:8080/ > /dev/null 2>&1; do
        sleep 0.3
        tries=$((tries + 1))
        if [ "$tries" -ge 20 ]; then
            fail "webserv did not start in time"
            tail -6 /tmp/webserv_test.log | sed 's/^/      /'
            kill "$SERVER_PID" 2>/dev/null || true
            SERVER_PID=""
            return 1
        fi
    done
    info "server up  (pid $SERVER_PID)"
}

stop_server() {
    if [ -n "$SERVER_PID" ]; then
        info "stopping webserv (pid $SERVER_PID) …"
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
        SERVER_PID=""
    fi
}

trap 'stop_server' EXIT

# ── build ─────────────────────────────────────────────────────────────────────
echo ""
echo -e "${WHITE}${BOLD}  webserv — test suite${R}"
echo -e "${DIM}  ════════════════════${R}"
echo ""
info "running make re …"
if ! make re 2>&1 | tail -2; then
    echo -e "${RED}  Build failed — aborting.${R}"
    exit 1
fi

# ── 0. pre-flight ─────────────────────────────────────────────────────────────
banner "0" "Pre-flight"
[ -f "Makefile" ]          && pass "Makefile found"          || { fail "no Makefile — run from project root"; exit 1; }
[ -f "config/default.conf" ] && pass "config/default.conf found" || fail "config/default.conf missing"

# ── 1. 42 tester ──────────────────────────────────────────────────────────────
banner "1" "42 tester  (run_42_tester.sh)"
if ! $RUN_42; then
    skip "42 tester"
elif [ ! -f "tests/42_test/run_42_tester.sh" ]; then
    fail "tests/42_test/run_42_tester.sh not found"
else
    bash tests/42_test/run_42_tester.sh
    divider
    [ $? -eq 0 ] && pass "42 tester passed" || fail "42 tester FAILED"
fi

# ── 2. unit tests ─────────────────────────────────────────────────────────────
banner "2" "Unit tests  (make test)"
if ! $RUN_UNIT; then
    skip "unit tests"
else
    UNIT_OUT=$(make test 2>&1)
    UNIT_EXIT=$?
    echo "$UNIT_OUT" | grep -E "^\." | head -5 || true   # brief dot output if present
    U_PASS=$(echo "$UNIT_OUT" | grep -oP 'Passed:\s*\K[0-9]+' | tail -1 || echo "0")
    U_FAIL=$(echo "$UNIT_OUT" | grep -oP 'Failed:\s*\K[0-9]+' | tail -1 || echo "0")
    divider
    if [ "$UNIT_EXIT" -eq 0 ] && [ "${U_FAIL:-0}" -eq 0 ]; then
        pass "Unit tests  passed=$U_PASS  failed=$U_FAIL"
        TOTAL_PASS=$((TOTAL_PASS + U_PASS))
    else
        fail "Unit tests  passed=$U_PASS  failed=$U_FAIL"
        TOTAL_PASS=$((TOTAL_PASS + U_PASS))
        TOTAL_FAIL=$((TOTAL_FAIL + U_FAIL))
        echo "$UNIT_OUT" | grep "FAIL\|Error" | head -10 | sed 's/^/    /' || true
    fi
fi

# ── 3. integration tests ──────────────────────────────────────────────────────
banner "3" "Integration tests  (integration_test.py)"
if ! $RUN_INTEGRATION; then
    skip "integration tests"
elif ! command -v python3 > /dev/null 2>&1; then
    warn "python3 not found — skipping"
elif ! python3 -c "import requests" 2>/dev/null; then
    warn "python 'requests' not installed — skipping  (pip install requests)"
elif [ ! -f "tests/integration/integration_test.py" ]; then
    fail "tests/integration/integration_test.py not found"
else
    start_server || true
    if [ -n "$SERVER_PID" ]; then
        for SCRIPT in tests/integration/integration_test.py tests/integration/integration_extra.py; do
            [ -f "$SCRIPT" ] || continue
            LABEL=$(basename "$SCRIPT" .py)
            LOG="/tmp/webserv_${LABEL}.log"
            python3 "$SCRIPT" 2>&1 | tee "$LOG"
            P=$(grep -oP 'Passed:\s*\K[0-9]+' "$LOG" | tail -1 || echo "0")
            F=$(grep -oP 'Failed:\s*\K[0-9]+' "$LOG" | tail -1 || echo "0")
            TOTAL_PASS=$((TOTAL_PASS + P))
            TOTAL_FAIL=$((TOTAL_FAIL + F))
            divider
            [ "${F:-0}" -eq 0 ] \
                && pass "${LABEL}  passed=$P  failed=$F" \
                || fail "${LABEL}  passed=$P  failed=$F"
        done
        stop_server
    fi
fi

# ── 4. stress tests ───────────────────────────────────────────────────────────
banner "4" "Stress tests  (stress_test.sh)"
if ! $RUN_STRESS; then
    skip "stress tests"
elif [ ! -f "tests/stress/stress_test.sh" ]; then
    fail "tests/stress/stress_test.sh not found"
else
    start_server || true
    if [ -n "$SERVER_PID" ]; then
        for SCRIPT in tests/stress/stress_test.sh tests/stress/stress_extra.sh; do
            [ -f "$SCRIPT" ] || continue
            LABEL=$(basename "$SCRIPT" .sh)
            LOG="/tmp/webserv_${LABEL}.log"
            chmod +x "$SCRIPT"
            bash "$SCRIPT" 2>&1 | tee "$LOG"
            P=$(grep -oP 'Passed:\s*\K[0-9]+' "$LOG" | tail -1 || echo "0")
            F=$(grep -oP 'Failed:\s*\K[0-9]+' "$LOG" | tail -1 || echo "0")
            TOTAL_PASS=$((TOTAL_PASS + P))
            TOTAL_FAIL=$((TOTAL_FAIL + F))
            divider
            [ "${F:-0}" -eq 0 ] \
                && pass "${LABEL}  passed=$P  failed=$F" \
                || fail "${LABEL}  passed=$P  failed=$F"
        done
        stop_server
    fi
fi

# ── 5. valgrind ───────────────────────────────────────────────────────────────
banner "5" "Valgrind  (valgrind_multi.sh + server_shutdown.sh)"
if ! $RUN_VALGRIND; then
    skip "Valgrind checks"
elif ! command -v valgrind > /dev/null 2>&1; then
    warn "valgrind not found — skipping"
else
    for SCRIPT in tests/valgrind/valgrind_multi.sh tests/valgrind/server_shutdown.sh; do
        [ -f "$SCRIPT" ] || { fail "$SCRIPT not found"; continue; }
        LABEL=$(basename "$SCRIPT" .sh)
        LOG="/tmp/webserv_${LABEL}_runner.log"
        bash "$SCRIPT" 2>&1 | tee "$LOG"
        P=$(grep -oP 'Passed:\s*\K[0-9]+' "$LOG" | tail -1 || echo "0")
        F=$(grep -oP 'Failed:\s*\K[0-9]+' "$LOG" | tail -1 || echo "0")
        TOTAL_PASS=$((TOTAL_PASS + P))
        TOTAL_FAIL=$((TOTAL_FAIL + F))
        divider
        [ "${F:-0}" -eq 0 ] \
            && pass "${LABEL}  passed=$P  failed=$F" \
            || fail "${LABEL}  passed=$P  failed=$F"
    done
fi

# ── summary ───────────────────────────────────────────────────────────────────
echo ""
echo -e "${WHITE}${BOLD}  ┌──────────────────────────────────────────────────┐${R}"
echo -e "${WHITE}${BOLD}  │  RESULTS${R}"
echo -e "${WHITE}${BOLD}  ├──────────────────────────────────────────────────┤${R}"
echo -e "  │  ${GREEN}${BOLD}Passed : ${TOTAL_PASS}${R}"
if [ "$TOTAL_FAIL" -gt 0 ]; then
    echo -e "  │  ${RED}${BOLD}Failed : ${TOTAL_FAIL}${R}"
else
    echo -e "  │  ${GREEN}${BOLD}Failed : ${TOTAL_FAIL}${R}"
fi
echo -e "${WHITE}${BOLD}  └──────────────────────────────────────────────────┘${R}"
echo ""

[ "$TOTAL_FAIL" -eq 0 ]
