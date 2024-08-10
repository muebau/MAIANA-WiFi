# MAIANA-WiFi
This project is an approach for an new adapter for the MAIANA AIS by peterantypas (https://github.com/peterantypas/maiana).

The idea is to be able to operate the AIS only with a mobile. Many people use their mobile to navigate and would love to have a integration of the AIS signals into their prefered app.

Upon making connection the ESP will send the most recent AIS messages from all ships seen in the last 10 minutes.
This way your app will be updates fast with the surrounding ships.

We use a ESP32 and the "USB-only" adapter by peterantypas. All messages between the serial output and the AIS will be forwarded unchanged, the ESP just listens in the middle and will add some extra messages. The adapter is expanded through a additional button "config mode". If this button is pressed, the ESP enters the configuration mode, in which he provides a unencrypted WiFi. In this WiFi the user can configure the AIS and WiFi settings. The ESP can either connect to a existing WiFi or create its own. After 5 minutes the ESP closes the connection to the configuration page and switch to normal operation mode. In the "normal" mode the ESP provides each message of the AIS over WiFi as NMEA 0183, which can be used with many apps. 

This way this adapter mades the AIS a stand alone system if wanted and the user just have to plug it in and it works. If the adapter also gets a step up converter, it could be powered over USB, which makes it even more simple to use. Just grab a USB cable and a USB car plug and it works out of the box. As the configuration is made in the browser, this approach is usable with all devices, smartphones, tablets or a laptop, independent of the brand. 

## How it looks
This is the latest version of the GUI

![](docu/img/ScreenshotMAIANA.png)


with a config page to configure the Maiana and other settings

![](docu/img/MaianaConfig.png)


with a detailed view for the GPS information

![](docu/img/gpsDashboard.png)

## OTA Firmware Update
Firmware can be update over-the-air OTA by going to the http://[your device ip]/update page and select the file to upload (firmware or SPIFF file system with webpages).
Note that when the SPIFF / webpages are updated configuration details like port setup and wifi details will be overwritten. (not the Maiana settings!)

## Included JS projects
* GPS parsing https://github.com/infusion/GPS.js
* AIS parsing https://github.com/mjaros/ais-decoder 
