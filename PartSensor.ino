#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>

#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINTLN(x)  Serial.println(x)
#define DEBUG_PRINT(x)  Serial.print(x)
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#endif

// ThingSpeak
// PM Channel:
// Field1= CF1 PM1
// Field2= CF1 PM2.5
// Field3= CF1 PM10
// Field4= Atm PM1
// Field5= Atm PM2.5
// Field6= Atm PM10
// Field7= Humidity
// Field8= Temperature

// Raw Dust Channel:
// Field1= Particule count 0.3 µm
// Field2= Particule count 0.5 µm
// Field3= Particule count 1.0 µm
// Field4= Particule count 2.5 µm
// Field5= Particule count 5.0 µm
// Field6= Particule count 10 µm
// Field7= AQI

String ThingSpeakPMapiKey = "PMKEY"; //PM API Channel
String ThingSpeakRawapiKey = "RAWKEY"; //Raw API Channel
const char* ThingSpeakServer = "api.thingspeak.com";

const int   SLEEP_TIME = 20 * 1000;

// Wifi
const char* cWifiSSID = "SSID";
const char* cWifiPassword = "PASSWORD";
const int MAX_WIFI_TRY = 4;

//DHT22
#define PINDHT 4 // what pin DHT sensor is connected to
float humidity = 0;
float temperature = 0;

//PMS5003
#define MSG_LENGTH 31   //0x42 + 31 bytes equal to PMS5003 serial message packet lenght
#define HTTP_TIMEOUT 20000 //maximum http response wait period, sensor disconects if no response
#define MIN_WARM_TIME 35000 //warming-up period requred for sensor to enable fan and prepare air chamber
unsigned char buf[MSG_LENGTH];

int atmPM01Value = 0;  //define Atmospheric PM1.0 value of the air detector module
int atmPM25Value = 0;  //define Atmospheric PM2.5 value of the air detector module
int atmPM10Value = 0;  //define Atmospheric PM10 value of the air detector module
int CF1PM01Value = 0;  //define CF1 PM1.0 value of the air detector module
int CF1PM25Value = 0;  //define CF1 PM2.5 value of the air detector module
int CF1PM10Value = 0;  //define CF1 PM10 value of the air detector module
int Partcount0_3 = 0;  //define Particule Count > 0.3 µm
int Partcount0_5 = 0;  //define Particule Count > 0.5 µm
int Partcount1_0 = 0;  //define Particule Count > 1.0 µm
int Partcount2_5 = 0;  //define Particule Count > 2.5 µm
int Partcount5_0 = 0;  //define Particule Count > 5.0 µm
int Partcount10 = 0;   //define Particule Count > 10.0 µm
int airQualityIndex = 0;// Calulated Air Quality Index

void setupWIFI() {
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(cWifiSSID, cWifiPassword);
  int WifiTry = 0;
  DEBUG_PRINTLN("MAC address: ");
  DEBUG_PRINTLN(WiFi.macAddress());

  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    WifiTry++;
	// Try to connect until success. If too many failed try, force ESP Wifi reset
    if (WifiTry > MAX_WIFI_TRY) {
          WifiTry = 0;
          DEBUG_PRINTLN("WiFi failed, retry");
          Serial.printf("WiFi status:%d\n",  WiFi.status());
          WiFi.disconnect();
          WiFi.persistent(false);
          WiFi.mode(WIFI_OFF);
          WiFi.mode(WIFI_STA);
          WiFi.begin(cWifiSSID, cWifiPassword);
      }
    DEBUG_PRINT(".");
  }

  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("WiFi connected");
  DEBUG_PRINTLN("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
}

boolean validatePMS5003frame() {

  int receiveSum = 0;
  for (int i = 0; i < (MSG_LENGTH - 2); i++) {
    receiveSum = receiveSum + buf[i];
  }
  receiveSum = receiveSum + 0x42;
  return receiveSum == ((buf[MSG_LENGTH - 2] << 8) + buf[MSG_LENGTH - 1]);
}

// AQI formula: https://en.wikipedia.org/wiki/Air_Quality_Index#United_States
int toAQI(int I_high, int I_low, int C_high, int C_low, int C) {
  return (I_high - I_low) * (C - C_low) / (C_high - C_low) + I_low;
}

