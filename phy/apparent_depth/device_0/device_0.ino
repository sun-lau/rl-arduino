#include <Arduino.h>
#include "BasicStepperDriver.h" // generic
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
IPAddress local_IP(10, 15, 120, 30);
// Set your Gateway IP address
IPAddress gateway(10, 15, 120, 2);
IPAddress subnet(255, 255, 252, 0);
IPAddress primaryDNS(158, 132, 18, 1);   //optional
IPAddress secondaryDNS(158, 132, 14, 1); //optional

// this pin should connect to Ground when want to stop the motor
#define STOPPER_PIN1 33
// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS1 200
#define RPM1 150
// Microstepping mode. If you hardwired it to save pins, set to the same value here.
#define MICROSTEPS1 16
#define DIR1 4
#define STEP1 26

// this pin should connect to Ground when want to stop the motor
#define STOPPER_PIN2 5
// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS2 200
#define RPM2 150
// Microstepping mode. If you hardwired it to save pins, set to the same value here.
#define MICROSTEPS2 16
#define DIR2 25
#define STEP2 27



#define EN 2
BasicStepperDriver stepper1(MOTOR_STEPS1, DIR1, STEP1 , EN);        //horizontal
BasicStepperDriver stepper2(MOTOR_STEPS2, DIR2, STEP2 , EN);        //vertical

String equipment_id="set_0";
String device_id="device_0";
String experiment="APPARENT_DEPTH";

long cmd_set_at;
long cmd_got_at;
long temp_set = 0;
int Vdistance = 0;
int Hdistance = 0;

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
    stepper1.enable();
    stepper1.startMove(100 * MOTOR_STEPS1 * MICROSTEPS1);
    do {
      if (
        digitalRead(STOPPER_PIN1) == LOW) {
        stepper1.stop();
      }

    } while (stepper1.nextAction());
    stepper1.startMove(-1 * MOTOR_STEPS1 * MICROSTEPS1);
    do {

    } while (stepper1.nextAction());
    stepper1.disable();

    stepper2.enable();
    stepper2.startMove(100 * MOTOR_STEPS2 * MICROSTEPS2);
    do {
      if (
        digitalRead(STOPPER_PIN2) == LOW) {
        stepper2.stop();
      }

    } while (stepper2.nextAction());
    stepper2.startMove(-1 * MOTOR_STEPS2 * MICROSTEPS2);
    do {

    } while (stepper2.nextAction());
    stepper2.disable();

    Vdistance = 0;
    Hdistance = 0;

   Serial.println("#" +String(Hdistance) + "," + String(Vdistance));

  }
  }

  else if (type == "POSITION_MAP")
  {

    String p_x = value.substring(0, value.indexOf(","));
    String p_y = value.substring(value.indexOf(",") + 1);
    int xstep =0;
    if (p_x.toInt() != Hdistance)
    {
      if (p_x.toInt()==1&&Hdistance==0)
      {
        xstep=6;
      }else if (p_x.toInt()==2&&Hdistance==0)
      {
        xstep=8;
      }else if (p_x.toInt()==0&&Hdistance==1)
      {
        xstep=-6;
      }else if (p_x.toInt()==2&&Hdistance==1)
      {
        xstep=2;
      }else if (p_x.toInt()==0&&Hdistance==2)
      {
        xstep=-8;
      }else if (p_x.toInt()==1&&Hdistance==2)
      {
        xstep=-2;
      }
      stepper1.enable();

      stepper1.startMove(-2 * MOTOR_STEPS1 * MICROSTEPS1 * xstep);
      do {

      } while (stepper1.nextAction());
      stepper1.disable();
      Hdistance = p_x.toInt();
    }
    if (p_y.toInt() != Vdistance)
    {
      stepper2.enable();

      stepper2.startMove(-0.5 * MOTOR_STEPS2 * MICROSTEPS2 * (p_y.toInt() - Vdistance));
      do {

      } while (stepper2.nextAction());
      stepper2.disable();
      Vdistance = p_y.toInt();
    }
    Serial.println("#" +String(Hdistance) + "," + String(Vdistance));
    
    

  }
  else if (type == "MEASURE")
  {
    HTTPClient http;
    http.begin("https://stem-ap.polyu.edu.hk/remotelab/api/experiment/setValue.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.POST("value={\"x\":" + (String)Hdistance + ",\"y\":" + (String)Vdistance +"}&experiment="+experiment+"&equipment_id="+ equipment_id+ "&device_id="+device_id);
    http.writeToStream(&Serial);
    http.end();
  }else{}
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
