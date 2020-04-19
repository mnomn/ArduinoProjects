/*
 * BLE Car
 * 
 * Tested with Arduino nano 33 BLE
 * 
*/

#include <ArduinoBLE.h>

const int ledPin = LED_BUILTIN; // set ledPin to on-board LED
const int buttonPin = 4; // set buttonPin to digital pin 4

BLEService carService("19B10020-E8F2-537E-4F6C-D104768A1214");

BLEUnsignedIntCharacteristic ctrlChar("19B10021-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLEByteCharacteristic notifyChar("19B10022-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);

unsigned long nextNotify = 0L;
byte dbg = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(ledPin, OUTPUT); // use the LED as an output
  pinMode(buttonPin, INPUT); // use button pin as an input

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  // set the local name peripheral advertises
  BLE.setLocalName("BLECar");
  // set the UUID for the service this peripheral advertises:
  BLE.setAdvertisedService(carService);

  // add the characteristics to the service
  carService.addCharacteristic(ctrlChar);
  carService.addCharacteristic(notifyChar);

  // add the service
  BLE.addService(carService);

  // start advertising
  BLE.advertise();

  Serial.println("Bluetooth device active, waiting for connections...");
}

void loop() {
  BLE.poll();

  unsigned long ms = millis();
  if (ms > nextNotify) {
    nextNotify = nextNotify + 5000L;
    dbg++;
    if (dbg>200)dbg=0;
    Serial.print("Notify");
    Serial.println(dbg);
    notifyChar.writeValue(dbg);
  }

  if (ctrlChar.written())
  {
    Serial.print("Ctrl: ");
    byte data[4];
    unsigned int *val2 = (unsigned int*)data;

    Serial.print(" L ");
    Serial.print(ctrlChar.valueLength());
    ctrlChar.readValue(val2, 4);
    
    Serial.print("Vx: ");
    
    Serial.print(" V2 ");
    Serial.print(*val2);
    Serial.print(" ");
    Serial.println(*val2,16);
    Serial.print(" DATA ");
    Serial.print(data[0]);
    Serial.print(" ");
    Serial.print(data[1]);
    Serial.print(" ");
    Serial.print(data[2]);
    Serial.print(" ");
    Serial.println(data[3]);

    analogWrite(LED_BUILTIN, data[0]);
  }
}
