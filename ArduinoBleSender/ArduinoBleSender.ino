/*
 * Send sensor data on ble characteristics.
 * Tested on Arduino Nano 33 Ble (plain and sense).
*/

#include <ArduinoBLE.h>
#include <Arduino_LSM9DS1.h>
#include <Arduino_HTS221.h>
#include <Arduino_LPS22HB.h>

// Enable/disable sensors
// #define USE_ACCELERATION /* Onboard accelerometer*/
#define USE_DS18B20 /* External temperature */

// How often to send data
#define SEND_DATA_SEC 30

#ifndef USE_SERIAL
#define USE_SERIAL 0
#endif

#define DBG_PRINT(x)   \
  do                   \
  {                    \
    if (USE_SERIAL)    \
      Serial.print(x); \
  } while (0)
#define DBG_PRINTLN(x)   \
  do                     \
  {                      \
    if (USE_SERIAL)      \
      Serial.println(x); \
  } while (0)

#ifdef USE_DS18B20
#include <MaximWire.h>
#define DS18B20_PIN 9
MaximWire::Bus ds18b20_bus(DS18B20_PIN);
MaximWire::DS18B20 ds18b20_device;
#endif

#define CHAR_SERVICE "19B10110-E8F2-537E-4F6C-D104768A1214"
#define CHAR_ACC "19B10112-E8F2-537E-4F6C-D104768A1214"
#define CHAR_TEMP "19B10113-E8F2-537E-4F6C-D104768A1214"
#define CHAR_HUM "19B10114-E8F2-537E-4F6C-D104768A1214"
#define CHAR_BARO "19B10115-E8F2-537E-4F6C-D104768A1214"
#define CHAR_EXT_TEMP "19B10116-E8F2-537E-4F6C-D104768A1214"

BLEService service(CHAR_SERVICE); // create service

#ifdef USE_ACCELERATION
BLECharCharacteristic acceleration(CHAR_ACC, BLERead | BLENotify);
#endif

// Multiply by ten and send as short ( Measure 23.5 ->  Send 235)
BLEShortCharacteristic tempChar(CHAR_TEMP, BLERead | BLENotify);
// Round to whole number (int8_t)
BLECharCharacteristic humChar(CHAR_HUM, BLERead | BLENotify);

// Preasure measured in kPa around, value around 100
BLECharCharacteristic barometer(CHAR_BARO, BLERead | BLENotify);

#ifdef USE_DS18B20
BLEShortCharacteristic externalTemperature(CHAR_EXT_TEMP, BLERead | BLENotify);
#endif

bool accelerationOk = false;
bool humidityTemperatureOk = false;
bool barometerOk = false;
bool externalTemperatureOk = false;

unsigned long sendDataIntervalMs = (SEND_DATA_SEC * 1000);
unsigned long previousMillis = sendDataIntervalMs;

void setup()
{
#ifdef USE_SERIAL
  Serial.begin(9600);
  int serialRetries = 30; // Do not block if serial (usb) not connected

  while (!Serial && serialRetries--)
  {
    delay(100);
  }
#endif

  pinMode(LED_BUILTIN, OUTPUT); // initialize the built-in LED pin to indicate when a central is connected

  // begin initialization
  if (!BLE.begin())
  {
    DBG_PRINTLN("starting BLE failed!");

    while (1)
      ;
  }

  accelerationOk = initAcceleration();

  humidityTemperatureOk = initHumidityTemperature();

  barometerOk = initBarometer();

  externalTemperatureOk = initExternalTemperature();

  BLE.setLocalName("A2");
  BLE.setAdvertisedService(service);
  BLE.addService(service);
  BLE.advertise();

  digitalWrite(LED_PWR, LOW); // turn off power LED
}

void loop()
{
  BLEDevice central = BLE.central();

  DBG_PRINTLN("Waiting to connect");

  // if a central is connected to the peripheral:
  while (central && central.connected())
  {
    unsigned long diff = timeSincePrevious(millis(), previousMillis);
    if (diff >= sendDataIntervalMs)
    {
      measureAll();
      previousMillis = millis();
    }
    delay(1000);
  }
  delay(1000);
}

