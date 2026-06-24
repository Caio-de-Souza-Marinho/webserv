#!/bin/bash
# run_all_tests.sh — full webserv test suite
# Usage: ./run_all_tests.sh [--no-stress] [--no-integration]
#
# Runs in order:
#   1. Unit tests   (make test)
#   2. Integration  (integration_test.py)   — starts/stops the server
#   3. Stress       (stress_test.sh)        — starts/stops the server
#
# Exit code: 0 if everything passed, 1 if anything failed.

set -uo pipefail

# ─────────────────────────── colours ────────────────────────────────────────
RESET="\033[0m"
BOLD="\033[1m"
GREEN="\033[1;32m"
RED="\033[1;31m"
YELLOW="\033[1;33m"
CYAN="\033[1;36m"

# ─────────────────────────── flags ──────────────────────────────────────────
RUN_INTEGRATION=true
RUN_STRESS=true

for arg in "$@"; do
    case "$arg" in
        --no-integration) RUN_INTEGRATION=false ;;
        --no-stress)      RUN_STRESS=false ;;
        --help|-h)
            echo "Usage: $0 [--no-integration] [--no-stress]"
            exit 0
            ;;
    esac
done

# ─────────────────────────── 42 tests ─────────────────────────────────────
echo "======================================"
echo "Running official 42 tester"
echo "======================================"

./tests/42_test/run_42_tester.sh

echo
echo "======================================"
echo "Running custom tests"
echo "======================================"

# ─────────────────────────── helpers ─────────────────────────────────────────
TOTAL_PASS=0
TOTAL_FAIL=0
SERVER_PID=""

section() {
    echo ""
    echo -e "${CYAN}${BOLD}╔══════════════════════════════════════════════╗${RESET}"
    echo -e "${CYAN}${BOLD}  $1${RESET}"
    echo -e "${CYAN}${BOLD}╚══════════════════════════════════════════════╝${RESET}"
}

pass() { echo -e "  ${GREEN}✔${RESET}  $1"; TOTAL_PASS=$((TOTAL_PASS + 1)); }
fail() { echo -e "  ${RED}✘${RESET}  $1"; TOTAL_FAIL=$((TOTAL_FAIL + 1)); }

