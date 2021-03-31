#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <BasicStepperDriver.h> // generic
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
IPAddress local_IP(10, 15, 120, 71);
// Set your Gateway IP address
IPAddress gateway(10, 15, 120, 2);
IPAddress subnet(255, 255, 252, 0);
IPAddress primaryDNS(158, 132, 18, 1);   //optional
IPAddress secondaryDNS(158, 132, 14, 1); //optional

//Define for motor driver
//define motor 1 pins
// this pin should connect to Ground when want to stop the motor
#define STOPPER_PIN1 33
// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS1 200
// Microstepping mode. If you hardwired it to save pins, set to the same value here.
#define MICROSTEPS1 16
#define DIR1 4
#define STEP1 26

#define EN 2
BasicStepperDriver stepper1(MOTOR_STEPS1, DIR1, STEP1, EN);

String equipment_id="set_0";
String device_id="device_1";
String experiment="INTERFERENCE";

int RPM = 200;
int movement_range = 50 * 10; //unit = mm
int stage = 0;
int movement = 0;

long cmd_set_at;
long cmd_got_at;
long temp_set = 0;
String Data = "";
//Define for light sensor TSL2561

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

void displaySensorDetails(void)
{
  sensor_t sensor;
  tsl.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" lux");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" lux");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" lux");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void configureSensor(void)
{
  /* You can also manually set the gain or enable auto-gain support */
  tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  //tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  //tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */

  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above! */
  Serial.println("------------------------------------");
  Serial.print  ("Gain:         "); Serial.println("Auto");
  Serial.print  ("Timing:       "); Serial.println("13 ms");
  Serial.println("------------------------------------");
}

DynamicJsonDocument doc(1000);

void setup(void)
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  pinMode(STOPPER_PIN1, INPUT);
  stepper1.begin(RPM, MICROSTEPS1);

  stepper1.setEnableActiveState(LOW);

  stepper1.disable();

  command("RESTART", "END");
  //Serial.println("Light Sensor Test"); Serial.println("");

  /* Initialise the sensor */
  //use tsl.begin() to default to Wire,
  // tsl.begin(&Wire2) directs api to use Wire2, etc.
  if (!tsl.begin())
  {
    /* There was a problem detecting the TSL2561 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while (1);
  }

  /* Display some basic information on this sensor */
  displaySensorDetails();

  /* Setup the sensor gain and integration time */
  configureSensor();

  /* We're ready to go! */
  //Serial.println("");

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
    stepper1.startMove(-100 * MOTOR_STEPS1 * MICROSTEPS1);
    do {
      if (
        digitalRead(STOPPER_PIN1) == LOW) {
        stepper1.stop();
      }

    } while (stepper1.nextAction());
    stepper1.startMove(2 * MOTOR_STEPS1 * MICROSTEPS1);
    do {

    } while (stepper1.nextAction());
    stepper1.disable();
    Serial.println("#");
  }
  }

  else if (type == "MEASURE")      // POSITION MEASURE
  {
    movement = 0;
    Data = "";
    stepper1.enable();
    while ((movement < movement_range) && (digitalRead(STOPPER_PIN1) != LOW)) {
      /* Get a new sensor event */
      sensors_event_t event;
      tsl.getEvent(&event);

      /* Display the results (light is measured in lux) */
      if (event.light)
      {
        Data += (String)(movement / 10.) + "," + (String)event.light + "|";
        //Serial.print(movement/10.); Serial.print(",");Serial.println(event.light);
        //data1.add(event.light);
      }
      else
      {
        /* If event.light = 0 lux the sensor is probably saturated
           and no reliable data could be generated! */
        Serial.println("Sensor overload");
      }
      stepper1.startMove(0.5 * 0.1 * MOTOR_STEPS1 * MICROSTEPS1);
      do {
        if (
          digitalRead(STOPPER_PIN1) == LOW) {
          stepper1.stop();
        }

      } while (stepper1.nextAction());
      movement += 1;
    }
    stepper1.startMove(-100 * MOTOR_STEPS1 * MICROSTEPS1);
    do {
      if (
        digitalRead(STOPPER_PIN1) == LOW) {
        stepper1.stop();
      }

    } while (stepper1.nextAction());
    stepper1.startMove(2 * MOTOR_STEPS1 * MICROSTEPS1);
    do {

    } while (stepper1.nextAction());
    stepper1.disable();

    Data = Data.substring(0, Data.length() - 1);
    HTTPClient http;
    http.begin("https://stem-ap.polyu.edu.hk/remotelab/api/experiment/setChart.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.POST("chart=" + Data + "&experiment="+experiment+"&equipment_id="+ equipment_id+ "&device_id="+device_id);
    http.writeToStream(&Serial);
    http.end();

    Serial.println("#");
  }
  else
  {}
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



  }
  else
  {
    WIFI_Connect();
  }

}
