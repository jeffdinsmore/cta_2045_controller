#!/usr/bin/env bash

set -euo pipefail

# Test events are written as "seconds after start,CTA-2045 command".
# Commands: s=Shed, e=End Shed, l=Load Up, g=Grid Emergency,
#           c=Critical Peak Event
EVENTS=(
  "0,o"
  "15,l"
  "(60*20)-15,o"
  "60*20,e"
  "60*25,g"
  "(60*60*1.5)-15,o"
  "60*60*1.5,e"
  "(60*60*2.5)-15,o"
  "60*60*2.5,s"
  "(60*60*4.5)-15,o"
  "60*60*4.5,e"
)

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SCHEDULE_FILE="${SCRIPT_DIR}/schedule.csv"
ARCHIVE_DIR="${SCRIPT_DIR}/schedule_history"
NOW="$(date +%s)"

echo "# time,command" > "${SCHEDULE_FILE}"

for event in "${EVENTS[@]}"; do
  offset="${event%%,*}"
  command="${event##*,}"
  timestamp="$((NOW + offset))"
  echo "${timestamp},${command}" >> "${SCHEDULE_FILE}"
done

mkdir -p "${ARCHIVE_DIR}"
ARCHIVE_FILE="${ARCHIVE_DIR}/schedule_$(date +%Y-%m-%d_%H-%M-%S).csv"

echo "# scheduled time,command" > "${ARCHIVE_FILE}"
while IFS=, read -r timestamp command; do
  if [[ -z "${timestamp}" || "${timestamp}" == \#* ]]; then
    continue
  fi

  readable_time="$(date -d "@${timestamp}" '+%Y-%m-%d %I:%M %p')"
  echo "${readable_time},${command}" >> "${ARCHIVE_FILE}"
done < "${SCHEDULE_FILE}"

echo "Created ${SCHEDULE_FILE}:"
cat "${SCHEDULE_FILE}"
echo "Saved original schedule as ${ARCHIVE_FILE}"
