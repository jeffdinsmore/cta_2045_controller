# CTA-2045 Controller

This directory contains the executable application that communicates with a
CTA-2045 device over the serial port configured in `main.cpp` (currently
`/dev/ttyUSB0`).

- `main.cpp` opens the serial connection, runs the command loop, and processes
  scheduled commands. The schedule is checked once per second, independently
  of the 60-second commodity and operational-state polling interval.
- `UCMImpl.cpp` and `UCMImpl.h` handle responses received from the water heater.
- `logs/cta_events.csv` records UTC timestamps for command dispatch and
  completion, link/application ACK or NAK callbacks, operational states, and
  controller lifecycle events. It is separate from the commodity `logs/log.csv`.
  Set `CTA_EVENT_LOG_PATH` and `CTA_COMMODITY_LOG_PATH` to place both files in
  a per-run results directory.
  Operational-state rows include both the numeric code and its human-readable
  state name.
- Commodity rows are paired with the operational-state response from the same
  periodic polling cycle. Standalone state queries, including the final
  shutdown query, remain in `cta_events.csv` and do not create partial
  commodity rows.
- `schedule.csv` is the live command schedule. Rows use
  `time,command,argument,event_id,value,units`. Basic DR commands use the
  one-byte `argument`. Advanced Load Up uses `argument` as its duration in
  minutes plus the 16-bit `value` and unit code in `units`. Trailing fields are
  optional for compatibility with older schedules.
- `create_schedule.sh` creates a new schedule and archives the original entries.

From the `dcs` directory, use `make`, `make schedule`, or `make run`.
