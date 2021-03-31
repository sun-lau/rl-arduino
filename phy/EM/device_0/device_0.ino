#include <Arduino.h>
#include "BasicStepperDriver.h" // generic
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "ArduinoJson.h"
#include "SD.h"
#include "SPI.h"
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_wpa2.h>

// wifi config
const char* ssid = "STEM"; //your ssid
#define EAP_ID "gststem01"
#define EAP_USERNAME "gststem01"
#define EAP_PASSWORD "A1b2c3d4"
// Set your Static IP address
IPAddress local_IP(10, 15, 120, 50);
//Set your Gateway IP address
IPAddress gateway(10, 15, 120, 2);
IPAddress subnet(255, 255, 252, 0);
IPAddress primaryDNS(158, 132, 18, 1);    // optional
IPAddress secondaryDNS(158, 132, 14, 1);  // optional

// ##### MOTOR CONFIG #####
// Coil Selector
// define motor 1 pins
// this pin should connect to Ground when want to stop the motor
#define STOPPER_PIN1 33
// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS1 200
// Microstepping mode. If you hardwired it to save pins, set to the same value here.
#define MICROSTEPS1 16
#define DIR1 4
#define STEP1 26

//Magnet Selector
//define motor 2 pins
// this pin should connect to Ground when want to stop the motor
#define STOPPER_PIN2 5
// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS2 200
// Microstepping mode. If you hardwired it to save pins, set to the same value here.
#define MICROSTEPS2 16
#define DIR2 25
#define STEP2 27

#define EN 2
BasicStepperDriver stepper1(MOTOR_STEPS1, DIR1, STEP1, EN);
BasicStepperDriver stepper2(MOTOR_STEPS2, DIR2, STEP2, EN);

//define revolution speed
int RPM = 120;
#define MOTOR_ACCEL 2000
#define MOTOR_DECEL 1000

char stage = 0;
int magnet_stage = 1;
int current_magnet = 1;
//1 -> 3: weak -> strong
int current_coil = 1;
//0: nothing
//1: 100t, 2: 150t, 3: 200t
int current_speed = 1;

//define ADS1115 parameters
Adafruit_ADS1115 ads1115_a(0x48);  //address connect to ground/nothing
Adafruit_ADS1115 ads1115_b(0x49); //address connect to VDD
unsigned long time1;
unsigned long time2;
int period = 10;
int length = 2000; //no of data = length/period
int16_t results;
String return_result = "";

String equipment_id="set_0";
String device_id="device_0";
String experiment="EM_INDUCTION";


DynamicJsonDocument doc(1000);
long cmd_set_at;
long cmd_got_at;
long temp_set = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  Serial2.begin(250000, SERIAL_8N1, 16,17);

  // Config ADS1115
  ads1115_a.setGain(GAIN_FOUR);
  ads1115_b.setGain(GAIN_FOUR);
  ads1115_a.begin();
  ads1115_b.begin();
	// Configure stopper pin to read HIGH unless grounded
	pinMode(STOPPER_PIN1, INPUT);
	pinMode(STOPPER_PIN2, INPUT);

	stepper1.begin(200, MICROSTEPS1);
	stepper2.begin(200, MICROSTEPS2);

	stepper1.setEnableActiveState(LOW);
	stepper2.setEnableActiveState(LOW);

	stepper1.disable();
	stepper2.disable();

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

	stepper1.setSpeedProfile(stepper1.LINEAR_SPEED, MOTOR_ACCEL, MOTOR_DECEL);
	stepper2.setSpeedProfile(stepper2.LINEAR_SPEED, MOTOR_ACCEL, MOTOR_DECEL);

	Serial.println("START");

	delay(1000);
	command("RESTART", "a");

}

