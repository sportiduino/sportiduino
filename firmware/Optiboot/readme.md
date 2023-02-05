## Building Optiboot for Base station

```sh
cd <Optiboot-local-repo>/optiboot/bootloaders/optiboot

# Bootloader without LED flashes
make AVR_FREQ=8000000L BAUD_RATE=38400 LED_START_FLASHES=0 LED=D4 atmega328
mv optiboot_atmega328.hex optiboot_38400_sportiduino.hex

# Bootloader with LED starting flashes
make AVR_FREQ=8000000L BAUD_RATE=38400 LED_START_FLASHES=3 LED=D4 atmega328
mv optiboot_atmega328.hex optiboot8_38400_sportiduino_led.hex
```

