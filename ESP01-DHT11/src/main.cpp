#include <Arduino.h>
#include <ESPWebConfig.h> // https://github.com/mnomn/ESPWebConfig
#include <ESPXtra.h> // https://github.com/mnomn/ESPXtra

#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11

#define MEASURE_INTERRVAL_MIN 5

DHT dht(DHTPIN, DHTTYPE);

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
  dht.begin();

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

  if (!espx.TimeToWork(MEASURE_INTERRVAL_MIN * 60 * 1000)) return;

  float temp;
  float hum;
  int err_code = getDHT11Values(&temp, &hum);

  char *postHost = espConfig.getParameter(POST_HOST_KEY);
  char json[64];

  XTRA_PRINTF("Post to URL %s\n", postHost);

  snprintf(json, sizeof(json), "{\"t\":%.1f,\"h\":%.1f,\"err\":%d}",temp, hum, err_code);
  digitalWrite(LED_BUILTIN, HIGH);
  espx.PostJsonString(postHost, NULL, json);
  delay(300);
  digitalWrite(LED_BUILTIN, LOW);
}

// Return -1 on error, else number of retries (0 - 2)
int getDHT11Values(float *temp, float *hum)
{
  int retries = 0;
  const int max_retries = 2;
  while(1)
  {
    *hum = dht.readHumidity();
    *temp = dht.readTemperature();

    // Return tries to log eventual retries needed
    if (!isnan(*hum) && !isnan(*temp)) return retries;

    // Return -1 to indicate error
    if (retries >= max_retries) return -1;

    retries++;
    delay(100);
  }
}
