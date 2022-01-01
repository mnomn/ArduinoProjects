/*
 * Sketch for Sonoff WIFI Smart Switch or other ESP8266 based switch.
 */

#include "Arduino.h"
#include <ESPWebConfig.h> // https://github.com/mnomn/ESPWebConfig
#include <ESPXtra.h>      // https://github.com/mnomn/ESPXtra
#include <ArduinoOTA.h>

/*
Sonoff WIFI Smart Switch or other ESP8266 device:
Arduino serttings: Generic esp8266, Flash size 1MB (or more) (fs: none, OTA: 502KB)

Default pins (Sonoff)
Relay: GPIO 12
Button: GPIO 0
Led: GPIO 13
*/
ESP8266WebServer server(80);

char *user = NULL;
char *pass = NULL;
const char* USER_KEY = "User:";
const char* PASS_KEY = "Pass:";
String parameters[] = {USER_KEY, PASS_KEY};
ESPWebConfig espConfig("configpass", parameters, 2);
ESPXtra espx;

unsigned long lastButtonTime = 0;
int otaEnabled = 0;

#define ESP01 1

const int buttonPin = 0;
#define LED_ON HIGH

# ifdef ESP01
const int relayPin = 3; // RX pin on ESP 01
const int ledPin = 1; // TX pin on ESP-01
#else
const int relayPin = 12;
const int ledPin = 13;
#endif


#ifdef SSS_DEBUG
#define SSS_PRINT(x) (Serial.print(x))
#define SSS_PRINTLN(x) (Serial.println(x))
#else
#define SSS_PRINT(x)
#define SSS_PRINTLN(x)
#endif

void handleRoot() {
  server.send(200, "text/html", F("<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
  "<style type=text/css>"
  "body { margin:5%; font-family: Arial;} form p label {display:block;float:left;width:100px;}"
  "</style></head>"
  "<body><h1>Web Switch</h1>"
  "<h3>Switch power by pressing the button or with the http interface.</h3>"
  "Use the links below."
  "<p>Get value: <a href=/switch>/switch</a>"
  "<p>Turn power on: /switch?set=1 (POST/PUT)"
  "<p>Turn power off: /switch?set=0 (POST/PUT)"
  "<p>Toggle power: /switch?set=t (POST/PUT)"
  "<h3>Add argument \"json=1\" to get the reply in json format</h3>"
  "<p>Get value: <a href=/switch?json=1>/switch?json=1</a>"
  "<p>Toggle power: /switch?set=t&json=1 (POST/PUT)"
  "</body></html>"));
}

void handleSwitch() {
  String out;
  bool err = 0;
  int code = 200;

  String set = server.arg("set");
  SSS_PRINT("setArgument: ");
  SSS_PRINTLN(set?set:"NULL");
  if (set && set.length() > 0) {
    if (pass && *pass && !server.authenticate(user, pass)) {
      return server.requestAuthentication();
    } else {
      SSS_PRINTLN("no pass");
    }
    if (server.method() == HTTP_GET) {
      err = 1;
      code = 405; // Method not allowed
    } else if (set == "0") {
      digitalWrite(relayPin, LOW);
    } else if (set == "1") {
      digitalWrite(relayPin, HIGH);
    } else if (set == "t") {
      digitalWrite(relayPin, !digitalRead(relayPin));
    } else {
      err = 1;
      code = 400; // Generic error
    }
    // Takes some time for pin to flip
    delay(10);
  }
  int val = digitalRead(relayPin);

  String json = server.arg("json");
  if (json && json == "1") {
    out = "{\"switch\":";
    out += val;
    if (err) {
      out += ",\"error\":\"set must be 0, 1 or t and use POST\"";
    }
    out += "}";
    server.send(code, "application/json", out);
  } else {
    String out = String(val, DEC);
    server.send(code, "text/plain", out);
  }
}

void setup() {
  int configOk = 0;

  #ifdef SSS_DEBUG
  Serial.begin(115200);
  while(!Serial) {
    delay(1);
  }
  #endif

  // espWebConfig is configuring the button as input
  pinMode(relayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  SSS_PRINTLN("Starting ...");
  configOk = espConfig.setup();
  if (!configOk) {
    SSS_PRINTLN("Config failed");
  }

  user = espConfig.getParameter(USER_KEY);
  pass = espConfig.getParameter(PASS_KEY);

  SSS_PRINTLN("Normal boot");
  SSS_PRINTLN(WiFi.localIP());
  server.on ("/", handleRoot);
  server.on ("/switch", handleSwitch);

  server.begin();
  pinMode(buttonPin, INPUT_PULLUP);

  // Wait for ota
  int otaWait = 50;
  int pressed = 0;
  while (configOk && !otaEnabled && (otaWait--) > 0) {
    delay(100);// 100 * 50 ms = 5 sec
    int but = digitalRead(buttonPin);
    SSS_PRINT("OTA button ");
    SSS_PRINTLN(but);
    if (but == LOW) {
      digitalWrite(ledPin, !digitalRead(ledPin));
      SSS_PRINTLN("OTA Pressed");
      pressed = 1;
      continue; // Don't do anything until button released
    }

    if(!pressed) continue;

    // Button pressed and released
    otaEnabled = 1;

    ArduinoOTA.onStart([]() {
      SSS_PRINTLN("Start updating");
    });
    ArduinoOTA.onEnd([]() {
      SSS_PRINTLN("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      SSS_PRINT("Error:")
      SSS_PRINTLN(error)
    });
    ArduinoOTA.begin();
  }
  SSS_PRINT("OTA Enabled ");
  SSS_PRINTLN(otaEnabled);
  SSS_PRINTLN(WiFi.localIP());
  // Turn off green led
  digitalWrite(ledPin, 1);
}

void loop() {
  if (otaEnabled) {
    digitalWrite(ledPin, (millis()>>8)%2);
    ArduinoOTA.handle();
    return;
  }

  if (LOW == digitalRead(buttonPin))
  {
    SSS_PRINTLN("Pressed, toggle");
    // Togel directly, while button is pressed.
    digitalWrite(relayPin, !digitalRead(relayPin));

    int buttonMode = espx.ButtonPressed(buttonPin, ledPin);

    if (buttonMode == ESPXtra::ButtonMedium)
    {
      server.close();
      XTRA_PRINTLN("Start config");
      espConfig.startConfig(120000); // blocking
    }
    digitalWrite(ledPin, !LED_ON);
  }

  server.handleClient();
}
