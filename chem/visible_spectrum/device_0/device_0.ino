#include <Arduino.h>
#include "BasicStepperDriver.h" // generic
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_wpa2.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
//#include <Adafruit_NeoPixel.h>
#include <FastLED.h>

// wifi config
const char* ssid = "STEM"; // your ssid
#define EAP_ID "gststem01"
#define EAP_USERNAME "gststem01"
#define EAP_PASSWORD "A1b2c3d4"
// Set your Static IP address
IPAddress local_IP(10, 15, 121, 135);
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

#define EN 2

CRGB leds[1];
#define LEDPIN  23
BasicStepperDriver stepper1(MOTOR_STEPS1, DIR1, STEP1 , EN);        //horizontal
//Adafruit_NeoPixel pixels(1, LEDPIN, NEO_GRB + NEO_KHZ800);

String equipment_id="set_0";
String device_id="device_0";
String experiment="VISIBLE_SPECTRUM";

int Hdistance = 0;
int lamp = 21;
int power = 22;
String source = "0";
String p_x;
char spec[9001];

long cmd_set_at;
long cmd_got_at;
long temp_set = 0;


DynamicJsonDocument doc(1000);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  Serial2.begin(250000, SERIAL_8N1, 16, 17);
  Serial2.setRxBufferSize(4000);
  //pixels.begin();
  //pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    //pixels.show();
    FastLED.addLeds<NEOPIXEL, LEDPIN>(leds, 1);  // GRB ordering is assumed
  pinMode(lamp, OUTPUT);
  pinMode(power, OUTPUT);

  // Configure stopper pin to read HIGH unless grounded
  pinMode(STOPPER_PIN1, INPUT);

  stepper1.begin(RPM1, MICROSTEPS1);

  stepper1.setEnableActiveState(LOW);

  stepper1.disable();

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

  if (type == "RESTART")            //RESTART  power off
  {
    if (value=="ALL")
  {
    ESP.restart();
  }
  else{
    stepper1.enable();
    stepper1.startMove(-100 * MOTOR_STEPS1 * MICROSTEPS1);
    do {
      if (
        digitalRead(STOPPER_PIN1) == LOW) {
        stepper1.stop();
      }

    } while (stepper1.nextAction());
    stepper1.startMove(1 * MOTOR_STEPS1 * MICROSTEPS1);
    do {

    } while (stepper1.nextAction());
    stepper1.disable();

//pixels.clear();
    //pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    
    //pixels.show();
  leds[0] = CRGB::Black;
  FastLED.show();

    Hdistance = 0;
    source = "0";
    Serial.println("#" + String(Hdistance));

  }
  }

  else if (type == "SOURCE")
  {
    if ( value=="MERCURY")
    {source="0";}
    else if ( value=="SODIUM")
    {source="1";}
    else if ( value=="RED")
    {source="2";}
    else if ( value=="GREEN")
    {source="3";}
    else if ( value=="BLUE")
    {source="4";}
    else if ( value=="WHITE")
    {source="5";}
    else if ( value=="OFF")
    {source="6";}
    else 
    {source="6";}

        if(source !="6")
        {
    if (source == "2" || source == "3" || source == "4" || source == "5")
    {
      p_x = "2";
    } else
    { p_x = source;
    }

    int xstep = 0;
    if (p_x.toInt() != Hdistance)
    {
      if (p_x.toInt() == 1 && Hdistance == 0)
      {
        //pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        //pixels.show();
        leds[0] = CRGB::Black;
        FastLED.show();
        digitalWrite(power, LOW);
        digitalWrite(lamp, HIGH);
        xstep = 50;
      } else if (p_x.toInt() == 2 && Hdistance == 0)
      {
        //pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        //pixels.show();
        leds[0] = CRGB::Black;
        FastLED.show();
        digitalWrite(power, LOW);
        xstep = 100;
      } else if (p_x.toInt() == 0 && Hdistance == 1)
      {
        //pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        //pixels.show();
        leds[0] = CRGB::Black;
        FastLED.show();
        digitalWrite(power, LOW);
        digitalWrite(lamp, LOW);
        xstep = -50;
      } else if (p_x.toInt() == 2 && Hdistance == 1)
      {
        //pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        //pixels.show();
        leds[0] = CRGB::Black;
        FastLED.show();
        digitalWrite(power, LOW);
        xstep = 50;
      } else if (p_x.toInt() == 0 && Hdistance == 2)
      {
        //pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        //pixels.show();
        leds[0] = CRGB::Black;
        FastLED.show();
        digitalWrite(power, LOW);
        digitalWrite(lamp, LOW);
        xstep = -100;
      } else if (p_x.toInt() == 1 && Hdistance == 2)
      {
        //pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        //pixels.show();
        leds[0] = CRGB::Black;
        FastLED.show();
        digitalWrite(power, LOW);
        digitalWrite(lamp, HIGH);
        xstep = -50;
      }
      stepper1.enable();

      stepper1.startMove(0.5 * MOTOR_STEPS1 * MICROSTEPS1 * xstep);
      do {

      } while (stepper1.nextAction());
      stepper1.disable();
      Hdistance = p_x.toInt();
    }

    if (source == "0" || source == "1")
    {
      digitalWrite(power, HIGH);
    } else if (source == "2")
    {
      //pixels.setPixelColor(0, pixels.Color(255, 0, 0));
      //pixels.show();
      leds[0] = CRGB::Red;
        FastLED.show();
    } else if (source == "3")
    {
      //pixels.setPixelColor(0, pixels.Color(0, 255, 0));
      //pixels.show();
      leds[0] = CRGB::Green;
        FastLED.show();
    } else if (source == "4")
    {
      //pixels.setPixelColor(0, pixels.Color(0, 0, 255));
      //pixels.show();
      leds[0] = CRGB::Blue;
        FastLED.show();
    } else if (source == "5")
    {
      //pixels.setPixelColor(0, pixels.Color(255, 255, 255));
      //pixels.show();
      leds[0] = CRGB::White;
        FastLED.show();
    } else {}


    Serial.println("#" + String(Hdistance));
        } else {
          leds[0] = CRGB::Black;
        FastLED.show();
        digitalWrite(power, LOW);
        
        }

  } else if (type == "MEASURE")
  {
    
      Serial2.write("SPEC.READ?\n");

      delay(300);

      while (Serial2.available() > 0) {
        memset(spec, 0, 6001);

        Serial2.readBytesUntil( '\n', spec, 6000);
        Serial.println(spec);
      }
      HTTPClient http;
    http.begin("https://stem-ap.polyu.edu.hk/remotelab/api/experiment/setChart.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.POST("chart=" + (String)spec + "&experiment="+experiment+"&equipment_id="+ equipment_id+ "&device_id="+device_id);
    http.writeToStream(&Serial);
    http.end();
    
  } 
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
    delay(100);

  } else
  {
    WIFI_Connect();
  }



}
