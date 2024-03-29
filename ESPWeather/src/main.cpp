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
#elif DALLAS_T
# define ONE_WIRE_BUS 0 // Use pin that already has a pull up
#include <OneWire.h>
#include <DallasTemperature.h>
#endif

#ifdef OTA_ENABLED
#include <ArduinoOTA.h>
#endif

#include <ESP8266WiFi.h>

#ifdef USE_INERNAL_VCC
ADC_MODE(ADC_VCC); // to use getVcc
#endif

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
#elif DALLAS_T
  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature sensors(&oneWire);
  #define initSensor() {sensors.begin();}
  int getDS18B20Values(float *t,float *x);
  #define getSensorValues(t,x) getDS18B20Values(t,x)
#else // Dummy mode
  #define GET_DUMMY
  int getDummyValues(float *t,float *hum);
  #define initSensor() {;}
  #define getSensorValues(t,x) getDummyValues(t,x)
#endif

int wakeupReason = 0;

String postHost;
const char *POST_HOST_KEY = "Post to:";
String parameters[] = {POST_HOST_KEY};
ESPWebConfig espConfig(NULL, parameters, 1);

#ifndef MONITOR_SPEED
#define MONITOR_SPEED 74880
#endif

#define DEEP_SLEEP_TIME_M 15

#define MAX_UPTIME_M 30UL

/*
 WemosD1/GPIO5 SCL
 WemosD2/GPIO4 SDA
 WemosD3/GOPI0 Button pin
 WemosD5/GPIO14
*/

// In some cases the system hang somewhere and does not go too sleep.
// This interrupt will forc it to go to sleep after a certain amount of time
void IRAM_ATTR onTimerISR(){
  // Blink every X sec to se that circuit is running
  int val = GPIP(LED_BUILTIN);
  if(!val) GPOS = (1 << LED_BUILTIN);
  else GPOC = (1 << LED_BUILTIN);

  if (millis() > MAX_UPTIME_M * 60UL * 1000UL) {
    ESP.deepSleep(DEEP_SLEEP_TIME_M * 60UL * 1000000UL, WAKE_RF_DEFAULT);
  }
}

void setup() {
  // Timer to reboot fail-safe after X sec
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_LOOP);
  // 80 MHz/TIM_DIV256, 80000000/256=312500
  // 1 sec = 312500, 10 sec = 3125000
  timer1_write(3125000);

#ifdef XTRA_DEBUG
  Serial.begin(MONITOR_SPEED);
  while(!Serial && millis() < 2000) {
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
    espx.SleepSetMinutes(DEEP_SLEEP_TIME_M);
  }

  wakeupReason = myResetInfo->reason;

  XTRA_PRINT("Connected, IP address: ");
  XTRA_PRINTLN(WiFi.localIP());

  char* tmp = espConfig.getParameter(POST_HOST_KEY);
  if (tmp) {
    if (strstr(tmp, "://") == NULL) {
      postHost = String("http://");
      postHost = postHost + tmp;
    }
    else {
      postHost = String(tmp);
    }
  }

  XTRA_PRINT("POST HOST ");
  XTRA_PRINTLN(postHost?postHost.c_str():"NULL");

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
  // Medium press: Configure
  // Long press: Go to OTA
  int buttonMode = espx.ButtonPressed(buttonPin, LED_BUILTIN);

  XTRA_PRINTF("PRESSED %d\n", buttonMode);
  if (buttonMode == ESPXtra::ButtonLong)
  {
    XTRA_PRINTLN("Start OTA");
    otaBegin();
    serviceMode = 1;
    return;
  }
  else if (buttonMode == ESPXtra::ButtonMedium)
  {
    XTRA_PRINTLN("Start config");
    espConfig.startConfig(120000);
    serviceMode = 1;
    return;
  }

  digitalWrite(LED_BUILTIN, LOW);

#ifdef USE_INERNAL_VCC
  int volt = ESP.getVcc();
#else
  int volt = analogRead(A0); /* Read the Analog Input value */
#endif

  float temp = 0;
  float v2 = 0; // Humidity or preassure
  int err_code = 0;

  err_code = getSensorValues(&temp, &v2);

  if (!postHost) {
    XTRA_PRINTLN("POST HOST IS NULL!!!");
    delay(5000);
    return;
  }

  char json[64];

  XTRA_PRINTF("Post to URL %s\n", postHost.c_str());

#ifdef DALLAS_T
  snprintf(json, sizeof(json), "{\"t\":%.1f,\"boot\":%d,\"volt\":%d,\"err\":%d}",temp, wakeupReason, volt, err_code);
#else
  snprintf(json, sizeof(json), "{\"t\":%.1f,\"v2\":%d,\"volt\":%d,\"err\":%d}",temp, (int)v2, volt, (err_code?err_code:wakeupReason));
#endif
  espx.PostJsonString(postHost, json);

  espx.SleepSetMinutes(DEEP_SLEEP_TIME_M);
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

#ifdef DALLAS_T
int getDS18B20Values(float *t, float *x)
{
  *x = 0;

  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0);
  if (tempC != DEVICE_DISCONNECTED_C) {
    *t = tempC;
    return 0;
  }

  // oh no
  *t = 0;
  return 0xD; // D for disconnected
}
#endif

#ifdef GET_DUMMY
int getDummyValues(float *t,float *hum)
{
  XTRA_PRINTLN("Get dummy values");
  *t = 22.2;
  *hum = 56;
  return 0;
}
#endif // GET_DUMMY

#ifdef OTA_ENABLED
void otaSetup()
{
  XTRA_PRINTLN("otaSetup");

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
    XTRA_PRINTF("Error[%u]: ", error);
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
