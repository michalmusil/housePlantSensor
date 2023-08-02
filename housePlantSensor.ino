#include <WiFiManager.h>
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include <BH1750.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>


// DECLARATIONS
  // Constants
  String communicationIdentifier = "T43u6Jjr1keIY3E";
  #define moisturePin A0
  #define oneWirePin 0
  #define minMoisture 670 // Equivalent to moisture sensor being completely dried out in air
  #define maxMoisture 270 // Equivalent to moisture sensort being in soaking wet soil

  #define secondsBetweenMeasurements 1800 // Measurement interval in seconds
  #define secondsForWifiSetup 360 // Time after startup for end user to configure WiFi connection
  #define connectTimeoutSeconds 20 // Time after which WiFiManager gives up connecting 

  String mac;

  // Api endpoints
  String serverName = "http://hpma.aspifyhost.cz/";
  String measurementPostEndpoint = serverName + "api/v1/measurements";

  // Peripheries
  OneWire oneWire(oneWirePin); // Initializing 1wire bus for temperature sensor
  DallasTemperature thermometer(&oneWire);
  BH1750 lightSensor;

  //Helper constructs
  struct Measurement{
    float temperature;
    int moisture;
    unsigned int lightIntensity;
  };

  String powerOnStartupCode = "Power On";

// END OF DECLARATIONS



void setup() {
  // Setting up the peripherals
  Serial.begin(9600);
  Wire.begin();
  thermometer.begin();
  lightSensor.begin();
  delay(2000);
  
  String startupType = ESP.getResetReason();
  Serial.println(startupType);
  WiFiManager wm;
  if(startupType == powerOnStartupCode){
    accessPointStartupProcedure(wm);
  } else {
    connectToSavedWifiCredentials(wm);
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("Connected to network");
    Measurement takenMeasurement = takeAveragedMeasurement();
    postMeasurement(takenMeasurement);
  } else {
    Serial.println("Failed to connect");
  }
 
  Serial.println("Going to sleep mode");
  ESP.deepSleep(secondsBetweenMeasurements * 1000000);
}

void loop() {
}


// WIFI CONFIG FUNCTIONS
  void accessPointStartupProcedure(WiFiManager& wm){
    // Setting up AP Server user interface
    std::vector<const char *> frontPageMenuItems = {"wifi","exit"};
    wm.setTitle("Plant monitor");
    wm.setMenu(frontPageMenuItems);
    wm.setConfigPortalTimeout(secondsForWifiSetup); // After timeout, AP is turned off and normal measuring mode begins
    wm.setConnectTimeout(connectTimeoutSeconds); // Connection timeout
    wm.setAPClientCheck(false);
    wm.setWebPortalClientCheck(false);
    wm.setShowInfoErase(false);
    wm.setShowInfoUpdate(false);


    const char* SSID = getAPSSID();
    const char* password = getAPPassword();
    bool newWifiConnected = wm.startConfigPortal(SSID, password);
    if(newWifiConnected){
      mac = WiFi.macAddress();
      Serial.println(mac);
    } else {
      connectToSavedWifiCredentials(wm);
    }
  }

  void connectToSavedWifiCredentials(WiFiManager& wm){
    // Try to connect from previously remembered WiFi credentials
    wm.setConnectTimeout(connectTimeoutSeconds); // Connection timeout
    wm.setEnableConfigPortal(false);
    bool connected = wm.autoConnect();
    if(connected){
      mac = WiFi.macAddress();
      Serial.println(mac);
    }
  }

  const char* getAPSSID(){
    String base = "PLANT_MONITOR_";
    int suffixSize = 3;
    int commIdSize = communicationIdentifier.length();
    int startPosition = commIdSize - (suffixSize);
    String SSID = base + String(communicationIdentifier).substring(startPosition);
    return SSID.c_str();
  }

  const char* getAPPassword(){
    return communicationIdentifier.c_str();
  }
// END OF WIFI CONFIG FUNCTIONS



// HELPER FUNCTIONS
  // Measures all 3 parameters several times and returns avereged value
  Measurement takeAveragedMeasurement(){
    int numberOfMeasurements = 10;
    int delayBetweenIndividualMeasurements = 250;
    Measurement takenMeasurements[numberOfMeasurements];
    for(int i = 0; i < numberOfMeasurements; i++){
      Measurement measurement = takeSingleMeasurement();
      takenMeasurements[i] = measurement;
      delay(delayBetweenIndividualMeasurements);
    }

    float averagedTemperature = 0;
    int averagedMoisturePercentage = 0;
    unsigned int averagedLightIntensity = 0;

    for(int i = 0; i < numberOfMeasurements; i++){
      averagedTemperature = averagedTemperature + takenMeasurements[i].temperature;
      averagedMoisturePercentage = averagedMoisturePercentage + takenMeasurements[i].moisture;
      averagedLightIntensity = averagedLightIntensity + takenMeasurements[i].lightIntensity;
    }

    averagedTemperature /= numberOfMeasurements;
    averagedMoisturePercentage /= numberOfMeasurements;
    averagedLightIntensity /= numberOfMeasurements;

    return {averagedTemperature, averagedMoisturePercentage, averagedLightIntensity};
  }

  Measurement takeSingleMeasurement(){
    thermometer.requestTemperatures(); // command for reading the temperature
    float temperature = thermometer.getTempCByIndex(0); // get the temperature
    int moistureAnalog = analogRead(moisturePin); // read analog value of moisture sensor
    int moisturePercentage = mapAnalogMoistureToPercentage(moistureAnalog); // convert analog value to moisture percentage
    unsigned int lightIntensity = lightSensor.readLightLevel(); // read the light intensity in lux from I2C
    return {temperature, moisturePercentage, lightIntensity};
  }  

  // Converts analog moisture sensor input to percentage of moisture
  int mapAnalogMoistureToPercentage(int analogMoisture){
    int minPercentage = 0;
    int maxPercentage = 100;
    int moisturePercentage = map(analogMoisture, minMoisture, maxMoisture, minPercentage, maxPercentage);
    if(moisturePercentage > maxPercentage){
      moisturePercentage = maxPercentage;
    }
    else if(moisturePercentage < minPercentage){
      moisturePercentage = minPercentage;
    }
    return moisturePercentage;    
  }





  // Turns taken measurement into postable JSON
  String getMeasurementJsonString(Measurement measurement){
    String postRequest = "{ \"deviceCommunicationIdentifier\": \"" + communicationIdentifier +
     "\", \"deviceMacAddress\": \"" + mac + 
     "\", \"measurementValues\": [ " +
     "{ \"type\": 0, " +
     "\"value\": " + String(measurement.temperature) +
     " }," +
     "{ \"type\": 1, " +
     "\"value\": " + String(measurement.lightIntensity) +
     " }," +
     "{ \"type\": 2, " +
     "\"value\": " + String(measurement.moisture) +
     " }" +
     "]" +
     "}";
     
    return postRequest;
  }

  // Posts measurement to defined API endpoint
  bool postMeasurement(Measurement measurement){
    WiFiClient client;
    HTTPClient http;

    http.begin(client, measurementPostEndpoint);
    http.addHeader("Content-Type", "application/json");

    String measurementJson = getMeasurementJsonString(measurement);
    int httpResponseCode = http.POST(measurementJson);
    String response = "Post proceeded with code: " + String(httpResponseCode) + ". \n Posted object: " + getMeasurementJsonString(measurement);
    Serial.println(response);
    return httpResponseCode == 200;
  }
// END OF HELPER FUNCTIONS

