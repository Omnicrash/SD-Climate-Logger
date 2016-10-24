/*
  SD Climate Logger
  2016 Yve Verstrepen

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
*/

#include <stdlib.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include "DHT.h"
#include "SparkFunISL29125.h"

#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// The amount of time to wait between samples, in milliseconds
const unsigned int sampleDelta = 10000;

const int logLed = 8; //D8
const int statusLed = 9; //D9

const int gasPin = 0; //A0

// Dust sensor
const int dustPin = 1; //A1
const int dustLed = 3; //D3

// RTC
RTC_DS1307 rtc;

// RGB sensor
SFE_ISL29125 rgb;

const int CS_SD = SS; // Default SS/CS pin (D10)
File dataFile;

// Timestamp to take the next sample
unsigned long nextSampleTime = 0;

// [code] parameter is the number of short pulses to display
void panic(String message, int code)
{
  Serial.println();
  Serial.print("ERROR: ");
  Serial.println(message);
  Serial.println("Halting...");

  // Blink LED to indicate failure
  while (1)
  {
    // Long pulse
    digitalWrite(statusLed, HIGH);
    delay(700);
    digitalWrite(statusLed, LOW);
    delay(300);

    // Short pulse(s)
    for (int i = 0; i < code; i++)
    {
      digitalWrite(statusLed, HIGH);
      delay(200);
      digitalWrite(statusLed, LOW);
      delay(300);
    }
  }
}

void log(String dataString)
{
  dataFile.println(dataString);
  Serial.println(dataString);

  // Commenting out this line would save power, but the file would only be saved every 512 bytes. At 64 bytes, only the last 8 measurements (under 2 minutes at 10s intervals) would be lost.
  dataFile.flush();
}

void setup()
{
  // LEDs
  pinMode(statusLed, OUTPUT);
  pinMode(logLed, OUTPUT);

  // Set status LED while initializing
  digitalWrite(statusLed, HIGH);

  // Serial
  Serial.begin(9600);
  while (!Serial);

  // RTC
  Serial.print("Initializing RTC...");
  if (!rtc.begin())
    panic("RTC initialization failed", 0);
  Serial.println("OK!");

  if (!rtc.isrunning())
  {
    Serial.println("Setting RTC to "  __DATE__  " "  __TIME__ "...");
    // This will set the clock to the time this sketch was compiled
    // Use in case battery was removed
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

    // Flash bot LEDs to indicate update time
    for (int i = 0; i < 5; i++)
    {
      digitalWrite(statusLed, HIGH);
      digitalWrite(logLed, HIGH);
      delay(250);
      digitalWrite(statusLed, LOW);
      digitalWrite(logLed, LOW);
      delay(250);
    }
    digitalWrite(statusLed, HIGH);
  }

  // DHT22 temperature/humidity sensor
  Serial.print("Initializing DHT22...");
  dht.begin();
  Serial.println("OK!");

  // ISL29125 RGB sensor
  Serial.print("Initializing ISL29125...");
  // Default init function sets the sensor to 10k lux, 16-bit registers per color and high IR compensation
  if (!rgb.init())
    panic("ISL29125 initialization failed", 3);
  // Uncomment for low light (375 lux) mode
  //rgb.config(CFG1_MODE_RGB | CFG1_375LUX, CFG2_IR_ADJUST_HIGH, CFG_DEFAULT);
  Serial.println("OK!");
  
  // Dust sensor
  pinMode(dustLed, OUTPUT);

  // SD Card
  Serial.print("Initializing SD...");
  pinMode(CS_SD, OUTPUT);
  if (!SD.begin(CS_SD))
    panic("SD initialization failed", 1);
  Serial.println("OK!");

  // Create filename with timestamp in format 'MMDDhhmmss.txt'
  char filename[12];
  DateTime now = rtc.now();
  sprintf(filename, "%02d%02d%02d%02d.csv", now.month(), now.day(), now.hour(), now.minute());

  // Open file for writing
  Serial.print("Opening '");
  Serial.print(filename);
  Serial.print("'...");
  dataFile = SD.open(filename, FILE_WRITE);
  if (!dataFile)
    panic("Could not open file for writing!", 2);
  Serial.println("OK!");

  // Write header
  log("Timestamp,Date,Time,Temperature,Humidity,Red,Green,Blue,Gas,Dust");

  // Set initial LED status
  digitalWrite(logLed, HIGH);
  digitalWrite(statusLed, LOW);
}

void loop()
{
  String dataString = "";
  
  // Timestamp
  dataString = String(rtc.now().unixtime()) + ",,,";
  
  // Temperature data
  float t = dht.readTemperature();
  char temp[6]; // 6 characters, including null terminator, allows for a range of (-99.9, 999.9)
  dtostrf(t, 5, 1, temp); // 5 = width, 1 = precision
  dataString += temp;
  dataString += ",";
  
  // Humidity data
  float h = dht.readHumidity();
  char humidity[6];
  dtostrf(h, 5, 1, humidity); // 5 = width, 1 = precision
  dataString += humidity;
  dataString += ",";
  
  /*
  if (isnan(t) || isnan(h))
  {
    Serial.println("Failed to read DHT22 data!");
    digitalWrite(statusLed, HIGH);
    return;
  }
  */

  // Light (RGB) data
  dataString += String(rgb.readRed()) + ",";
  dataString += String(rgb.readGreen()) + ",";
  dataString += String(rgb.readBlue()) + ",";
  
  // Gas (MQ-135) sensor
  dataString += String(analogRead(gasPin)) + ",";
  
  // Dust (GP2Y1010AU0F) sensor
  digitalWrite(dustLed, LOW); // LED ON
  delayMicroseconds(280);
  int dust = analogRead(dustPin);
  delayMicroseconds(40);
  digitalWrite(dustLed, HIGH); // LED OFF
  delayMicroseconds(9680);
  
  dataString += String(dust);
  
  // Store the data
  log(dataString);
  
  // Successful sample taken, clear status LED
  digitalWrite(statusLed, LOW);

  // Correct delta time compensating for sampling time
  nextSampleTime += sampleDelta;
  int delta = nextSampleTime - millis();

  // Wait for [delta] seconds, and blink the log LED to indicate we're still up and running
  Serial.print("Waiting for ");
  Serial.print(delta);
  Serial.println("ms...");
  while (delta > 0)
  {
    delay(50);
    digitalWrite(logLed, LOW);
    if (delta > 1000)
      delay(950);
    else
      delay(delta - 50);
    digitalWrite(logLed, HIGH);
    delta -= 1000;
  }
}

