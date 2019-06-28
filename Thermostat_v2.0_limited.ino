/*
  ESP32-EVB development board (https://www.olimex.com/Products/IoT/ESP32/ESP32-EVB/open-source-hardware)
  BME280 + DS18B20 sensors
  2.24" OLED SSD1309
  I2C rotary (https://github.com/Fattoresaimon/I2CEncoderV2)
  Blynk service
*/

//Includes
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>
#include <NTPtimeESP.h>
#include <BlynkSimpleEsp32_SSL.h>
#include <HTTPClient.h>
#include <i2cEncoderLibV2.h>

//Enum
enum DISPLAY_SM {
  INIT      = 0,
  MAIN      = 1,
  INFO      = 2,
  SETTING   = 3,
  FAILED    = 7,
  FW_UPDATE = 9
  };

enum HEAT_SM {
  OFF         = 0,
  RADIATOR_ON = 1,
  FLOOR_ON    = 2,
  ALL_ON      = 3,
  PUMPOVERRUN = 4,
  JOKER       = 9
  };


//Defines
#define RELAYPIN1 32
#define RELAYPIN2 33
#define RELAYPIN3 4
#define WATERPIN  17
#define TIMEOUT   5000  //5 sec
#define AFTERCIRCTIME 300000 //5min
#define BUTIMER   61
#define MAINTIMER 60013 //1min
#define RADIATOR_HYST 0.1
#define DS18B20_RESOLUTION 11
#define ENCODER_ADDRESS 0x02

//Global variables
float actualTemperature;
float actualHumidity;
float actualPressure;
float transData;
float setValue = 22.0;
float setFloorTemp = 33.0;
float setFloorHyst = 8.0;
float waterTemperature;
int setControlBase = 2;
int buttonTime = 0;
bool firstRun =1;
bool boilerON = 0;
bool floorON = 0;
bool radiatorON = 0;
bool heatingON = 1;

const char* ssid      = "DarpAsusNet_2.4";
const char* password  = "andrew243";
const char* host      = "http://192.168.178.53/"; //Temperature transmitter
const char* auth = "08d1012dc20747899e929ef8a44a7486"; //Bylink auth

//Init services
strDateTime dateTime;
Adafruit_BME280 bme;
//U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0,U8X8_PIN_NONE,SCL,SDA);
U8G2_SSD1309_128X64_NONAME2_F_HW_I2C u8g2(U8G2_R0,U8X8_PIN_NONE,SCL,SDA);
NTPtime NTPhu("hu.pool.ntp.org");   // Choose server pool as required
BlynkTimer timer;
HTTPClient webclient;
WidgetTerminal terminal(V19);
OneWire oneWire(WATERPIN);
DallasTemperature sensor(&oneWire);
DeviceAddress sensorDeviceAddress;
i2cEncoderLibV2 Encoder(ENCODER_ADDRESS);

void GetWaterTemp()
{
  unsigned short tempRaw;
  static float lastvalidTemperature;
  
  sensor.requestTemperaturesByAddress(sensorDeviceAddress);
  tempRaw = sensor.getTemp(sensorDeviceAddress);
  while (tempRaw == 0x2A80)
  {
    sensor.requestTemperaturesByAddress(sensorDeviceAddress);
    tempRaw = sensor.getTemp(sensorDeviceAddress);
  }
  waterTemperature=tempRaw/128.0;
  if (waterTemperature > 84.0) 
  {
    waterTemperature = lastvalidTemperature;
  }
  else 
  {
    lastvalidTemperature = waterTemperature;
  }
  Blynk.virtualWrite(V10, waterTemperature);
  //Serial.println(waterTemperature);
}

