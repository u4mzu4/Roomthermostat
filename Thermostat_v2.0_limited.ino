/*
  ESP32-EVB development board (https://www.olimex.com/Products/IoT/ESP32/ESP32-EVB/open-source-hardware)
  BME280 + DS18B20 sensors
  2.42" OLED SSD1309
  I2C rotary (https://github.com/Fattoresaimon/I2CEncoderV2)
  Blynk service
  
*/

//Includes
#include <Adafruit_BME280.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>
#include <NTPtimeESP.h>
#include <BlynkSimpleEsp32_SSL.h>
#include <HTTPClient.h>
#include <i2cEncoderLibV2.h>
#include <icons.h>
#include <OpenTherm.h>

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
  THERMO    = 6,
  THERMO_SET= 8,
  R3S3RV3D  = 9
  };


//Defines
#define RELAYPIN1 32
#define RELAYPIN2 33
#define WATERPIN  17
#define OTPIN_IN 36
#define OTPIN_OUT 4
#define TIMEOUT   5000  //5 sec
#define AFTERCIRCTIME 300000 //5min
#define BUTIMER   61
#define MAINTIMER 60013 //1min
#define HYSTERESIS 0.1
#define DS18B20_RESOLUTION 11
#define DS18B20_DEFREG 0x2A80
#define ENCODER_ADDRESS 0x02
#define RADIATOR_TEMP 60.0
#define FLOOR_TEMP 40.0
#define MAXWATERTEMP 36.0
#define NROFTRANSM 2
#define OFFSET 2.0


//Global variables
float waterTemperature;
float actualTemperature;
float actualHumidity;
float actualPressure;
float transData[NROFTRANSM];
float bmeTemperature;
float setValue = 22.0;
float setFloorTemp = 22.0;
float kitchenTemp;

int setControlBase = 2;
int buttonTime = 0;
bool boilerON = 0;
bool floorON = 0;
bool radiatorON = 0;
bool heatingON = 1;
bool disableMainTask = 0;

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
OpenTherm ot(OTPIN_IN, OTPIN_OUT);

void GetWaterTemp()
{
  unsigned short tempRaw = DS18B20_DEFREG;
  static float lastvalidTemperature;
  static int ds18b20Errorcounter = 0;
  unsigned long DS18B20timeout = millis();
  
  while (tempRaw == DS18B20_DEFREG)
  {
    sensor.requestTemperaturesByAddress(sensorDeviceAddress);
    tempRaw = sensor.getTemp(sensorDeviceAddress);
    if (millis()-DS18B20timeout > TIMEOUT)
    {
      break;
    }
  }
  waterTemperature=tempRaw/128.0;
  if (waterTemperature > 84.0) 
  {
    waterTemperature = lastvalidTemperature;
    ds18b20Errorcounter++;
  }
  else 
  {
    lastvalidTemperature = waterTemperature;
    ds18b20Errorcounter = 0;
  }
  if (ds18b20Errorcounter > 4)
  {
    Encoder.writeLEDR(0xFF);
    terminal.println("DS18B20 error");
    terminal.flush();
  }
  
  Blynk.virtualWrite(V10, waterTemperature);
  //Serial.println(waterTemperature);
}

