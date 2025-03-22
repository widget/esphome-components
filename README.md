# esphome-components

Components I've been working up to enable the [Lilygo T-Watch 2020 V1](https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library/blob/master/docs/watch_2020_v1.md).
This is not the same as the V2 or V3!

Example YAML will be in a separate repo.

## TODO

- Test YAML power control
- Why is the Fuel gauge returning 127?  Docs say percentage
- Non-polling support
  - Button

## Detail

The AXP202 controls power lines and battery information on the watch.
The absolute minimum it needs to do for the watch is enable LDO2 at 3.3V to run the backlight.

It is not reading the PEK button, it only polls voltages.

Some information also available [here (fr)](http://destroyedlolo.info/ESP/Tech%20TWatch/)

| Power Domain | Use |
|-----|----|
|LDO1| RTC (always on, not controllable)|
|LDO2| LCD VDD 3v0|
|LDO3| SPK_VDD 3V3|
|LDO4| VDD 1V8 but not acctually used maybe?|
|ACIN| N/C|
|DCDC2| N/C|
|GPIO1-3| N/C|
|TS |is connected|

## Credits

AXP202 code is inspired from the esphome-m5stickC repo which has an AXP192 in it.
