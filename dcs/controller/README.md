# CTA-2045 Controller

This directory contains the executable application that communicates with a
CTA-2045 device over the serial port configured in `main.cpp` (currently
`/dev/ttyUSB0`).

- `main.cpp` opens the serial connection, runs the command loop, and processes
  scheduled commands.
- `UCMImpl.cpp` and `UCMImpl.h` handle responses received from the water heater.
- `schedule.csv` is the live command schedule.
- `create_schedule.sh` creates a new schedule and archives the original entries.

From the `dcs` directory, use `make`, `make schedule`, or `make run`.
