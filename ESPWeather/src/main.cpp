// #include <Arduino.h>
#include <ESPWebConfig.h> // https://github.com/mnomn/ESPWebConfig
#include <ESPXtra.h> // https://github.com/mnomn/ESPXtra
#include <Ticker.h>  //Ticker Library
extern "C" {
  #include <user_interface.h>
}

#ifdef DHT21_ENABLE
#include <dht.h> // DHTStable
#include <Wire.h>
#elif BMP280_ENABLE
#include <SPI.h>
#include <Adafruit_BMP280.h>
#elif AM2320_ENABLE
#include <AM232X.h>
#endif

#ifdef OTA_ENABLED
#include <ArduinoOTA.h>
#endif

#include <ESP8266WiFi.h>

ADC_MODE(ADC_VCC); // to use getVcc

ESPXtra espx;

void initSensor();
int getSensorValues(float *t, float *x);

#ifdef OTA_ENABLED
void otaSetup();
#define otaBegin() {ArduinoOTA.begin();}
#define otaHandle() {ArduinoOTA.handle();}
#else
#define otaSetup()
#define otaBegin()
#define otaHandle()
#endif

int buttonPin = 0;
bool serviceMode = 0; // Config or ota

#ifdef DHT21_ENABLE
  dht DHT;
  int dhtPin = 0;//5; // Set to 0 to disable
  void initDHT21Sensor();
  int getDHT21Values(float *t,float *h);
  #define initSensor() initDHT21Sensor()
  #define getSensorValues(t,x) getDHT21Values(t,x)
#elif BMP280_ENABLE
  Adafruit_BMP280 bmp; // use I2C interface
  Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
  Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();
  void initBMPSensor();
  int getBMPValues(float *temp, float *preas);
  #define initSensor() initBMPSensor()
  #define getSensorValues(t,x) getBMPValues(t,x)
#elif AM2320_ENABLE
  AM232X am2320;
  int getAM2320Values(float *t,float *hum);
  #define initSensor() {am2320.begin();}
  #define getSensorValues(t,x) getAM2320Values(t,x)
#endif

int wakeupReason = 0;
bool ota = false;

char *postUrl = NULL;
const char *POST_HOST_KEY = "Post to:";
String parameters[] = {POST_HOST_KEY};
ESPWebConfig espConfig(NULL, parameters, 1);

/*
 WemosD1/GPIO5 SCL
 WemosD2/GPIO4 SDA
 WemosD3/GOPI0 Button pin
 WemosD5/GPIO14
*/
void setup() {
#ifdef XTRA_DEBUG
  Serial.begin(74880);
  while(!Serial) {
    delay(1);
  }
  delay(200);
  Serial.println();
  Serial.println(F("Starting esp weather..."));
#endif

  pinMode(LED_BUILTIN, OUTPUT);
  rst_info *myResetInfo;
  myResetInfo = ESP.getResetInfoPtr();
  XTRA_PRINT(F("Reset reason="));
  XTRA_PRINTLN(myResetInfo->reason);

  if(myResetInfo->reason == REASON_DEEP_SLEEP_AWAKE) {
    // Check if we shall continue to sleep.
    // Only check if started from sleep (not power cycle or crash).
    espx.SleepCheck();
  }

  initSensor();

  if(!espConfig.setup())
  {
    espx.SleepSetMinutes(15);
  }

  wakeupReason = myResetInfo->reason;

  XTRA_PRINT("Connected, IP address: ");
  XTRA_PRINTLN(WiFi.localIP());

  otaSetup();
}