void initalize() { // Initalize XY Stage
  //initalize Coil Selector
  stepper1.enable();
  stepper1.startMove(-1000 * MOTOR_STEPS1 * MICROSTEPS1);
  do {
    if (digitalRead(STOPPER_PIN1) == LOW) {
      stepper1.stop();
    }
  } while (stepper1.nextAction());
  delay(1000);
  stepper1.startMove(1 * MOTOR_STEPS1 * MICROSTEPS1);
  do {
  } while (stepper1.nextAction());
  delay(500);
  stepper1.disable();
  
  
  //initalize Magnet Selector
  stepper2.enable();
  stepper2.startMove(-1000 * MOTOR_STEPS2 * MICROSTEPS2);
  do {
    if (digitalRead(STOPPER_PIN2) == LOW) {
      stepper2.stop();
    }
  } while (stepper2.nextAction());
  delay(1000);
  stepper2.startMove(3 * MOTOR_STEPS2 * MICROSTEPS2);
  do {
  } while (stepper2.nextAction());
  delay(500);
  stepper2.disable();

  
  stage = 0;
  current_magnet = 1;
  current_coil = 1;
  current_speed = 1;
 
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


void command(String type, String value) {

  if (type == "RESTART" )          // RESTART  power off
  {
    if (value=="ALL")
  {
    Serial2.println("");
    Serial2.println("RESTART|ALL");
    ESP.restart();
    
  }
  else
  {
    initalize();
    Serial2.println("");
    Serial2.println("RESTART|a");
  }
  }

  else if (type == "MAGNET")                             // MAGNET   select magnet
  {
    
    if (value == "1") { magnet_stage = 1;}
    else if (value == "2") { magnet_stage = 2;}
    else if (value == "3") { magnet_stage = 3;}
    else return;
        
  }

  else if (type == "COIL")                               // COIL     select coil
  {
    int coil_stage;
    
    if (value == "1") { coil_stage = 1;}
    else if (value == "2") { coil_stage = 2;}
    else if (value == "3") { coil_stage = 3;}
    else return;

    if (coil_stage == current_coil) return;
    current_coil = coil_stage;
  }

  else if (type == "SPEED")                              // SPEED    select speed
  {
      int speed_stage;

    if (value == "1") { speed_stage = 1;}
    else if (value == "2") { speed_stage = 2;}
    else if (value == "3") { speed_stage = 3;}
    else return;

    if (speed_stage == current_speed) return;
    current_speed = speed_stage;
  }

  else if (type == "MEASURE")                           // MEASURE   do experiment
  {
   return_result=""; 
    if (magnet_stage != current_magnet)
     {
      int movement = magnet_stage - current_magnet;
      current_magnet = magnet_stage;
      stepper2.enable();
      stepper2.startMove(movement * 47 * MOTOR_STEPS2 * MICROSTEPS2);
        do
        {
//          if (digitalRead(STOPPER_PIN2) == LOW) {
//            stepper2.stop();
//          }
        } while (stepper2.nextAction());
        delay(500);
        stepper2.disable();
    }
    // Send signal to slave - 3 motors
    
    String cmd = "M" + String(current_magnet) + "|" + String(current_speed);
    Serial2.println("");
    Serial2.println(cmd);

    //wait 3 motor ok signal
    int read_stage = 0;
    while (read_stage != 100) {
      while (!Serial2.available()) {};
      if (Serial2.available() > 0) {
        read_stage = Serial2.read();
      }
    }

    

    stepper1.enable();
    stepper1.startMove((22 + 25 * (current_coil - 1)) * MOTOR_STEPS1 * MICROSTEPS1);
    do
    {
    } while (stepper1.nextAction());
    delay(500);

    Serial2.print("d");
    

    delay(2000);

    //take data from ads1115
    switch (current_coil) {
    case 1:
      time1 = millis();
      time2 = millis();
      while (millis() < time2 + period) {}
      while (millis() < time1 + length) {
        time2 = millis();
        results = ads1115_a.readADC_Differential_0_1();
        
        return_result += String(time2-time1)+"," + String(results)+ "|";
        
        while (millis() < time2 + period) {}

      }
      Serial.println(return_result);
      Serial.println("#");
      break;

    case 2:
      time1 = millis();
      time2 = millis();
      while (millis() < time2 + period) {}
      while (millis() < time1 + length) {
        time2 = millis();
        results = ads1115_a.readADC_Differential_2_3();
        
        return_result += String(time2-time1)+"," + String(results)+ "|";
        
        while (millis() < time2 + period) {}

      }
      Serial.println(return_result);
      Serial.println("#");
      break;

    case 3:
      time1 = millis();
      time2 = millis();
      while (millis() < time2 + period) {}
      while (millis() < time1 + length) {
        time2 = millis();
        results = ads1115_b.readADC_Differential_0_1();
        
        return_result += String(time2-time1)+"," + String(results)+ "|";
        
        while (millis() < time2 + period) {}

      }
      Serial.println(return_result);
      Serial.println("#");
      break;
    }

    //after finish wait for signal

    read_stage = 0;
    while (read_stage != 100) {
      while (!Serial2.available()) {};
      if (Serial2.available() > 0) {
        read_stage = Serial2.read();
      }
    }

    stepper1.startMove((-22 - 25 * (current_coil - 1)) * MOTOR_STEPS1 * MICROSTEPS1);
    do
    {

    } while (stepper1.nextAction());
    delay(500);
    stepper1.disable();
    stepper2.disable();

    return_result = return_result.substring(0, return_result.length() - 1);
    HTTPClient http;
    http.begin("https://stem-ap.polyu.edu.hk/remotelab/api/experiment/setChart.php");
    http.addHeader("Content-type", "application/x-www-form-urlencoded");
    http.POST("chart=" + return_result + "&experiment="+experiment+"&equipment_id="+ equipment_id+ "&device_id="+device_id);
    http.writeToStream(&Serial);
    http.end();

    
  }

}



// the loop function runs over and over again until power down or reset
void loop() {
  
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    //send data to BasicStepperDriver
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
    delay(500);

  } else
  {
    WIFI_Connect();
  }
}
