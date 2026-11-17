# Troubleshooting

Problems we actually ran into while building this project, with the cause and fix. Rule of thumb: **test each module with its sketch in `firmware/tests/` before blaming the combined firmware.**

## RFID

### Startup line shows `Firmware Version: 0x0`, `0xFF` or `0x1 = (unknown)`
The Arduino is not talking to the reader properly. Check, in order:
1. **Supply** is 3.3 V (not 5 V) and GND is connected.
2. **MOSI/MISO** not swapped (MOSI -> D11, MISO -> D12) and **RST** -> D9, **SDA/SS** -> D10.
3. **Solder joints** on the reader's header: they should be shiny and cone-shaped. Wiggle each wire; if the version value changes, reflow that pin.

A good result is `0x91` or `0x92`.

### Scan prints nothing at all
`checkRFID()` is returning early, meaning no card was detected - this is a wiring/solder problem, not a UID problem. Fix the version check first.

### Scan prints the UID but says `Access Denied`
- Compare the stored UID to the scanned one. Digits such as `8` and `B` look alike in some fonts - copy the bytes exactly as printed.
- Make sure you **uploaded** the sketch (not just *Verify*) after editing `config.h`; the board may still run an older build.
- Uploading `rfid_only_test` prints the stored UID at startup so you can compare both lines in the same run.

## Servo

### Serial says "unlocking" but the servo does not move (works alone)
Power and ground, not code:
- Power the servo from the **external 5 V adapter**, not the Arduino 5 V pin.
- **Tie adapter GND to Arduino GND.** Without the shared ground the PWM signal has no reference and the servo ignores it.
- The servo needs **no relay**.

## Temperature / fan

### Readings like `218 C` or random values
That is an analog-read of a floating pin - the code was written for an LM35 on A0 but a DHT11 was connected. The DHT11 is digital: wire DATA to **D2** and use the DHT library.

### `Failed to read from DHT11 - check wiring`
Check DATA is on D2, VCC has power and the solder joints are good. The DHT11 can be read at most once per second.

### Fan stays on after the temperature falls
Not a bug in the combined firmware: the final sketch switches off the moment the reading drops below `TEMP_THRESHOLD`. The standalone `dht11_fan_test` includes a 1 deg C hysteresis, so it only switches off below `threshold - 1`.

## Pump / ultrasonic

### Serial says `Pump: ON` but nothing happens
See *Relays* below - it is almost always the relay wiring or relay polarity.

### Distance is always 0 or wildly wrong
Check Trig -> D7 and Echo -> D8, 5 V on VCC, and that nothing is closer than 2 cm to the sensor. Readings time out at ~5 m.

## Relays

### Relay LED lights, but the fan/pump is off (and the reverse)
Relay modules differ in polarity (active-HIGH vs active-LOW) and in which terminal you used (NO vs NC). Observe the behaviour:
- Load runs when the relay LED is **off** and the firmware says ON -> active-LOW module wired to **NC**. This is how our build works.
- Load runs when the relay LED is **on** -> wired to NO on an active-HIGH module (or active-LOW module and inverted logic).

Fix by moving the load wire between NO and NC, or by swapping `HIGH`/`LOW` in the sketch. Details: [wiring.md](wiring.md#relay-wiring-comnonc).

### Relay does not click at all
Check IN -> D3/D4, relay VCC 5 V and GND. On 2-channel boards, check the **VCC/JD-VCC jumper**.

## LCD

### Backlight on, but no text (contrast pot has no effect)
1. Run `i2c_scanner`. If it finds `0x27` (or `0x3F`), the I2C bus works - update `LCD_I2C_ADDR` if different.
2. Run `lcd_block_test` and sweep the contrast pot. If no blocks appear at any setting, the problem is the **backpack-to-LCD solder joints** (RS, E, D4-D7, plus RW). Reflow them and look for solder bridges between adjacent pins.

### Garbled characters
Re-adjust contrast; check the SDA/SCL wires are firm.

## Upload / IDE

- **Library not found:** install the libraries listed in the README, then restart the IDE.
- **Board unchanged after "upload":** confirm the log ends with *Done uploading*, the right **Port** is selected, and Serial Monitor is closed during upload.
- **Garbage in Serial Monitor:** set the baud rate to **9600**.