void ReadBME280()
{
  static float lastvalidTemperature;
  static float lastvalidHumidity;
  static float lastvalidPressure;

  if (setControlBase == 1)
  {
    actualTemperature = bme.readTemperature();
    if (actualTemperature < 1.0) || actualTemperature > 100.0) {
      actualTemperature = lastvalidTemperature;
    }
    else {
      lastvalidTemperature = actualTemperature;
    }
  }
  actualHumidity = bme.readHumidity();
  actualPressure = bme.readPressure() / 100.0F;
  
  if (actualPressure > 1086.0 || actualPressure < 870.0) {
    actualPressure = lastvalidPressure;
  }
  else {
    lastvalidPressure = actualPressure;
  }
  if (firstRun){
    lastvalidHumidity = actualHumidity;
  }
  if (actualHumidity > 99.0) {
    actualHumidity = lastvalidHumidity;
  }
  else {
    lastvalidHumidity = actualHumidity;
  }
  Blynk.virtualWrite(V3, actualHumidity);
  Blynk.virtualWrite(V4, actualPressure);
}

void Draw_RoomTemp()
{
  char temperatureString[7];

  dtostrf(actualTemperature, 4, 1, temperatureString);
  strcat(temperatureString, "°C");
  //Serial.println(temperatureString);
  //Serial.println();
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_logisoso38_tf); // choose a suitable font
  u8g2.drawUTF8(0,44,temperatureString);  // write temperature
  if (radiatorON)
  {
    u8g2.drawDisc(10, 55, 8, U8G2_DRAW_ALL);  // virtual LED1
  }
  else
  {
    u8g2.drawCircle(10, 55, 8, U8G2_DRAW_ALL);  // virtual LED1
  }
  if (boilerON)
  {
    u8g2.drawDisc(64, 55, 8, U8G2_DRAW_ALL);  // virtual LED2  
  }
  else
  {
    u8g2.drawCircle(64, 55, 8, U8G2_DRAW_ALL);  // virtual LED2  
  }
  if (floorON)
  {
    u8g2.drawDisc(116, 55, 8, U8G2_DRAW_ALL);  // virtual LED3  
  }
  else
  {
    u8g2.drawCircle(116, 55, 8, U8G2_DRAW_ALL);  // virtual LED3
  }
  u8g2.sendBuffer();          // transfer internal memory to the display
}

void Draw_Setting()
{
  char temperatureString[8];
  char setValueString[8];
  
  dtostrf(actualTemperature, 4, 1, temperatureString);
  strcat(temperatureString, "°C");
  dtostrf(setValue, 4, 1, setValueString);
  strcat(setValueString, "°C");

  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_t0_12_tf); // choose a suitable font
  u8g2.drawUTF8(0,10,"Set:");  // write temperature
  u8g2.setFont(u8g2_font_logisoso30_tf); // choose a suitable font
  u8g2.drawUTF8(20,40,setValueString); // write temperature
  u8g2.drawLine(0, 42, 127, 42);
  u8g2.setFont(u8g2_font_t0_12_tf); // choose a suitable font
  u8g2.drawUTF8(0,54,"Actual:");  // write watertemp
  u8g2.setFont(u8g2_font_helvB18_tf); // choose a suitable font
  u8g2.drawUTF8(45,64,temperatureString); // write watertemp
  
  u8g2.sendBuffer();          // transfer internal memory to the display
}

bool RefreshDateTime()
{
  dateTime = NTPhu.getNTPtime(1.0, 1);
  if (dateTime.year > 2035)
  {
    return 0;  
  }
  return(dateTime.valid);
}

