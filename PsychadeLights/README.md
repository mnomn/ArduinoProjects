# PsychadeLights

Psychadelic three color led strip controller

Control three colored led strip. It has eight modes and can shine stedy ot pulsate in various colors and pulse speed.

Tested with Arduino nano and digispark

## Connection
Three PWM outputs (RED/GREEB/BLUE_LEDS) to the base/gate of three transistors (via a resistor).
Pin 4 (aka RED_LEDS) -> 1kOhm -> base of NPN

Button between BUTTON_PIN and ground. Choose a pin with pull up or change code to pinMode(BUTTON_PIN, INPUT_PULLUP).
