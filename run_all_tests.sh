#!/bin/bash
# run_all_tests.sh — full webserv test suite
#
# Usage: ./run_all_tests.sh [options]
#
# Options:
#   --no-42           skip the official 42 tester
#   --no-unit         skip unit tests  (make test)
#   --no-integration  skip integration tests
#   --no-stress       skip stress tests
#   --no-valgrind     skip all Valgrind checks
#   --help / -h       show this message
#
# Runs in order:
#   0. Pre-flight checks
#   1. Unit tests        (make test)
#   2. 42 tester         (run_42_tester.sh)
#   3. Integration       (integration_test.py + integration_extra.py)
#   4. Stress            (stress_test.sh + stress_extra.sh)
#   5. Valgrind multi    (valgrind_multi.sh)
#   6. Valgrind shutdown (server_shutdown.sh)
#
# Exit code: 0 if everything passed, 1 if anything failed.

set -uo pipefail

# ─────────────────────────── colours ─────────────────────────────────────────
RESET="\033[0m"
BOLD="\033[1m"
GREEN="\033[1;32m"
RED="\033[1;31m"
YELLOW="\033[1;33m"
CYAN="\033[1;36m"

# ─────────────────────────── flags ───────────────────────────────────────────
RUN_42=true
RUN_UNIT=true
RUN_INTEGRATION=true
RUN_STRESS=true
RUN_VALGRIND=true

for arg in "$@"; do
    case "$arg" in
        --no-42)          RUN_42=false ;;
        --no-unit)        RUN_UNIT=false ;;
        --no-integration) RUN_INTEGRATION=false ;;
        --no-stress)      RUN_STRESS=false ;;
        --no-valgrind)    RUN_VALGRIND=false ;;
        --help|-h)
            sed -n '3,17p' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *)
            echo "Unknown option: $arg  (try --help)"
            exit 1
            ;;
    esac
done

make re

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
skip() { echo -e "  ${YELLOW}–${RESET}  $1  ${YELLOW}(skipped)${RESET}"; }

