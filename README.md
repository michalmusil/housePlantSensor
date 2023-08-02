# Prototype of an application for monitoring houseplant environment - MEASURING DEVICE
This is a 1/3 of a complete project solution for a bachelor's thesis. The thesis (and the underlying project) deals with designing and implementing multitier application for continuous monitoring of houseplant's environment (ambient temperature, light intensity and soil moisture). The application is meant to be used by multiple users with multiple plants and measuring devices.

# The functional parts of the projects
1. REST API with database for application logic (ASP.NET Core with Entity Framework Core): https://github.com/michalmusil/housePlantMeasurementsApi
2. Android navive app for presenting the data to users (Kotlin, Jetpack Compose): https://github.com/michalmusil/plant_monitor
3. ESP 8266 code for the device measuring the houseplant environment (C++ in Arduino IDE): https://github.com/michalmusil/housePlantMeasuringDevice

To make the project work as a whole, you have to get all of the parts working together.

## Measuring device
The measuring device serves as a data collection tool for the project. Users register and connect their measuring devices to local WiFi network. After sticking the device into the soil of a houseplant's pot and connecting the device to power, it starts up every ~30 minutes to measure plant's environment and sends the data to the REST API. The measurements taken by this device will then be assigned to a plant, that the measuring device is assigned to and saved in the DB for user to see.

## Wiring up
The code in this repo is meant to run on an ESP 8266 WeMos D1 mini microcontroller module with the following peripheral sensors connected to it:

* Ambient temperature sensor DS18B20: https://lastminuteengineers.com/ds18b20-arduino-tutorial/
* Light intensity module BH1750: https://randomnerdtutorials.com/arduino-bh1750-ambient-light-sensor/
* Soil moisture sensor HW-390 (or similar): https://blog.zerokol.com/2021/01/hw-390-capacitive-soil-moisture-sensor.html

The schema for wiring up the sensors with Wemos D1 mini ESP 8266 module is in the schema folder. The resistor used for the DS18B20 1-Wire bus is 4.7k.


## How to make it work
After wiring up the device, you need to:
1. Change the URL of the REST API to your instance in the .ino code.
2. Generate and change the communication identifier (has to be 15 character string) in the .ino code. This identifier should be unique for each and every device
3. Add the device's representation to REST API's database via the /measurements POST method (must add your ESP's MAC address and communication identifier that you generated)
4. Upload the changed .ino code into your measuring device
5. Register the new device with your user account in the Android app using the MAC and communication identifier and assign it to one of your plants.
5. Power the device on by connecting it to power. Connect the device to your local WiFi network (manual is included on the device detail in the Android app)
6. The device will now start to measure your plant every ~30 minutes and send the data to REST API.

Please note, that you can't upload code into ESP 8266 after connecting the RESET pin with D0 pin. Therefore you must disconnect these two pins before attempting to upload the new .ino file version to the device. You can safely connect them again after the upload has finished.

## Code prerequisites
* Installing the speciffic USB driver for your ESP 8266 module on your computer
* Installing the necessary libraries included at the start of the .ino file