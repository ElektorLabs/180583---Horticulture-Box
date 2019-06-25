# 180583 Horticulture Box  

This is the firmware for the horticulture box ( 180583 ) using the EPS32-Wrover as its main mcu. For project detais you can have a look at https://www.elektormagazine.com/labs/horticulture-box and also if you like the project or have some suggestions for us leave a comment. The software is written using the ardioino famework and the arduino-ide.  

## Getting Started

The horticulture box comes ready made, all you need to to is connect the wires. After you doublecheked the connection. Afterwords power on the system and follow the instuctions of our manual.If you need, or like to upgrade the firmware with a current version form the git we will provide here the instructions to build the firmware your own.


### Prerequisites

The preinstalled firmware uses the ArduinoOTA feature, so you can update the system over wifi. In raw cases it might be requierd to have a FTDI cable or a CH340G Board connected for an update. The pin description fot the serial update can be found also on the Elektor labs page. For an over the air update make sure that your compter and the horticulture box are in the same network and mDNS messages and UDP traffic is not blocked.

Make sure you have the ESP32 SPIFFS Uploader installed.

Clone the repository, or download it as .zip file. For stable releases please use the master branch. 

Make sure you have the Arduino IDE 1.8.x or newer installed on your computer, also the ESP32 board support package. 

Make sure you have the following libraries installed:
 * Time by Michael Margolis ( Arduino )
 * Arduino JSON 6.x
 * CRC32 by Christopher Baker 
 * PubSubClient
 * WebSockets by Markus Sattler
 * NTP client libary from https://github.com/gmag11/NtpClient/

 In the IDE select ESP32-Wrover with the default settings as board and click on verfiy. If no errors appear you can uplod it to the ESP32. For a generic programming guide see:
 https://www.elektormagazine.de/labs/esp32-getting-started

