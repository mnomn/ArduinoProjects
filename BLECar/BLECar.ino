/*
 * BLE Car
 * 
 * Tested with Arduino nano 33 BLE
 * 
*/

#include <ArduinoBLE.h>

//const int ledPin = LED_BUILTIN; // set ledPin to on-board LED

const int fwPin = LED_BUILTIN;//A0;
const int revPin = LED_PWR;//A1;
const int leftPin = A2;
const int rightPin = A7;

#define ZERO_VAL 63 // 127/2;

BLEService carService("19B10020-E8F2-537E-4F6C-D104768A1214");

BLEUnsignedIntCharacteristic ctrl("19B10021-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLEByteCharacteristic notify("19B10022-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);

unsigned long nextNotify = 0L;
byte dbg = 0;
// Stop if no data in channel
unsigned long lastWright = 0L;

void setup() {
  Serial.begin(9600);
  // while (!Serial); // Don't do this, it will block execution if serial not connected

  pinMode(fwPin, OUTPUT);
  pinMode(revPin, OUTPUT);
  pinMode(leftPin, OUTPUT);
  pinMode(rightPin, OUTPUT);

  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  // set the local name peripheral advertises
  BLE.setLocalName("BLECar");
  // set the UUID for the service this peripheral advertises:
  BLE.setAdvertisedService(carService);

  carService.addCharacteristic(ctrl);
  carService.addCharacteristic(notify);

  BLE.addService(carService);

  BLE.advertise();

  Serial.println("Bluetooth device active, waiting for connections...");
}

/*  speed is a char (0:127)
 *  "Cast it to signed: speed -63:64
 *  Positives are written to fw pin.
 *  Negative are written to rev pin.
 *
*/
void Speed(int spee)
{
  spee = spee - 127/2;
  Serial.print(spee);
  Serial.print(" ");

  // Speed is -63:64, analogWrite is 0 - 255. Multiply by 4.

  if (spee > 0)
  {
    if (spee>255) spee = 255;
    analogWrite(revPin, 0);
    analogWrite(fwPin, spee*4);
  }
  else
  {
    spee *=-4;
    if (spee>255) spee = 255;
    analogWrite(fwPin, 0);
    analogWrite(revPin, spee);
  }
}

void Steer(int steer)
{
  steer = steer - 127/2;
  Serial.println(steer);

  // Steer is -63:64, analogWrite is 0 - 255. Multiply by 4.
  // Neg: Left, Pos: Right

  if (steer > 0)
  {
    if (steer>255) steer = 255;
    analogWrite(leftPin, 0);
    analogWrite(rightPin, 4*steer);
  }
  else
  {
    steer *=-4;
    if (steer>255) steer = 255;
    analogWrite(rightPin, 0);
    analogWrite(leftPin, steer);
  }
}

void loop() {
  BLE.poll();

  unsigned long ms = millis();

  // Stop if no data written. Dead mans grip.
  if(lastWright) {
    unsigned long silentTime = ms - lastWright;
    if (silentTime > 500) {
      Serial.print("STOP ");
      Speed(ZERO_VAL);
      Steer(ZERO_VAL);
      lastWright = 0;
    }
  }

  byte b_period = 100; // ms
  byte b_duty = 15; // x^2 - 1
  if (ms > nextNotify) {
    nextNotify = nextNotify + 5000L;
    dbg++;
    if (dbg>200)dbg=0;
    Serial.print("Notify");
    Serial.println(dbg);
    notify.writeValue(dbg);
  }

  if (ctrl.written())
  {
    lastWright = ms;
    // ctrl Characteristic is unsigned inte, but it is devided into 4 bytes
    // [speed, steer, notUsed, notUsed]
    // To avoid sign issues, only 0 to 127 is used.
    byte data[4];

    ctrl.readValue(data, 4);
    
    Serial.print(">");
    Speed(data[0]);
    Steer(data[1]);

  }
}
