

/*
 * Charlieplexing christmas lights. 5 pins on ATTIny85 controlls 20 leds.
 * 
 * ATTInyCore, Attiny85
 * 
 *  To use ISPtinyISP on linix/ubuntu:
 *  USBTinyISP, set udevrule
 *  SUBSYSTEM==”usb”, ATTR{idVendor}==”1781″, ATTR{idProduct}==”0c9f”, GROUP=”adm”, MODE=”0666″
*/
#define NO_CHANNELS 5
int ledpins[NO_CHANNELS] = {0, 1, 2, 3, 4};
int modes[NO_CHANNELS] = {1, 1, 1, 1, 1};

unsigned long interval = 5000UL;
unsigned long prev = 0;

void setPinMode(int chan, int mode)
{
  #if MY_DEBUG
  Serial.print("\nChan ");
  Serial.print(chan);
  Serial.print(" pin ");
  Serial.print(ledpins[chan]);
  Serial.print(" ");
  Serial.print(mode==OUTPUT?"OUT":"IN");
  #endif
  // pinMode(ledpins[chan], mode);
  #if 1
  if (mode == OUTPUT && modes[chan] != OUTPUT) {
    pinMode(ledpins[chan], OUTPUT);
    modes[chan] = OUTPUT;
  }
  if (mode == INPUT && modes[chan] != INPUT) {
    pinMode(ledpins[chan], INPUT);
    modes[chan] = INPUT;
  }
  #endif
}

void setOut(int chan, int val /*HIGH/LOW*/)
{
  setPinMode(chan, OUTPUT);
  #if MY_DEBUG
  Serial.print(val==HIGH?" H":" L");
  #endif
  digitalWrite(ledpins[chan], val);
}

void Sequence()
{
  // pins are default input.
  for(int i=0; i<NO_CHANNELS; i++) {
    for (int j=i+1; j<NO_CHANNELS; j++) {
      // One direction
      setOut(i, HIGH);
      setOut(j, LOW);
      delay(500);
      // Other direction
      setOut(i, LOW);
      setOut(j, HIGH);
      delay(500);
      // Turn off
      setPinMode(i, INPUT);
      setPinMode(j, INPUT);
    }
  }
}

void setup() {
//  Serial.begin(9600);
  randomSeed(analogRead(0));
  Sequence();
}

void loop() {
  unsigned long now = millis();
  unsigned long this_period = (unsigned long)(now-prev);
  if (this_period < interval) {
    return;
  }
  prev = now;
  interval = (unsigned long)random(1000);

  // Only change one pin at the time
  int chan = (int)random(NO_CHANNELS);
  int lhi = (int)random(5);// 0, 1:low: 2, 3: high, 4: IN (Less likley to be IN)
  if (lhi == 4) {
    setPinMode(chan, INPUT);
  } else if (lhi > 1) {// 2 or 3
    setOut(chan, HIGH);
  } else { // 0 or 1
    setOut(chan, LOW);
  }}
