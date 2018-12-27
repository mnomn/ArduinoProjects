# DoorBellBLE

A simple modification of the buzzer sketch provided by arduino tutorials.

If PLAY_ON_INTERRUPT_PIN is defined, it will play a melody when it gets an interript on pin. If not set it lays a melody when it is powered up (by door bell).

Connect a ble board and it will be powered whn door bell rings, so it can broadcast a preconfirured ble signal.

Use an arduino without USB, becasue usb bootloader causes a timeout.