void Draw_Info()
{
  char temperatureString[8];
  char humidityString[5];
  char pressureString[8];
  char watertempString[8];
  static char dateChar[13];
  static char timeChar[11];
  static byte lastrefresh;
  bool validdate;

  validdate=RefreshDateTime();
  if (lastrefresh == dateTime.second)
  {
    return;
  }
  
  if (validdate)
  {
    sprintf(dateChar,"%i-%02i-%02i",dateTime.year,dateTime.month,dateTime.day);
    sprintf(timeChar,"%02i:%02i:%02i",dateTime.hour,dateTime.minute,dateTime.second);
  }
  dtostrf(actualTemperature, 4, 1, temperatureString);
  strcat(temperatureString, "°C");
  dtostrf(actualHumidity, 2, 0, humidityString);
  strcat(humidityString, " %");
  dtostrf(actualPressure, 4, 0, pressureString);
  strcat(pressureString, " Pa");
  dtostrf(waterTemperature, 4, 1, watertempString);
  strcat(watertempString, "°C");
  
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_t0_12_tf); // choose a suitable font
  u8g2.drawUTF8(0,10,dateChar);  // write date
  u8g2.drawUTF8(65,10,timeChar);  // write time
  u8g2.drawUTF8(0,20,"Roomtemp:");  // write temperature
  u8g2.drawUTF8(65,20,temperatureString); // write temperature
  u8g2.drawUTF8(0,30,"Humidity:");  // write humidity
  u8g2.drawUTF8(65,30,humidityString); // write humidity
  u8g2.drawUTF8(0,40,"Pressure:");  // write pressure
  u8g2.drawUTF8(65,40,pressureString); // write pressure
  u8g2.drawUTF8(0,50,"Watertemp:");  // write watertemp
  u8g2.drawUTF8(65,50,watertempString); // write watertemp
  u8g2.sendBuffer();          // transfer internal memory to the display
  lastrefresh == dateTime.second;
}

void ReadTransmitter() 
{
  static float lastvalidtransTemp;

  webclient.begin(host);
  if(webclient.GET() == HTTP_CODE_OK) 
  {
    transData = webclient.getString().toFloat();
  }
  webclient.end();

  if (transData < 10.0) {
    transData = lastvalidtransTemp;
    //setControlBase = 1;
    return;
  }
  else
  {
    lastvalidtransTemp = transData;
  }
  if (setControlBase == 2)
  {
    actualTemperature = transData;
  }
  Blynk.virtualWrite(V1, actualTemperature);
  Blynk.virtualWrite(V13, transData);
}

