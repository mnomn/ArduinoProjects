/*

  Based on HelloWorld.ino from u8g2

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
#include <ESPXtra.h> // https://github.com/mnomn/ESPXtra
#include <U8g2lib.h>
//#include <U8x8lib.h>
#include <MQTTClient.h> // MQTT by Joel G in lib manager or https://github.com/256dpi/arduino-mqtt
#include <ESP8266WiFi.h>
// #include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h> // Also install Adafruit Unified Sensor


// Calibrate temp
#define TEMP_OFFSET 1.0
#define TIN_OFFSET -1.5

const char* MURL = "Broker*";
const char* MPORT = "number|Port";
const char* MUSER = "User";
const char* MPASS = "Pass";
const char* TTOPIC = "Temp topic";
const char* HTOPIC = "Humid topic";

String parameters[] = {MURL, MPORT, MUSER, MPASS, TTOPIC, HTOPIC};

WiFiClient net;
MQTTClient mqttClient;
ESPWebConfig espConfig(NULL, parameters, 6);
ESPXtra espXtra;

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

char *topicT;
char *topicH;
char *mqttUser;
char *mqttPass;

byte displayNumber = 0;
int configMode = 0;
int contrast = 0;

#define NR_OF_DISPLAYS 4

void setup(void)
{
//   Serial.begin(74880);
  u8g2.setDisplayRotation(U8G2_R0);
  Serial.begin(9600);
  while(!Serial);
  Serial.println("");
  delay(10);
  Serial.println("Start");

  // Display
  u8g2.begin();
  u8g2.setContrast(contrast);
  PrintSendStartScreen();

  espConfig.setHelpText("MQTT:");
  int res = espConfig.setup();
  displayNumber = espConfig.getRaw(511);
  Serial.print("Wifi connected ");
  Serial.print(res);
  Serial.print(", disp ");
  Serial.println(displayNumber);

  char *broker = espConfig.getParameter(MURL);
  mqttUser = espConfig.getParameter(MUSER);
  mqttPass = espConfig.getParameter(MPASS);
  topicT = espConfig.getParameter(TTOPIC);
  topicH = espConfig.getParameter(HTOPIC);

  char *port = espConfig.getParameter(MPORT);
  if (strlen(port) > 1) {
    mqttClient.begin(broker, atoi(port), net);
  }
  else {
    mqttClient.begin(broker, net);
  }

  mqttClient.onMessage(messageReceived);

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
  mqttClient.loop();
  delay(10);
  unsigned long now = millis();

  if (configMode) {
    digitalWrite(LED_BUILTIN, (now/1000)%2 == 0?HIGH:LOW);
    exit;
  }

  int pressed = espXtra.ButtonPressed(0);
  if (pressed > 5) {
    if (!configMode) {
      espConfig.clearConfig();
    }
    configMode = 1;
  } else if (pressed > 0) {
    displayNumber = (displayNumber+1)%NR_OF_DISPLAYS;
    espConfig.setRaw(511, displayNumber);
    Serial.print("Display ");
    Serial.println(displayNumber);

    nextRefresh = now;
    delay(500);
  }

  if (now >= nextRefresh) {
    nextRefresh = nextRefresh + 15000; // 15 sec later
    readIndoors();

    u8g2.clearBuffer();
    if (displayNumber/2==0)
    {
      // Display 0 and 1
      PrintLayoutA(displayNumber);
    }
    else
    {
      PrintLayoutB(displayNumber);
    }
    u8g2.sendBuffer();
  }

  if (!mqttClient.connected()) {
    mqttConnect();
  }
}

void PrintSendStartScreen() {
  u8g2.setFont(u8g2_font_fub17_tf);
  u8g2.drawStr(2,22,"Solstickan");
  u8g2.sendBuffer();
}

void PrintLayoutA(int rotation) {
  u8g2.setDisplayRotation(rotation%2?U8G2_R0:U8G2_R2);

  u8g2.setFont(u8g2_font_fub17_tf);
  u8g2.drawStr(0,19,temp);
  unsigned int width1 = u8g2.getStrWidth(temp);

  int col2 = width1 + 35;

  u8g2.drawStr(col2, 19,tin);
  unsigned int width2 = u8g2.getStrWidth(tin);

  u8g2.setFont(u8g2_font_fub11_tf);
//    u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.drawStr(width1 + 3, 13, "C");
  u8g2.drawStr(col2 + width2 + 3, 13, "C");

  // u8g2.setFont(u8g2_font_fur11_tf);
  u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.drawStr(0, 30, hum);
  u8g2.drawStr(col2, 30, preas);

}

void PrintLayoutB(int rotation) {
  u8g2.setDisplayRotation(rotation%2?U8G2_R0:U8G2_R2);
//    u8g2.setFont(u8g2_font_fub25_tf);
  u8g2.setFont(u8g2_font_helvB24_tf);
  u8g2.drawStr(0,30,temp);
  unsigned int width1 = u8g2.getStrWidth(temp);
  u8g2.setFont(u8g2_font_fub11_tf);
  u8g2.drawStr(width1 + 2, 13, "c");

//    u8g2.setFont(u8g2_font_courB08_tf);
  u8g2.setFont(u8g2_font_helvB08_tf);
  int col2 = 90;
  int col3 = 104;
  u8g2.drawStr(col3, 10, tin);
  width1 = u8g2.getStrWidth(tin);
  u8g2.drawStr(col3 , 20, hum);
  u8g2.drawStr(col2, 30, preas);
  // Write a small c after temp
  u8g2.drawStr(col3 + width1 + 1, 8, "c");
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
    // Adjust for heat from mcu and display.
    tinf += TIN_OFFSET;

    // String temporary = String(tinf);
    // strcpy(tin, temporary.c_str());
    // SetNoOfDecimals(tin, 1);

    // Simpler without decimals...
    snprintf(tin, sizeof tin, "%d", round(tinf));

    pf = pf/133.322365;
    sprintf(preas, "%d Hg", (int)pf);

    // Debug prining
    Serial.print("BME tinf: ");
    // Serial.print(temporary);
    Serial.print(tinf);
    Serial.print(", p: ");
    Serial.println(pf);
}

int mqttConnect() {
  Serial.print("checking wifi...");
  int maxcount = 10;
  while (WiFi.status() != WL_CONNECTED && maxcount--) {
    Serial.print(".");
    delay(1000);
  }

#ifdef DEBUG_PRINT
  Serial.println("\nconnecting broker...");
  Serial.println(mqttUser);
  Serial.print(mqttPass);
#endif
  int res = mqttClient.connect("arduino223", mqttUser, mqttPass);

  if (!res) {
    Serial.println("\nFailed to connect");
    return false;
  }

  Serial.println("\nconnected!");

  mqttClient.subscribe(topicT);
  mqttClient.subscribe(topicH);

  return true;
}

void messageReceived(String &topic, String &payload) {

  int ll = payload.length();
  int dot = payload.indexOf('.');
  Serial.println("incoming: " + topic + " - " + payload + " l:" + ll);

  if (topic.endsWith("d5t")) {
    temptime_ms = millis();
    float f = payload.toFloat();
    // Convert to float so we can calibrate, then convert back to string
    f += TEMP_OFFSET;
    snprintf(temp, sizeof(temp), "%d", round(f), 8);
    // SetNoOfDecimals(temp, 1);
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
  if (noOfDecimals == 0 && dot) {
    *dot = '\0';
    return;
  }
  dot++;
  int l = strlen(dot);
  if (l > noOfDecimals) {
    *(dot + noOfDecimals ) = '\0';
  }
}