void ReadBME280()
{
  static float lastvalidTemperature;
  static float lastvalidHumidity;
  static float lastvalidPressure;
  static int bmeErrorcounter = 0;
  
  bmeTemperature = bme.readTemperature();
  actualHumidity = bme.readHumidity();
  actualPressure = bme.readPressure() / 100.0F;
  
  if (bmeTemperature < 1.0 || bmeTemperature > 100.0) {
    bmeTemperature = lastvalidTemperature;
    bmeErrorcounter++;
  }
  else {
    lastvalidTemperature = bmeTemperature;
    bmeErrorcounter = 0;
  }
  if (actualPressure > 1086.0 || actualPressure < 870.0) {
    actualPressure = lastvalidPressure;
  }
  else {
    lastvalidPressure = actualPressure;
  }
  if (actualHumidity > 99.0) {
    actualHumidity = lastvalidHumidity;
  }
  else {
    lastvalidHumidity = actualHumidity;
  }
  if (bmeErrorcounter > 4)
  {
    setControlBase = 2;
    Blynk.virtualWrite(V12, setControlBase);
    Encoder.writeLEDR(0xFF);
    terminal.println("BME280 error");
    terminal.flush();
  }
  
  Blynk.virtualWrite(V14,bmeTemperature);
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
      disableMainTask = 0;
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
      disableMainTask = 1;
      Draw_Info();
      if (millis()-stateStartTime > TIMEOUT)
      {
	Encoder.writeRGBCode(0x000000);
        Encoder.updateStatus();
        Draw_RoomTemp();
        displayBox = MAIN;
      }
      break;
    }
    case SETTING:
    {
      disableMainTask = 1;
      if (Draw_Setting(newSettings))
      {
	Encoder.writeRGBCode(0x000000);
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
  u8g2.setContrast(0);
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

void Draw_Bitmap (int posx,int posy,int width,int height,unsigned char* varname,byte arrows)
{ 
  byte mask_right = B00000001;
  byte mask_left  = B00000010;
  
  u8g2.clearBuffer(); 
  u8g2.drawXBM(posx, posy, width, height, varname);
  if (arrows & mask_right)
  {
    u8g2.drawTriangle(107,16,123,32,107,48);
  }
  if (arrows & mask_left)
  {
    u8g2.drawTriangle(21,16,5,32,21,48);
  }
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
      Draw_Bitmap(32,0,radiator_width, radiator_height,radiator_bits,1);
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
      Draw_Bitmap(32,0,floor_width, floor_height,floor_bits,3);
      if (Encoder.updateStatus()) {
        stateEnterTime = millis();
        if (Encoder.readStatus(RDEC)){
          settingState = RADIATOR;
        }
        if (Encoder.readStatus(RINC)){
          settingState = HOLIDAY;
        }
        if (Encoder.readStatus(PUSHP)){
          prevState = FLOOR;
	        settingState = THERMO;
        }
      }
      break;
    }
    case HOLIDAY:
    {
      Draw_Bitmap(32,0,holiday_width, holiday_height,holiday_bits,2);
      if (Encoder.updateStatus()) {
        stateEnterTime = millis();
        if (Encoder.readStatus(RDEC)){
          settingState = FLOOR;
        }
        if (Encoder.readStatus(PUSHP)){
          setValue = 20.0;
          setFloorTemp = 20.0;
          Blynk.virtualWrite(V5, setValue);
          Blynk.virtualWrite(V6, setFloorTemp);
          leaveMenu = 1;
        }
      }
      break;
    }
    case CHILD:
    {
      Draw_Bitmap(32,0,childroom_width, childroom_height,childroom_bits,1);
      if (Encoder.updateStatus()) {
        stateEnterTime = millis();
        if (Encoder.readStatus(RINC)){
          settingState = SOFA;
        }
        if (Encoder.readStatus(PUSHP)){
          setControlBase = 2;
          Blynk.virtualWrite(V12, setControlBase);
          settingState = THERMO;
        }
      }
      break;
    }
    case SOFA:
    {
      Draw_Bitmap(32,0,sofa_width, sofa_height,sofa_bits,2);
      if (Encoder.updateStatus()) {
        stateEnterTime = millis();
        if (Encoder.readStatus(RDEC)){
          settingState = CHILD;
        }
        if (Encoder.readStatus(PUSHP)){
          setControlBase = 1;
          Blynk.virtualWrite(V12, setControlBase);
          settingState = THERMO;
        }
      }
      break;
    }

    case THERMO:
    {
      Draw_Bitmap(32,0,thermometer_width, thermometer_height,thermometer_bits,0);

      if (Encoder.updateStatus()) {
        stateEnterTime = millis();
        if (Encoder.readStatus(PUSHP)){
          settingState = THERMO_SET;
        }
      }
      break;
    }

    case THERMO_SET:
    {
      static unsigned int posCounter=33;
      static bool initSet = 1;
      
      if (initSet)
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
        Draw_Bitmap(posCounter,0,thermometer_width, thermometer_height,thermometer_bits,0);
      }
      rotaryPosition = Encoder.readCounterInt();
      dtostrf(rotaryPosition/10.0, 4, 1, setString);
      strcat(setString, "°C");
      
      Encoder.writeLEDR((rotaryPosition-180)*3);
      Encoder.writeLEDB((255-rotaryPosition)*3);        
      
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
          if (prevState == FLOOR)
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
  char bme280String[8];
  char transDataString0[8];
  char transDataString1[8];
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
  dtostrf(bmeTemperature, 4, 1, bme280String);
  strcat(bme280String, "°C");
  dtostrf(transData[0], 4, 1, transDataString0);
  strcat(transDataString0, "°C");
  dtostrf(kitchenTemp, 4, 1, transDataString1);
  strcat(transDataString1, "°C");
  dtostrf(waterTemperature, 4, 1, watertempString);
  strcat(watertempString, "°C");

  Encoder.writeRGBCode(0x77D700);
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_t0_12_tf); // choose a suitable font
  u8g2.drawUTF8(0,10,dateChar);  // write date
  u8g2.drawUTF8(65,10,timeChar);  // write time
  u8g2.drawUTF8(0,20,"LivingR:");  
  u8g2.drawUTF8(65,20,bme280String); // write living room temperature
  u8g2.drawUTF8(0,30,"ChildR:");  
  u8g2.drawUTF8(65,30,transDataString0); // write child room temperature
  u8g2.drawUTF8(0,40,"Kitchen:");  
  u8g2.drawUTF8(65,40,transDataString1); // write kitchen temperature
  u8g2.drawUTF8(0,50,"Watertemp:");  
  u8g2.drawUTF8(65,50,watertempString); // write water temperature
  u8g2.sendBuffer();          // transfer internal memory to the display
  lastRefresh == dateTime.second;
}

