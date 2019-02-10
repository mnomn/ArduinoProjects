/*
 * Sketch for Sonoff WIFI Smart Switch
 *
 * Sketch to use for sonoff "original" relay module
 * - Button press turns light on or off
 * - Long press on button clear wifi and security config
 *
 * First boot/after longpress, device is in config mode.
 * Connect a phone/laptop to access point ESP_192.168.X.Y and browse
 * to ESP_192.168.X.Y and fill in wifi info and optionally user and
 * password. (Not sure how complicated/long the password can be)
 *
 * After config the sonof listens to thgese urls:
 * - path: http://ip/switch get the value
 *         http://ip/switch?set=1 (POST) turn on power
 *         http://ip/switch?set=0 (POST) turn off power
 *         http://ip/switch?set=t (POST) toggle power
 */

#include "Arduino.h"
#include <ESPWebConfig.h> // https://github.com/mnomn/ESPWebConfig

/*
Sonoff WIFI Smart Switch:
Arduino serttings: Generic esp8266, 1 M flash, 64 K SPIFFS

Relay: GPIO 12
Button: GPIO 0
Led: GPIO 13
*/
ESP8266WebServer server(80);
int buttonPin = 0;

char *user = NULL;
char *pass = NULL;
const char* USER_KEY = "User:";
const char* PASS_KEY = "Pass:";
String parameters[] = {USER_KEY, PASS_KEY};
ESPWebConfig espConfig("configpass", parameters, 2);

const int relayPin = 12;
unsigned long lastButtonTime = 0;

void handleRoot() {
  server.send(200, "text/html", F("<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
  "<style type=text/css>"
  "body { margin:5%; font-family: Arial;} form p label {display:block;float:left;width:100px;}"
  "</style></head>"
  "<body><h1>Switch sonoff</h1>"
  "<h3>Press button on the Sonoff to switch power on/off. "
  "You can also switch the relay with the http interface. "
  "Use the links below as bookmarks or in an app.</h3>"
  "<p>Get value: <a href=/switch>/switch</a>"
  "<p>Turn power on: /switch?set=1 (POST)"
  "<p>Turn power off: /switch?set=0 (POST)"
  "<p>Toggle power: /switch?set=t (POST)"
  "<h3>Add argument \"json=1\" to get the reply in json format</h3>"
  "<p>Get value: <a href=/switch?json=1>/switch?json=1</a>"
  "<p>Toggle power: /switch?set=t&json=1 (POST)"
  "</body></html>"));
}

void handleSwitch() {
  String out;
  bool err = 0;
  int code = 200;

  String set = server.arg("set");
  Serial.print("setArgument: ");
  Serial.println(set?set:"NULL");
  if (set && set.length() > 0) {
    if (pass && *pass && !server.authenticate(user, pass)) {
      return server.requestAuthentication();
    } else {
      Serial.println("no pass");
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
  Serial.begin(115200);
  while(!Serial) {
    delay(1);
  }
  // espWebConfig is configuring the button as input
  pinMode(relayPin, OUTPUT);
  Serial.println("Starting ...");
  if (!espConfig.setup()) {
    Serial.println("Config failed");
  }

  user = espConfig.getParameter(USER_KEY);
  pass = espConfig.getParameter(PASS_KEY);

  Serial.println("Normal boot");
  Serial.println(WiFi.localIP());
  server.on ("/", handleRoot);
  server.on ("/switch", handleSwitch);


  server.begin();
  /* Perhaps the internal pull up is not needed/bad?.
     Since there probbaly is an external pull up. */
  pinMode(buttonPin, INPUT_PULLUP);
}

void loop() {
  // Check reset, 5 sec long press
  static long resetCounter = 5*1000;
  if (LOW == digitalRead(buttonPin))
  {
    if (resetCounter == (5*1000 - 100)) {// Toggle after 100 ms to avoid bounce
      Serial.println("Pressed, toggle");
      Serial.println(WiFi.localIP());
      digitalWrite(relayPin, !digitalRead(relayPin));
    }

    delay(100);
    resetCounter -= 100;
    if (resetCounter <= 0) {
      Serial.println("Clear config");
      espConfig.clearConfig();
      ESP.restart();
    }
  } else {
    // Reset counter if button not low.
    resetCounter = 5*1000;
  }

  server.handleClient();

}
