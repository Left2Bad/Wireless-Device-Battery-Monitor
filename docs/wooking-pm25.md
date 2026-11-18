# Wooking PM25 (personal build)

Tested on the owner's Windows PC on 2026-11-19, over the 2.4 GHz USB receiver.

- USB VID:PID: `1EA7:0064`; product string: `2.4G Mouse`.
- Vendor collection: usage page `FFB5`, usage `0001`.
- Input/output: 8 bytes including report ID `B5`; no feature reports.
- Request: `B5 01 00 00 00 00 00 00`.
- Captured reply: `B5 01 01 38 62 3C 00 72`.
- Byte 3 (`38` hex) is the firmware's percentage: 56% at the time of the capture.

The generic HID battery fallback cannot handle this interface because its descriptor has no standard battery usage. The new driver sends only the status request above, accepts a full eight-byte response with `B5 01 01` and percentage 0..100, and ignores invalid packets. Requests are restricted to the tested USB ID and collection shape. Other USB IDs are not probed. No configuration, pairing, DPI or firmware commands are sent.

The ID is shared by several OEM products. The model name in this personal build is based on the owner's identification; this is not automatic model detection. Multiple receivers with the same ID are not supported by this driver.

## Limits

The program displays the percentage returned by the mouse firmware. Accuracy over a full discharge cycle has not been established; some related firmware can keep reporting a fixed percentage. The other response bytes have not been decoded for this unit, so voltage and charging state are not inferred. A missing or sleeping mouse that does not answer is omitted until it responds again.

Protocol starting point (different mouse/USB ID, then verified on this PM25):
https://github.com/ksanjeev284/acer-popgo-companion/blob/main/docs/PROTOCOL_MAP.md

## Build and checks

Run `build.bat` with MinGW-w64 `g++` and `windres` on PATH, or use the Makefile.
This build also includes a header compatibility workaround in `xinput.cpp` for MinGW's duplicate `IReference<boolean>` / `IReference<BYTE>` definitions (w64devkit 2.11.0).
`make test` includes the captured PM25 frame, 0/100 boundaries, invalid response and device filtering, failed writes and timeouts using a simulated transport. `WirelessBattery.exe --list` performs a real battery query. `--dump` lists HID collections.