void ReadTransmitter() 
{
  static float lastvalidtransTemp[NROFTRANSM];
  static int transmErrorcounter[NROFTRANSM] = {0,0};

  for (int i = 0; i <NROFTRANSM; i++) 
  {
    webclient.begin(host[i]);
    webclient.setConnectTimeout(500);
    if(webclient.GET() == HTTP_CODE_OK) 
    {
      transData[i] = webclient.getString().toFloat();
    }
    else
    {
      transData[i] = 0.0;
    }
    webclient.end();
    if ((transData[i] < 10.0)||transData[i] > 84.0)  
    {
      transData[i] = lastvalidtransTemp[i];
      transmErrorcounter[i]++;
    }
    else
    {
      lastvalidtransTemp[i] = transData[i];
      transmErrorcounter[i] = 0;
    }
  }
  if (transmErrorcounter[0] > 4)
  {
    setControlBase = 1;
    Blynk.virtualWrite(V12, setControlBase);
    Encoder.writeLEDR(0xFF);
    terminal.println("Transmitter0 error");
    terminal.flush();
  }
  if (transmErrorcounter[1] > 4)
  {
    Encoder.writeLEDR(0xFF);
    terminal.println("Transmitter1 error");
    terminal.flush();
  }
  kitchenTemp=transData[1]+OFFSET;
  Blynk.virtualWrite(V11, kitchenTemp);
  if (setControlBase == 2)
  {
    actualTemperature = transData[0];
  }
  else
  {
    actualTemperature = bmeTemperature;
  }
  Blynk.virtualWrite(V1, actualTemperature);
  Blynk.virtualWrite(V13, transData[0]);
}

