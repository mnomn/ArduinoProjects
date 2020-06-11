/*
 * BLE Car
 * 
 * Tested with Arduino nano 33 BLE
 * 
*/

#include <ArduinoBLE.h>

//const int ledPin = LED_BUILTIN; // set ledPin to on-board LED

const int fwPin = LED_BUILTIN; // A0
const int revPin = A1;
const int leftPin = A2;
const int rightPin = LED_PWR;

//const int powerLedPin = LED_PWR;
//const int buttonPin = 4; // set buttonPin to digital pin 4

BLEService carService("19B10020-E8F2-537E-4F6C-D104768A1214");

BLEUnsignedIntCharacteristic ctrl("19B10021-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLEByteCharacteristic notify("19B10022-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);

unsigned long nextNotify = 0L;
byte dbg = 0;

void setup() {
  Serial.begin(9600);
  // while (!Serial); // Don't do this, it will block execution if serial not connected

  pinMode(fwPin, OUTPUT);
  pinMode(revPin, OUTPUT);
  pinMode(leftPin, OUTPUT);
  pinMode(rightPin, OUTPUT);

//  pinMode(powerLedPin, OUTPUT);
//  pinMode(ledPin, OUTPUT); // use the LED as an output
//  pinMode(buttonPin, INPUT); // use button pin as an input

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
  carService.addCharacteristic(ctrl);
  carService.addCharacteristic(notify);

  // add the service
  BLE.addService(carService);

  // start advertising
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
//  Serial.print("Speed ");
  Serial.print(spee);
  Serial.print(" ");

  static int prevSpeed = 0;
  // Write less often to pins. Detect different signs
  if (prevSpeed * spee <= 0)
  {
    if (spee > 0) {
      // Speed is 0-62, analogWrite is 0 - 255. Multiply by 4.
      digitalWrite(revPin, LOW);
    }
    else {
      digitalWrite(fwPin, LOW);
    }
  }
  prevSpeed = spee;

  // Speed is 0-62, analogWrite is 0 - 255. Multiply by 4.

  if (spee > 0)
  {
    digitalWrite(revPin, LOW);
    analogWrite(fwPin, spee*4);
  }
  else
  {
    digitalWrite(fwPin, LOW);
    analogWrite(revPin, -4*spee);
  }
}

void Steer(int steer)
{
  steer = steer - 127/2;
//  Serial.print("Steer: ");
  Serial.println(steer);

  // Speed is 0-62, analogWrite is 0 - 255. Multiply by 4.
  // Neg: Left, Pos: Right
#if 0
  static int prevSteer = 0;
  // Write less often to pins. Detect different signs
  if (prevSteer * steer <= 0)
  {
    if (steer > 0) {
      digitalWrite(leftPin, LOW);
    }
    else {
      digitalWrite(rightPin, LOW);
    }
  }
  prevSteer = steer;
#endif

#if 1
  if (steer > 0)
  {
    digitalWrite(leftPin, LOW);
    analogWrite(rightPin, 4*steer);
  }
  else
  {
    digitalWrite(rightPin, LOW);
    analogWrite(leftPin, -4*steer);
  }
#endif
}


void loop() {
  BLE.poll();

  unsigned long ms = millis();

  byte b_period = 100; // ms
  byte b_duty = 15; // x^2 - 1
//  digitalWrite(powerLedPin, ((ms/100)&b_duty)==b_duty);
  if (ms > nextNotify) {
    nextNotify = nextNotify + 5000L;
    dbg++;
    if (dbg>200)dbg=0;
    Serial.print("Notify");
    Serial.println(dbg);
    notify.writeValue(dbg);

    Serial.print("PINS D1 ");
    Serial.println(1);
    Serial.print("PINS A1 ");
    Serial.println(A1);
//    Serial.print("PINS LED ");
//    Serial.println(LED_BUILTIN);
//    Serial.print("PINS LED_PWR ");
//    Serial.println(LED_PWR);
  }

  if (ctrl.written())
  {
    // ctrl Characteristic is unsigned inte, but it is devided into 4 bytes
    // [speed, steer, notUsed, notUsed]
    byte data[4];

    ctrl.readValue(data, 4);
    
    Speed(data[0]);
    Steer(data[1]);
    Serial.print(".");

  }
  else
  {
//    int mm = millis()/100;
  //  analogWrite(LED_BUILTIN, mm & 0xFF);
  }
}
