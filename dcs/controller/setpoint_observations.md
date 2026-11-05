# Water-heater setpoint observations

Observed on November 4, 2026. Station labels identify these observations, not
all devices of the same type, brand, or protocol revision.

The supplied ANSI/CTA-2045-B definition identifies Set Point 1 as the water
heater top-element setpoint and Set Point 2 as the bottom-element setpoint.
Setpoints are signed 16-bit values; `0x8000` means not supported on GET and
"don't change" on SET. The supplied example encodes 72 degrees F as `00 48`
(whole degrees). Present Temperature is a separate field with hundredths-of-a-
degree encoding; do not assume the two fields have the same scale.

## WH-3: reported top-element setpoint versus display

| Display setting (F) | Reply payload | Reported Set Point 1 (F) | Reported minus display (F) |
| --- | --- | --- | --- |
| 125 | `03 83 00 00 03 00 00 8F` | 143 | +18 |
| 130 | `03 83 00 00 03 00 00 94` | 148 | +18 |
| Restored to 125 | `03 83 00 00 03 00 00 8F` | 143 | +18 |

Evidence: station 3 run `read_check_2026_11_04_132903_sjUHYB`.
The initial 143 reply occurred at 13:29:03.980 PST; 148 replies occurred from
13:40:42.786 through 13:43:16.391; the restored 143 reply occurred at
13:44:10.798. All are successful eight-byte replies with device type 3,
Fahrenheit units, and a single setpoint. The second setpoint is absent.

This establishes a repeatable +18 F difference (10 C temperature difference)
at the two tested display settings. It does not establish the cause, prove a
constant correction across other settings or modes, or demonstrate use of the
protocol's Temperature Offset command. The standard says GetSetpoint excludes
that protocol offset. Manufacturer confirmation is still needed to explain
this device's behavior.

Preserve the reported value as-is: 143 stays 143, and 148 stays 148. Do not
subtract 18 F automatically or relabel the reported top-element value as the
user's displayed setting. Record the display reading separately when comparing.

The attempted SET at 13:32:15.435 returned `03 83 01` (command not implemented).
It did not report a successful change; subsequent GET still returned 143.

## WH-4: apparent scale difference remains device-specific

Station 4 run `read_check_2026_11_04_130840_haqUDp` repeatedly returned
`03 83 00 00 03 01 14 28`: success, device type 3, Celsius units, value 5160.
Dividing this value by 100 gives 51.60 C (124.88 F), close to the displayed
125 F. This suggests a device-specific scale difference from the supplied
whole-degree example; it does not authorize a universal setpoint conversion.
Preserve 5160 and its unit code as received until that behavior is verified.

## Saved data

Startup `cta_device_information.csv` retains `setpoint1_raw`, optional
`setpoint2_raw`, and `setpoint_units` (0 = F, 1 = C). Interactive GET replies
are recorded in `cta_events.csv`; exact payload bytes are in
`cta_raw_messages.csv` under `get_set_setpoints_reply`. These paths already
preserve the device-reported numbers without an offset correction. Existing
CSV files are not modified to add these observations.
