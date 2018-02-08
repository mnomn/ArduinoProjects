/*
Control three colored led strip. It has eight modes and can shine stedy ot pulsate in various colors and pulse speed.

Tested with Arduino nano and digispark

Connection:
Three PWM outputs (RED/GREEB/BLUE_LEDS) to the base/gate of three transistors (via a resistor).
Pin 4 (aka RED_LEDS) -> 1kOhm -> base of NPN

Button between BUTTON_PIN and ground. Choose a pin with pull up or change code to pinMode(BUTTON_PIN, INPUT_PULLUP).

*/


#include <avr/eeprom.h>

#ifdef ARDUINO_AVR_NANO
  #define RED_LEDS 9
  #define BLUE_LEDS 10
  #define GREEN_LEDS 11
  #define ONBOARD_LED 13
  #define BUTTON_PIN 3
#elif ARDUINO_AVR_DIGISPARK
  #define RED_LEDS 4
  #define BLUE_LEDS 1
  #define GREEN_LEDS 0
  #define ONBOARD_LED 1

  // Digispark: Pin is conected to physical resistor.
  // Button does not work properly when powered to USB
  #define BUTTON_PIN 3

#else
// Compilation will fail
#endif

// Color pins/times etc
#define NO_OF_COLORS 3
#define NO_OF_MODES 8
#define INDEX_RED 0
#define INDEX_BLUE 1
#define INDEX_GREEN 2
int current_color[NO_OF_COLORS] = {0, 0, 0};
int target_color[NO_OF_COLORS] = {0, 0, 0};
int pins[NO_OF_COLORS] = {RED_LEDS, BLUE_LEDS, GREEN_LEDS};
long next_time[NO_OF_COLORS] = {0, 0, 0};
#define UPDATE_INTERVAL 100

// Modes. Cycled by button click.
// 2^3 (8) modes.
typedef enum {
  MODE_DARK,
  MODE_AMBIENT_RED,
  MODE_AMBIENT_BLUE,
  MODE_AMBIENT_DARK,
  MODE_AMBIENT_BRIGHT,
  MODE_AMBIENT_COLORS,
  MODE_AMBIENT_FAST,
  MODE_FULL
} mode_type;

mode_type mode = MODE_FULL;

long loop_time = 0;
int button = HIGH;

void blinkOnboard(int ms) {
  digitalWrite(ONBOARD_LED, HIGH);
  delay(ms);
  digitalWrite(ONBOARD_LED, LOW);
  delay(ms);
}

void updateTargetColors(long now) {

  // For solid modes, do nothing
  if (mode == MODE_DARK || mode == MODE_FULL)
  {
    return;
  }

  for (int i = 0; i<3; i++)
  {
    if (now > next_time[i])
    {
      if (mode == MODE_AMBIENT_FAST)
      {
        next_time[i] = random(200, 700);
      }
      else
      {
        next_time[i] = random(2000, 7000);
      }
      // Will overflow after 49 days
      next_time[i] += now;
      if (mode == MODE_FULL)
      {
        target_color[i] = 255;
      }
      else if(mode == MODE_AMBIENT_DARK || mode == MODE_AMBIENT_FAST)
      {
        target_color[i] = random(0, 128);
      }
      else if(mode == MODE_AMBIENT_BRIGHT)
      {
        target_color[i] = random(128, 255);
      }
      else if(mode == MODE_AMBIENT_RED)
      {
        if (i == INDEX_RED)
        {
          target_color[i] = random(128, 255);
        }
        else
        {
          target_color[i] = random(0, 255);
        }
      }
      else if(mode == MODE_AMBIENT_BLUE)
      {
        if (i == INDEX_BLUE)
        {
          target_color[i] = random(128, 255);
        }
        else
        {
          target_color[i] = random(0, 255);
        }
      }
      else if(mode == MODE_AMBIENT_COLORS)
      {
        // Random if color high or low. Most colors low.
        // Causes big shift in colors.
        if ((now>1)%3 == 0)
        {
          // One chance in three bright
          target_color[i] = random(128, 255);
        }
        else
        {
          // two out of three, dark
          target_color[i] = random(0, 255);
        }
      }
      else
      {
        // Unknown, impossible!
        target_color[i] = 0;
      }
    }
  }
}

void showColors(long now, long update_interval) {
  // For solid modes, do nothing
  if (mode == MODE_DARK || mode == MODE_FULL)
  {
    return;
  }

  for (int color = 0; color < NO_OF_COLORS; color++)
  {
    int colordiff = target_color[color] - current_color[color];
    // How long time untill new color shall be reached
    long timediff = next_time[color] - now;
    // How many more steps to reach target time:
    int steps = timediff / update_interval;
    if (steps == 0) return;
    colordiff /= steps;
    current_color[color] += colordiff;
    analogWrite(pins[color], current_color[color]);
  }
}

void initMode(int mode)
{
  if (mode == MODE_FULL)
  {
    analogWrite(pins[0], 255);
    analogWrite(pins[1], 255);
    analogWrite(pins[2], 255);
  }
 else if (mode == MODE_DARK)
 {
    analogWrite(pins[0], 128);
    analogWrite(pins[1], 128);
    analogWrite(pins[2], 128);
 }
  // Reset all times to start new mode now
  for (int ix = 0; ix < NO_OF_COLORS; ix++)
  {
    next_time[ix] = 0;
  }

}

void readMode()
{
  eeprom_busy_wait();
  mode = (mode_type)eeprom_read_byte((const uint8_t *) 0);
  if (mode >= NO_OF_MODES) {
    mode = (mode_type) 0;
  }
  indicateMode();
  initMode(mode);
}

void indicateMode()
{
  // Indicate mode by showing a color 0b000 - 0b111 (0 - 7)
  analogWrite(pins[0], 255 * (mode & 1));
  analogWrite(pins[1], 255 * (mode & 2));
  analogWrite(pins[2], 255 * (mode & 4));
  delay(300);
}

void incMode()
{
  mode = (mode_type)((mode + 1) % NO_OF_MODES);

  indicateMode();

  eeprom_busy_wait();
  eeprom_update_byte((uint8_t *) 0 , (uint8_t)mode);
  initMode(mode);
}

void setup() {
#ifndef ARDUINO_AVR_DIGISPARK
  Serial.begin(74880);
  while (!Serial)
  {
    delay(1);
  }
#endif
  pinMode(RED_LEDS, OUTPUT);
  pinMode(BLUE_LEDS, OUTPUT);
  pinMode(GREEN_LEDS, OUTPUT);
  pinMode(BUTTON_PIN, INPUT); // Pin 3 has a physical pull up resistor

  readMode();
}

void loop() {
  long now = millis();

  // Check if button pressed
  int temp_button = digitalRead(BUTTON_PIN);
  if (button != temp_button)
  {
    button = temp_button;
    if(temp_button == LOW)
    {
      incMode();
    }

    // Delay for a while to hadle bounses.
    delay(100);
  }

  if (now < loop_time+UPDATE_INTERVAL)
  {
    delay(5);
    return;
  }
  loop_time = now;

  updateTargetColors(now);
  showColors(now, UPDATE_INTERVAL);
}

