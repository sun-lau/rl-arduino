/*
 Name:		Photoelectric.ino
 Created:	9/6/2019 4:46:35 PM
 Author:	RemoteLab
*/
#include <Arduino.h>
#include "BasicStepperDriver.h" // generic
#include <ESP32Servo.h>
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
IPAddress local_IP(10, 15, 120, 110);
// Set your Gateway IP address
IPAddress gateway(10, 15, 120, 2);
IPAddress subnet(255, 255, 252, 0);
IPAddress primaryDNS(158, 132, 18, 1);   //optional
IPAddress secondaryDNS(158, 132, 14, 1); //optional

//Motor1
// this pin should connect to Ground when want to stop the motor
#define STOPPER_PIN1 33
// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS1 200
#define RPM1 30
// Microstepping mode. If you hardwired it to save pins, set to the same value here.
#define MICROSTEPS1 16
#define DIR1 4
#define STEP1 26


//Motor2
// this pin should connect to Ground when want to stop the motor
#define STOPPER_PIN2 5
// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS2 200
#define RPM2 30
// Microstepping mode. If you hardwired it to save pins, set to the same value here.
#define MICROSTEPS2 16
#define DIR2 25
#define STEP2 27


#define EN 2
BasicStepperDriver stepper1(MOTOR_STEPS1, DIR1, STEP1, EN);
BasicStepperDriver stepper2(MOTOR_STEPS2, DIR2, STEP2 , EN);

Servo myservo;
String equipment_id="set_0";
String device_id="device_0";
String experiment="RADIATION";

String cover_state = "";
String shutter_state = "";
long cmd_set_at;
long cmd_got_at;
long temp_set = 0;

int servo_pin=17;
int dist = 0;

DynamicJsonDocument doc(1000);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);

  // Configure stopper pin to read HIGH unless grounded
  pinMode(STOPPER_PIN1, INPUT);
  pinMode(STOPPER_PIN2, INPUT);
  
  stepper1.begin(RPM1, MICROSTEPS1);
  stepper2.begin(RPM2, MICROSTEPS2);
  
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
    if (value == "1")
    {
      ESP.restart();
    }
    else {
      stepper1.enable();
      stepper1.startMove(100 * MOTOR_STEPS1 * MICROSTEPS1);
      do {
        if (digitalRead(STOPPER_PIN1) == LOW) {

          stepper1.stop();
        }
      } while (stepper1.nextAction());
      delay(100);
      stepper1.rotate(-102);
      delay(100);
      stepper1.disable();
      cover_state ="0";

      stepper2.enable();
      stepper2.startMove(50 * MOTOR_STEPS2 * MICROSTEPS2);
      do {
        if (digitalRead(STOPPER_PIN2) == LOW) {

          stepper2.stop();
        }
      } while (stepper2.nextAction());
      stepper2.startMove(-0.05 * MOTOR_STEPS2 * MICROSTEPS2);
    do {

    } while (stepper2.nextAction());
      delay(100);
      //stepper2.disable();
      dist = 0;
    }
    myservo.attach(servo_pin, 550, 2350);
    myservo.write(180);
    delay(200);
    shutter_state = "close";

    //myservo.detach();
    setvalue(experiment, equipment_id, device_id, shutter_state, String (dist), cover_state);
  }

  else if (type == "COVER")
  {
    stepper1.enable();

    if (value.toInt() <= 9 && value.toInt() >= 0)
    {
      if (value != cover_state) {

        stepper1.rotate((value.toInt() - cover_state.toInt()) * -36);
        delay(500);

        cover_state = value;
      }


      //stepper1.disable();
    }
    setvalue(experiment, equipment_id, device_id, shutter_state, String (dist), cover_state);
  }
  else if (type == "DISTANCE")  // DISTANCE UP
  {
    if ( value == "INCREASE")
    {
      if (dist < 50)
      {
        stepper2.enable();
        stepper2.startMove(-0.1 * MOTOR_STEPS2 * MICROSTEPS2);
        do {

        } while (stepper2.nextAction());
        //stepper2.disable();

        dist = dist + 1;
        
      }

      setvalue(experiment, equipment_id, device_id, shutter_state, String (dist), cover_state);

    }
    else if (value == "DECREASE")  // DISTANCE DOWN
    {
      if (dist > 0)
      {
        stepper2.enable();
        stepper2.startMove(0.1 * MOTOR_STEPS2 * MICROSTEPS2);
        do {
        } while (stepper2.nextAction());
        //stepper2.disable();

        dist = dist - 1;
        
      }

      setvalue(experiment, equipment_id, device_id, shutter_state, String (dist), cover_state);

    }
  }
  else if (type == "SHUTTER")  
  {
    if (value == "OPEN")  
    {
      myservo.write(0);
      delay(200);
      shutter_state = "open";
    }
    if (value == "CLOSE")
    {
      myservo.write(180);
      delay(200);
      shutter_state = "close";
    }else{}

    setvalue(experiment, equipment_id, device_id, shutter_state, String (dist), cover_state);

  }
  else {}


}

void setvalue(String exp_name, String equip, String device, String data1, String data2, String data3)
{
  HTTPClient http;
    http.begin("https://stem-ap.polyu.edu.hk/remotelab/api/experiment/setValue.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String out= "value={\"shutter\":\"" + data1 + "\",\"distance\":" + data2 + ",\"cover\":" + data3 + "}&experiment="+exp_name+"&equipment_id="+ equip+ "&device_id="+device;
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
