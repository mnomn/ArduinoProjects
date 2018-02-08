/*

  Based on HelloWorld.ino from 
  
  "Hello World" version for U8x8 API

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  

*/

#include <Arduino.h>
#include <ESPWebConfig.h> // github.com/mnomn/ESPWebConfig
#include <U8g2lib.h>
//#include <U8x8lib.h>
#include <MQTTClient.h>
#include <ESP8266WiFi.h>
// #include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>

#include "local_secret.h"


const char* MURL = "url|MQTT broker";
const char* MPORT = "MQTT number";
const char* MUSER = "MQTT number";
const char* MPASS = "MQTT number";
const char* TTOPIC = "Temp topic";
const char* HTOPIC = "Humid topic";

String parameters[] = {MURL, MPORT, MPASS, MUSER, TTOPIC, TTOPIC};

WiFiClient net;
MQTTClient client;
ESPWebConfig espConfig(NULL, parameters, 4);

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

char temp[8];
char hum[8];
char tin[8];
char preas[8];
unsigned long temptime_ms = 0;


/* Pins
 * WEMOS: D1/GPIO5/SCL
 *        D2/GPIO4/SDA
 */

// The complete list is available here: https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
// Full frame constructor
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA);   // pin remapping with ESP8266 HW I2C

// The complete list is available here: https://github.com/olikraus/u8g2/wiki/u8x8setupcpp
// U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA);   // pin remapping with ESP8266 HW I2C

unsigned long nextRefresh = 0;

Adafruit_BMP280 bm; // I2C

void setup(void)
{
  Serial.begin(74880);
  while(!Serial);
  Serial.println("");
  delay(10);
  Serial.println("Start");

  // Display
  u8g2.begin();
  // u8x8.begin();
  // u8x8.setPowerSave(0);
  
  WiFi.begin(WIFISSID, WIFIPWD);
  client.begin(MQTTBROKER, net);
  client.onMessage(messageReceived);

  espConfig.setup();

  mqttConnect();

  if (!bm.begin(0x76)) {
    Serial.println("BME ERROR!!!!");
  }  else {
    Serial.println("BME Connected");
  }

  // Initialize some strings...
  strcpy(temp, "99.9");
  strcpy(hum, "99%");

}

void loop(void)
{
  client.loop();
  delay(10);

  if (!client.connected()) {
    mqttConnect();
  }

  unsigned long now = millis();
  if (now > nextRefresh) {
    char kladd[8];
    nextRefresh = nextRefresh + 5000; // 5 sec later

    readIndoors();

    u8g2.setContrast(now%256);
    u8g2.clearBuffer();
    PrintLayout1();
    sprintf(kladd, "%lu", now%256);
    u8g2.drawStr(40, 32, kladd);
    u8g2.sendBuffer();
  }
}

void PrintLayout1() {
    u8g2.setFont(u8g2_font_fub17_tf);
    u8g2.drawStr(0,19,temp);
    unsigned int width1 = u8g2.getStrWidth(temp);

    u8g2.drawStr(width1 + 9, 19,tin);
    unsigned int width2 = u8g2.getStrWidth(tin);

    u8g2.setFont(u8g2_font_fub11_tf);
    u8g2.drawStr(width1 + 9 + width2 + 3, 13, "C");

    u8g2.setFont(u8g2_font_fur11_tf);
    u8g2.drawStr(0, 32, hum);
}

void PrintLayout2() {
    u8g2.setFont(u8g2_font_fub17_tf);
    u8g2.drawStr(0,19,temp);
    unsigned int width1 = u8g2.getStrWidth(temp);

    u8g2.drawStr(width1 + 9, 19,tin);
    unsigned int width2 = u8g2.getStrWidth(tin);

    u8g2.setFont(u8g2_font_fub11_tf);
    u8g2.drawStr(width1 + 9 + width2 + 3, 13, "C");

    u8g2.setFont(u8g2_font_fur11_tf);
    u8g2.drawStr(0, 32, hum);
}

void printProgressBar() {
  unsigned long age_sec = (millis() - temptime_ms)/1000;
  if (age_sec > 5/* *60 */) { // Do not show progress bar the first 5min
    #define bar_time_sec (30*60)
    #define bar_len_px (120)
    unsigned long px = age_sec/bar_time_sec*bar_len_px;
    if (px > bar_time_sec) px = bar_time_sec;

    u8g2.drawLine(0, 31, px, 31);
  }

}

void readIndoors() {
    float pf = bm.readPressure();
    float tinf = bm.readTemperature();

    String temporary = String(tinf);

    strcpy(tin, temporary.c_str());
    Serial.print("BME t: ");
    Serial.print(temporary);
    //Serial.print(", h: ");
    //Serial.print(h);
    Serial.print(", p: ");
    Serial.print(pf/133.322365);
    Serial.println(" hg");

    SetNoOfDecimals(tin, 1);
}

void mqttConnect() {
  Serial.print("checking wifi...");
  int maxcount = 100;
  while (WiFi.status() != WL_CONNECTED && maxcount--) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("arduino2", MQTTUSER, MQTTPASS) && maxcount--) {
    Serial.print(".");
    delay(1000);
  }
  if (maxcount <= 0) {
    ESP.restart();
  }

  Serial.println("\nconnected!");

  client.subscribe(MQTTTOPIC);
  client.subscribe(MQTTTOPIC2);
}

void messageReceived(String &topic, String &payload) {

  int ll = payload.length();
  int dot = payload.indexOf('.');
  Serial.println("incoming: " + topic + " - " + payload + " l:" + ll);

  if (topic.endsWith("d5t")) {
    temptime_ms = millis();
    strncpy(temp, payload.c_str(), 8);
    SetNoOfDecimals(temp, 1);
  } else if (topic.endsWith("d5h")) {
    if (dot > -1) {
      // Do not print decimal on hum 
      String hstr = payload.substring(0, dot);
      hstr = hstr += '%';
      strncpy(hum, hstr.c_str(), 8);
    } else {
      strncpy(hum, payload.c_str(), 8);
    }
  }
}

void SetNoOfDecimals(char *str, int noOfDecimals) {
  char *dot = strchr(str, '.');
  Serial.print("DOT");
  Serial.println(dot);
  if (noOfDecimals == 0 && dot) {
    *dot = '\0';
    return;
  } 
  dot++;
  int l = strlen(dot);
  Serial.println(l);
  if (l > noOfDecimals) {
    *(dot + noOfDecimals ) = '\0';
  }
}