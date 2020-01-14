#include <Arduino.h>
#include <ESPWebConfig.h> // https://github.com/mnomn/ESPWebConfig
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include "index.h"

unsigned long blinkInterval = 100;
unsigned long lastBlinkTime = 0;

#define BUTTON1 D3 // GPIO0 hardware pullup
#define BUTTON_ONOFF D2 // GPIO4
#define WHITE_LED D6

// Pull up button, grounded when "ON"
#define BUTTON_ON 0
#define BUTTON_OFF 1

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define NEO_LED_PIN    13 //Wemos_D7


// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 8

Adafruit_NeoPixel strip(LED_COUNT, NEO_LED_PIN, NEO_GRB + NEO_KHZ800);
void oneColor(int rgb);
void flow();
void rainbow();
void handleRoot();
void handleModes();
void handleSet();
void setPos(int newPos);
ICACHE_RAM_ATTR void handleInterrupt();

ESPWebConfig espConfig;
ESP8266WebServer server(80);
bool ota = false;
int mod = 0;
unsigned col = 0;
int spe = 0;
int whi = 0;
int onoff = -1;
int pos;
int flowVal = 0; // Used for color changing modes

volatile bool modePressed = false;
bool modePressHandled = false;

void setup() {
  Serial.begin(74880);
  while(!Serial) {
    delay(1);
  }
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON_ONOFF, INPUT_PULLUP);

  pinMode(WHITE_LED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // TURN OFF LED

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  oneColor(0);
  digitalWrite(WHITE_LED, LOW);
  strip.setBrightness(255); // Set BRIGHTNESS (max = 255)

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
  setPos(-1);

  server.on("/", handleRoot);
  server.on("/modes", handleModes);
  server.on("/set", handleSet);
  server.begin();

  attachInterrupt(digitalPinToInterrupt(BUTTON1), handleInterrupt, FALLING);
}

void loop() {
  delay(10);
  ArduinoOTA.handle();
  if (ota) return;

  // Only change setting if it is on.
  if (onoff == BUTTON_ON && modePressed && !modePressHandled) {
    modePressHandled = true;
    if (pos >= 4) setPos(1);
    else setPos(++pos);
    delay(100);
  }

  if (modePressHandled && digitalRead(BUTTON1)) {
    Serial.printf("BUTTON1! \n");
    delay(200); // Debounce time
    if (digitalRead(BUTTON1)) { // Button released
      modePressed = false;
      modePressHandled = false;
    }
  }

  server.handleClient();

  unsigned long now = millis();

  int tmp = digitalRead(BUTTON_ONOFF);
  if (tmp != onoff) {
    Serial.printf("BUTTON_ONOFF %d\n", tmp);
    // Value changhed
    onoff = tmp;
    if (onoff == BUTTON_OFF)
    {
      // TODO: Move up above blink interval (button response)
      // TODO: Set interrupt wake up and start sleep.
      whi = 0;
      col = 0;
      oneColor(0);
      digitalWrite(WHITE_LED, LOW);
      // gpio_pin_wakeup_enable(GPIO_ID_PIN(1), GPIO_PIN_INTR_LOLEVEL);
      // wifi_set_sleep_type(LIGHT_SLEEP_T);
      // delay(100);
      return;
    }
    else setPos(-1);
  }

  if (!modePressed && now - lastBlinkTime < blinkInterval) return;
  lastBlinkTime = now;

  if (onoff == BUTTON_OFF) {
    oneColor(0);
    digitalWrite(WHITE_LED, LOW);
    return;
  }

  if (whi == 100) {
    digitalWrite(WHITE_LED, HIGH);
  } else {
    analogWrite(WHITE_LED,whi*10); // esp8266 has analogWrite 0-1023
  }

  switch (mod)
  {
  case 2:
    flow();
    break;
  case 3:
    rainbow();
    break;

  default:
    oneColor(col);
    break;
  }

}

void handleInterrupt() {
  modePressed = true;
}

void oneColor(int rgb) {
  strip.fill(rgb, 0, 8);
  strip.show(); // Update strip with new contents
}

void rainbow() {
  // speed = 10; Go through all colors in 10 sec
  // called every 100 ms (blinkTime).
  // 10 sec = blinkTime * blinks => 100 blinks
  long blinks = spe * (1000/blinkInterval);

  if (flowVal >= 65536) flowVal = 0;
  else flowVal += 65536/blinks;

  // Serial.printf("rainbow %d\n", flowVal);

  strip.fill(strip.gamma32(strip.ColorHSV(flowVal)), 0, 8);
  strip.show(); // Update strip with new contents

}

