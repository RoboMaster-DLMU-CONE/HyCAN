#!/bin/sh
set -e

if command -v systemctl >/dev/null 2>&1; then
    if [ "$1" -eq 0 ]; then
        systemctl stop hycan-daemon.service || true
        systemctl disable hycan-daemon.service || true
    fi
    systemctl daemon-reload || true
fi

exit 0
