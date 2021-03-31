/*
 Name:		Photoelectric.ino
 Created:	9/6/2019 4:46:35 PM
 Author:	RemoteLab
*/
#include <Arduino.h>
#include "BasicStepperDriver.h" // generic
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <BH1750.h>
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
IPAddress local_IP(10, 15, 120, 90);
// Set your Gateway IP address
IPAddress gateway(10, 15, 120, 2);
IPAddress subnet(255, 255, 252, 0);
IPAddress primaryDNS(158, 132, 18, 1);   //optional
IPAddress secondaryDNS(158, 132, 14, 1); //optional

//define motor 1 pins
#define STOPPER_PIN1 33
#define MOTOR_STEPS1 200
#define MICROSTEPS1 16
#define DIR1 4
#define STEP1 26
#define EN 2
#define MOTOR_ACCEL 2000
#define MOTOR_DECEL 1000

//define LED
#define PWM 17	//TX
#define LED1 16	//RX
#define LED2 18	//SCK
#define LED3 19	//MISO
#define LED4 23	//MOSI


BasicStepperDriver stepper1(MOTOR_STEPS1, DIR1, STEP1, EN);

//define Light sensor
BH1750 lightMeter;

String equipment_id="set_0";
String device_id="device_0";
String experiment="PHOTO_ELECTRIC";

String LED_state = "";
String stage_state = "";
String power_state ="";
String intensity ="";
long cmd_set_at;
long cmd_got_at;
long temp_set = 0;

void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  // calculate duty, 8191 from 2 ^ 13 - 1
  uint32_t duty = (8191 / valueMax) * min(value, valueMax);

  // write duty to LEDC
  ledcWrite(channel, duty);
}

void LEDOn(int current_LED) {
	switch (current_LED) {
  default:
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, HIGH);
    break;
  case 0:
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, HIGH);
    break;

  case 1:
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, HIGH);
    break;

  case 2:
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, HIGH);
    break;

  case 3:
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, LOW);
    break;
	}
}

DynamicJsonDocument doc(1000);

// the setup function runs once when you press reset or power the board
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  pinMode(STOPPER_PIN1, INPUT);
	Serial.begin(115200);
	Wire.begin();
	lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE);
	stepper1.begin(30, MICROSTEPS1);
  stepper1.setEnableActiveState(LOW);
  stepper1.disable();
  stepper1.setSpeedProfile(stepper1.LINEAR_SPEED, MOTOR_ACCEL, MOTOR_DECEL);
  
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  // attach the channel to the GPIO to be controlled
  LEDOn(0);
  ledcSetup(0, 5000, 13);
  ledcAttachPin(PWM, 0);
  ledcAnalogWrite(0, 0);

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
    if (value=="1")
  {
    ESP.restart();
  }
  else{
    
    stepper1.enable();
  stepper1.startMove(1 * MOTOR_STEPS1 * MICROSTEPS1);
  do {
    if (digitalRead(STOPPER_PIN1) == HIGH) {

      stepper1.stop();
    }
  } while (stepper1.nextAction());
  delay(100);
  stepper1.rotate(-174);
  delay(100);
  stepper1.disable();
  
  LEDOn(0);
  ledcAnalogWrite(0, 0);

  LED_state = "0";
  stage_state = "ptube";
  power_state ="0";
  intensity ="0";
  
  }
  setvalue(experiment, equipment_id, device_id, stage_state, LED_state, power_state, intensity);
  Serial.println("#");
  }

  else if (type == "PTUBE")
  {
    LEDOn(value.toInt());
    stepper1.enable();
  
    if (stage_state == "sensor")
    {
      stepper1.rotate(18);
      stage_state = "ptube";
    }
  
    if (value.toInt() < 4 && value.toInt() >= 0)
    { 
      if (value != LED_state) {
      
      stepper1.rotate((value.toInt() - LED_state.toInt()) * -90);
      delay(500);
      
      LED_state = value;
      }
      
    
    stepper1.disable();
  }
  setvalue(experiment, equipment_id, device_id, stage_state, LED_state, power_state, intensity);
  Serial.println("#");
  }
  else if (type == "SENSOR")
  {
    LEDOn(value.toInt());
    stepper1.enable();
  
    if (stage_state == "ptube")
    {
      stepper1.rotate(-18);
      stage_state = "sensor";
    }
  
    if (value.toInt() < 4 && value.toInt() >= 0)
    { 
      if (value != LED_state) {
      
      stepper1.rotate((value.toInt() - LED_state.toInt()) * -90);
      delay(500);
      
      LED_state = value;
      }
    
    stepper1.disable();
    
      }
      setvalue(experiment, equipment_id, device_id, stage_state, LED_state, power_state, intensity);
      Serial.println("#");
  }
  else if (type == "POWER")   // Power intensity
  {

    ledcAnalogWrite(0, value.toInt());
    power_state =value;
    delay(1000);
    float lux = lightMeter.readLightLevel();
    intensity= (String)lux;
    setvalue(experiment, equipment_id, device_id, stage_state, LED_state, power_state, intensity);
    Serial.println("#");

  }
  
}

void setvalue(String exp_name, String equip, String device, String data1, String data2, String data3, String data4)
{
  HTTPClient http;
    http.begin("https://stem-ap.polyu.edu.hk/remotelab/api/experiment/setValue.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String out="value={\"side\":\"" + data1 + "\",\"light\":" + data2 + ",\"power\":" + data3 + ",\"intensity\":" + data4 +"}&experiment="+exp_name+"&equipment_id="+ equip+ "&device_id="+device;
    Serial.println(out);
    http.POST(out);
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