void flow() {
  // Speed:
  // blinkTime == 100, speed 10*1000: 100 blinks per flow-loop, skip flowVal 3 to complete round in 10 sec
  // blinkTime == 100, speed 30*1000: 300 blinks per flow-loop, skip flowVal 1 to complete round in 30 sec
  // Over 30 not supported
  long blinks = spe * (1000/blinkInterval);
  int skip = 300/blinks;
  if (skip < 1) skip = 1;

  // Slightly shift color. Use RGB representation
  int r = col>>16;
  int g = (col>>8) & 0xFF;
  int b = col & 0xFF;

  // flowVal 0 - 300. 100: change R. 100-200, Change G. 200-300 change B
  if (flowVal >= 300) flowVal = 0;
  int diff = 2 * flowVal%100; // 0..100..0
  if (diff > 50) diff = 100 - diff;
  int select = flowVal/100; // 0-2
  switch (select)
  {
  case 0:
    if (r < 105) r += diff;
    else r -= diff;
    break;
  case 1:
    if (g < 105) g += diff;
    else g -= diff;
    break;
  case 2:
    if (b < 105) b += diff;
    else b -= diff;
    break;
  default:
    break;
  }
  flowVal += skip;

  Serial.printf("flow %d %d, %X %X %X\n", diff, select, r, g, b);

  strip.fill(strip.Color(r, g, b), 0, 8);
  strip.show(); // Update strip with new contents

}

void handleRoot() {
  // String page = indexPage;
  // TODO: Use send_P
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
  yield();

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
  yield();
  EEPROM.write(addr++, (val>>8)&0xFF);
  yield();
  EEPROM.write(addr++, val&0xFF);
  yield();

  memcpy(key, "spe", 3); // Do not copy null terminator
  valStr = server.arg(key);
  val = valStr.toInt();
  Serial.printf("%s %d %s %d\n", key, addr, valStr.c_str(), val);
  EEPROM.write(addr++, val);
  yield();

  memcpy(key, "whi", 3); // Do not copy null terminator
  valStr = server.arg(key);
  val = valStr.toInt();
  Serial.printf("%s %d %s %d\n", key, addr, valStr.c_str(), val);
  EEPROM.write(addr++, val);
  yield();
}

void setPos(int newPos) {
  // Read parameters from eeprom.
  // If mode <= 0, read which mode to use from flash
  // If mode is not zero, srite mode to flash
  Serial.printf("setPos %d\n", pos);
  if (newPos > 4) return;
  if (newPos <= 0) {
    pos = EEPROM.read(399);
    if (pos <= 0 || pos > 4) {
      Serial.printf("sBad pos %d, sewt to 1\n", pos);
      pos = 1;
    }
  } else {
    Serial.printf("Write %d\n", newPos);
    EEPROM.write(399, newPos);
    EEPROM.commit();
    pos = newPos;
  }
  int addr = 400 + 10 * (pos - 1);
  mod = EEPROM.read(addr++);
  col = EEPROM.read(addr++) << 16;
  col += EEPROM.read(addr++) << 8;
  col += EEPROM.read(addr++);
  spe = EEPROM.read(addr++);
  whi = EEPROM.read(addr++);
  Serial.printf("setPos %d VAL mod=%d col=0x%0X spe=%d whi=%d\n", pos, mod, col, spe, whi);
  if (spe <= 0) spe = 1;
}

void getModeJson(int pos, char *json, int len) {
  /*
  {"mod1":1,"col1":"#ffffff","spe1":255,"whi1":100,
   "mod1":1,"col1":"#ffffff","spe1":255,"whi1":100,
   "mod1":1,"col1":"#ffffff","spe1":255,"whi1":100,
   "mod1":1,"col1":"#ffffff","spe1":255,"whi1":100}
   */
  // Every setting is max 49 bytes long
  int val, wrt;
  char *jj=json;

  int addr = 400 + 10 * (pos - 1);
  val = EEPROM.read(addr++);
  wrt = sprintf(jj, "%s\"mod%d\":%d,",(pos==1)?"{":"",pos,val);
  jj+= wrt;
  val = EEPROM.read(addr++) << 16;
  val += EEPROM.read(addr++) << 8;
  val += EEPROM.read(addr++);
  wrt = sprintf(jj, "\"col%d\":\"#%x\",",pos,val);
  jj+= wrt;
  val = EEPROM.read(addr++);
  wrt = sprintf(jj, "\"spe%d\":%d,",pos,val);
  jj+= wrt;
  val = EEPROM.read(addr++);
  wrt = sprintf(jj, "\"whi%d\":%d%c",pos,val,(pos==4)?'}':',');
}

void handleModes() {
  if (server.method() == HTTP_POST) {
    Serial.println("Modes METHOD POST");

    saveMode(1);
    saveMode(2);
    saveMode(3);
    saveMode(4);
    EEPROM.commit();

    server.send(200, "text/plain", "OK");
    return;
  }
  else
  {
    char buff[50];
    Serial.println("Modes METHOD GET");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", "");
    for (int i = 1;i<=4;i++) {
      getModeJson(i, buff, 50);
      server.sendContent(buff);
    }
    // Closing bracket generated in getModeJson
    server.sendContent(""); // Terminate the HTTP chunked transmission with a 0-length chunk
  }
}

// handle /set?pos
void handleSet() {
  Serial.printf("/set method %d", server.method());
  int res = 405;
  if (server.method() == HTTP_POST) {
    Serial.printf("/set method %d\n", server.method());
    String val;
    val = server.arg("pos");
    int v = val.toInt();
    if (1 <= v && v <=4) {
      setPos(v);
      res = 200;
    } else {
      res=400;
    }
  }
  server.send(res, "text/plain", res==200?"OK":"?");
}
