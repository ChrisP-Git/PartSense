#define DEBUG //Comment to remove all debug & trace instructions


//////////////////////////////////////////////////////
///////// Replace Wifi connexion information by //////
///////// your own values                       //////
//////////////////////////////////////////////////////
const char* Wifi_SSID = "SSID";
const char* Wifi_PASSWORD = "PASSWORD";

//Thingspeak
//#define USE_THINGSPEAK //comment to remove thinkspeak data feeding
#ifdef USE_THINGSPEAK
  //////////////////////////////////////////////////////
  ///////// if you want to push data on thingspeak /////
  ///////// Replace API keys by your own values ////////
  //////////////////////////////////////////////////////
  const String ThingSpeak_PM_APIKey = "XXXXXXXXXXXXXXXX";
  const String ThingSpeak_Raw_APIKey = "XXXXXXXXXXXXXXXX";
  const char* ThingSpeak_API_Server = "api.thingspeak.com";
#endif

//MQTT
#define USE_MQTT //comment to remove MQTT data feeding (not implemented yet)
#ifdef USE_MQTT
  //////////////////////////////////////////////////////
  ///////// if you want to push data on a MQTT broker //
  ///////// Replace settings by your own values ////////
  //////////////////////////////////////////////////////
  #define MQTT_Server "MQTT server adress"
  #define MQTT_Port 1883
  #define MQTT_ClientName "PartSense-"
  #define MQTT_temperature_topic "3DPrint/Enclosure1/temperature"
  #define MQTT_humidity_topic "3DPrint/Enclosure1/humidity"
  #define MQTT_atmPM01Value_topic "3DPrint/Enclosure1/atmPM01Value"
  #define MQTT_atmPM25Value_topic "3DPrint/Enclosure1/atmPM25Value"
  #define MQTT_atmPM10Value_topic "3DPrint/Enclosure1/atmPM10Value"
  #define MQTT_CF1PM01Value_topic "3DPrint/Enclosure1/CF1PM01Value"
  #define MQTT_CF1PM25Value_topic "3DPrint/Enclosure1/CF1PM25Value"
  #define MQTT_CF1PM10Value_topic "3DPrint/Enclosure1/CF1PM10Value"
  #define MQTT_Partcount0_3_topic "3DPrint/Enclosure1/Partcount0_3"
  #define MQTT_Partcount0_5_topic "3DPrint/Enclosure1/Partcount0_5"
  #define MQTT_Partcount1_0_topic "3DPrint/Enclosure1/Partcount1_0"
  #define MQTT_Partcount2_5_topic "3DPrint/Enclosure1/Partcount2_5"
  #define MQTT_Partcount5_0_topic "3DPrint/Enclosure1/Partcount5_0"
  #define MQTT_Partcount10_topic "3DPrint/Enclosure1/Partcount10"
  #define MQTT_airQualityIndex_topic "3DPrint/Enclosure1/airQualityIndex"
#endif



