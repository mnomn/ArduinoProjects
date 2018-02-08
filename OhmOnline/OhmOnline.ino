#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFiMulti.h>
#include <ESPXtra.h> // https://github.com/mnomn/ESPXtra
#include "local_secret.h"
/**
 * Board: Wemos mini/esp-12/esp-7
 */

#define BUTTON_PIN 0
#define CHANNELS 3

int channel[CHANNELS] = {13, 12, 14};
uint32_t measure_interval_min = 15;
int ota_started = 0;

ESPXtra espXtra;
ESP8266WiFiMulti WiFiMulti;

void setup() {
  Serial.begin(74880);
  while(!Serial) {
    delay(1);
  }
  espXtra.SleepCheck();

  // Initialize first pin here, for loads that
  // need some seconds to stabalize.
  pinMode(channel[0], OUTPUT);
  digitalWrite(channel[0], LOW);

  Serial.println("");
  Serial.println("");
  Serial.println("Setting up Analog1... ");
  Serial.print("LED PIN:");
  Serial.println(LED_BUILTIN);

  pinMode(BUTTON_PIN, INPUT); // Pin 0 has external pull up

  otaSetupCallbacks();

  WiFiMulti.addAP(OO_WIFI_NAME, OO_WIFI_PASS);

  while(WiFiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
}

void loop() {

  if (ota_started) {
    ArduinoOTA.handle();
    // Blink to indicate ota mode
    digitalWrite(LED_BUILTIN, (millis()/200)%2 == 0?HIGH:LOW);
  }

  if (!ota_started && espXtra.ButtonPressed(BUTTON_PIN)) {
    Serial.println("Start OTA");
    ArduinoOTA.begin();
    pinMode(LED_BUILTIN, OUTPUT);
    ota_started = 1;
  }

  if (!ota_started) {
    measure_all();
    espXtra.SleepSetMinutes(measure_interval_min);
    // Never reached.
  }
}

void measure_all() {
  // Measure on all pins

  int values[CHANNELS];
  for (int i = 0; i < CHANNELS; i++) {
    values[i] = measure(i);
    Serial.print("GPIO ");
    Serial.print(channel[i]);
    Serial.print(", id:");
    Serial.print(i);
    Serial.print(", analog::");
    Serial.println(values[i]);

  }
  char host[64];
  sprintf(host, "192.168.1.182/%X", ESP.getChipId());
  espXtra.PostJson(host, 8080, NULL,
                   (char *)"value1", values[0],
                   (char *)"value2", values[1],
                   (char *)"value3", values[2]);
}

/* Measure on chanel[mix]. Set all other pins to input. */
int measure(int mix) {
  for (int ix = 0; ix < CHANNELS; ix++) {
    int GPIO = channel[ix];
    if (ix == mix) {
      pinMode(GPIO, OUTPUT);
      digitalWrite(GPIO, LOW);
    } else {
      pinMode(GPIO, INPUT);
    }
  }
  delay(50);
  return analogRead(A0);
}


/// OTA STUFF

void otaSetupCallbacks()
{
  ArduinoOTA.onStart(onOTAStart);
  ArduinoOTA.onError(onOTAError);
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
}

void onOTAStart()
{
  Serial.println(F("OTA Start"));
}

void onOTAError(ota_error_t error)
{
  Serial.print(F("OTA Error"));
  Serial.println(error);
}
