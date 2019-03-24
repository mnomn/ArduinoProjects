# DoorBellBLE

A simple modification of the buzzer sketch provided by arduino tutorials.

If PLAY_ON_INTERRUPT_PIN is defined, it will play a melody when it gets an interrupt on pin. If not set it plays a melody when it is powered up (by door bell).

Use an arduino without USB, becasue usb bootloader causes a delay before play.

Connect a preconfigured ble board to the arduino/microcontroller and it will be powered up and start broadcasting when the door bell rings.
