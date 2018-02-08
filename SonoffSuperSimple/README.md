# SonoffSuperSimple

Firmware for Sonoff wifi smart switch. Switch power with button, simple links on web interface or http requests (from bookmarks, app etc).

## First boot, configuration:
Device will become an access point, called something like ESP_192.168.4.1. Connect your wifi to the new network, the password for the configuration network is "configpass". After that goto ip 192.168.4.1 and fill in your normal wifi information: SSID and password. When the device is rebooted it will be connected to your normal wifi network. The ip number of the device in the normal wifi network is a bit tricky to find, but can found on the wifi router.

Longpress on button to come back to the config mode

## Http url:

/ Documentation (short version of this page)

### Plain text replies
* Get value /switch
* Turn power on: /switch?set=1
* Turn power off: /switch?set=0 
* Toggle power: /switch?set=t 

### Json replies:
Add "json=1" to get the reply in json format
* Toggle power: /switch?set=t&json=1
* Turn power on: /switch?set=1&json=1
* Turn power off: /switch?set=0&json=1
* Toggle power: /switch?set=t&json=1
