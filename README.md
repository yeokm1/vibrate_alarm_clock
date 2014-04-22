vibrate_alarm_clock
===================

An alarm clock with vibration capabilities. During an alarm, plays the Super Mario tune. Buttons available to set the date and alarm. (Date setting not completed)

![Screen](/misc/front.jpg)

Front shot.

![Screen](/misc/internal.jpg)

Just for the curious.


<b>Main Parts used:</b>  

1. Sparkfun Arduino Fio V3 
2. Adafruit 128x64 OLED set to I2C  
3. Chronodot v2.1 Real Time Clock (Based on DS3231 temperature compensated RTC crystal.)  
4. 2x button switches  
5. 2x 10k ohm resistors  
6. 8ohm thin speaker  
7. 100 ohm resistor  
8. Pololu DRV8833 dual motor driver  
9. 10mm vibration motor


Optional:  

1. Lithium battery  
3. 2x half-breadboard 
4. Translucent case  



![Screen](misc/schematic-vibrate-clock.png)

This schematic only represents the logical connections I made. The physical connections differs due to space issues. The DRV8833 motor driver is incorrectly represented by another driver as Fritzing does not have this component. Pin A1 on the Arduino is connected to the sleep pin on the DRV8833.

<b>Stuff to note:</b>  
1. The Arduino Fio V3 is quite a finicky thing. Occasionally it will repeatedly refuse to accept code uploads and just disconnect the USB connection. Unplugging/Replugging the USB cable even to another USB port does not work. Pressing the reset button three times before an upload seems to make it work again.  
2. The OLED is set by default to use the SPI interface. If you want to use I2C, remember to solder the jumpers at the back I used I2C to reduce the wire clutter.  
3. Super Mario tune is contained in the tune.h header file. To same memory, the tune is stored into flash memory via PROGMEM to minimise SRAM usage.  



References and libraries:  

1. Adafruit SSD1306 OLED library  
(https://github.com/adafruit/Adafruit_SSD1306)

2. Get day of week from date.  
(http://stackoverflow.com/a/21235587)



The MIT License (MIT)<br>
Copyright (c) 2014-2014 Yeo Kheng Meng<br>