void ManageHeating()
{
  static unsigned long aftercirc;
  static HEAT_SM heatstate = OFF;
  static HEAT_SM laststate;
  static float storeFloorHyst;

  if (!heatingON)
  {
    if (laststate = OFF)
    {
      return;
    }
    else
    {
      digitalWrite(RELAYPIN1, 0);
      digitalWrite(RELAYPIN2, 0);
      digitalWrite(RELAYPIN3, 0);
      boilerON = 0;
      radiatorON = 0;
      floorON = 0;
      laststate = OFF;
      Blynk.virtualWrite(V7, boilerON);
      Blynk.virtualWrite(V8, floorON);
      Blynk.virtualWrite(V9, radiatorON); 
      return;
    }
  }
  
  switch (heatstate)
  {
    case OFF:
    {
    if (laststate!=OFF)
    {
      digitalWrite(RELAYPIN1, 0);
      digitalWrite(RELAYPIN2, 0);
      digitalWrite(RELAYPIN3, 0);
      boilerON = 0;
      radiatorON = 0;
      floorON = 0;
      storeFloorHyst = setFloorHyst;
      laststate = OFF;
      Blynk.virtualWrite(V7, boilerON);
      Blynk.virtualWrite(V8, floorON);
      Blynk.virtualWrite(V9, radiatorON); 
      break;
    }
    if (actualTemperature < (setValue - RADIATOR_HYST))
    {
      digitalWrite(RELAYPIN1, 1);
      digitalWrite(RELAYPIN2, 1);
      boilerON = 1;
      radiatorON = 1;
      storeFloorHyst = setFloorHyst;
      setFloorHyst = 1.0;
      heatstate = RADIATOR_ON;
      Blynk.virtualWrite(V7, boilerON);
      Blynk.virtualWrite(V9, radiatorON); 
      break;
    }
    if (waterTemperature < (setFloorTemp - setFloorHyst))
    {
      digitalWrite(RELAYPIN1, 1);
      digitalWrite(RELAYPIN3, 1);
      boilerON = 1;
      floorON = 1;
      storeFloorHyst = setFloorHyst;
      heatstate = FLOOR_ON;
      Blynk.virtualWrite(V7, boilerON);
      Blynk.virtualWrite(V8, floorON);
      break;
    }
    break;
    }
    
    case RADIATOR_ON:
    {
     if (waterTemperature < (setFloorTemp - setFloorHyst))
     {
      digitalWrite(RELAYPIN3, 1);
      floorON = 1;
      heatstate = ALL_ON;
      laststate = RADIATOR_ON;
      Blynk.virtualWrite(V8, floorON);
      break;
     }
     if (actualTemperature > (setValue + RADIATOR_HYST))
     {
      digitalWrite(RELAYPIN1, 0);
      digitalWrite(RELAYPIN2, 0);
      digitalWrite(RELAYPIN3, 1);
      boilerON = 0;
      radiatorON = 0;
      floorON = 1;
      setFloorHyst = storeFloorHyst;
      heatstate = PUMPOVERRUN;
      laststate = RADIATOR_ON;
      Blynk.virtualWrite(V11, setFloorHyst);
      Blynk.virtualWrite(V7, boilerON);
      Blynk.virtualWrite(V8, floorON);
      Blynk.virtualWrite(V9, radiatorON); 
      break;
     }
     break;
    }

    case FLOOR_ON:
    {
     if (actualTemperature < (setValue - RADIATOR_HYST))
     {
      digitalWrite(RELAYPIN2, 1);
      radiatorON = 1;
      storeFloorHyst = setFloorHyst;
      setFloorHyst = 1.0;
      heatstate = ALL_ON;
      laststate = FLOOR_ON;
      Blynk.virtualWrite(V11, setFloorHyst);
      Blynk.virtualWrite(V9, radiatorON); 
      break;
     }
     if (waterTemperature > setFloorTemp)
     {
      digitalWrite(RELAYPIN1, 0);
      boilerON = 0;
      heatstate = PUMPOVERRUN;
      laststate = FLOOR_ON;
      Blynk.virtualWrite(V7, boilerON);
     }
     break;
    }
    
    case ALL_ON:
    {
     if (actualTemperature > (setValue + RADIATOR_HYST))
     {
      digitalWrite(RELAYPIN2, 0);
      radiatorON = 0;
      setFloorHyst = storeFloorHyst;
      heatstate = FLOOR_ON;
      laststate = ALL_ON;
      Blynk.virtualWrite(V11, setFloorHyst);
      Blynk.virtualWrite(V9, radiatorON); 
      break;
     }
     if (waterTemperature > setFloorTemp)
     {
      digitalWrite(RELAYPIN3, 0);
      floorON = 0;
      heatstate = RADIATOR_ON;
      laststate = ALL_ON;
      Blynk.virtualWrite(V8, floorON);
      break;
     }
     break;
    }

    case PUMPOVERRUN:
    {
     if (laststate!=PUMPOVERRUN)
     {
      aftercirc=millis();
      laststate = PUMPOVERRUN;
      break;
     }
     if (millis()-aftercirc > AFTERCIRCTIME)
     {
      heatstate = OFF;
      laststate = PUMPOVERRUN;
      break;
     }
     break;
    }
  }
}

