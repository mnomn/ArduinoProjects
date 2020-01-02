#include <Arduino.h>
#include <ESPWebConfig.h> // https://github.com/mnomn/ESPWebConfig
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include "index.h"

// App,    Wmos, descriptuin, ArduinoPin
// Color1, D3	IO, 10k Pull-up	GPIO0
// Button, D4	IO, 10k Pull-up, BUILTIN_LED	GPIO2
// Color2, D5	IO, SCK	GPIO14

// White led: D2, GPIO_4
// Neo LED: D7, GPIO 13

// Ota

unsigned long blinkInterval = 1000;
unsigned long lastBlinkTime = 0;

#define BUTTON1 D5 // Pull up
#define BUTTON2 D8 // Pull down
#define WHITE_LED D2

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define NEO_LED_PIN    13 //Wemos_D7


// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 8

Adafruit_NeoPixel strip(LED_COUNT, NEO_LED_PIN, NEO_GRB + NEO_KHZ800);
void rainbow(int wait);
void oneColor(int pixelHue /* 0 - 65535*/);
void handleRoot();
void handleModes();

ESPWebConfig espConfig;
ESP8266WebServer server(80);
bool ota = false;
void setup() {
  Serial.begin(115200);
  while(!Serial) {
    delay(1);
  }
  pinMode(WHITE_LED, OUTPUT);
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLDOWN_16);

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(150); // Set BRIGHTNESS (max = 255)

  espConfig.setup();

  ArduinoOTA.onStart([]() {
    server.stop();
    ota=true;
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();

  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  uint32_t cid = ESP.getChipId();
  Serial.printf("CID %d %X ",cid, cid);
  Serial.println(WiFi.macAddress());

  server.on("/", handleRoot);
  server.on("/modes", handleModes);
  server.begin();
}

void loop() {
  unsigned long now = millis();
  ArduinoOTA.handle();
  if (ota) return;

  server.handleClient();

  if (now - lastBlinkTime < blinkInterval) return;
  lastBlinkTime = now;
  Serial.printf("now %lu B1:%d B2:%d \n", now, digitalRead(BUTTON1), digitalRead(BUTTON2));

  // delay(1000);
  Serial.printf("now %lu D2: %lu\n", now, now%255);
//  Serial.printf("LED2 %d %s\n", D5 , digitalRead(D5)?"ON":"OFF");

//  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
//  digitalWrite(D2, now%2);
  analogWrite(D2,now%255);

//  rainbow(10);             // Flowing rainbow cycle along the whole strip
  oneColor(12000);

}

void oneColor(int pixelHue /* 0 - 65535*/) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
  }
  // uint32_t color = strip.getPixelColor(1);
  // Serial.printf("COL %u", color);
  strip.show(); // Update strip with new contents
}

void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

void handleRoot() {
  // String page = indexPage;
  server.send(200, "text/html", indexPage);
}


void getMod(int pos) {
  char *spe = (char*)"spe_";
  spe[3] = 48+pos;
  Serial.printf("Get SPE %s  - ", spe);
  Serial.println(server.arg(spe));
}

void handleModes() {
  if (server.method() == HTTP_POST) {
    Serial.println("Modes METHOD POST");
    Serial.println("speeds ");
    getMod(1);
    getMod(2);
    server.send(200, "text/plain", "OK");
    return;
  }
  // Get 
  Serial.printf("Modes METHOD GET");
  server.send(200, "application/json", "{\"spe1\":33, \"col2\":\"#0000AA\", \"mod1\":1, \"mod2\":2, \"mod3\":3, \"mod4\":4}");
}