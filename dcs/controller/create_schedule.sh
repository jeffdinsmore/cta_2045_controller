#!/usr/bin/env bash

set -euo pipefail

# Test events are written as
# "time after start,seconds adjustment,CTA-2045 command,optional argument,optional event ID,optional value,optional units".
#
# Durations support hours, minutes, and seconds, for example:
#   1h 30m
#   20m
#   15s
# Use a negative adjustment to schedule an event shortly before that time.
# Commands: s=Shed, e=End Shed, l=Load Up, g=Grid Emergency,
#           c=Critical Peak Event, o=Outside Communication Found
EVENTS=(
  "0m,0,o,"
  "0m,15,l,"
  "20m,-15,o,"
  "20m,0,e,"
  "25m,0,g,"
  "1h 30m,-15,o,"
  "1h 30m,0,e,"
  "2h 30m,-15,o,"
  "2h 30m,0,s,"
  "4h 30m,-15,o,"
  "4h 30m,0,e,"
)

duration_to_seconds() {
  local duration="${1//[[:space:]]/}"

  if [[ ! "${duration}" =~ ^([0-9]+h)?([0-9]+m)?([0-9]+s)?$ ]] ||
     [[ -z "${BASH_REMATCH[1]}${BASH_REMATCH[2]}${BASH_REMATCH[3]}" ]]; then
    echo "Invalid duration '${1}'. Use a value such as '1h 30m', '20m', or '15s'." >&2
    return 1
  fi

  local hours="${BASH_REMATCH[1]%h}"
  local minutes="${BASH_REMATCH[2]%m}"
  local seconds="${BASH_REMATCH[3]%s}"
  echo $((10#${hours:-0} * 3600 + 10#${minutes:-0} * 60 + 10#${seconds:-0}))
}

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SCHEDULE_FILE="${SCRIPT_DIR}/schedule.csv"
ARCHIVE_DIR="${SCRIPT_DIR}/schedule_history"
NOW="$(date +%s)"

echo "# time,command,argument,event_id,value,units" > "${SCHEDULE_FILE}"

for event in "${EVENTS[@]}"; do
  IFS=, read -r duration adjustment command argument event_id value units <<< "${event}"
  offset="$(duration_to_seconds "${duration}")"
  timestamp="$((NOW + offset + adjustment))"
  echo "${timestamp},${command},${argument},${event_id},${value},${units}" >> "${SCHEDULE_FILE}"
done

mkdir -p "${ARCHIVE_DIR}"
ARCHIVE_FILE="${ARCHIVE_DIR}/schedule_$(date +%Y-%m-%d_%H-%M-%S).csv"

echo "# scheduled time,command,argument,event_id,value,units" > "${ARCHIVE_FILE}"
while IFS=, read -r timestamp command argument event_id value units; do
  if [[ -z "${timestamp}" || "${timestamp}" == \#* ]]; then
    continue
  fi

  readable_time="$(date -d "@${timestamp}" '+%Y-%m-%d %I:%M %p')"
  echo "${readable_time},${command},${argument},${event_id},${value},${units}" >> "${ARCHIVE_FILE}"
done < "${SCHEDULE_FILE}"

echo "Created ${SCHEDULE_FILE}:"
cat "${SCHEDULE_FILE}"
echo "Saved original schedule as ${ARCHIVE_FILE}"
