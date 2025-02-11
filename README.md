# Homekit Thermostat

- introduction to the project

- video of how it works

## How it works
- describe code

## WiFi provisioning
- how wifi access is set up

## Homekit
- describe homekit
- library used, what it does

## GUI
- describe lvgl & display

## Peripherals
- temperature ,relay

## Homekit Setup
After first start, the thermostat will show a WiFi setup screen with a QR code. Install [ESP Provisioning app](https://apps.apple.com/us/app/esp-ble-provisioning/id1473590141) to configure your WiFi network credentials first.

- SHOW WIFI QR CODE SCREEN

After that, in order to add the thermostat to your Apple Home app, scan the following QR code.

Unfortunately, these two actions cannot be done in a single action, as Apple doesn't support WiFi provisioning for [MFi](https://mfi.apple.com/) uncertified (custom) Homekit devices. 

Homekit Code (Scan this after WiFi is provisioned)

<img src="./assets/homekit-qrcode.png" width="200"/>

## Components
- esp32 - write exact model
- sht40
- relay type
- display type