#!/bin/sh
# Cách 2 (Linux/macOS, đã cài mosquitto)
exec mosquitto -c "$(dirname "$0")/mosquitto.conf" -v
