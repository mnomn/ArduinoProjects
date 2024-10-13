# SuperSimpleSwitch

Firmware for an ESP8266 based wifi switch like Sonoff. Switch power with the button or with the http API.

## First boot, configuration

Device will setup a wifi network for configuration, called something like ESP_192.168.4.1. Connect a phone or cumputer the new network, the default wifi password is "configpass". When connected to the "configuration wifi" go to ip 192.168.4.1 and fill in your normal wifi information: SSID and password. Optionally, set up basic authentication for the api calls to the switch, so clients need to add username and password when they call the switch api.

When the device is rebooted it will connect to your normal wifi network. The ip number of the device in the normal wifi can be found with a network scan or in the router.

## Button

- Short press to toggle the relay.
- Medium long press (over 2 sec): go to config mode. Configure in web UI or restart to ignore configuration.
- Long press (over 6 sec) enter OTA upgrade mode.

## Web UI

In web browser: http://{ip}/  
Documentation (short version of this page)

## Http API

Text response
* Read current value: GET http://{ip}/switch
* Turn power on: POST/PUT http://{ip}/switch?set=1
* Turn power off: POST/PUT http://{ip}/switch?set=0 
* Toggle power: POST/PUT http://{ip}/switch?set=t 

Response in json:
* http://{ip}/switch?set=1&json=1

Add basic authentication
* http://usener:pass@{ip}/switch?set=t
