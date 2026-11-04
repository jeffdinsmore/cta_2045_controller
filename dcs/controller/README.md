# CTA-2045 Controller

This directory contains the executable application that communicates with a
CTA-2045 device over the serial port configured in `main.cpp` (currently
`/dev/ttyUSB0`).

- `main.cpp` opens the serial connection, runs the command loop, and processes
  scheduled commands. The schedule is checked once per second, independently
  of the periodic queries. Commodity data is requested every 60 seconds and
  operational state is requested every 30 seconds.
- `UCMImpl.cpp` and `UCMImpl.h` handle responses received from the water heater.
- `logs/cta_events.csv` records Pacific timestamps for command dispatch and
  completion, link/application ACK or NAK callbacks, operational states, and
  controller lifecycle events. Its normalized columns separate source,
  Opcode1/Opcode2, NAK code/reason, and operational-state code/name from
  free-form details. It is separate from the commodity `logs/log.csv`.
  Set `CTA_EVENT_LOG_PATH` and `CTA_COMMODITY_LOG_PATH` to place both files in
  a per-run results directory.
  Operational-state rows include both the numeric code and its human-readable
  state name.
- `logs/cta_raw_messages.csv` records the raw application payload of Advanced
  Load Up GET replies for byte-level auditing without adding noise to
  `cta_events.csv`. Set `CTA_RAW_MESSAGE_LOG_PATH` to place it in a per-run
  results directory.
- Ten seconds after a successful Advanced Load Up SET, the controller requests
  one GET readback. The associated verification rows in `cta_events.csv`
  include `delay_seconds=10` so the reply can be correlated with its SET
  command.
- Commodity rows are paired with the operational-state response from the same
  periodic polling cycle. Standalone state queries, including the final
  shutdown query, remain in `cta_events.csv` and do not create partial
  commodity rows. New commodity logs include a header and three uniquely named
  commodity groups containing code, cumulative Wh, and instantaneous W. Blank
  columns visually separate the groups; the redundant Estimated/Measured text
  is omitted.
- `schedule.csv` is the live command schedule. Rows use
  `time,command,argument,event_id,value,units`. Basic DR commands use the
  one-byte `argument`. Advanced Load Up uses `argument` as its duration in
  minutes plus the 16-bit `value` and unit code in `units`. Trailing fields are
  optional for compatibility with older schedules.
- `create_schedule.sh` creates a new schedule and archives the original entries.

From the `dcs` directory, use `make run` for interactive operation without a
schedule, or `make test` to create and process the standalone test schedule.

Temperature setpoints are supported by the bundled library as intermediate
GET/SET messages (Opcode1 `0x03`, Opcode2 `0x03`). In the interactive menu,
`j` requests setpoints and `w` sends a SET. The SET prompt
requires four decimal integers: device type, units (`0` = Fahrenheit, `1` =
Celsius), and two unsigned 16-bit protocol setpoint values. Device type `2`
means electric water heater; `3` means heat pump water heater. Values are
passed through as raw protocol values; temperature scaling and special values
must be checked against the applicable device/protocol documentation before
use. Both setpoints are required by the current library SET API.

GET responses with either one or two setpoints and SET response codes are
reported through the controller logger and `cta_events.csv`. A successful
transport result alone does not establish that the device accepted the SET;
check the intermediate response code. Use `j` to request a readback. These
commands are interactive only; the schedule format is unchanged.

`t` requests present temperature. Successful replies are decoded only
with the known eight-byte payload and units 0 or 1. Error responses are logged
without reading temperature fields; malformed or unknown layouts are logged as
`malformed_or_unrecognized`. `command_not_implemented` is an explicit device
response; malformed replies and timeouts do not prove lack of support or identify
a CEA/CTA revision. Device-information version data remains the revision evidence.
Present temperature is a signed 16-bit value in hundredths of a degree. Events
include the signed raw value, decoded degrees with two decimal places, and F/C
units. For water heaters this is average tank temperature.

Startup requests setpoints immediately after device information. The public
`cta_device_information.csv` row includes `setpoint_status`,
`setpoint_device_type`, `setpoint_units`, `setpoint1_raw`, and `setpoint2_raw`.
Setpoints remain raw pending confirmation of their encoding. Unsupported or
failed requests leave value columns blank; later interactive requests do not
change the startup snapshot. Present temperature is requested in each 60-second
commodity polling cycle, independently of commodity success. Dispatch, completion,
and decoded response events go to `cta_events.csv`. An explicit
`command_not_implemented` disables further automatic temperature requests for that
session. Other errors and timeouts remain retryable.

`cta_events.csv` has dedicated `present_temperature` (numeric degrees) and
`temperature_unit` (`F` or `C`) columns. They are populated only on valid
present-temperature response rows. Error and missing-data rows leave both blank;
response codes and names use the existing intermediate-response columns. Payload
length remains diagnostic text in `details`. New test logs use this format.
Existing CSVs are never rewritten or converted. If the configured event log has
an older header, the writer reports the mismatch and leaves that file untouched;
start a new test log to collect events in the updated format.
