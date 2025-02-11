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

## Electrical circuit

- [ESP32-C6](https://www.laskakit.cz/en/espressif-esp32-c6-devkitm-1-wifi-6--bluetooth-5--zigbee/) board
- [SHT-40](https://www.laskakit.cz/en/laskakit-sht40-senzor-teploty-a-vlhkosti-vzduchu/) temperature and humidity sensor
- [Optical relay](https://www.laskakit.cz/en/1-kanal-5v-rele-modul-s-optickym-oddelenim--high-low-level--250vac-10a/) 5V 250V 10A 
- [2.4" 240x320](https://www.laskakit.cz/en/2-4--palcovy-barevny-dotykovy-tft-lcd-displej-240x320-ili9341-spi/) TFT display

<p align="center">
<img src="./assets/diagram.png" width="400" />
</p>

| SHT-40 | ESP32 |
| --- | ---- |
| VCC | 3.3V |
| GND | GND  |
| SDA | 6    |
| SCL | 7    |

| Relay | ESP32 |
| --- | ---- |
| VCC | 5V |
| GND | GND  |
| IN | 12    |

| LCD | ESP32 |
| --- | ---- |
| VCC | 3.3V |
| GND | GND  |
| SCLK | 2  |
| MISO | 1  |
| MOSI | 10  |
| DC | 3    |
| Reset | 4 |
| CS | 0    |
| Backlight | 5 |
| T_CS | 11   |
| T_CLK | 2  |
| T_MISO | 1 |
| T_MOSI | 10 |

_Note_: Both LCD and the touch screen share the same SPI connection.