void DrawDisplay()
{
  static DISPLAY_SM infobox = INIT;
  static unsigned long stateTimeout;
  
  switch (infobox)
  {
    case INIT:
    {
      infobox = MAIN;
      break;
    }
    case MAIN:
    {
      Draw_RoomTemp();
      if (buttonTime>2000)
      {
        infobox = SETTING;
        stateTimeout=millis();
        break;
      }
      if (buttonTime>73)
      {
        infobox = INFO;
        stateTimeout=millis();
        break;
      }
      break;
    }
    case INFO:
    {
      Draw_Info();
      if (millis()-stateTimeout > TIMEOUT)
      {
        infobox = MAIN;
      }
      break;
    }
    case SETTING:
      {
        Draw_Setting();
        if (buttonTime>73 && buttonTime<2000)
        {
          stateTimeout=millis();
          setValue = setValue+0.5;
          if (setValue>25.0)
          {
            setValue = 18.0;
          }
        }
        if (millis()-stateTimeout > TIMEOUT)
        {
          Blynk.virtualWrite(V5, setValue);
          infobox = MAIN;
        }
        break;
      }
  }
}

BLYNK_WRITE(V2)
{
  heatingON = param.asInt();
  MainTask();
}
BLYNK_WRITE(V5)
{
  setValue = param.asFloat();
  MainTask();
}
BLYNK_WRITE(V6)
{
  setFloorTemp = param.asFloat();
  MainTask();
}
BLYNK_WRITE(V11)
{
  setfloorHyst = param.asFloat();
  MainTask();
}
BLYNK_WRITE(V12)
{
  setControlBase = param.asInt();
  MainTask();
}

void MainTask()
{
  unsigned long tic = millis();
  GetWaterTemp();
  ReadBME280();
  ReadTransmitter();
  ManageHeating();
  Draw_RoomTemp();
  terminal.println("Main task time:");
  terminal.println(millis()-tic);
  terminal.flush();
}

void setup() {
  pinMode(RELAYPIN1, OUTPUT);
  digitalWrite(RELAYPIN1, 0);
  pinMode(RELAYPIN2, OUTPUT);
  digitalWrite(RELAYPIN2, 0);
  pinMode(RELAYPIN3, OUTPUT);
  digitalWrite(RELAYPIN3, 0);

  //timer.setInterval(BUTIMER,ButtonCheck);
  timer.setInterval(MAINTIMER,MainTask);
  
  Serial.begin(115200);
  delay(100);
  Wire.begin(SDA,SCL);
  Wire.setClock(400000);
  delay(100);
  bme.begin();
  delay(100);
  u8g2.begin();
  delay(100);
  sensor.begin();
  delay(100);
  sensor.getAddress(sensorDeviceAddress, 0);
  delay(100);
  sensor.setResolution(sensorDeviceAddress, DS18B20_RESOLUTION);
  delay(100);
  Encoder.begin(INT_DATA | WRAP_DISABLE | DIRE_RIGHT | IPUP_DISABLE | RMOD_X1 | RGB_ENCODER);
  delay(100);
  Encoder.writeCounter((int32_t)180); /* Reset the counter value */
  Encoder.writeMax((int32_t)255); /* Set the maximum  */
  Encoder.writeMin((int32_t)180); /* Set the minimum  */
  Encoder.writeStep((int32_t)5); /* Set the step to 5 */
  Encoder.writeInterruptConfig(0x00); /* Disable all the interrupt */
  Encoder.writeAntibouncingPeriod(20);  /* Set an anti-bouncing of 200ms */
  Encoder.writeDoublePushPeriod(50);  /*Set a period for the double push of 500ms*/

  WiFi.mode(WIFI_STA);
  WiFi.begin (ssid, password);
  
  // Wait for connection
  unsigned long wifitimeout = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
    if (millis()-wifitimeout > TIMEOUT)
    {
      break;
    }
  }
  //Serial.println("");
  //Serial.println("Wifi connected!");
  
  Blynk.config(auth);
  Blynk.connect();
  Blynk.syncAll();
  Blynk.virtualWrite(V7, boilerON);
  Blynk.virtualWrite(V8, floorON);
  Blynk.virtualWrite(V9, radiatorON); 

  MainTask();
  firstRun = 0;
}

void loop() {
  if (Blynk.connected())
  {
    Blynk.run();
  }
  else
  {
    Blynk.disconnect();
    delay(1000);
    Blynk.connect();
  }
  timer.run();
}