//thanks to https://gist.github.com/nfjinjing/8d63012c18feea3ed04e
int calculateAQI25(float density) {

  int d10 = (int)(density * 10);
  if (d10 <= 0) {
    return 0;
  } else if (d10 <= 120) {
    return toAQI(50, 0, 120, 0, d10);
  } else if (d10 <= 354) {
    return toAQI(100, 51, 354, 121, d10);
  } else if (d10 <= 554) {
    return toAQI(150, 101, 554, 355, d10);
  } else if (d10 <= 1504) {
    return toAQI(200, 151, 1504, 555, d10);
  } else if (d10 <= 2504) {
    return toAQI(300, 201, 2504, 1505, d10);
  } else if (d10 <= 3504) {
    return toAQI(400, 301, 3504, 2505, d10);
  } else if (d10 <= 5004) {
    return toAQI(500, 401, 5004, 3505, d10);
  } else if (d10 <= 10000) {
    return toAQI(1000, 501, 10000, 5005, d10);
  } else {
    return 1001;
  }
}

void powerOnPMS5003() {
  digitalWrite(D0, HIGH);
}

void powerOffPMS5003() {
  WiFi.disconnect();

  digitalWrite(D0, LOW);
  //ESP.deepSleep(SLEEP_TIME * 1000000); //deep sleep in microseconds, unfortunately doesn't work properly
  DEBUG_PRINTLN("going to sleep zzz...");
  delay(SLEEP_TIME);
}

void sendPMDataToCloud() {
  DEBUG_PRINTLN("sendPMDataToCloud start");

  // Enclosure-PM Channel
    String postStr = ThingSpeakPMapiKey;
    postStr += "&field1=";
    postStr += String(CF1PM01Value);
    postStr += "&field2=";
    postStr += String(CF1PM25Value);
    postStr += "&field3=";
    postStr += String(CF1PM10Value);
    postStr += "&field4=";
    postStr += String(atmPM01Value);
    postStr += "&field5=";
    postStr += String(atmPM25Value);
    postStr += "&field6=";
    postStr += String(atmPM10Value);
    if (!isnan(temperature)) {
        postStr += "&field7=";
        postStr += String(temperature);
    }
    if (!isnan(humidity)) {
        postStr += "&field8=";
        postStr += String(humidity);
    }
    postStr += "\r\n\r\n";

    sendDataToCloud(ThingSpeakPMapiKey, postStr);
}

void sendRawDataToCloud() {
  DEBUG_PRINTLN("sendRawDataToCloud start");

  // Enclosure-RawDust Channel
    String postStr = ThingSpeakRawapiKey;
    postStr += "&field1=";
    postStr += String(Partcount0_3);
    postStr += "&field2=";
    postStr += String(Partcount0_5);
    postStr += "&field3=";
    postStr += String(Partcount1_0);
    postStr += "&field4=";
    postStr += String(Partcount2_5);
    postStr += "&field5=";
    postStr += String(Partcount5_0);
    postStr += "&field6=";
    postStr += String(Partcount10);
    postStr += "&field7=";
    postStr += String(airQualityIndex);
    postStr += "\r\n\r\n";

    sendDataToCloud(ThingSpeakRawapiKey, postStr);
}

void sendDataToCloud(String apikey, String strfield) {
  DEBUG_PRINTLN("sendDataToCloud start");
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect("api.thingspeak.com", 80)) {
    DEBUG_PRINTLN("connection failed");
    return;
  }

  DEBUG_PRINTLN("Sending:" + strfield);

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apikey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(strfield.length());
    client.print("\n\n");
    client.print(strfield);

  client.flush();
  delay(10);
  DEBUG_PRINTLN("wait for response");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > HTTP_TIMEOUT) {
      DEBUG_PRINTLN(">>> Client Timeout !");
      client.stop();
      DEBUG_PRINTLN("closing connection by timeout");
      return;
    }
  }

  // Read all the lines of the reply from server and print them to Serial Debug
  while (client.available()) {
    String line = client.readStringUntil('\r');
    DEBUG_PRINT(line);
  }

  client.stop();
  DEBUG_PRINTLN("closing connection");
}

void setup() {
  Serial.begin(9600);
  
  delay(10);
  DHT dht(PINDHT, DHT22);
  WiFiClient client;
  
  dht.begin();

#ifdef DEBUG
  Serial.println(" Init started: DEBUG MODE");
#else
  Serial.println(" Init started: NO DEBUG MODE");
#endif
  Serial.setTimeout(1500); //set the Timeout to 1500ms, longer than the data transmission time of the sensor
  pinMode(D0, OUTPUT);
  DEBUG_PRINTLN("Initialization finished");
}

