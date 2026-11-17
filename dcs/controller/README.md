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
include the signed raw value in `present_temperature_raw`, decoded degrees
in `present_temperature`, and F/C units in `present_temperature_unit`. For water heaters this is average tank temperature.

Startup requests setpoints immediately after device information. The public
`cta_device_information.csv` row includes `setpoint_status`,
`setpoint_device_type`, `setpoint_units`, `setpoint1_raw`, and `setpoint2_raw`.
Setpoints retain device-reported numeric values without scaling or offset
corrections; see [setpoint observations](setpoint_observations.md) for the
verified WH-3 display relationship and WH-4 scale discrepancy. Unsupported or
failed requests leave value columns blank; later interactive requests do not
change the startup snapshot. Both present temperature and setpoints are requested
in each 60-second commodity polling cycle, independently of commodity success.
Each query has independent error handling and unsupported-command tracking. Dispatch, completion,
and decoded response events go to `cta_events.csv`. An explicit
`command_not_implemented` disables further automatic requests for that particular
query for the session. Other errors and timeouts remain retryable.

`cta_events.csv` has dedicated `present_temperature` (numeric degrees) and
`present_temperature_unit` (`F` or `C`) columns. They are populated only on valid
present-temperature response rows. Error and missing-data rows leave both blank;
response codes and names use the existing intermediate-response columns. Payload
length remains diagnostic text in `details`. New test logs use this format.
Existing CSVs are never rewritten or converted. If the configured event log has
an older header, the writer reports the mismatch and leaves that file untouched;
start a new test log to collect events in the updated format.

`make run` creates a fresh `controller/logs/read_check_<timestamp>_<suffix>/`
directory for each interactive run, containing events, raw messages, device
information, commodity data, and console output. It does not enable scheduling.
Existing logs remain untouched. Raw `get_set_setpoints_reply` records contain
application payload bytes beginning with the two opcodes, excluding framing and
checksum; the shared GET/SET reply opcode does not identify which request caused
it. Short and unrecognized setpoint replies are captured before decoding.

For a five-minute read check, note the heater display setpoint, run `make run`,
enter `j` and `t`, repeat after two minutes, then exit with `q`. Do not select
`w` or DR commands during this check. Save the printed log directory with the
heater identity and displayed setpoint for comparison.

The November 4 WH-3 check showed reported top-element setpoints of 143 F at a
125 F display setting, 148 F at 130 F, and 143 F after restoring 125 F. Logging
preserves those reported values without subtracting the observed 18 F
difference. Evidence and interpretation limits are recorded in
[setpoint observations](setpoint_observations.md).

`f` requests Temperature Offset (the old hidden uppercase `O` shortcut is
replaced). Leave the prompt blank for the current offset, or enter a decimal
Basic DR opcode (0-255), such as `1` for Shed or `10` for Critical Peak. The
request is read-only. Capability bitmap bit 3 advertises support, but manual
queries are allowed even when it is clear so device behavior can be checked.
The value is unsigned 8-bit whole degrees, with units 0 = F and 1 = C.

New event logs have numeric `cta_temperature_offset`, `cta_temperature_offset_unit`
(`F`/`C`), and optional `cta_temperature_offset_basic_dr_opcode` columns. Error,
short status-only, and malformed replies leave these blank and retain response
codes/names in the existing intermediate-response columns. Reply lengths of 5
and 6 bytes are accepted only with success status and recognized units. Raw
payloads use `get_set_temperature_offset_reply` in `cta_raw_messages.csv`.
GET and SET share a reply opcode, so a three-byte success reply is logged as
`status_only`, not as a zero offset. Existing CSVs are never rewritten; use a
new test log. Offset polling is manual only. This protocol offset is separate
from the observed WH-3 display-to-top-element difference; no correction is
applied to setpoint values.


Temperature and setpoint event columns (new runs only):

| Column | Meaning |
| --- | --- |
| `present_temperature_raw` | Signed device-reported integer, before dividing by 100 |
| `present_temperature` | Present temperature decoded as raw / 100 |
| `present_temperature_unit` | F or C for the decoded present temperature |
| `present_temperature_units_raw` | Present-temperature unit byte: 0 = F, 1 = C |
| `setpoint_device_type` | Device type from the setpoint response |
| `setpoint1_raw`, `setpoint2_raw` | Unscaled device-reported setpoints; second is blank when absent |
| `setpoint_unit` | F or C from the setpoint unit byte; not an assertion of scale |
| `setpoint_units_raw` | Setpoint unit byte: 0 = F, 1 = C |
| `cta_temperature_offset` | Separate CTA Temperature Offset response, unsigned whole degrees |
| `cta_temperature_offset_unit` | F or C for that protocol offset |
| `cta_temperature_offset_basic_dr_opcode` | Optional Basic DR command opcode associated with that offset |

The renamed columns replace `temperature_unit`, `temperature_offset`,
`temperature_offset_unit`, and `temperature_offset_basic_dr_code` in new logs.
Historical CSVs are not rewritten. Temperature and setpoint raw values are now
stored in their own columns, not embedded in `details`. Error responses leave
measurement columns blank. The observed WH-3 display difference (+18 F in its
reported setpoint) and WH-4 setpoint scaling (C / 100) are not applied to raw
logging; the separate CTA Temperature Offset fields do not represent those
station-specific corrections.

Hardware-independent build and logging regressions can be run on Linux with
`python3 test/run_temperature_regressions.py` from `dcs/`. This compiles the
controller without starting it and runs both regression executables in temporary
folders.