start_server() {
    echo -e "${YELLOW}  → starting webserv on ports 8080 / 8081 …${RESET}"
    ./webserv config/default.conf > /tmp/webserv_test.log 2>&1 &
    SERVER_PID=$!
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

trap 'stop_server' EXIT

# ─────────────────────────── 0. pre-flight ───────────────────────────────────
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
if $RUN_UNIT; then
    section "Unit tests  (make test)"

    UNIT_OUTPUT=$(make test 2>&1)
    UNIT_EXIT=$?
    echo "$UNIT_OUTPUT"

    if [ $UNIT_EXIT -eq 0 ]; then
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
else
    section "Unit tests  (make test)"
    skip "unit tests"
fi

# ─────────────────────────── 2. 42 tester ────────────────────────────────────
if $RUN_42; then
    section "Official 42 tester  (run_42_tester.sh)"

    if [ ! -f "tests/42_test/run_42_tester.sh" ]; then
        fail "tests/42_test/run_42_tester.sh not found"
    else
        bash tests/42_test/run_42_tester.sh
        TESTER_EXIT=$?
        if [ $TESTER_EXIT -eq 0 ]; then
            pass "42 tester passed"
        else
            fail "42 tester FAILED (exit $TESTER_EXIT)"
        fi
    fi
else
    section "Official 42 tester  (run_42_tester.sh)"
    skip "42 tester"
fi

# ─────────────────────────── 3. integration tests ────────────────────────────
if $RUN_INTEGRATION; then
    section "Integration tests  (integration_test.py)"

    if ! command -v python3 > /dev/null 2>&1; then
        echo -e "${YELLOW}  SKIP: python3 not found${RESET}"
    elif ! python3 -c "import requests" 2>/dev/null; then
        echo -e "${YELLOW}  SKIP: python requests not installed  (pip install requests)${RESET}"
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

            if [ -f "tests/integration/integration_extra.py" ]; then
                python3 tests/integration/integration_extra.py 2>&1 | tee /tmp/webserv_integration_extra.log
                EXTRA_EXIT=${PIPESTATUS[0]}

                EXTRA_PASSED=$(grep -oP 'Passed:\s*\K[0-9]+' /tmp/webserv_integration_extra.log | tail -1 || echo "0")
                EXTRA_FAILED=$(grep -oP 'Failed:\s*\K[0-9]+' /tmp/webserv_integration_extra.log | tail -1 || echo "0")
                TOTAL_PASS=$((TOTAL_PASS + EXTRA_PASSED))
                TOTAL_FAIL=$((TOTAL_FAIL + EXTRA_FAILED))

                if [ $EXTRA_EXIT -eq 0 ]; then
                    echo -e "  ${GREEN}✔${RESET}  Extra integration tests passed  (passed=$EXTRA_PASSED  failed=$EXTRA_FAILED)"
                else
                    echo -e "  ${RED}✘${RESET}  Extra integration tests FAILED  (passed=$EXTRA_PASSED  failed=$EXTRA_FAILED)"
                fi
            fi

            stop_server
        fi
    fi
else
    section "Integration tests  (integration_test.py)"
    skip "integration tests"
fi

# ─────────────────────────── 4. stress tests ─────────────────────────────────
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

            if [ -f "tests/stress/stress_extra.sh" ]; then
                bash tests/stress/stress_extra.sh 2>&1 | tee /tmp/webserv_stress_extra.log
                SEXTRA_EXIT=${PIPESTATUS[0]}

                SEXTRA_PASSED=$(grep -oP 'Passed:\s*\K[0-9]+' /tmp/webserv_stress_extra.log | tail -1 || echo "0")
                SEXTRA_FAILED=$(grep -oP 'Failed:\s*\K[0-9]+' /tmp/webserv_stress_extra.log | tail -1 || echo "0")
                TOTAL_PASS=$((TOTAL_PASS + SEXTRA_PASSED))
                TOTAL_FAIL=$((TOTAL_FAIL + SEXTRA_FAILED))

                if [ $SEXTRA_EXIT -eq 0 ]; then
                    echo -e "  ${GREEN}✔${RESET}  Extra stress tests passed  (passed=$SEXTRA_PASSED  failed=$SEXTRA_FAILED)"
                else
                    echo -e "  ${RED}✘${RESET}  Extra stress tests FAILED  (passed=$SEXTRA_PASSED  failed=$SEXTRA_FAILED)"
                fi
            fi

            stop_server
        fi
    fi
else
    section "Stress tests  (stress_test.sh)"
    skip "stress tests"
fi

# ─────────────────────────── 5. valgrind multi ───────────────────────────────
if $RUN_VALGRIND; then
    section "Valgrind leak check  (valgrind_multi.sh)"

    if ! command -v valgrind > /dev/null 2>&1; then
        echo -e "${YELLOW}  SKIP: valgrind not found${RESET}"
    elif [ ! -f "tests/valgrind/valgrind_multi.sh" ]; then
        fail "tests/valgrind/valgrind_multi.sh not found"
    else
        bash tests/valgrind/valgrind_multi.sh 2>&1 | tee /tmp/webserv_valgrind_multi.log
        VMULTI_EXIT=${PIPESTATUS[0]}

        VMULTI_PASSED=$(grep -oP 'Passed:\s*\K[0-9]+' /tmp/webserv_valgrind_multi.log | tail -1 || echo "0")
        VMULTI_FAILED=$(grep -oP 'Failed:\s*\K[0-9]+' /tmp/webserv_valgrind_multi.log | tail -1 || echo "0")
        TOTAL_PASS=$((TOTAL_PASS + VMULTI_PASSED))
        TOTAL_FAIL=$((TOTAL_FAIL + VMULTI_FAILED))

        if [ $VMULTI_EXIT -eq 0 ]; then
            echo -e "  ${GREEN}✔${RESET}  Valgrind multi-request passed  (passed=$VMULTI_PASSED  failed=$VMULTI_FAILED)"
        else
            echo -e "  ${RED}✘${RESET}  Valgrind multi-request FAILED  (passed=$VMULTI_PASSED  failed=$VMULTI_FAILED)"
        fi
    fi
else
    section "Valgrind leak check  (valgrind_multi.sh)"
    skip "valgrind multi-request check"
fi

# ─────────────────────────── 6. valgrind shutdown ────────────────────────────
if $RUN_VALGRIND; then
    section "Valgrind shutdown check  (server_shutdown.sh)"

    if ! command -v valgrind > /dev/null 2>&1; then
        echo -e "${YELLOW}  SKIP: valgrind not found${RESET}"
    elif [ ! -f "tests/valgrind/server_shutdown.sh" ]; then
        fail "tests/valgrind/server_shutdown.sh not found"
    else
        bash tests/valgrind/server_shutdown.sh 2>&1 | tee /tmp/webserv_valgrind_shutdown.log
        VSHUT_EXIT=${PIPESTATUS[0]}

        VSHUT_PASSED=$(grep -oP 'Passed:\s*\K[0-9]+' /tmp/webserv_valgrind_shutdown.log | tail -1 || echo "0")
        VSHUT_FAILED=$(grep -oP 'Failed:\s*\K[0-9]+' /tmp/webserv_valgrind_shutdown.log | tail -1 || echo "0")
        TOTAL_PASS=$((TOTAL_PASS + VSHUT_PASSED))
        TOTAL_FAIL=$((TOTAL_FAIL + VSHUT_FAILED))

        if [ $VSHUT_EXIT -eq 0 ]; then
            echo -e "  ${GREEN}✔${RESET}  Valgrind shutdown passed  (passed=$VSHUT_PASSED  failed=$VSHUT_FAILED)"
        else
            echo -e "  ${RED}✘${RESET}  Valgrind shutdown FAILED  (passed=$VSHUT_PASSED  failed=$VSHUT_FAILED)"
        fi
    fi
else
    section "Valgrind shutdown check  (server_shutdown.sh)"
    skip "valgrind shutdown check"
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
