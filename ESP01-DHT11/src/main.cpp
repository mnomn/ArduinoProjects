#include <Arduino.h>
#include <dhtnew.h>
#include <ESPWebConfig.h> // https://github.com/mnomn/ESPWebConfig
#include <ESPXtra.h> // https://github.com/mnomn/ESPXtra

// #include <Adafruit_Sensor.h>
// #include <DHT.h>

// #define DHTPIN 2
// #define DHTTYPE DHT11

// DHT dht(DHTPIN, DHTTYPE);

DHTNEW mySensor(2);

char *postUrl = NULL;
const char *POST_HOST_KEY = "Post to:";
String parameters[] = {POST_HOST_KEY};
ESPWebConfig espConfig(NULL, parameters, 1);

ESPXtra espx;

void setup() {
  #ifdef XTRA_DEBUG
  Serial.begin(9600);
  delay(200);
  #endif
  // dht.begin();

  delay(3000);
  XTRA_PRINTLN("Starting!\n");

  delay(3000);
  XTRA_PRINTLN("Starting2!\n");

  delay(3000);
  XTRA_PRINTLN("Starting3!\n");

  delay(3000);
  XTRA_PRINTLN("Starting4 X!");
  Serial.println("Starting4!\n");

  // mySensor.setHumOffset(10);
  // mySensor.setTempOffset(-3.5);

  bool setupOk = espConfig.setup();
  XTRA_PRINTF("Setup %d", setupOk);
}

int getDHT11Values(float *temp, float *hum);

void loop() {

  int bb = espx.ButtonPressed(0, LED_BUILTIN);
  if (bb != 0)
  {
    XTRA_PRINTF("BUTTON! %d", bb);
  }

  if (bb == espx.ButtonLong)
  {
    espConfig.clearConfig();
    delay(100);
    return;
  }

  if (!espx.TimeToWork(60000)) return;

  float temp;
  float hum;
  int err_code = getDHT11Values(&temp, &hum);

  char *postHost = espConfig.getParameter(POST_HOST_KEY);
  char json[64];

  XTRA_PRINTF("Post to URL %s\n", postHost);

  snprintf(json, sizeof(json), "{\"t\":%.1f,\"h\":%.1f,\"err\":%d}",temp, hum, err_code);
  espx.PostJsonString(postHost, NULL, json);
  
  delay(3000);
}


int getDHT11Values(float *temp, float *hum)
{

  // *hum = dht.readHumidity();
  // *temp = dht.readTemperature();

  // XTRA_PRINT(F("TEST DHT sensor!"));

  // if (isnan(*hum) || isnan(*temp)) {
  //   XTRA_PRINT(F("Failed to read from DHT sensor!"));
  //   *hum=0;
  //   *temp=0;

  //   return -1;
  // }

  int read_code = mySensor.read();
  *hum = mySensor.getHumidity();
  *temp = mySensor.getTemperature();
  XTRA_PRINTF("DTH11 %d H:%f T: %f\n", read_code, *hum, *temp);

  return read_code;
}
