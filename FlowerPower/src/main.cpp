#include <Arduino.h>
#include <EEPROM.h>

/*
  Pump 5v -> 65 mA, no load.
  
Res:
10 kOhm
500 Ohm

Short press: set level
Long press: reset/disable

Two pumps, two ADC, each adc has 2 chanels.
*/


#if defined(ARDUINO_AVR_ATTINYX5) || defined(ARDUINO_AVR_DIGISPARK)
#define ATT85
#endif

#define LED_PIN LED_BUILTIN

#ifdef ARDUINO_AVR_NANO
#define PUMPPIN0 4
#define PUMPPIN1 0
#else
// Not nano, Pro micro for example
PUMPPIN0 4
PUMPPIN1 0
#endif

#define PUMPS 2
// Settings:
int pumpPin[PUMPS] = {PUMPPIN0, PUMPPIN1};
int buttonPin[PUMPS] = {3, 0}; // Only 2 and 3 are possible, due to interrupt capabilities
unsigned int analogPin[PUMPS] = {A3,A4};
unsigned int adcPumpChanPins[PUMPS][2] = {5, 6, 0, 0}; // {P0Chan0, P0Chan1, p1Chan0, P1Chan1}

unsigned long nextMeasureTimeMS = 10UL*1000UL; // Some time before first measure, so user has time to set level
unsigned long measureIntervalMS = 20UL*1000UL;
unsigned long pumpTimeMS = 5UL*1000UL;


// Dynamic values
volatile unsigned long button0Time = 0;
volatile unsigned long button1Time = 0;

struct pumpStatus_T
{
    unsigned thres : 10;
    unsigned chan : 1;
    unsigned pumping : 1; // Not used
};

pumpStatus_T pumpStatus[2];

void button0();
void button1();
void handleButton(int b);
int measure(int pump, int chan);
void setLevel(int p);
void getLevels();
void checkPump(int p);
void pump(int p);
void showStatus();
void blink();

void setup() {
    Serial.begin(9600);
    pinMode(LED_PIN, OUTPUT);

    Serial.println("Starting!");

    for (int p = 0; p<PUMPS; p++)
    {
        if (pumpPin[p] > 0) pinMode(pumpPin[p], OUTPUT);
        if (buttonPin[p] > 0) pinMode(buttonPin[p], INPUT_PULLUP);
        if (buttonPin[p] > 0 && digitalRead(buttonPin[p]) == LOW) 
        {
            Serial.println("Manual pump ");
            pump(p);
        }
        attachInterrupt(digitalPinToInterrupt(buttonPin[p]), p?button1:button0, FALLING);
    }

    getLevels();
    showStatus();
    //Clear initial button press
    button0Time = 0;
    button1Time = 0;
}

void loop() {

    delay(10);

    handleButton(0);
    handleButton(1);

    if (millis() < nextMeasureTimeMS) return;

    nextMeasureTimeMS += measureIntervalMS;

    checkPump(0);
    checkPump(1);
}

int measure(int p, int chan)
{
    int cpin = adcPumpChanPins[p][chan];
    if (cpin <= 0) return -1;

    Serial.print(" (cpin ");
    Serial.print(cpin);
    Serial.print(") ");
    pinMode(cpin, OUTPUT);
    digitalWrite(cpin, HIGH);
    delay(200);
    int val = analogRead(analogPin[p]);

    digitalWrite(cpin, LOW);
    pinMode(cpin, INPUT);
    return val;
}

void button0()
{
    if(button0Time) return;
    button0Time = millis();
}

void button1()
{
    if(button1Time) return;
    button1Time = millis();
}

int calcButton(int b) // Button 0 or 1
{
    unsigned long buttonTime = b?button1Time:button0Time;
    int pin = buttonPin[b];

    if (!buttonTime) return 0;
    if (pin<=0) return 0; // pin disabled

    int bval = digitalRead(pin);
    if (bval == HIGH)
    {
        // Calc how long button is pressed
        unsigned long now = millis();
        unsigned long len = now - buttonTime;
        if(b) button1Time = 0;
        else button0Time = 0;
        if (len < 100) return 0; // Too short. Bounse?
        if (len < 3000) return 1; // short
        return 2; // Long reset
    }
    return 0;
}

void handleButton(int b)
{
    int press = calcButton(b);
    if (!press) return; // Too short or button not released

    if (press == 1)
    {
        blink();
        setLevel(b);
    }
    if (press == 2)
    {
        pumpStatus[b].thres = 0;
        // Reset
        blink();
        blink();
        blink();
        EEPROM.put(b, pumpStatus[b]);
    }
    getLevels();
    Serial.print("buttonPressed: ");
    Serial.println(press);
    Serial.println(sizeof(pumpStatus[1]));
}

void setLevel(int p)
{
    int res0 = measure(p, 0);
    int res1 = measure(p, 1);
    // Which is the most "middle"?
    int d0 = 512-res0;
    if (d0 < 0) d0 *= -1;
    int d1 = 512-res1;
    if (d1 < 0) d1 *= -1;

    if (d0 < d1)
    {
        pumpStatus[p].thres = res0;
        pumpStatus[p].chan = 0;
    }
    else
    {
        pumpStatus[p].thres = res1;
        pumpStatus[p].chan = 1;
    }
    EEPROM.put(sizeof(pumpStatus[p]) * p, pumpStatus[p]);

}

void getLevels()
{
    EEPROM.get(0, pumpStatus[0]);
    EEPROM.get(sizeof(int), pumpStatus[1]);
}

void checkPump(int p)
{
    if (pumpPin[p] <= 0) {
        return;
    }

    Serial.print("Check pump: ");
    Serial.print(p);
    Serial.print(" ");

    if (pumpStatus[p].thres==0) {
        Serial.println("Disabled");
        return;
    }

    int meas = measure(p, pumpStatus[p].chan);
    Serial.print(", Chan ");
    Serial.print(pumpStatus[p].chan);
    Serial.print(", Meas: ");
    Serial.println(meas);

    Serial.print("DBG: Other chan: ");
    Serial.println(measure(p, (pumpStatus[p].chan)?0:1));

    int thr = pumpStatus[p].thres;
    // High volt-> High res -> Dry
    if (meas > thr) pump(p);
}

void pump(int p)
{
    Serial.print("Start pump ");
    Serial.print(p);
    digitalWrite(pumpPin[p], HIGH);
    delay(pumpTimeMS);
    digitalWrite(pumpPin[p], LOW);
    Serial.println("... done");
}

void showStatus()
{
    for (int i = 0; i<PUMPS;i++)
    {
        Serial.print("Status P");
        Serial.print(i);
        Serial.print(": ");
        if (buttonPin[i] > 0) 
        {
            Serial.print(pumpStatus[i].thres);
            Serial.print(", C:");
            Serial.println(pumpStatus[i].chan);
        }
        else
        {
            Serial.println("No button.");
        }
    }

    // Blink for set values
    if (buttonPin[0] > 0 && pumpStatus[0].thres>0) blink();
    if (buttonPin[1] > 0 && pumpStatus[1].thres>0)
    {
        blink();
        blink();
    }
}

void blink()
{
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
}
