
#define USE_DHT
#define USE_BLE
// #define USE_BLE_NAME


// ATTiny85:
// https://github.com/SpenceKonde/ATTinyCore

#include <SoftwareSerial.h>
#ifdef USE_DHT
#include "dht.h" // https://github.com/RobTillaart/Arduino
#endif //USE_DHT
#ifdef ARDUINO_AVR_ATTINYX5
#include "TinyXtra.h" //https://github.com/mnomn/TinyXtra
#endif

/*
  Wemos pins:
  Serial: TX=GPIO1, RX=GPIO3
  SoftwareSerial() RX: GPIO14 D5, TX GPIO12 D6

  Attiny85
  SoftwareSerial() RX: 1, TX: 2
*/

// Protocol 16 bits
// Crappy BLE clone board. Jam all data into 16 bits.
// 6 bit humidity: 15 .. 10: humidity/2     , 0 .. 50
// 10 buit temp  :  9 ..  0: (temp + 30) *10, 0..1023

#define MYDEBUG
#define AT_NL "\r\n"

#ifdef ARDUINO_AVR_ATTINYX5
SoftwareSerial ble(1,2);

#define DBGSERIAL(x)
#define DBGSERIALC(x)
#define DEBWRITEC(c)

TinyXtra tX;
int sleep_count = 0;

#define POWER_PIN 4
#define DHTPIN 3
#else
// if esp8266...
SoftwareSerial ble(14,12);

#define DBGSERIAL(x) Serial.print(x)
#define DBGSERIALC(x) Serial.write(x)
#define DEBWRITEC(c) Serial.write(c)
#define DHTPIN 5 // Wemos D1
#define POWER_PIN LED_BUILTIN

#define LED_INVERTED
#endif

unsigned long intervalS = (5*60);
unsigned long nextSendS = 10; // first interval shorter

#ifdef USE_DHT
dht DHT;
#endif

#ifdef USE_BLE_NAME
// Round to "int + half", 2.3->2.5, 3.9->4.0 etc
void roundHalf(float tin, int *h, int *d)
{
	int sign = 1;
	if (tin < 0) sign = -1;
	int t2 = (tin + 0.25*sign)*2;
	*h = t2/2;
	*d = 5*sign*(t2%2);
}
#endif

void sendcmd(char *cmd)
{
  int pos = -1;
  char cmdbuff[32];
  char c;
  unsigned long timeout = millis() + 1000;

  #ifndef USE_BLE
  return;
  #endif

  if(cmd)
  {
    sprintf(cmdbuff, "AT+%s"AT_NL, cmd);
  }
  else
  {
    sprintf(cmdbuff, "AT"AT_NL, cmd);
  }

  #ifdef MYDEBUG
  #ifndef ARDUINO_AVR_ATTINYX5
  Serial.print("Send: ");
  Serial.println(cmdbuff);
  #endif
  #endif

  ble.print(cmdbuff);

  // Wait for response for max 1 sec
  while(millis() < timeout && !ble.available()) { delay(50); }

  // Read data
  while(ble.available())
  {
    c = ble.read();
    DBGSERIALC(c);
  }
}

void setup()
{
  ble.begin(9600);
#ifdef ARDUINO_AVR_ATTINYX5
  tX.disable_adc();
#else
  Serial.begin(9600);
  while(!Serial);
  Serial.print("SETUP... LETST START\n");
#endif

  // sendcmd("DEFAULT"); // Reset device
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  delay(300);// Give ble device to boot
  sendcmd("ADVI0");// broadcast interval
  sendcmd("ADVEN1");// broadcast

// Clear name
// #ifndef USE_BLE_NAME
//   sendcmd("NAME ");// Set name to space
// #endif

  digitalWrite(POWER_PIN, LOW);

}

void loop()
{
  char cmdbuff[32];
  int tint, tdeci;

#ifdef ARDUINO_AVR_ATTINYX5
  if (sleep_count * 8 < nextSendS) {
    digitalWrite(POWER_PIN, LOW);
    sleep_count++;
    tX.sleep_8s();
    return;
  }
  nextSendS = intervalS;
  sleep_count = 0;
#else
  if (millis() < nextSendS*1000)
  {
    digitalWrite(POWER_PIN, LOW);
    delay(1000);
    return;
  }
  DBGSERIAL("Measure!\n");
  nextSendS=nextSendS + intervalS;
#endif

  digitalWrite(POWER_PIN, HIGH);
  delay(300);// Wait for boot

#ifdef USE_DHT
  int dht_err = DHT.read21(DHTPIN);
#endif
  sendcmd("NAME");

  //wakeupBle();
#ifdef USE_DHT
#ifdef USE_BLE_NAME
  if(dht_err) {
    sprintf(cmdbuff,"NAME:(:(");
  }
  else
  {
    roundHalf(DHT.temperature, &tint, &tdeci);
    sprintf(cmdbuff,"NAME:)%d.%d:(%d", tint, tdeci, (int)DHT.humidity);
  }
#else
    sprintf(cmdbuff,"NAME:)%d:(%d", millis()%98, millis()%89);
#endif

  sendcmd(cmdbuff);
  delay(100);
#endif // #endif


  unsigned int t, h;
#ifdef USE_DHT
  // -30 ... +70 -> 0000  ... 1000
  t = (unsigned)((DHT.temperature + 30) * 10);
  h = (unsigned)(DHT.humidity/2);// 0 - 50
#else
  t = millis()%998;
  static int hh = 0;
  h = hh;
  hh++;
  if (hh>50) hh=0;

#endif
  // CHAR doe not work/updates, so I jam all data into UUID
  // sprintf(cmdbuff,"CHARFE01", h);
  // sendcmd(cmdbuff);
  // sendcmd("RESET");// Must reset for new UUID and char to broadcast
  unsigned int uuid = 0;

  if(! dht_err) {
    // No err
    uuid = (h << 10) + t;
  }
  sprintf(cmdbuff,"UUID%04X", uuid);

  sendcmd(cmdbuff);

  delay(100);
  sendcmd("RESET");// Must reset for new UUID and CHAR to broadcast

  // static int uuid_char = 0;
  // uuid_char++;
  // if (uuid_char == 256) uuid_char = 0;
  // sprintf(cmdbuff,"CHARFE%02X", uuid_char);
  // sendcmd(cmdbuff);
  // delay(500);
  // sendcmd("RESET");// Must reset for new UUID and CHAR to broadcast
  delay(100);
  sendcmd(NULL);//Send AT
  sendcmd("ADVEN1");// broadcast
  delay(1000);
  sendcmd("ADVEN0");// broadcast
}
