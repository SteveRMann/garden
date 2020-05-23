#define sketchName "garden.ino, V1.0"

/*
   Forked from GardenProject
   Removed sleep.

   IDE:
     Board: Lolin(Wemos) D1 R2 & Mini

   Reference:
   http://www.esp8266learning.com/wemos-mini-ds18b20-temperature-sensor-example.php#codesyntax_3

*/


#include <OneWire.h>            // Driver for DS18X Temperature Sensors.
#include <ESP8266WiFi.h>        // Connect (and reconnect) an ESP8266 to the a WiFi network.
#include <PubSubClient.h>       // connect to a MQTT broker and publish/subscribe messages in topics.
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

#ifndef Kaywinnet
#include "D:\River Documents\Arduino\libraries\Kaywinnet.h"  // WiFi credentials
#endif


// ****************************** Globals  ******************************
#define NODENAME "garden"                         //Must be unique on the net.
const char *connectName =  NODENAME "garden";     //Must be unique on the network
WiFiClient GardenClient;                          //The constructor MUST be unique on the network.

//Probe Calibration
int dry = 457;                  //Dry sensor
int wet = 811;                  //Wet sensor


#define hostPrefix NODENAME     // For setupWiFi()
char macBuffer[24];             // Holds the last three digits of the MAC, in hex.

const char *statusTopic = NODENAME "/status";             //Sends the temperature, moisture, IP and RSSI in one payload.
const char *statusJsonTopic = NODENAME "/statusJson";
const char *cmdTopic = NODENAME "/cmd";                   //Sends a command string: readTime, temp correction
const int mqttPort = 1883;


int sleepSeconds = 120;
const int pubsubDelay = 20;           //Time between publishes
long rssi;                            //Used in the WiFi tab
float tCorrection = 0.0;              //Temperature correction.
char rssi_string[50];                 //RSSI in char array


OneWire  ds(D4);                      //Create an instance of the ds18b20 on pin D4


//Build an array of topics to subscribe to in mqttConnect()
static const char *mqttSubs[] = {
  cmdTopic
};



PubSubClient client(GardenClient);

#define DEBUG true  //set to true for debug output, false for no debug ouput
#define Serial if(DEBUG)Serial




// =================================== setup() ===================================
void setup(void)
{
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.println(sketchName);
  Serial.println();
  Serial.print(F("statusTopic= "));
  Serial.println(statusTopic);
  Serial.print(F("statusJsonTopic= "));
  Serial.println(statusJsonTopic);

  Serial.println();


  pinMode(D2, OUTPUT);
  digitalWrite(D2, HIGH);          // Turn on LED- We're awake.

  setup_wifi();
  start_OTA();


  // Call the PubSubClent setServer method, passing the broker address and the port.
  client.setServer(mqtt_server, mqttPort);
  mqttConnect();
  client.setCallback(callback);


  //Ensure we've sent & received everything before sleeping
  //This adds 1-second to the wake time.
  for (int i = 0; i < 5; i++)
  {
    client.loop();                 // Normally at the top of the loop.
    delay(100);
  }


  String rssiTemp;                                              //RSSI in String
  rssiTemp = String(rssi);                                      //convert the rssi to a String
  rssiTemp.toCharArray(rssi_string, rssiTemp.length() + 1);     //packaging up the data to publish to mqtt whoa...

  float fahrenheit = readDS();
  Serial.print(F("temperatureRaw= "));
  Serial.println(fahrenheit);
  Serial.print(F("tCorrection= "));
  Serial.print(tCorrection);
  fahrenheit = fahrenheit + tCorrection;

  int moistureVal = analogRead(0);                              //Read the moisture sensor
  Serial.print(F("\n\nraw= "));
  Serial.println(moistureVal);
  int moistureValPct = map(moistureVal, wet, dry, 100, 0);
  Serial.print(F("mapped= "));
  Serial.print(moistureValPct);
  Serial.println(F(" % "));


  String temperatureString = String(fahrenheit).c_str();
  String moistureRawString = String(moistureVal).c_str();
  String moisturePctString = String(moistureValPct).c_str();
  Serial.print(F("moistureRawString = "));
  Serial.println(moistureRawString);
  Serial.print(F("moisturePctString = "));
  Serial.println(moisturePctString);
  Serial.print(F("rssi_string = "));
  Serial.println(rssi_string);
  Serial.print(F("Temperature = "));
  Serial.print(fahrenheit);
  Serial.println(F(" Fahrenheit"));
  Serial.println();
  Serial.print(sleepSeconds);
  Serial.println(F(" seconds"));
  Serial.print(F("IP = "));
  Serial.println(WiFi.localIP());



  // Send data to statusTopic
  String status = temperatureString + ", " +
                  moisturePctString + ", " +
                  moistureRawString + ", " +
                  WiFi.localIP().toString() + ", " +
                  rssi_string + ", " +
                  String(sleepSeconds);


  Serial.print(F("status = \""));
  Serial.println(F("Temp, moistureRaw, moisture%, IP, rssi, sleep"));
  Serial.print(F("status = \""));
  Serial.print(status);
  Serial.println(F("\""));                                                 // Cloing quote.


  StaticJsonDocument<240> doc;          //Allocate the JSON document
  // Add values in the document
  doc["temperature"] = temperatureString;
  doc["moisture"] = moisturePctString;
  doc["moistureraw"] = moistureRawString;
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = rssi_string;
  doc["sleep"] = String(sleepSeconds);
 
  String statusJson;
  serializeJson(doc, statusJson);       //Prints a JSON String to the buffer

  Serial.print(F("statusJson= "));      //Debug, make sure the JSON string is good.
  Serial.println(statusJson);
  Serial.println();

  client.publish(statusJsonTopic, (char*) statusJson.c_str());             // Publish the JSON string
  delay(pubsubDelay);                                                      // Publish never completes without a delay


  client.publish(statusTopic, (char*) status.c_str());                     // Publish all data
  delay(pubsubDelay);                                                      // Publish never completes without a delay

}


// ==================================== loop() ====================================
void loop(void) {
  client.loop();                      //Check for MQTT messages
  ArduinoOTA.handle();                //check for OTA commands
  ArduinoOTA.onEnd([]() {
    Serial.println("\nUpload Ended");
  });


}
