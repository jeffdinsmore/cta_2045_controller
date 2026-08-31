# One-shot Commodity diagnostic

This diagnostic opens a CTA-2045 serial connection, sends one Commodity Read,
prints every returned commodity record, queries the operational state once,
prints link/application ACK or NAK activity, and exits. It does not run a
schedule, start periodic polling, or save CSV/event data.

From the `dcs` directory, build and run it with the default `/dev/ttyUSB0` port:

```sh
make commodity-read
```

To use a different serial port:

```sh
make diagnostic
./build/debug/cta2045_commodity_read /dev/ttyAMA0
```

Show command help with:

```sh
./build/debug/cta2045_commodity_read --help
```
