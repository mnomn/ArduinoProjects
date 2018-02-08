/*
 * IFTTT Button for ESP8266.
 * Sketch will wake up and send an ifttt message, then go back to sleep.
 */

#include <ESPWebConfig.h> // https://github.com/mnomn/ESPWebConfig

// Connect this pin to GND to reset web config
int resetPin = 0; // Good for ESP01



//////////////////////////////////////
// No need to edit below /////////////

ADC_MODE(ADC_VCC);

const char* KEY = "Maker key"; // Do not edit. Configure in web UI
const char* EVENT = "Event name"; // Do not edit. Configure in web UI

ESP8266WebServer server(80);
String parameters[] = {KEY, EVENT};
ESPWebConfig espConfig(NULL, parameters, 2);
unsigned long lastButtonTime = 0;
#define MODE_CALLING 0
#define MODE_RESETTING 1
#define MODE_CONFIG 2
#define MODE_CALL_COMPLETED 2
int mode = -1;

void callIFTTT(int v1) {
  WiFiClient client;
  const char *host = "maker.ifttt.com";
  Serial.println(F("Connect to host"));
  if (client.connect(host, 80)) {
    char *key = espConfig.getParameter(KEY);
    char *event = espConfig.getParameter(EVENT);
    String postData = "{\"value1\":";
    postData += v1;
    postData += "}";

    String httpData = "POST /trigger/";
    httpData += event;
    httpData += "/with/key/";
    httpData += key;
    httpData += " HTTP/1.1\r\nHost: ";
    httpData += host;
    httpData += "\r\nContent-Type: application/json\r\n";
    httpData += "Content-Length:";
    httpData += postData.length();
    httpData += "\r\nConnection: close\r\n\r\n";// End of header
    httpData += postData;

    Serial.println(F("Sending a request"));
    Serial.println(httpData);
    client.println(httpData);

    Serial.println(F("Read response"));
    // Read response
    while (client.connected()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
    }
    Serial.println(F("Done!"));
    client.stop();
  }
}

void setup() {
  Serial.begin(115200);
  while(!Serial) {
    delay(1);
  }
  // espWebConfig is configuring the button as input
  Serial.println(F("Starting ..."));
  espConfig.setHelpText("Parameters for IFTTT Maker <i>https://ifttt.com/maker</i>");
  if (espConfig.setup(server)) {
    Serial.println(F("Normal boot"));
    Serial.println(WiFi.localIP());
    mode = MODE_CALLING;
  } else {
    Serial.println(F("Config mode"));
    mode = MODE_CONFIG;
  }
  server.begin();
  pinMode(resetPin, INPUT_PULLUP);

}

void loop() {
  // Check reset, 5 sec long press
  static long resetCounter = 5*1000;
  if (LOW == digitalRead(resetPin))
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
  if (LOW == digitalRead(resetPin)) {
    mode = MODE_RESETTING;
  }
  if (mode == MODE_CALLING) {
    uint16_t vcc = ESP.getVcc();
    mode = MODE_CALL_COMPLETED;
    callIFTTT(vcc);
    delay(10);
    ESP.deepSleep(0);
  }
}
