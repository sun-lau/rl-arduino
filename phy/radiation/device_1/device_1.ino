/*
 Name:		Radiation_turntable.ino
 Created:	8/7/2019 2:49:43 PM
 Author:	RemoteLab
*/

#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_wpa2.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// wifi config
const char* ssid = "STEM"; // your ssid
#define EAP_ID "gststem01"
#define EAP_USERNAME "gststem01"
#define EAP_PASSWORD "A1b2c3d4"
// Set your Static IP address
IPAddress local_IP(10, 15, 120, 111);
// Set your Gateway IP address
IPAddress gateway(10, 15, 120, 2);
IPAddress subnet(255, 255, 252, 0);
IPAddress primaryDNS(158, 132, 18, 1);   //optional
IPAddress secondaryDNS(158, 132, 14, 1); //optional

//#define DEBUG 1

/// Geiger counter
#define RECEIVER_PIN 22  // 4=GPIO4=D2 any interrupt able pin (Wemos) // 2=GPIO2 on ESP-01
/// Geiger tube parameters
#define TUBE_NAME "j305"
#define TUBE_FACTOR 0.00812

/// counter part

long LOG_PERIOD = 1000;     // Logging period in milliseconds, recommended value 15000-60000.
#define MAX_PERIOD 60000     // Maximum logging period

volatile unsigned long counts;  // variable for GM Tube events
volatile bool sig;              // flag that at least one impulse occured interrupt
unsigned long cpm;              // variable for CPM
float uS=0;
unsigned int multiplier;        // variable for calculation CPM in this sketch
unsigned long previousMillis;   // variable for time measurement


String equipment_id="set_0";
String device_id="device_1";
String experiment="RADIATION";

long cmd_set_at;
long cmd_got_at;
long temp_set = 0;


void tube_impulse(){            // procedure for capturing events from Geiger Kit
  counts++;
  sig = true;
}

void setupGeiger() {
  // setup interrupt for falling edge on receier pin
  sig = false;
  counts = 0;
  cpm = 0;

  multiplier = MAX_PERIOD / LOG_PERIOD;     // calculating multiplier, depend on your log period    
  pinMode(RECEIVER_PIN, INPUT);             // set pin as input for capturing GM Tube events
  attachInterrupt(digitalPinToInterrupt(RECEIVER_PIN), tube_impulse, RISING);  // define external interrupts  
}

void loopGeiger() {
  counts = 0;
  previousMillis = millis();
  unsigned long currentMillis = millis();

  while(currentMillis - previousMillis < LOG_PERIOD){
    currentMillis = millis();
    
    }
    cpm = counts * multiplier;
    
    uS = TUBE_FACTOR * cpm;

    Serial.print(cpm); Serial.print(" cpm\t");
    Serial.print(uS); Serial.println(" uSv/h");

  
}

void WIFI_Connect() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid);
  int k=0;
  while (WiFi.status() != WL_CONNECTED) {
    if(k>=10)
    {
      WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid);
  k=0;
  Serial.print("aaa");
    }
    delay(500);
    Serial.print(".");
    k++;
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void command(String type , String value) {

  if (type == "RESTART")            //RESTART  power off
  {
    if (value == "1")
    {
      ESP.restart();
    }
    else {
      LOG_PERIOD = 1000; 
      cpm=0;
      }
     
    Serial.println("#");
  }

  else if (type == "MEASURE")
  {
    
    loopGeiger();
    setvalue(experiment, equipment_id, device_id, String(cpm), String(LOG_PERIOD/1000));

  }
  else if (type == "POINTS")
  {
    LOG_PERIOD=value.toInt()*1000;

    setvalue(experiment, equipment_id, device_id, String(cpm), String(LOG_PERIOD/1000));

  }
  else {}


}

DynamicJsonDocument doc(1000);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
	Serial.begin(115200);
setupGeiger();

  delay(4000);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_ID, strlen(EAP_ID));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_ID, strlen(EAP_ID));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
  esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();
  esp_wifi_sta_wpa2_ent_enable(&config);

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  WIFI_Connect();
  command("RESTART","0");
  
}

void setvalue(String exp_name, String equip, String device, String data1, String data2)
{
  HTTPClient http;
    http.begin("https://stem-ap.polyu.edu.hk/remotelab/api/experiment/setValue.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String out= "value={\"reading\":\"" + data1 + "\",\"points_of_average\":" + data2 + "}&experiment="+exp_name+"&equipment_id="+ equip+ "&device_id="+device;
    http.POST(out);
    Serial.println(out);
    http.writeToStream(&Serial);
    http.end();
}

// the loop function runs over and over again until power down or reset
void loop() {

  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
    HTTPClient http;
    //send data to server
    http.begin("https://stem-ap.polyu.edu.hk/remotelab/api/experiment/getCommand.php?experiment="+experiment+"&equipment_id="+ equipment_id+ "&device_id="+device_id);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.GET();

    //    Serial.println(httpCode);
    //
    //    Serial.println(http.getString());
    if  (http.getSize()>0)
    {
    deserializeJson(doc, http.getString());
    }
    http.end();
    cmd_set_at = doc["command_set_at"];
    cmd_got_at = doc["command_got_at"];
    Serial.println(cmd_set_at);
    Serial.println(cmd_got_at);


    if (cmd_set_at >= cmd_got_at && temp_set != cmd_set_at)

    {
      String cmd = doc["command"];
      command(cmd.substring(0, cmd.indexOf("|")), cmd.substring(cmd.indexOf("|") + 1));
      temp_set = cmd_set_at;
    }
    delay(100);

  } else
  {
    WIFI_Connect();
  }
}
