#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_ADS1015.h>

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
IPAddress local_IP(10, 15, 120, 91);
// Set your Gateway IP address
IPAddress gateway(10, 15, 120, 2);
IPAddress subnet(255, 255, 252, 0);
IPAddress primaryDNS(158, 132, 18, 1);   //optional
IPAddress secondaryDNS(158, 132, 14, 1); //optional

String equipment_id="set_0";
String device_id="device_1";
String experiment="PHOTO_ELECTRIC";

#define CMD_SETA_UPDATEA          0x18
#define CMD_SETB_UPDATEB          0x19
#define CMD_UPDATE_ALL_DACS        0x0F

#define CMD_GAIN            0x02
#define DATA_GAIN_B2_A2         0x0000
#define DATA_GAIN_B2_A1           0x0001
#define DATA_GAIN_B1_A2           0x0002
#define DATA_GAIN_B1_A1           0x0003

#define CMD_PWR_UP_A_B            0x20
#define DATA_PWR_UP_A_B           0x0003

#define CMD_RESET_ALL_REG         0x28
#define DATA_RESET_ALL_REG        0x0001

#define CMD_LDAC_DIS            0x30
#define DATA_LDAC_DIS           0x0003

#define CMD_INTERNAL_REF_DIS      0x38
#define DATA_INTERNAL_REF_DIS     0x0000
#define CMD_INTERNAL_REF_EN         0x38
#define DATA_INTERNAL_REF_EN      0x0001

Adafruit_ADS1115 ads1(0x48);

const int slaveSelectPin = 5;
const int SCLKPin = 18;
const int DINPin = 23;

long cmd_set_at;
long cmd_got_at;
long temp_set = 0;
float current;

DynamicJsonDocument doc(1000);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector


  pinMode(slaveSelectPin, OUTPUT);
  pinMode(SCLKPin, OUTPUT);
  pinMode(DINPin, OUTPUT);
  Init_DAC8562();
  ads1.setGain(GAIN_ONE);
  ads1.begin();

  Serial.begin(115200);

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

void Init_DAC8562(void)
{

  DAC_WR_REG(CMD_RESET_ALL_REG, DATA_RESET_ALL_REG);
  DAC_WR_REG(CMD_PWR_UP_A_B, DATA_PWR_UP_A_B);
  DAC_WR_REG(CMD_INTERNAL_REF_EN, DATA_INTERNAL_REF_EN);
  DAC_WR_REG(CMD_GAIN, DATA_GAIN_B2_A2);
  DAC_WR_REG(CMD_LDAC_DIS, DATA_LDAC_DIS);
}


void Output_DAC8562(unsigned int data_a)
{
  DAC_WR_REG(CMD_SETA_UPDATEA, data_a);

}


void DAC_WR_REG(unsigned char cmd_byte, unsigned int data_byte)
{
  digitalWrite(slaveSelectPin, LOW);
  SPI_SendByte(cmd_byte);
  SPI_SendHalfWord(data_byte);
  digitalWrite(slaveSelectPin, HIGH);

}


void SPI_SendHalfWord(unsigned int m)
{
  unsigned char i, r = 0;
  for (i = 0; i < 16; i++)
  {
    digitalWrite(SCLKPin, HIGH);
    r = r << 1;

    if (m & 0x8000)
    {
      digitalWrite(DINPin, HIGH);
    }
    else
    {
      digitalWrite(DINPin, LOW);
    }
    m = m << 1;
    digitalWrite(SCLKPin, LOW);
  }
}


void SPI_SendByte(unsigned char m)
{
  unsigned char i, r = 0;
  for (i = 0; i < 8; i++)
  {
    digitalWrite(SCLKPin, HIGH);
    r = r << 1;

    if (m & 0x80)
    {
      digitalWrite(DINPin, HIGH);
    }
    else
    {
      digitalWrite(DINPin, LOW);
    }
    m = m << 1;
    digitalWrite(SCLKPin, LOW);
  }
}

unsigned int Voltage_Convert(float voltage)
{
  unsigned int _D_;
  _D_ = (unsigned int)((32768 * voltage / 10.46 ) + 32768);
  return _D_;
}

void command(String type , String value) {

  if (type == "RESTART")            //RESTART  power off
  {
    if (value=="1")
  {
    ESP.restart();
  }
  else{
    Output_DAC8562( Voltage_Convert(0));
    current = 0;
  }
    Serial.println("#");
  }

  else if (type == "MEASURE")
  {
    String Data = "";
    float v=5;
      Output_DAC8562( Voltage_Convert(v));
      delay(200);
      do
      {
      Output_DAC8562( Voltage_Convert(v));
      delay(50);
      current=0;
      for(int i=0;i<10;i++)
      {
      current+=(ads1.readADC_Differential_0_1() * -4.096 / 32768)/10.;
      }  //mA
      
    Data += String(v,2) +","+String(current/20,6) + "|";
    Serial.println(String(v,2) +","+String(current/20,6));
    if(v>0)
    {
    v=v-0.2;
    }else
    {
      v=v-0.02;
    }
      }while(v>=-2);

      Output_DAC8562( Voltage_Convert(0));

      Data = Data.substring(0, Data.length() - 1);
    HTTPClient http;
    http.begin("https://stem-ap.polyu.edu.hk/remotelab/api/experiment/setChart.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    //String out="chart=5.00,0.12344|4.50,0.12345&experiment="+experiment+"&equipment_id="+ equipment_id+ "&device_id="+device_id;
    String out="chart=" + Data + "&experiment="+experiment+"&equipment_id="+ equipment_id+ "&device_id="+device_id;
    Serial.println(out);
    http.POST(out);
    http.writeToStream(&Serial);
    http.end();


   
  Serial.println("#");
  }
  else{}
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
