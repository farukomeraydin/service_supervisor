#!/usr/bin/env bash
set -e
COUNT_FILE=${COUNT_FILE:-/tmp/flaky_count}
count=0; [[ -f "$COUNT_FILE" ]] && count=$(cat "$COUNT_FILE")
count=$((count+1)); echo "$count" > "$COUNT_FILE"
echo "flaky run $count"
if [[ $count -lt 3 ]]; then exit 1; fi
exit 0
