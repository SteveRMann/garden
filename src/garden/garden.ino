#define sketchName "garden.ino, V1.1"

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
#define NODENAME "garden2"                         //Must be unique on the net.
const char *connectName =  NODENAME "garden2";     //Must be unique on the network

//Probe Calibration
int dry = 457;                  //Dry sensor
int wet = 811;                  //Wet sensor


#define hostPrefix NODENAME     // For setupWiFi()
char macBuffer[24];             // Holds the last three digits of the MAC, in hex.

char jsonBuffer[150];           //Holds the JSON string from ArduinoJson.h


const char *statusTopic = NODENAME "/status";             //Sends the temperature, moisture, IP and RSSI in one payload.
const char *statusJsonTopic = NODENAME "/statusJson";
const char *cmdTopic = NODENAME "/cmd";                   //Sends a command string: readTime, temp correction
const int mqttPort = 1883;


int sleepSeconds = 15;
const int pubsubDelay = 20;           //Time between publishes
long rssi;                            //Used in the WiFi tab
float tCorrection = 0.0;              //Temperature correction.
char rssi_string[50];                 //RSSI in char array


OneWire  ds(D4);                      //Create an instance of the ds18b20 on pin D4


//Build an array of topics to subscribe to in mqttConnect()
static const char *mqttSubs[] = {
  cmdTopic
};



// Declare an object of class WiFiClient, which allows to establish a connection to a specific IP and port
// Declare an object of class PubSubClient, which receives as input of the constructor the previously defined WiFiClient.
WiFiClient GardenClient2;                // The constructor MUST be unique on the network.
PubSubClient client(GardenClient2);


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

}


// ==================================== loop() ====================================
void loop(void) {
  client.loop();                      //Check for MQTT messages
  ArduinoOTA.handle();                //check for OTA commands
  ArduinoOTA.onEnd([]() {
    Serial.println("\nUpload Ended");
  });

  Serial.println(F("\n----------"));    // Flag the top of the loop for debugging

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
  Serial.print(F("\nraw= "));
  Serial.println(moistureVal);
  int moistureValPct = map(moistureVal, wet, dry, 100, 0);
  Serial.print(F("mapped= "));
  Serial.print(moistureValPct);
  Serial.println(F(" % "));


  String temperatureString = String(fahrenheit).c_str();
  String moistureRawString = String(moistureVal).c_str();
  String moisturePctString = String(moistureValPct).c_str();


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


  //The maximum MQTT message size, including header, is 128 bytes by default in PubSubClient.h,and
  //20 bytes are consumed in the header, so 108 characters is the limit.
  StaticJsonDocument<240> doc;          //Allocate the JSON document
  // Add values in the document
  doc["temp"] = temperatureString;
  doc["wet"] = moisturePctString;
  doc["raw"] = moistureRawString;
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = rssi_string;
  doc["sleep"] = String(sleepSeconds);

  String statusJson;
  serializeJson(doc, statusJson);       //Prints a JSON String to the buffer

  Serial.print(F("statusJson= "));      //Debug, to make sure the JSON string is good.
  Serial.println(statusJson);
  Serial.print(F("statusJson.length= "));
  Serial.println(statusJson.length());

  int e = client.publish(statusJsonTopic, (char*) statusJson.c_str());     // Publish the JSON string
  delay(pubsubDelay);                                                      // Publish never completes without a delay
  Serial.print(F("e= "));
  Serial.println(e);


  client.publish(statusTopic, (char*) status.c_str());                     // Publish all data
  delay(pubsubDelay);                                                      // Publish never completes without a delay

  delay(sleepSeconds * 1000);       //Time between readings in ms
}
