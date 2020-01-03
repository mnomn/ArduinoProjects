#include <Arduino.h>
#include <TinyXtra.h> // https://github.com/mnomn/TinyXtra
#include <avr/power.h>
#include <EEPROM.h>
/*
* ATTiny85 control water to flower.
* Two modes: Timer or sense soil moist.
* 
* Timer. 
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

UI:
# Timer
  Boot : One long led light to indicate timer.
  Then x short blinks. 2**x min between pumping.
  x  minutes
  1  2
  2  4
  3  8
  4  16
  5  32
  6  64
  Button: 
  - Short press: Blink setting.
  - 1 sec: start pump
  - 3 sec: Setup (led flicker), then press X times for timer value.
  - 10 sec: Erase setting

# Sensor
  One button, one led
  - Short press: Show threshold level. Long blinks and short blinks. long*100 + short*10
  - Long press: Set level
  - Periodically: 

## Eeprom layout:
        1 byte check. 0x1a #Ola :)
        1 short (2 bytes) ADC threshold value/x-timer
*/


// Comment out for sennsor mode
#define TIMER_MODE 1

#define BUTTON_PIN 0
#define LED_PIN 1
#define PUMP_PIN 2
#define MEASURE_PIN 3
#define SERIAL_DBG_PIN 4

#define PUMP_SEC 30

//#ifdef F_CPU
#undef F_CPU
#define F_CPU 8000000UL
//#endif

// eeprom tag to mark that there is somethin written on following bits
#define EEPROM_WRITTEN 0x1a

// #define EXTRA_DBG 1

TinyXtra tinyX;

short CHECK_INTERVAL_SEC = 30;
// unsigned long intervalms = (unsigned long)(CHECK_INTERVAL_SEC * 1000);
unsigned long prevms = 0;
unsigned short thresVal = -1;

void blinkX(int x) {
    digitalWrite(LED_PIN, HIGH);
    delay(x);
    digitalWrite(LED_PIN, LOW);
    delay(x);
}

void flicker() {
    int fl = 30;
    while(fl--) {
        digitalWrite(LED_PIN, fl%2);
        delay(50);
    }
}

void blinkVal(unsigned short v) {
    #ifdef EXTRA_DBG
    char mess[16];
    snprintf(mess, sizeof(mess), "V:%hu\n", v);
    tinyX.dbgString(mess);
    #endif

    // Blink value
    #ifdef TIMER_MODE
    if (v > 8) {
        flicker();
        return;
    }
    while (v-- > 0) {
        blinkX(250);
    }
    if (v == 0) blinkX(100);
    #else
    int l = v/100;
    int s = (v - l*100)/10;
    while (l > 0 && l--) {
        blinkX(250);
    }
    delay(500);
    while (s > 0 && s--) {
        blinkX(100);
    }
    // One extra blink to show "alive"
    blinkX(100);
    #endif
}

void removeThres() {
    thresVal = -1;
    EEPROM.put(0, (char)0);
}

void setThres(unsigned short v) {
    thresVal = v;
    EEPROM.put(0, (char)EEPROM_WRITTEN);
    EEPROM.put(1, v);
}

unsigned short countClicks() {
    unsigned long now = millis();
    unsigned long pressTime = now;
    int prev = HIGH;
    short count = 0;

    while((now - pressTime) < 3000 ) {
        int butt = digitalRead(BUTTON_PIN);
        now = millis();
        if (butt == LOW) {
            pressTime = now;
            if (prev != LOW) count++;
        }
        prev = butt;
        digitalWrite(LED_PIN, (now&70)?HIGH:LOW);// Flicker every 70 ms???
        delay(10);
    }

    return (unsigned short)count;
}

void pump() {
        // Soft start with PWM
        //int timeMS = 5000;

        // Kick the pump to start???
        int i = 5;
        while(i) {
            digitalWrite(PUMP_PIN, i%2);
            digitalWrite(LED_PIN, i%2);
            delay(100);
            i--;
        }
        // Pin is high.
        delay(PUMP_SEC * 1000);
        digitalWrite(PUMP_PIN, LOW);
        digitalWrite(LED_PIN, LOW);

}

void setup() {
    char checkOK = 0;

    CLKPR = (1<<CLKPCE); // Prescaler enable
    CLKPR = (1<<CLKPS0); // prescaler "0", 8 mhz

    pinMode(LED_PIN, OUTPUT);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    tinyX.setupInterrupt(0);
#ifdef EXTRA_DBG
    tinyX.dbgSetTxPin(SERIAL_DBG_PIN);

    // Boot hello!
    tinyX.dbgString((char*)"\nHi!\n");
    #ifdef TIMER_MODE
    tinyX.dbgString((char*)"Timer mode!\n");
    blinkX(2000);
    #else
    tinyX.dbgString((char*)"Sense mode!\n");
    #endif
#endif

    #ifdef TIMER_MODE
    // int i = 255;
    // while(i>0) {
    //     i--;
    //     analogWrite(LED_PIN, i);
    //     delay(100);
    // }
    // i=0;
    // while(i!=255) {
    //     i++;
    //     analogWrite(LED_PIN, i);
    //     delay(100);
    // }
    blinkX(2000);
    #endif

    // Get saved values
    EEPROM.get(0, checkOK);
    if (checkOK == EEPROM_WRITTEN)
    {
        char mess[32];
        EEPROM.get(1, thresVal);
        sprintf(mess, "TH/X-timer: %hu\n", thresVal);
        tinyX.dbgString(mess);
        blinkX(100);
        #ifdef TIMER_MODE
        CHECK_INTERVAL_SEC = 60;
        if (thresVal <= 8) {
            int po = pow(2, thresVal);
            CHECK_INTERVAL_SEC *= po;

        }
        #endif
    }
    else {
        flicker();
        tinyX.dbgString((char *)"No threshold\n");
    }
}

void handleButton() {
    char mess[32];
    unsigned long start=millis();

    while(digitalRead(BUTTON_PIN) == LOW) delay(20);

    unsigned long lenMS=(millis() - start);

    if (lenMS > 10*1000) {
        removeThres();
    }
    else if (lenMS > 3*1000) {
        // 3 sec: set timer
        #ifdef TIMER_MODE
        unsigned short thres = countClicks();
        #else
        unsigned short thres = analogRead(MEASURE_PIN);
        if (thres < 50 || (1023-50) < thres ) {
            // Angry blink to show failure
            sprintf(mess, "Bad: %hd\n", thres);
            tinyX.dbgString(mess);
            flicker();
            return;
        }

        #endif
        digitalWrite(LED_PIN, LOW);
        if (thres == 0) return;
        delay(1000);
        setThres(thres);
        blinkVal(thres);
    }
    else if (lenMS > 1000) {
        // 1 sec: pump
        pump();
    }
    else if (lenMS > 50) {
        // Short: Blink values?
        blinkVal(thresVal);

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

    boolean pumpNow = 1;
    #ifndef TIMER_MODE
    unsigned short v = analogRead(MEASURE_PIN);
    pumpNow = (thresVal > 0 && v > thresVal);
    #endif
    if(pumpNow) {
        pump();
    }
}
