avrdude -v -p t84 -c avrisp2 -P usb -U lfuse:w:0xe2:m -U hfuse:w:0xd2:m -U efuse:w:0xfe:m
pause
avrdude -p t84 -P usb -c avrisp2 -U flash:w:bootloader_T.hex:i
pause
avrdude -p t84 -P usb -c avrisp2 -U eeprom:w:eeprom_b.eep:i"