void loop() {
  DEBUG_PRINTLN("loop start");
  unsigned long timeout = millis();

  powerOnPMS5003();
  setupWIFI();

  timeout = MIN_WARM_TIME - (millis() - timeout);
  if (timeout > 0) {
    DEBUG_PRINT("sensor warm-up: ");
    DEBUG_PRINTLN(timeout);
    delay(timeout);
  }

  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.print("DHT22 not ready, skipped");
  }


  for (int i = 0; i < 30; i++) {
    if (Serial.find(0x42)) {  //start to read when detect 0x42
      Serial.readBytes(buf, MSG_LENGTH);

      if (buf[0] == 0x4d && validatePMS5003frame()) {
		// PMS5003 frame format detail from http://www.aqmd.gov/docs/default-source/aq-spec/resources-page/plantower-pms5003-manual_v2-3.pdf?sfvrsn=2
        CF1PM01Value = (thebuf[9] << 8) + thebuf[10]; //PM1.0 CF1 value of the air detector module
        CF1PM25Value = (thebuf[5] << 8) + thebuf[6]; //PM2.5 CF1 value of the air detector module
        CF1PM10Value = (thebuf[7] << 8) + thebuf[8]; //PM10 CF1 value of the air detector module

        atmPM01Value = (thebuf[3] << 8) + thebuf[4]; //PM1.0 atmospheric value of the air detector module
        atmPM25Value = (thebuf[11] << 8) + thebuf[12]; //PM2.5 atmospheric value of the air detector module
        atmPM10Value = (thebuf[13] << 8) + thebuf[14]; //PM10 atmospheric value of the air detector module
    
        Partcount0_3 = (thebuf[15] << 8) + thebuf[16]; //count particules > 0.3 µm
        Partcount0_5 = (thebuf[17] << 8) + thebuf[18]; //count particules > 0.5 µm
        Partcount1_0 = (thebuf[19] << 8) + thebuf[20]; //count particules > 1.0 µm
        Partcount2_5 = (thebuf[21] << 8) + thebuf[22]; //count particules > 2.5 µm
        Partcount5_0 = (thebuf[23] << 8) + thebuf[24]; //count particules > 5.0 µm
        Partcount10 = (thebuf[25] << 8) + thebuf[26]; //count particules > 10.0 µm

        airQualityIndex = calculateAQI25(atmPM25Value);

#ifdef DEBUG //debug info only for serial connection debugging
        DEBUG_PRINTLN("Temperature: " + String(temperature));
        DEBUG_PRINTLN("Humidity: " + String(humidity));
        DEBUG_PRINTLN("Atm PM 1.0: " + String(atmPM01Value));
        DEBUG_PRINTLN("Atm PM 2.5: " + String(atmPM25Value));
        DEBUG_PRINTLN("Atm PM  10: " + String(atmPM10Value));
        DEBUG_PRINTLN("CF1 PM 1.0: " + String(CF1PM01Value));
        DEBUG_PRINTLN("CF1 PM 2.5: " + String(CF1PM25Value));
        DEBUG_PRINTLN("CF1 PM  10: " + String(CF1PM10Value));
        DEBUG_PRINTLN("AQI   : " + String(airQualityIndex));
        DEBUG_PRINTLN("Count 0.3 : " + String(Partcount0_3));
        DEBUG_PRINTLN("Count 0.5 : " + String(Partcount0_5));
        DEBUG_PRINTLN("Count 1.0 : " + String(Partcount1_0));
        DEBUG_PRINTLN("Count 2.5 : " + String(Partcount2_5));
        DEBUG_PRINTLN("Count 5.0 : " + String(Partcount5_0));
        DEBUG_PRINTLN("Count  10 : " + String(Partcount10));
        DEBUG_PRINTLN("--------------");
#endif

        sendPMDataToCloud();
        sendRawDataToCloud();
        break; //data processed, exit from for loop and sleep

      } else {
        DEBUG_PRINTLN("message validation error");
      }
    } else {
      DEBUG_PRINTLN("sensor msg start not found");
    }
  }

  powerOffPMS5003();
}

