#!/usr/bin/env bash
set -euo pipefail
# Each manual run gets its own logs. Existing test CSVs remain untouched.
mkdir -p logs
run_dir=$(mktemp -d "logs/read_check_$(date +%Y_%m_%d_%H%M%S)_XXXXXX")
export CTA_EVENT_LOG_PATH="$run_dir/cta_events.csv"
export CTA_RAW_MESSAGE_LOG_PATH="$run_dir/cta_raw_messages.csv"
export CTA_DEVICE_INFO_LOG_PATH="$run_dir/cta_device_information.csv"
export CTA_COMMODITY_LOG_PATH="$run_dir/cta_commodity.csv"
printf 'Read-check logs: %s\n' "$run_dir"
"${1:-../build/debug/cta2045_controller}" 2>&1 | tee "$run_dir/cta_controller.log"
