# SuperSimpleSwitch

Firmware for a a ESP8266 based smart switch like for Sonoff. Switch power with the button button or with the web API.

## First boot, configuration

Device will become an access point, called something like ESP_192.168.4.1. Connect your wifi to the new network, the password for the configuration network is "configpass". After that goto ip 192.168.4.1 and fill in your normal wifi information: SSID and password. Optionally, set user:password for api calls.

When the device is rebooted it will be connected to your normal wifi network. The ip number of the device in the normal wifi can be found with a network scan or in the router.

Longpress on button to come back to the config mode

## Force configuration

Long press on button (5 sec) and the config will go to config mode. Restart to keep old config.

## Upgrade

Press button within 5 seconds after boot. This will enable OTA, and the device can be updated from arduino IDE.

## Http url

In web browser: http://{ip}/  
Documentation (short version of this page)

### API calls with plain text replies

* Get value /switch
* Turn power on: POST http://{ip}/switch?set=1
* Turn power off: POST/PUT http://{ip}/switch?set=0 
* Toggle power: POST/PUT http://{ip}/switch?set=t 

### API calls with json replies

Add "json=1" to get the reply in json format
* Toggle power: POST/PUT http://{ip}/switch?set=t&json=1
* Turn power on: POST/PUT http://{ip}/switch?set=1&json=1
* Turn power off: POST/PUT http://{ip}/switch?set=0&json=1
* Toggle power: POST/PUT http://{ip}/switch?set=t&json=1

### With password

* POST/PUT http://{user}:{pass}@{ip}/switch?set=t 