unsigned long timeSincePrevious(unsigned long now, unsigned long prev)
{
  return (now - prev);
}

void measureAll()
{
  DBG_PRINTLN("Measure all");
  updateAcceleration();
  updateBaro();
  updateHumidityTemperature();
  updateExternalTemperature();
}

bool initAcceleration()
{
#ifdef USE_ACCELERATION
  if (!IMU.begin())
  {
    DBG_PRINTLN("Failed to initialize IMU!");
    return false;
  }

  DBG_PRINT("Accelerometer sample rate = ");
  DBG_PRINT(IMU.accelerationSampleRate());
  DBG_PRINTLN(" Hz");

  service.addCharacteristic(acceleration);
  return true;
#else
  return false;
#endif
}

void updateAcceleration()
{
  if (!accelerationOk)
    return;

#ifdef USE_ACCELERATION
  float x, y, z;

  if (IMU.accelerationAvailable())
  {
    IMU.readAcceleration(x, y, z);

    // Since acc is -1.0:1.0, multiply with 100.
    acceleration.writeValue((int)(100 * x));

    DBG_PRINT("ACCELERATION: ");
    DBG_PRINT(x);
    DBG_PRINT('\t');
    DBG_PRINT(y);
    DBG_PRINT('\t');
    DBG_PRINTLN(z);
  }
  else
  {
    DBG_PRINTLN("No acceleration :(");
  }
#endif
}

bool initBarometer()
{
  if (BARO.begin())
  {
    service.addCharacteristic(barometer);
    return true;
  }
  DBG_PRINTLN("Failed to initialize barometer!");
  return false;
}

void updateBaro()
{
  if (!barometerOk)
    return;

  float pressure = BARO.readPressure();

  DBG_PRINT("BARO: ");
  DBG_PRINTLN(pressure);

  barometer.writeValue(pressure);
}

bool initHumidityTemperature()
{
  if (HTS.begin())
  {
    service.addCharacteristic(tempChar);
    service.addCharacteristic(humChar);
    return true;
  }
  else
  {
    DBG_PRINTLN("Failed to initialize humidity temperature sensor!");
    return false;
  }
}

void updateHumidityTemperature()
{
  if (!humidityTemperatureOk)
    return;

  float temperature = HTS.readTemperature();
  float humidity = HTS.readHumidity();
  // Send temp * 10 as whole number
  short shortT = (short)(10.0 * temperature + 0.5);

  DBG_PRINT("T: ");
  DBG_PRINT(temperature);
  DBG_PRINT("\tTshort: ");
  DBG_PRINT(shortT);
  DBG_PRINT("\tH: ");
  DBG_PRINTLN(humidity);

  tempChar.writeValue(shortT);
  humChar.writeValue((int8_t)(humidity + 0.5));
}

bool initExternalTemperature()
{
#ifdef USE_DS18B20
  service.addCharacteristic(externalTemperature);
  return true;
#endif
  return false;
}

void updateExternalTemperature()
{
  if (!externalTemperatureOk)
    return;

#ifdef USE_DS18B20
  if (ds18b20_device.IsValid())
  {
    float temp = ds18b20_device.GetTemperature<float>(ds18b20_bus);
    if (!isnan(temp))
    {
      DBG_PRINT("Ext T: ");
      DBG_PRINTLN(temp);
      ds18b20_device.Update(ds18b20_bus);
      short shortT = (short)(10.0 * temp + 0.5);
      externalTemperature.writeValue(shortT);
    }
    else
    {
      DBG_PRINT("LOST ");
      DBG_PRINTLN(ds18b20_device.ToString());
      ds18b20_device.Reset();
    }
  }
  else
  {
    if (ds18b20_bus.Discover().FindNextDevice(ds18b20_device) && ds18b20_device.GetModelCode() == MaximWire::DS18B20::MODEL_CODE)
    {
      DBG_PRINT("FOUND ");
      DBG_PRINTLN(ds18b20_device.ToString());
      ds18b20_device.Update(ds18b20_bus);
    }
    else
    {
      DBG_PRINTLN("NOTHING FOUND");
      ds18b20_device.Reset();
    }
  }
#endif
}
