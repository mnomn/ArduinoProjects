/*
 * Sketch for Sonoff WIFI Smart Switch
 *
 * Sketch to use for sonoff "original" relay module
 * - Button press turns light on or off
 * - Long press on button clear config
 * - path: http://ip/switch get the value
 *         http://ip/switch?set=1 turn on power
 *         http://ip/switch?set=0 turn off power
 *         http://ip/switch?set=t toggle power
 */

#include <ESPWebConfig.h> // https://github.com/mnomn/ESPWebConfig

/*
Sonoff WIFI Smart Switch:
1 M flash, 64 K SPIFFS

Reley: GPIO 12
Button: GPIO 0
Led: GPIO 13
*/
ESP8266WebServer server(80);
int buttonPin = 0;
ESPWebConfig espConfig("configpass");
const int relayPin = 12;
unsigned long lastButtonTime = 0;
int mode = 0;
static int myTest = 0;

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
  "<p>Turn power on: <a href=/switch?set=1>/switch?set=1</a>"
  "<p>Turn power off: <a href=/switch?set=0>/switch?set=0</a>"
  "<p>Toggle power: <a href=/switch?set=t>/switch?set=t</a>"
  "<h3>Add argument \"json=1\" to get the reply in json format</h3>"
  "<p>Get value: <a href=/switch?json=1>/switch?json=1</a>"
  "<p>Toggle power: <a href=/switch?set=t&json=1>/switch?set=t&json=1</a>"
  "</body></html>"));
}

void handleSwitch() {
  String out;
  bool invalidArg = 0;

  String set = server.arg("set");
  Serial.print("setArgument: ");
  Serial.println(set?set:"NULL");
  if (set && set.length() > 0) {
    if (set == "0") {
      digitalWrite(relayPin, LOW);
    } else if (set == "1") {
      digitalWrite(relayPin, HIGH);
    } else if (set == "t") {
      digitalWrite(relayPin, !digitalRead(relayPin));
    } else {
      invalidArg = 1;
    }
    // Takes some time for pin to flip
    delay(10);
  }
  int val = digitalRead(relayPin);

  String json = server.arg("json");
  if (json && json == "1") {
    out = "{\"switch\":";
    out += val;
    if (invalidArg) {
      out += ",\"error\":\"Argument set can only have values 0, 1 or t\"";
    }
    out += "}";
    server.send(200, "application/json", out);
  } else {
    String out = String(val, DEC);
    server.send(200, "text/plain", out);    
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
  if (espConfig.setup(server)) {
    Serial.println("Normal boot");
    Serial.println(WiFi.localIP());
    server.on ("/", handleRoot);
    server.on ("/switch", handleSwitch);
  } else {
    Serial.println("Config mode");
    mode = 1;
  }
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
    resetCounter -= 100;
    delay(100);
    if (resetCounter == 0) {
      espConfig.clearConfig();
      ESP.restart();
    }
  } else {
    // Reset counter if button not low.
    resetCounter = 5*1000;
  }

  server.handleClient();

  if (LOW == digitalRead(buttonPin)) {
    unsigned long now = millis();
    if (now - lastButtonTime > 1000) {
      lastButtonTime = millis();
      Serial.println("Pressed, toggle.");
      digitalWrite(relayPin, !digitalRead(relayPin));
    }
  }
}
