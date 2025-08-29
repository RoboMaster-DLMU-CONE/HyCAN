#!/bin/bash

# HyCAN Test Runner with Daemon
# This script starts the daemon, runs tests, and cleans up

DAEMON_EXECUTABLE="./hycan_daemon"
SOCKET_PATH="/tmp/hycan_daemon.sock"

# Function to cleanup on exit
cleanup() {
    echo "Cleaning up..."
    if [ ! -z "$DAEMON_PID" ]; then
        echo "Stopping daemon (PID: $DAEMON_PID)"
        kill $DAEMON_PID 2>/dev/null
        wait $DAEMON_PID 2>/dev/null
    fi
    rm -f "$SOCKET_PATH"
}

# Set up signal handling
trap cleanup EXIT INT TERM

echo "Starting HyCAN daemon for testing..."

# Start daemon in background
$DAEMON_EXECUTABLE "$SOCKET_PATH" &
DAEMON_PID=$!

# Wait a moment for daemon to start
sleep 2

# Check if daemon is still running
if ! kill -0 $DAEMON_PID 2>/dev/null; then
    echo "ERROR: Daemon failed to start"
    exit 1
fi

echo "Daemon started (PID: $DAEMON_PID)"
echo "Running tests..."

# Run tests
TEST_EXIT_CODE=0

echo ""
echo "=== Running Daemon Test ==="
./HyCAN_DaemonTest
if [ $? -ne 0 ]; then
    TEST_EXIT_CODE=1
fi

echo ""
echo "=== Running Netlink Test ==="
./HyCAN_NetlinkTest
if [ $? -ne 0 ]; then
    TEST_EXIT_CODE=1
fi

echo ""
echo "=== Running Interface Test ==="
./HyCAN_InterfaceTest
if [ $? -ne 0 ]; then
    TEST_EXIT_CODE=1
fi

echo ""
echo "=== Running Interface Stress Test ==="
./HyCAN_InterfaceStressTest
if [ $? -ne 0 ]; then
    TEST_EXIT_CODE=1
fi

if [ $TEST_EXIT_CODE -eq 0 ]; then
    echo ""
    echo "All tests passed!"
else
    echo ""
    echo "Some tests failed!"
fi

exit $TEST_EXIT_CODE