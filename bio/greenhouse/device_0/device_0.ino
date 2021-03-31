#include <Arduino.h>
#include "BasicStepperDriver.h" // generic
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_wpa2.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <MHZ19_uart.h>

// wifi config
const char* ssid = "STEM"; // your ssid
#define EAP_ID "gststem01"
#define EAP_USERNAME "gststem01"
#define EAP_PASSWORD "A1b2c3d4"
// Set your Static IP address
IPAddress local_IP(10, 15, 121, 155);
// Set your Gateway IP address
IPAddress gateway(10, 15, 120, 2);
IPAddress subnet(255, 255, 252, 0);
IPAddress primaryDNS(158, 132, 18, 1);   //optional
IPAddress secondaryDNS(158, 132, 14, 1); //optional

String equipment_id="set_0";
String device_id="device_0";
String experiment="GREEN_HOUSE";

long cmd_set_at;
long cmd_got_at;
long temp_set = 0;

Adafruit_BMP280 bmp1;
Adafruit_BMP280 bmp2;
MHZ19_uart mhz19;           //5V supply

 //chamber1 : air , co2    ;  chamber2 : air
int air1_valve_pin = 18;    //sck
int co2_valve_pin = 19;     //miso  
int lamp_pin = 23;          //mosi

const int rx_pin = 16;  //Serial rx pin no
const int tx_pin = 17; //Serial tx pin no

int light_state = 0;
int air_state = 0;

DynamicJsonDocument doc(1000);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);

  bmp1.begin(0x76);
  bmp2.begin(0x77);
  mhz19.begin(rx_pin, tx_pin);
      mhz19.setAutoCalibration(false);
      mhz19.getPPM();
      mhz19.getTemperature();
  pinMode(air1_valve_pin, OUTPUT);
  pinMode(co2_valve_pin, OUTPUT);
  pinMode(lamp_pin, OUTPUT);
  digitalWrite(air1_valve_pin,LOW);
  digitalWrite(co2_valve_pin,LOW);
  digitalWrite(lamp_pin,LOW);

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
  command("RESTART", "a");
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

  if (type == "RESTART")            //RESTART  
  {
  if (value=="ALL")
  {
    ESP.restart();
  }
  else{
  digitalWrite(air1_valve_pin,LOW);
  digitalWrite(co2_valve_pin,LOW);
  digitalWrite(lamp_pin,LOW);
  light_state = 0;
  air_state = 0;
  }
  }

  else if (type == "TEMP1")
  {
    
    Serial.println(bmp1.readTemperature());
    
  }
  else if (type == "TEMP2")
  {
    
    Serial.println(bmp2.readTemperature());
    
  }
  else if (type == "CO2READ")
  {
    
    Serial.println(mhz19.getPPM());
    
  }
  else if (type == "CO2ZERO")
  {
    mhz19.calibrateZero();
    Serial.println(mhz19.getPPM());
    
  }  
  else if (type == "AIR") 
  {
    if (value == "ON")
    {
      digitalWrite(air1_valve_pin,HIGH);
      air_state = 1;
    }else 
    {
       digitalWrite(air1_valve_pin,LOW);
       air_state = 0;
    }


  }
  else if (type == "LIGHT") 
  {
    if (value == "ON")
    {
      digitalWrite(lamp_pin,HIGH);
      light_state = 1;
    }else 
    {
       digitalWrite(lamp_pin,LOW);
       light_state = 0;
    }


  }
  else if (type == "CO2")
  {
    
      digitalWrite(co2_valve_pin,HIGH);
      delay(100);
      digitalWrite(co2_valve_pin,LOW);
    
  }
}

void all_status()
{
  HTTPClient http;
    http.begin("https://stem-ap.polyu.edu.hk/remotelab/api/experiment/setValue.php");
    http.addHeader("Content-type", "application/x-www-form-urlencoded");
    String out= "value={\"temperature_0\":" + (String)bmp1.readTemperature() + ",\"temperature_1\":" + (String)bmp2.readTemperature()+ ",\"co2_value\":" + (String)mhz19.getPPM()+",\"light\":" + (String)light_state+ ",\"air\":" + (String)air_state+ "}&experiment="+experiment+"&equipment_id="+ equipment_id+ "&device_id="+device_id;
    Serial.println(out);
    http.POST(out);
    http.writeToStream(&Serial);
    http.end();
}

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
    long Status = doc["status"];
    Serial.println(cmd_set_at);
    Serial.println(cmd_got_at);
    Serial.println(Status);


    if (cmd_set_at >= cmd_got_at && temp_set != cmd_set_at)

    {
      String cmd = doc["command"];
Serial.println(cmd);
      command(cmd.substring(0, cmd.indexOf("|")), cmd.substring(cmd.indexOf("|") + 1));
      temp_set = cmd_set_at;
    }
    all_status();
    delay(500);

  } else
  {
    WIFI_Connect();
  }
}
