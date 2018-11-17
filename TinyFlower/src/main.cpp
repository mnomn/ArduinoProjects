#include <Arduino.h>
#include <TinyXtra.h> // https://github.com/mnomn/TinyXtra
#include <avr/power.h>
#include <EEPROM.h>
/*
* ATTiny85 control water to flower.
* Use USB pullup (1.5kOhm) to compare soil moist
* GND----soil----P3/A3---1K5---+5
*
* Pins
* ----
* P0: Button
* P1: LED
* P2: Pump
* P3: ADC
* P4: Serial DBG


Connect:
- Power: From USB connector (data pins not connected)
- Soil:Directly on ADC and GND
- Pump:  plus 5v, minus -> Mosfet drai  
- Mosfet: G -> P2, Source -> GND, Drain: connector

UI: One button, one led
  short press: Show level. Long blinks and short blinks. long*100 + short*10
  long press: set level

## Eeprom layout:
        1 byte check. 0x1a #Ola :)
        1 short (2 bytes) ADC threshold value
*/

#define BUTTON_PIN 0
#define LED_PIN 1
#define PUMP_PIN 2
#define MEASURE_PIN 3
#define SERIAL_DBG_PIN 4

#define CHECK_INTERVAL_SEC 30
#define PUMP_SEC 5

//#ifdef F_CPU
#undef F_CPU
#define F_CPU 8000000UL
//#endif

// eeprom tag to mark that there is somethin written on following bits
#define EEPROM_WRITTEN 0x1a

#define EXTRA_DBG 1

TinyXtra tinyX;

const unsigned long intervalms = (unsigned long)(CHECK_INTERVAL_SEC * 1000);
unsigned long prevms = 0;
unsigned short thresVal = -1;

void blinkLong() {
    digitalWrite(LED_PIN, HIGH);
    delay(250);
    digitalWrite(LED_PIN, LOW);
    delay(250);
}

void blinkShort() {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
}

void flickerBad() {
    int fl = 30;
    while(fl--) {
        digitalWrite(LED_PIN, fl%2);
        delay(50);
    }
}

void blinkVal(unsigned short v) {
    // Blink value
    char mess[64];
    if (v<0) {
        flickerBad();
        return;
    }
    int l = v/100;
    int s = (v - l*100)/10;
    while (l > 0 && l--) {
        blinkLong();
    }
    delay(500);
    while (s > 0 && s--) {
        blinkShort();
    }
    // One extra blink to show "alive"
    blinkShort();
    #ifdef EXTRA_DBG
    snprintf(mess, sizeof(mess), "V:%hu\n", v);
    tinyX.dbgString(mess);
    #endif
}

void removeThres() {
    #ifdef EXTRA_DBG
    tinyX.dbgString((char*)"RemoveThreshold!\n");
    #endif
    thresVal = -1;
    EEPROM.put(0, (char)0);
}

void setThres() {
    char mess[64];
    unsigned short v = analogRead(MEASURE_PIN);
    #ifdef EXTRA_DBG
    sprintf(mess, "Set val: %hd\n", v);
    tinyX.dbgString(mess);
    #endif
    if (v < 50 || (1023-50) < v ) {
        // Angry blink to show failure
        sprintf(mess, "Bad val: %hd\n", v);
        tinyX.dbgString(mess);
        flickerBad();
    }
    else {
        // Long blink OK
        blinkLong();
    }
    thresVal = v;
    EEPROM.put(0, (char)EEPROM_WRITTEN);
    EEPROM.put(1, v);
}

void setup() {
    char checkOK = 0;

    CLKPR = (1<<CLKPCE); // Prescaler enable
    CLKPR = (1<<CLKPS0); // prescaler "0", 8 mhz

    pinMode(LED_PIN, OUTPUT);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    tinyX.setupInterrupt(0);
    tinyX.dbgSetTxPin(SERIAL_DBG_PIN);

    // Boot hello!
    tinyX.dbgString((char*)"\nHi!\n");
    blinkShort();
    digitalWrite(PUMP_PIN, HIGH);
    delay(1000);
    digitalWrite(PUMP_PIN, LOW);

    // Get saved values
    EEPROM.get(0, checkOK);
    if (checkOK == EEPROM_WRITTEN)
    {
        char mess[32];
        EEPROM.get(1, thresVal);
        sprintf(mess, "TH: %hu\n", thresVal);
        tinyX.dbgString(mess);
        blinkShort();
    }
    else {
        flickerBad();
        tinyX.dbgString((char *)"No threshold\n");
    }
}

void handleButton() {
    char mess[64];
    unsigned long start=millis();

    while(digitalRead(BUTTON_PIN) == LOW) delay(50);

    unsigned long lenMS=(millis() - start);

    if (lenMS > 10*1000) {
        removeThres();
    }
    else if (lenMS > 3*1000) {
        setThres();
    }
    else if (lenMS > 100) {
        // What? Pump? Blink values?
        #ifdef EXTRA_DBG
        unsigned short v = analogRead(MEASURE_PIN);
        blinkVal(v);
        sprintf(mess, "Stored threshold val: %hu\n", thresVal);
        tinyX.dbgString(mess);
        #endif
    }
    else {
        // Too short. Bounce? Sleep wake up?
        #ifdef EXTRA_DBG
        tinyX.dbgString((char *)"Bounce\n");
        #endif
    }

}
void loop() {
    int sleep_counter = 0;

    while(sleep_counter * 8 < CHECK_INTERVAL_SEC) {
        if (TinyXtra::interrupt) {
            #ifdef EXTRA_DBG
            tinyX.dbgString((char *)"INT\n");
            #endif
            handleButton();
            TinyXtra::interrupt = false;
        }

        sleep_counter++;
        #ifdef EXTRA_DBG
        tinyX.dbgString((char *)"Sleep\n");
        #endif
        tinyX.sleep_8s();
    }

    #ifdef EXTRA_DBG
    tinyX.dbgString((char *)"No more sleep\n");
    #endif

    if (thresVal == (unsigned short)-1) {
        tinyX.dbgString((char *)"Thres not set\n");
        return;
    }

    unsigned short v = analogRead(MEASURE_PIN);
    blinkVal(v);
    if (thresVal > 0 && v > thresVal) {
        digitalWrite(PUMP_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
        delay(PUMP_SEC * 1000);
        digitalWrite(PUMP_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
    }
}
