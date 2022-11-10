#define ESPWC_DEBUG 1
#include <Arduino.h>
#include <ESPWebConfig.h> // https://github.com/mnomn/ESPWebConfig
#include <ESP8266HTTPClient.h>

/*
Device boots up, post a message and goes to sleep forever or until the reset buttom is pressed.

Af tirst boot the device will go into coinfig mode, where wifi and POST_HOST is configured. 

2 buttons:
  GPIO 0:
  - Press before boot: Set device in flash mode.
  - Press after boot: Set the device in config mode.

  Reset pin:
  - Reboot device. Will cause device to restart and message to be posted to POST_HOST.
*/

ADC_MODE(ADC_VCC);

String postHost;

const char *POST_HOST = "Post to*";
ESPWebConfig espConfig(nullptr, "Configure action", {POST_HOST});

// #define TESTMODE // or set in platformio.ini
#ifdef TESTMODE
constexpr bool testmode = true;
#else
constexpr bool testmode = false;
#endif

void setup() {
  if (testmode)
  {
    Serial.begin(74880);
    while(!Serial && millis() < 5000) {
      delay(100);
    }
    delay(100);
    Serial.println("Setup... ");
    delay(100);
    Serial.println(" ... ");

    pinMode(LED_BUILTIN, OUTPUT);
  }
  pinMode(0, INPUT); // HW pull up
  espConfig.setup();
}

void postData() {
  constexpr size_t jsonDataSize = 64;
  char jsonData[jsonDataSize];
  auto postTo = espConfig.getParameter(POST_HOST);
  if (!postTo) {
    return;
  }

  WiFiClient client;
  HTTPClient http;

  String hostStr(postTo);
  if (hostStr.indexOf("://") <0 ) {
    hostStr = "http://" + hostStr;
  }

  uint16_t vcc = ESP.getVcc();
  snprintf(jsonData, jsonDataSize, "{\"battery\":%u}", vcc);

  if (testmode)
  {
    Serial.printf("%s: %s\n", POST_HOST, hostStr.c_str());
    Serial.printf("DATA: %s\n", jsonData);
  }

  if (http.begin(client, hostStr)) {
    http.addHeader("Content-Type", "application/json");
    int responseCode = http.POST(jsonData);
    if (testmode) Serial.printf("response %d\n", responseCode);

    return;
  } else {
    if (testmode) Serial.printf("httpBegin failed \n");
  }

  return;
}

void loop() {
  static int dbg = 0;
  if (testmode)
  {
    // Blink while before posting
    if (dbg++ < 10) {
      digitalWrite(LED_BUILTIN, dbg%2);
      Serial.printf("BLINK!");
      delay(1000);
      return;
    }

    // Print post address and continue
    auto postTo = espConfig.getParameter(POST_HOST);
    Serial.printf("POST TO %s\n", postTo?postTo:"NULL");
  }

  postData();

  if (digitalRead(0) == LOW) {
    Serial.println("Start configuring");
    espConfig.startConfig();
  }

  ESP.deepSleep(0);
}
