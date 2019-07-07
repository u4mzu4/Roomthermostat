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
#include <icons.h>

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

enum SETTING_SM {
  RADIATOR  = 0,
  FLOOR     = 1,
  HOLIDAY   = 2,
  SOFA      = 3,
  CHILD     = 4,
  HYST      = 5,
  THERMO    = 6,
  HYST_SET  = 7,
  THERMO_SET= 8,
  RESERVED  = 9
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
float setFloorTemp = 31.0;
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
    if (actualTemperature < 1.0 || actualTemperature > 100.0) {
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

void ButtonCheck()
{
  static unsigned long stateStartTime;
  static DISPLAY_SM displayBox = INIT;
  static bool newSettings = 1;
  
  switch (displayBox)
  {
    case INIT:
    {
      displayBox = MAIN;
      break;
    }
    case MAIN:
    {
      if (Encoder.updateStatus()) {
        if (Encoder.readStatus(PUSHD)){
          displayBox = SETTING;
          stateStartTime = millis();
        }
        else if (Encoder.readStatus(PUSHP)){
          displayBox = INFO;
          stateStartTime = millis();
        }
      }
      break;
    }
    case INFO:
    {
      Draw_Info();
      if (millis()-stateStartTime > TIMEOUT)
      {
        Encoder.writeLEDR(0x00);
        Encoder.writeLEDG(0x00);
        Encoder.updateStatus();
        Draw_RoomTemp();
        displayBox = MAIN;
      }
      break;
    }
    case SETTING:
    {
      if (Draw_Setting(newSettings))
      {
        Encoder.writeLEDR(0x00);
        Encoder.writeLEDB(0x00);
        Encoder.updateStatus();
        Draw_RoomTemp();
        displayBox = MAIN;
        newSettings = 1;
      }
      else
      {
        newSettings = 0;
      }
      break;
    }
  }
}

void Draw_RoomTemp()
{
  char temperatureString[8];

  dtostrf(actualTemperature, 4, 1, temperatureString);
  strcat(temperatureString, "°C");
  //Serial.println(temperatureString);
  //Serial.println();
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_logisoso34_tf); // choose a suitable font
  u8g2.drawUTF8(8,42,temperatureString);  // write temperature
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

void Draw_Bitmap (int posx,int posy,int width, int height,unsigned char* varname)
{ 
  u8g2.clearBuffer(); 
  u8g2.drawXBM(posx, posy, width, height, varname); 
  u8g2.sendBuffer();
}

bool Draw_Setting(bool smReset)
{
  static SETTING_SM settingState = RADIATOR;
  static SETTING_SM prevState;
  static char actualString[8];
  static unsigned long stateEnterTime;
  char setString[8];
  bool leaveMenu = 0;
  int rotaryPosition;
  
  if (smReset)
  {
    settingState = RADIATOR;
    stateEnterTime = millis();
  }
  if (millis()-stateEnterTime > 2*TIMEOUT)
  {
    leaveMenu = 1;
  }
  switch (settingState)
  {
    case RADIATOR:
    {
      Draw_Bitmap(32,0,radiator_width, radiator_height,radiator_bits);
      if (Encoder.updateStatus()) {
        stateEnterTime = millis();
        if (Encoder.readStatus(RINC)){
          settingState = FLOOR;
        }
        if (Encoder.readStatus(PUSHP)){
          settingState = CHILD;
        }
      }
      break;
    }
    case FLOOR:
    {
      Draw_Bitmap(32,0,floor_width, floor_height,floor_bits);
      if (Encoder.updateStatus()) {
        stateEnterTime = millis();
        if (Encoder.readStatus(RDEC)){
          settingState = RADIATOR;
        }
        if (Encoder.readStatus(RINC)){
          settingState = HOLIDAY;
        }
        if (Encoder.readStatus(PUSHP)){
          settingState = HYST;
        }
      }
      break;
    }
    case HOLIDAY:
    {
      Draw_Bitmap(32,0,holiday_width, holiday_height,holiday_bits);
      if (Encoder.updateStatus()) {
        stateEnterTime = millis();
        if (Encoder.readStatus(RDEC)){
          settingState = FLOOR;
        }
        if (Encoder.readStatus(PUSHP)){
          setValue = 20.0;
          setFloorTemp = 31.0;
          setFloorHyst = 9.0;
          Blynk.virtualWrite(V5, setValue);
          Blynk.virtualWrite(V6, setFloorTemp);
          Blynk.virtualWrite(V11, setFloorHyst);
          leaveMenu = 1;
        }
      }
      break;
    }
    case CHILD:
    {
      Draw_Bitmap(32,0,childroom_width, childroom_height,childroom_bits);
      if (Encoder.updateStatus()) {
        stateEnterTime = millis();
        if (Encoder.readStatus(RINC)){
          settingState = SOFA;
        }
        if (Encoder.readStatus(PUSHP)){
          setControlBase = 2;
          Blynk.virtualWrite(V12, setControlBase);
          settingState = THERMO;
          prevState = CHILD;
        }
      }
      break;
    }
    case SOFA:
    {
      Draw_Bitmap(32,0,sofa_width, sofa_height,sofa_bits);
      if (Encoder.updateStatus()) {
        stateEnterTime = millis();
        if (Encoder.readStatus(RDEC)){
          settingState = CHILD;
        }
        if (Encoder.readStatus(PUSHP)){
          setControlBase = 1;
          Blynk.virtualWrite(V12, setControlBase);
          settingState = THERMO;
          prevState = SOFA;
        }
      }
      break;
    }
    case HYST:
    {
      Draw_Bitmap(0,0,hysteresis_width, hysteresis_height,hysteresis_bits);
      if (Encoder.updateStatus()) {
        stateEnterTime = millis();
        if (Encoder.readStatus(RINC)){
          settingState = THERMO;
          prevState = HYST;
        }
        if (Encoder.readStatus(PUSHP)){
          settingState = HYST_SET;
          prevState = HYST;
        }
      }
      break;
    }
    case THERMO:
    {
      Draw_Bitmap(32,0,thermometer_width, thermometer_height,thermometer_bits);
      if (Encoder.updateStatus()) {
        stateEnterTime = millis();
        if (Encoder.readStatus(RDEC) && prevState==HYST){
          settingState = HYST;
        }
        if (Encoder.readStatus(PUSHP)){
          settingState = THERMO_SET;
        }
      }
      break;
    }
    case HYST_SET:
    { 
      if (prevState == HYST)
      {
        dtostrf(setFloorHyst, 2, 0, actualString);
        strcat(actualString, "°C");
        Encoder.writeCounter((int32_t)setFloorHyst);
        Encoder.writeMax((int32_t)10); /* Set the maximum  */
        Encoder.writeMin((int32_t) 1); /* Set the minimum threshold */
        Encoder.writeStep((int32_t)1); 
        prevState = HYST_SET;
      }
      rotaryPosition = Encoder.readCounterInt();
      dtostrf(rotaryPosition, 2, 0, setString);
      Encoder.writeLEDB((rotaryPosition-10)*25);
      Encoder.writeLEDR((11-rotaryPosition)*25);
      u8g2.clearBuffer();
      u8g2.drawXBM(0,0,hysteresis_width,hysteresis_height,hysteresis_bits);
      u8g2.setFont(u8g2_font_t0_12_tf);
      u8g2.drawUTF8(0,12,"Set:");
      u8g2.setFont(u8g2_font_helvB18_tf); // choose a suitable font
      u8g2.drawUTF8(0,40,setString);
      u8g2.setFont(u8g2_font_t0_12_tf);
      u8g2.drawUTF8(78,50,"Actual:");
      u8g2.drawUTF8(78,64,actualString);
      u8g2.sendBuffer();
      if (Encoder.updateStatus()) {
        stateEnterTime = millis();
        if (Encoder.readStatus(PUSHP)){
          setFloorHyst = rotaryPosition;
          Blynk.virtualWrite(V11, setFloorHyst);
          leaveMenu = 1;
        }
      }
      break;
    }
    case THERMO_SET:
    {
      static int posCounter=33;
      static bool initSet = 1;
      
      if ((prevState == HYST) && initSet)
      {
        dtostrf(setFloorTemp, 4, 1, actualString);
        strcat(actualString, "°C");
        Encoder.writeCounter((int32_t)setFloorTemp*10);
        Encoder.writeMax((int32_t)360); /* Set the maximum  */
        Encoder.writeMin((int32_t)240); /* Set the minimum threshold */
        Encoder.writeStep((int32_t)10); 
        initSet = 0;
      }
      if ((prevState == CHILD || prevState == SOFA) && initSet)
      {
        dtostrf(setValue, 4, 1, actualString);
        strcat(actualString, "°C");
        Encoder.writeCounter((int32_t)setValue*10);
        Encoder.writeMax((int32_t)255); /* Set the maximum  */
        Encoder.writeMin((int32_t)180); /* Set the minimum threshold */
        Encoder.writeStep((int32_t)5);
        initSet = 0;
      }
      while (posCounter>0)
      {
        posCounter--;
        Draw_Bitmap(posCounter,0,thermometer_width, thermometer_height,thermometer_bits);
      }
      rotaryPosition = Encoder.readCounterInt();
      dtostrf(rotaryPosition/10.0, 4, 1, setString);
      strcat(setString, "°C");
      if (prevState == HYST)
      {
      Encoder.writeLEDR((rotaryPosition-240)*2);
      Encoder.writeLEDB((360-rotaryPosition)*2); 
      }
      else
      {
      Encoder.writeLEDR((rotaryPosition-180)*3);
      Encoder.writeLEDB((255-rotaryPosition)*3);        
      }
      u8g2.clearBuffer();
      u8g2.drawXBM(0,0,thermometer_width,thermometer_height,thermometer_bits);
      u8g2.setFont(u8g2_font_t0_12_tf);
      u8g2.drawUTF8(60,12,"Set:");
      u8g2.setFont(u8g2_font_logisoso18_tf); // choose a suitable font
      u8g2.drawUTF8(60,35,setString);
      u8g2.setFont(u8g2_font_t0_12_tf);
      u8g2.drawUTF8(60,52,"Actual:");
      u8g2.drawUTF8(60,64,actualString);
      u8g2.sendBuffer();
      if (Encoder.updateStatus()) {
        stateEnterTime = millis();
        if (Encoder.readStatus(PUSHP)){
          leaveMenu = 1;
          if (prevState == HYST)
          {
            setFloorTemp = rotaryPosition/10.0;
            Blynk.virtualWrite(V6, setFloorTemp);
          }
          else
          {
            setValue = rotaryPosition/10.0;
            Blynk.virtualWrite(V5, setValue);
          }
        }
      }
      if (leaveMenu)
      {
        posCounter = 33;
        initSet = 1;
      }
      break;
    }
  }
  return leaveMenu;
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
  static byte lastRefresh;
  bool validdate;

  validdate=RefreshDateTime();
  if (lastRefresh == dateTime.second)
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

  Encoder.writeLEDR(0x77);
  Encoder.writeLEDG(0xD7);
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
  lastRefresh == dateTime.second;
}

void ReadTransmitter() 
{
  static float lastvalidtransTemp;

  webclient.begin(host);
  webclient.setConnectTimeout(500);
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
  setFloorHyst = param.asFloat();
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

  timer.setInterval(BUTIMER,ButtonCheck);
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
