// Board: AdafruitTrinket @8MHz
// or
// Digispark


#import "TinyXtra.h" // https://github.com/mnomn/TinyXtra

#ifndef LED_BUILTIN
#define LED_BUILTIN 1 
#endif

#define POWER_PIN 0
// LED_BUILTIN in is 1
#define BUTTON_PIN 2 // Has proper interrupt (but currently only generic interrupt is used)

#define SHORT_BLINK_SHIFT 128
#define SHORT_BLINK(m) (((m) & SHORT_BLINK_SHIFT)==0)

// #define SLEEPS_PER_HOUR 400 // Trinket 9s sleep: 400 sleeps per hour
#define SLEEPS_PER_HOUR 418 // Digispark clone. 8.6s sleep: 418  per hour
//#define SLEEPS_PER_HOUR 2 // For debug, 1 h is 16 sec

// #define DBG_BLINK 1

enum press {NOT_PRESSED, SHORT, LONG};

int timer_on_hours = 0;
int timer_on = 0;
unsigned long sleep_counter = 0UL;
unsigned long target_counter = 0UL;

TinyXtra tinyXtra;

void blink() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}

press pressed() {
  press ret = NOT_PRESSED;
  int tt = 0;
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
    tt++;

    if (tt > 200) { // tt * 10 ms = 2 sec
      ret = LONG;
      digitalWrite(LED_BUILTIN, SHORT_BLINK(millis()));
    }
    else if (tt > 3) {
      ret = SHORT;
    }
  }

  return ret;
}

void handle_button() {
  press p = pressed();

  if (p == NOT_PRESSED) return;

  if (p == SHORT) {
    // TODO: What will happen to timer mode
    digitalWrite(POWER_PIN, !digitalRead(POWER_PIN));
    return;
  }

  // p is long pressed

  // Blink to show current settins
  if (timer_on_hours) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(2000);
    for (int i = 0; i < timer_on_hours; i++)
    {
      blink();
    }    
    delay(2000);      
  }

  // Click to set hours. Longpress to reset.
  unsigned long now = millis();
  unsigned long last_click = now;
  int new_setting = 0;
  while ((unsigned long)(now - last_click) < 3000UL) {
    now = millis();
    // Fast blink to indicate setting mode.
    digitalWrite(LED_BUILTIN, SHORT_BLINK(now));
    p = pressed();
    if (p == SHORT) {
      timer_on_hours++;
      timer_on = 1;
      last_click = now;
      new_setting = 1;
    }
    if (p == LONG) {
      timer_on_hours = 0;
      timer_on = 0;
      new_setting = 0;
      break;      
    }    
  }

  // If timer set, turn on power
  if (new_setting) set_power(timer_on);
}

void set_power(int power) {
  digitalWrite(POWER_PIN, power);
  sleep_counter = 0;
  int hh = power?timer_on_hours:(24-timer_on_hours);
  target_counter = hh*SLEEPS_PER_HOUR;
}

void add_counter() {
  if (target_counter == 0) return;
  sleep_counter++;
  
  if (target_counter && sleep_counter >= target_counter) {
    timer_on = !timer_on;
    // Timer overrides whatever manual on/off switches that are done. 
    set_power(timer_on);
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(POWER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  tinyXtra.setupInterrupt(BUTTON_PIN);
}

void loop() {
  handle_button();

  // Go to sleep
  #ifdef DBG_BLINK
  digitalWrite(LED_BUILTIN, HIGH);
  delay(50);
  digitalWrite(LED_BUILTIN, LOW);
  #endif
  tinyXtra.sleep_8s();
  add_counter();
}