void loop()
{
  otaHandle();

  if (serviceMode)
  {
    digitalWrite(LED_BUILTIN, millis() / 100 % 10);
    return;
  }

  // Short press: Nothing
  // Medium press: Reset config
  // Long press: Go to OTA
  int buttonMode = 0;
  while (1)
  {
    delay(20);
    int pressed = espx.ButtonPressed(buttonPin, LED_BUILTIN);
    if (!pressed)
      break;
    buttonMode = pressed;
  }

  Serial.printf("PRESSED %d\n", buttonMode);
  if (buttonMode == ESPXtra::ButtonLong)
  {
    XTRA_PRINTLN("Start OTA");
    otaBegin();
    serviceMode = 1;
    return;
  }
  else if (buttonMode == ESPXtra::ButtonMedium)
  {
    XTRA_PRINTLN("Clear config");
    espConfig.clearConfig();
    serviceMode = 1;
    return;
  }


  digitalWrite(LED_BUILTIN, LOW);

  int vcc = ESP.getVcc();
  float temp = 0;
  float v2 = 0; // Humidity or preassure
  int err_code = 0;

  err_code = getSensorValues(&temp, &v2);

  char *postHost = espConfig.getParameter(POST_HOST_KEY);
  char json[64];

  XTRA_PRINTF("Post to URL %s\n", postHost);

  snprintf(json, sizeof(json), "{\"t\":%.1f,\"v2\":%d,\"vcc\":%d,\"err\":%d}",temp, (int)v2, vcc, (err_code?err_code:wakeupReason));
  espx.PostJsonString(postHost, NULL, json);

  espx.SleepSetMinutes(15);
}

#ifdef BMP280_ENABLE
void initBMPSensor()
{
    if (!bmp.begin(0x76)) {
      XTRA_PRINTLN(F("BMP error."));
    } else {
      XTRA_PRINTLN2(F("BMP OK!"));
      // bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
      //                 Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
      //                 Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
      //                 Adafruit_BMP280::FILTER_X16,      /* Filtering. */
      //                 Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  #if XTRA_DEBUG>1
      bmp_temp->printSensorDetails();
  #endif
  }
}

int getBMPValues(float *temp, float *preas)
{
  sensors_event_t temp_event, pressure_event;
  bmp_temp->getEvent(&temp_event);
  bmp_pressure->getEvent(&pressure_event);
  *temp = temp_event.temperature;
  *preas = pressure_event.pressure;

  XTRA_PRINT(F("BMP Temperature = "));
  XTRA_PRINT(*temp);
  XTRA_PRINT(F(" *C, P="));
  XTRA_PRINT(*preas);
  XTRA_PRINTLN(F(" hPa"));

  return 0;
}
#endif

#ifdef DHT21_ENABLE
void initDHT21Sensor()
{
  if (dhtPin > 0) {
    XTRA_PRINT(F("Using DHT, pin "));
    XTRA_PRINTLN(dhtPin);
  } else {
    XTRA_PRINTLN2(F("Not using DHT"));
  }
}

int getDHT21Values(float *temp, float *hum)
{
  int read_code = DHT.read21(dhtPin);

  switch (read_code)
  {
  case DHTLIB_OK:
      XTRA_PRINTLN2(F("DHT OK"));
      break;
  case DHTLIB_ERROR_CHECKSUM:
      XTRA_PRINTLN(F("DHT Checksum error"));
      break;
  case DHTLIB_ERROR_TIMEOUT:
      XTRA_PRINTLN(F("DHT Time out error"));
      break;
  default:
      XTRA_PRINT(F("DHT Unknown error: "));
      XTRA_PRINTLN(read_code);
      break;
  }

  *hum = DHT.humidity;
  *temp = DHT.temperature;
  XTRA_PRINT("DTH H:");
  XTRA_PRINT(*hum);
  XTRA_PRINT(", T:");
  XTRA_PRINTLN(*temp);

  return read_code;
}
#endif

#ifdef AM2320_ENABLE
int getAM2320Values(float *t,float *hum)
{
  int status = am2320.read();
  if (status != AM232X_OK)
  {
    *t = 0;
    *hum = 0;
    return status;
  }

  *t = am2320.getTemperature();
  *hum = am2320.getHumidity();
  XTRA_PRINT("AM2320 T:");
  XTRA_PRINT(*t);
  XTRA_PRINT(", H:");
  XTRA_PRINTLN(*hum);
  return 0;
}
#endif

#ifdef OTA_ENABLED
void otaSetup()
{

  XTRA_PRINTLN2("Configure OTA");
  ota=true;

  ArduinoOTA.onStart([]() {
    // server.stop();
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    XTRA_PRINTLN("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    XTRA_PRINTLN("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    XTRA_PRINTF("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      XTRA_PRINTLN("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      XTRA_PRINTLN("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      XTRA_PRINTLN("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      XTRA_PRINTLN("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      XTRA_PRINTLN("End Failed");
    }
  });
}
#endif // OTA