void ManageHeating()
{
  static unsigned long aftercirc;
  static HEAT_SM heatstate = OFF;
  static HEAT_SM laststate;

  if (!heatingON)
  {
    if (laststate = OFF)
    {
      return;
    }
    else
    {
      ProcessOpenTherm(0, 0);
      digitalWrite(RELAYPIN1, 0);
      digitalWrite(RELAYPIN2, 0);
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
      ProcessOpenTherm(0, 0);
      digitalWrite(RELAYPIN1, 0);
      digitalWrite(RELAYPIN2, 0);
      boilerON = 0;
      radiatorON = 0;
      floorON = 0;
      laststate = OFF;
      Blynk.virtualWrite(V7, boilerON);
      Blynk.virtualWrite(V8, floorON);
      Blynk.virtualWrite(V9, radiatorON); 
      break;
    }
    if (actualTemperature < (setValue - HYSTERESIS))
    {
      heatstate = RADIATOR_ON;
      digitalWrite(RELAYPIN2, 1);
      boilerON = 1;
      radiatorON = 1;
      Blynk.virtualWrite(V7, boilerON);
      Blynk.virtualWrite(V9, radiatorON); 
      break;
    }
    if (kitchenTemp < (setFloorTemp - HYSTERESIS))
    {
      heatstate = FLOOR_ON;
      digitalWrite(RELAYPIN1, 1);
      boilerON = 1;
      floorON = 1;
      Blynk.virtualWrite(V7, boilerON);
      Blynk.virtualWrite(V8, floorON);
      break;
    }
    break;
    }
    
    case RADIATOR_ON:
    {
     ProcessOpenTherm(0, 1);
     if (kitchenTemp < (setFloorTemp - HYSTERESIS))
     {
      digitalWrite(RELAYPIN1, 1);
      floorON = 1;
      heatstate = ALL_ON;
      laststate = RADIATOR_ON;
      Blynk.virtualWrite(V8, floorON);
      break;
     }
     if (actualTemperature > (setValue + HYSTERESIS))
     {
      ProcessOpenTherm(0, 0);
      digitalWrite(RELAYPIN1, 1);
      digitalWrite(RELAYPIN2, 0);
      boilerON = 0;
      radiatorON = 0;
      floorON = 1;
      heatstate = PUMPOVERRUN;
      laststate = RADIATOR_ON;
      Blynk.virtualWrite(V7, boilerON);
      Blynk.virtualWrite(V8, floorON);
      Blynk.virtualWrite(V9, radiatorON); 
      break;
     }
     if (waterTemperature > MAXWATERTEMP)
     {
      digitalWrite(RELAYPIN1, 0);
      floorON = 0;
      Blynk.virtualWrite(V8, floorON);
      break;
     }
     if (waterTemperature < (MAXWATERTEMP - 4.0))
     {
      digitalWrite(RELAYPIN1, 1);
      floorON = 1;
      Blynk.virtualWrite(V8, floorON);
      break;
     }
     break;
    }

    case FLOOR_ON:
    {
     ProcessOpenTherm(0, 1);
     if (actualTemperature < (setValue - HYSTERESIS))
     {
      heatstate = ALL_ON;
      digitalWrite(RELAYPIN2, 1);
      radiatorON = 1;
      laststate = FLOOR_ON;
      Blynk.virtualWrite(V9, radiatorON); 
      break;
     }
     if ((kitchenTemp > (setFloorTemp + HYSTERESIS))||(waterTemperature > MAXWATERTEMP))
     {
      ProcessOpenTherm(0, 0);
      boilerON = 0;
      heatstate = PUMPOVERRUN;
      laststate = FLOOR_ON;
      Blynk.virtualWrite(V7, boilerON);
     }
     break;
    }
    
    case ALL_ON:
    {
     ProcessOpenTherm(0, 1);
     if (actualTemperature > (setValue + HYSTERESIS))
     {
      heatstate = FLOOR_ON;
      digitalWrite(RELAYPIN2, 0);
      radiatorON = 0;
      laststate = ALL_ON;
      Blynk.virtualWrite(V9, radiatorON); 
      break;
     }
     if ((kitchenTemp > (setFloorTemp + HYSTERESIS))||(waterTemperature > MAXWATERTEMP))
     {
      digitalWrite(RELAYPIN1, 0);
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
BLYNK_WRITE(V12)
{
  setControlBase = param.asInt();
  MainTask();
}

void MainTask()
{
  unsigned long tic = millis();
  static unsigned int mintask = 4000;
  static unsigned int maxtask = 0;
  unsigned int tasktime;
  
  if (!disableMainTask)
  {
    GetWaterTemp();
    ReadBME280();
    ReadTransmitter();
    ManageHeating();
    Draw_RoomTemp();
    ProcessOpenTherm(1, 0); //feed OpenTherm
    tasktime=millis()-tic;
    if (tasktime>maxtask)
    {
      maxtask=tasktime;
      terminal.println("Max task time:");
      terminal.println(maxtask);
      terminal.flush();
    }
    if (tasktime<mintask)
    {
      mintask=tasktime;
      terminal.println("Min task time:");
      terminal.println(mintask);
      terminal.flush();
    }
  }
}

void IRAM_ATTR handleInterrupt() {
  ot.handleInterrupt();
}

float CalculateBoilerTemp(HEAT_SM controlState)
{
  float boilerTemp;
  float errorSignal;
  
  if (controlState == FLOOR_ON)
  {
    errorSignal = setFloorTemp + HYSTERESIS - kitchenTemp;
    boilerTemp = 30.0 + errorSignal*50.0;
  }
  else
  {
    errorSignal = setValue + HYSTERESIS - actualTemperature;
    boilerTemp = FLOOR_TEMP + errorSignal*100.0;
  }
  if (boilerTemp > RADIATOR_TEMP)
  {
    boilerTemp = RADIATOR_TEMP;
  }
  if (boilerTemp < 0.0)
  {
    boilerTemp = 0.0;
  }
  terminal.println("Bolier temp:");
  terminal.println(boilerTemp);
  terminal.flush();
}

void ProcessOpenTherm(bool isOnlyFeed, bool tempCalcNeeded)
{
  unsigned long response;
  float boilerTemperature;
  
  if (isOnlyFeed)
  {
    response = ot.setBoilerStatus(1, 1, 0);
  }
  else if (tempCalcNeeded)
  {
    boilerTemperature = CalculateBoilerTemp(heatstate);
    response = ot.sendRequest(buildSetBoilerTemperatureRequest(boilerTemperature));
  }
  else
  {
    response = ot.sendRequest(buildSetBoilerTemperatureRequest(0.0));
  }
  
  if(!ot.isValidResponse(response))
  {
    Encoder.writeLEDR(0xFF);
    terminal.println("OpenTherm error");
    terminal.flush();
  }
}

void setup() {
  pinMode(RELAYPIN1, OUTPUT);
  pinMode(RELAYPIN2, OUTPUT);
  digitalWrite(RELAYPIN1, 0);
  digitalWrite(RELAYPIN2, 0);

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
  
  ot.begin(handleInterrupt);
  
  MainTask();
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
