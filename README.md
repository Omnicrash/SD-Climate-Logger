# SD-Climate-Logger
An Arduino UNO based climate logger

This sketch will log a variety of sensors to SD card using the Adafruit Data Logging shield.

Requires the following libraries:  
https://github.com/sparkfun/SparkFun_ISL29125_Breakout_Arduino_Library  
https://github.com/adafruit/RTClib  
https://github.com/adafruit/SD  
https://github.com/adafruit/DHT-sensor-library  

```
Digital Pins
------------
Serial (could be removed):
  D0: RX
  D1: TX
Sensors:
  D2: DHT22 (Temp/Humidity)
  D3: Dust sensor LED
LED:
  D8: Logging LED
  D9: Status LED
SD & RTC Shield (SPI):
  D10: Chip Select
  D11: MOSI
  D12: MISO
  D13: CLK

Analog Pins
-----------
  A0: MQ-135 (Gas)
  A1: GP2Y1010AU0F (Dust)
ISL29125 RGB (I2C):
  A4: SDA
  A5: SDL

Error Codes
-----------
  1: RTC error
  2: SD card error
  3: File error
  4: Sensor/hardware error
  BOTH LEDS FLASH 5x: RTC time reset to sketch compilation timestamp
```
