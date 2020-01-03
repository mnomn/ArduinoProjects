#include <Arduino.h>
#include <ESPWebConfig.h> // https://github.com/mnomn/ESPWebConfig
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
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
void oneColor(int rgb);
void handleRoot();
void handleModes();
void readMode(int mode);

ESPWebConfig espConfig;
ESP8266WebServer server(80);
bool ota = false;
int mod = 0;
unsigned col = 0;
int spe = 0;
int whi = 0;

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

  EEPROM.begin(512);
  readMode(1);

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

  analogWrite(D2,whi);

//  rainbow(10);             // Flowing rainbow cycle along the whole strip
  oneColor(col);

}

void oneColor(int rgb) {
  strip.fill(rgb, 0, 8);
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


void saveMode(int pos) {
  // Every setting has: mode(1), col(3), speed(1), white(1) = 6 bytes
  // Stored on address 400, 410, 420, 430
  // 399 store which mode is selected
  int addr = 400 + 10 * (pos-1);
  char key[5] = {'m', 'o', 'd', (char)(48+pos), 0};
  String valStr = server.arg(key);
  int val = valStr.toInt();
  Serial.printf("%s %d %s %d\n", key, addr, valStr.c_str(), val);
  EEPROM.write(addr++, val);

  // Color, 3 bytes
  memcpy(key, "col", 3); // Do not copy nullterminator
  valStr = server.arg(key);
  const char *colStr = valStr.c_str();
  if (colStr[0] == '#') colStr++; // If "#FFAACC" format, skip 1.
  char *end;
  val = strtol(colStr, &end, 16);
  Serial.printf("%s %d %s %d %0X\n", key, addr, valStr.c_str(), val, val);
  // Serial.printf("COL 0x%X %u\n", col, col);
  EEPROM.write(addr++, val>>16);
  EEPROM.write(addr++, (val>>8)&0xFF);
  EEPROM.write(addr++, val&0xFF);

  memcpy(key, "spe", 3); // Do not copy null terminator
  valStr = server.arg(key);
  val = valStr.toInt();
  Serial.printf("%s %d %s %d\n", key, addr, valStr.c_str(), val);
  EEPROM.write(addr++, val);

  memcpy(key, "whi", 3); // Do not copy null terminator
  valStr = server.arg(key);
  val = valStr.toInt();
  Serial.printf("%s %d %s %d\n", key, addr, valStr.c_str(), val);
  EEPROM.write(addr++, val);
}

void readMode(int pos) {
  // Read parameters from eeprom.
  // If mode <= 0, read which mode to use from flash
  if (pos <= 0) {
    // TODO
  }
  int addr = 400 + 10 * (pos - 1);
  mod = EEPROM.read(addr++);
  col = EEPROM.read(addr++) << 16;
  col += EEPROM.read(addr++) << 8;
  col += EEPROM.read(addr++);
  spe = EEPROM.read(addr++);
  whi = EEPROM.read(addr++);
  Serial.printf("READ pos %d VAL mod=%d col=0x%0X spe=%d whi=%d\n", pos, mod, col, spe, whi);

}
void handleModes() {
  if (server.method() == HTTP_POST) {
    Serial.println("Modes METHOD POST");

    saveMode(1);
    saveMode(2);
    saveMode(3);
    saveMode(4);
    EEPROM.commit();

    readMode(1);
    server.send(200, "text/plain", "OK");
    return;
  }
  // Get 
  Serial.printf("Modes METHOD GET");
  server.send(200, "application/json", "{\"spe1\":33, \"col2\":\"#0000AA\", \"mod1\":1, \"mod2\":2, \"mod3\":3, \"mod4\":4}");
}