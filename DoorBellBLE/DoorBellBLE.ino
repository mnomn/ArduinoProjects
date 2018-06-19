/*
  Melody

  Plays a melody

  circuit:
  - 8 ohm speaker on digital pin 8

  created 21 Jan 2010
  modified 30 Aug 2011
  by Tom Igoe

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Tone

  OlaA:

  Build for AtTiny85. No USB boot loder to get faster response
  Programmer: Arduino as ISP (Program AtTiny85 via arduino)
  Add BLE Module on 5v (no code)

  Notes from // http://www.astlessons.com/pianoforkidssv.html
  
  
*/

#include "pitches.h"

#ifdef ARDUINO_AVR_ATTINYX5
// 4 is next to ground
#define PLAYPIN 4
#else
#define PLAYPIN 4
#endif


#if 0
// Kalle Johansson
int melody2[] = {
  NOTE_DS4, NOTE_CS4, NOTE_FS3, NOTE_FS4, NOTE_FS4
};
// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  8, 8, 4, 4, 4,
};
int notes = 5;
int speed = 1000;
int repeat = 1;
#endif

#if 0
// Glassbilen
int melody[] = {
  NOTE_G3,
  NOTE_C4, NOTE_E4, NOTE_C4, NOTE_G3, NOTE_G3,
  NOTE_C4, NOTE_E4, NOTE_C4, NOTE_G3, NOTE_G3,
  NOTE_F4, NOTE_F4, NOTE_D4, NOTE_D4,
  NOTE_C4,
};
// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  8,
  8, 8, 8, 4, 8,
  8, 8, 8, 4, 8,
  4, 8, 4, 8,
  2
};
int notes = sizeof(noteDurations)/sizeof(noteDurations[0]);
int speed = 1300; // ms of a whole note
int repeat = 1;
#endif

#if 1
//Sweet child of mine
int melody[] = {
  NOTE_D3, NOTE_D4, NOTE_A3, NOTE_G3, NOTE_G4, NOTE_A3, NOTE_FS4, NOTE_A3
};
// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  8, 8, 8, 8, 8, 8, 8, 8
};
int notes = 8;
int speed = 3500; // ms of a whole note
#endif

void setup() {
  int repeat = 2;
  int noteDuration;
  int pauseBetweenNotes;
  while (repeat--)
  {
    // iterate over the notes of the melody:
    for (int thisNote = 0; thisNote < notes; thisNote++) {

      noteDuration = speed / noteDurations[thisNote];
      tone(PLAYPIN, melody[thisNote], noteDuration);

      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      pauseBetweenNotes = noteDuration * 1.10;
      delay(pauseBetweenNotes);
      // stop the tone playing:
    }
  }
  noTone(PLAYPIN);
}

void loop() {
  // no need to repeat the melody.
}