start_server() {
    echo -e "${YELLOW}  → starting webserv on ports 8080 / 8081 …${RESET}"
    ./webserv config/default.conf > /tmp/webserv_test.log 2>&1 &
    SERVER_PID=$!
    # wait until both ports accept connections (max 5 s)
    local tries=0
    while ! (curl -s --max-time 1 http://localhost:8080/ > /dev/null 2>&1); do
        sleep 0.3
        tries=$((tries + 1))
        if [ "$tries" -ge 17 ]; then
            echo -e "${RED}  ERROR: webserv did not start in time.${RESET}"
            echo "  Last log lines:"
            tail -10 /tmp/webserv_test.log | sed 's/^/    /'
            kill "$SERVER_PID" 2>/dev/null || true
            SERVER_PID=""
            return 1
        fi
    done
    echo -e "${GREEN}  → server up (pid $SERVER_PID)${RESET}"
}

stop_server() {
    if [ -n "$SERVER_PID" ]; then
        echo -e "${YELLOW}  → stopping webserv (pid $SERVER_PID) …${RESET}"
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
        SERVER_PID=""
    fi
}

# make sure we always stop the server on exit
trap 'stop_server' EXIT

# ─────────────────────────── 0. sanity ───────────────────────────────────────
section "Pre-flight checks"

if [ ! -f "Makefile" ]; then
    echo -e "${RED}  ERROR: run this script from the project root (no Makefile found).${RESET}"
    exit 1
fi
pass "Makefile found"

if [ ! -f "config/default.conf" ]; then
    fail "config/default.conf missing"
fi

# ─────────────────────────── 1. unit tests ───────────────────────────────────
section "Unit tests  (make test)"

UNIT_OUTPUT=$(make test 2>&1)
UNIT_EXIT=$?

echo "$UNIT_OUTPUT"

if [ $UNIT_EXIT -eq 0 ]; then
    # extract Passed/Failed counts from tester output if present
    UNIT_PASSED=$(echo "$UNIT_OUTPUT" | grep -oP 'Passed:\s*\K[0-9]+' | tail -1 || echo "?")
    UNIT_FAILED=$(echo "$UNIT_OUTPUT" | grep -oP 'Failed:\s*\K[0-9]+' | tail -1 || echo "0")
    pass "Unit tests passed  (passed=$UNIT_PASSED  failed=$UNIT_FAILED)"
    TOTAL_PASS=$((TOTAL_PASS + 1))
    if [ "${UNIT_FAILED}" != "0" ] && [ "${UNIT_FAILED}" != "?" ]; then
        TOTAL_FAIL=$((TOTAL_FAIL + UNIT_FAILED))
        TOTAL_PASS=$((TOTAL_PASS - UNIT_FAILED))
    fi
else
    fail "Unit tests FAILED (exit $UNIT_EXIT)"
fi

# ─────────────────────────── 2. integration tests ────────────────────────────
if $RUN_INTEGRATION; then
    section "Integration tests  (integration_test.py)"

    if ! command -v python3 > /dev/null 2>&1; then
        echo -e "${YELLOW}  SKIP: python3 not found${RESET}"
    elif ! python3 -c "import requests" 2>/dev/null; then
        echo -e "${YELLOW}  SKIP: python requests library not installed  (pip install requests)${RESET}"
    elif [ ! -f "tests/integration/integration_test.py" ]; then
        fail "tests/integration/integration_test.py not found"
    else
        start_server || { fail "server failed to start for integration tests"; }

        if [ -n "$SERVER_PID" ]; then
            python3 tests/integration/integration_test.py 2>&1 | tee /tmp/webserv_integration.log
            INT_EXIT=${PIPESTATUS[0]}

            INT_PASSED=$(grep -oP 'Passed:\s*\K[0-9]+' /tmp/webserv_integration.log | tail -1 || echo "0")
            INT_FAILED=$(grep -oP 'Failed:\s*\K[0-9]+' /tmp/webserv_integration.log | tail -1 || echo "0")
            TOTAL_PASS=$((TOTAL_PASS + INT_PASSED))
            TOTAL_FAIL=$((TOTAL_FAIL + INT_FAILED))

            if [ $INT_EXIT -eq 0 ]; then
                echo -e "  ${GREEN}✔${RESET}  Integration tests passed  (passed=$INT_PASSED  failed=$INT_FAILED)"
            else
                echo -e "  ${RED}✘${RESET}  Integration tests FAILED  (passed=$INT_PASSED  failed=$INT_FAILED)"
            fi

            stop_server
        fi
    fi
fi

# ─────────────────────────── 3. stress tests ─────────────────────────────────
if $RUN_STRESS; then
    section "Stress tests  (stress_test.sh)"

    if [ ! -f "tests/stress/stress_test.sh" ]; then
        fail "tests/stress/stress_test.sh not found"
    else
        chmod +x tests/stress/stress_test.sh
        start_server || { fail "server failed to start for stress tests"; }

        if [ -n "$SERVER_PID" ]; then
            bash tests/stress/stress_test.sh 2>&1 | tee /tmp/webserv_stress.log
            STRESS_EXIT=${PIPESTATUS[0]}

            STRESS_PASSED=$(grep -oP 'Passed:\s*\K[0-9]+' /tmp/webserv_stress.log | tail -1 || echo "0")
            STRESS_FAILED=$(grep -oP 'Failed:\s*\K[0-9]+' /tmp/webserv_stress.log | tail -1 || echo "0")
            TOTAL_PASS=$((TOTAL_PASS + STRESS_PASSED))
            TOTAL_FAIL=$((TOTAL_FAIL + STRESS_FAILED))

            if [ $STRESS_EXIT -eq 0 ]; then
                echo -e "  ${GREEN}✔${RESET}  Stress tests passed  (passed=$STRESS_PASSED  failed=$STRESS_FAILED)"
            else
                echo -e "  ${RED}✘${RESET}  Stress tests FAILED  (passed=$STRESS_PASSED  failed=$STRESS_FAILED)"
            fi

            stop_server
        fi
    fi
fi

# ─────────────────────────── summary ─────────────────────────────────────────
echo ""
echo -e "${BOLD}╔══════════════════════════════════════════════╗${RESET}"
echo -e "${BOLD}  FINAL RESULTS${RESET}"
echo -e "${BOLD}╚══════════════════════════════════════════════╝${RESET}"
echo -e "  ${GREEN}${BOLD}Passed : $TOTAL_PASS${RESET}"
if [ "$TOTAL_FAIL" -gt 0 ]; then
    echo -e "  ${RED}${BOLD}Failed : $TOTAL_FAIL${RESET}"
else
    echo -e "  ${GREEN}${BOLD}Failed : $TOTAL_FAIL${RESET}"
fi
echo ""

[ "$TOTAL_FAIL" -eq 0 ]
