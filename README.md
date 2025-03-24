# esphome-components

Components I've been working up to enable the [Lilygo T-Watch 2020 V1](https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library/blob/master/docs/watch_2020_v1.md).
This is not the same as the V2 or V3!

## References

- https://t-watch-document-en.readthedocs.io/en/latest/introduction/product/2020.html
- https://github.com/Xinyuan-LilyGO/LilyGo-HAL

## TODO

- Test YAML power control
  - Replace booleans with switches
- Why is the Fuel gauge returning 127 when it's not charging?  Docs say percentage
- Non-polling support
  - usb disconnect doesn't send a charge cancelled interrupt

## Detail

The AXP202 controls power lines and battery/voltage/charging information on the watch.
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

## YAML

```yaml
substitutions:
  esp_name: "watch"

esphome:
  name: "$esp_name"
  friendly_name: "$esp_name"

external_components:
  - source: github://widget/esphome-components@main
    components: [ axp202 ]

esp32:
  board:  ttgo-t-watch
  framework:
    type: esp-idf
    version: recommended
  flash_size: 16MB
  
psram:
  mode: octal
  speed: 80MHz
  
# Enable logging
logger:
  level: DEBUG

# Enable Home Assistant API
api:

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

i2c:
  - id: tt_sensor
    sda: 21
    scl: 22
# [22:19:46][I][i2c.idf:102]: Results from i2c bus scan:
# [22:19:46][I][i2c.idf:108]: Found i2c device at address 0x19 <- BMA423
# [22:19:46][I][i2c.idf:108]: Found i2c device at address 0x35 <- AXP202
# [22:19:46][I][i2c.idf:108]: Found i2c device at address 0x51 <- RTC

  - id: tsc # ft6336 only
    sda: 23
    scl: 32
  
spi:
  - id: tft
    mosi_pin: 19
    clk_pin: 18
    interface: hardware  

i2s_audio:
  - id: i2s_out
    i2s_lrclk_pin: 25 # also known as Word Select
    i2s_bclk_pin: 26

speaker:
  - platform: i2s_audio
    dac_type: external
    i2s_audio_id: i2s_out
    i2s_dout_pin: 33

axp202:
  i2c_id: tt_sensor
  backlight: true
  speaker: false
  interrupt_pin:
      number: 35

binary_sensor:
  - platform: axp202
    charging:
      name: charging
    usb:
      name: usb

sensor:
  # this turns on power but doesn't actually need brightness control as that's done later
  - platform: axp202
    bus_voltage:
      name: "USB Voltage"
    battery_voltage:
      name: "Battery Voltage"
    battery_level:
      name: "Battery"

output:
  - platform: ledc
    pin: 
      number: 12
      ignore_strapping_warning: true
    id: backlight_pwm

light:
  - platform: monochromatic
    id: led
    entity_category: config
    output: backlight_pwm
    default_transition_length: 500ms
    restore_mode: RESTORE_AND_ON
    # on_turn_on:
    #   - delay: 30s 
    #   -  light.turn_off: led
      # - display.page.show: home_page

time:
  - platform: pcf8563
    id: rtc_time
    i2c_id: tt_sensor
    address: 0x51
    update_interval: never

  - platform: homeassistant
    on_time_sync:
      then:
        pcf8563.write_time:

# IR tx unused
remote_transmitter:
  pin: 13
  carrier_duty_percent: 50%

font:
  - file:
      type: gfonts
      family: Roboto
    id: roboto
    size: 25

touchscreen:
  - platform: ft63x6
    interrupt_pin: 
      number: 38
    address: 0x38
    threshold: 200
    display: screen
    id: tscreen
    i2c_id: tsc
    calibration:
      x_min: 0
      x_max: 240
      y_min: 0
      y_max: 240
    # on_touch:
    #   - light.turn_on: 
    #       id: led
    #       brightness: 50%

color:
  - id: green
    hex: '75D15F'
  - id: red
    hex: 'FF3131'
  - id: blue
    hex: '47B7E9'
  - id: blue_drk
    hex: '085296'
  - id: amber
    hex: 'FBAB35'
  - id: lime
    hex: '20FC30'
  - id: pink
    hex: 'D92BBC'
  - id: yellow
    hex: 'FFC000'
  - id: black
    hex: '000000'
  - id: white
    hex: 'ffffff'
  - id: purple
    hex: '73264D'

display:
  - platform: ili9xxx
    model: ST7789V
    id: screen
    transform:
      mirror_x: true
      mirror_y: true
      swap_xy: false
    dimensions:
      width: 240
      height: 240
      offset_width: 0
      offset_height: 80 # not sure why this fixes it
    # no reset_pin
    invert_colors: true
    dc_pin: 27
    data_rate: 80MHz
    cs_pin:
      number: 5
      ignore_strapping_warning: true
    pages:
      - id: home_page
        lambda: |-
          it.printf(10, 2, id(roboto), purple, "test1");       
          it.printf(80, 170, id(roboto), blue_drk, "test2"); 
```
