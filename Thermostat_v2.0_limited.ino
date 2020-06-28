/*
  ESP32-EVB development board (https://www.olimex.com/Products/IoT/ESP32/ESP32-EVB/open-source-hardware)
  BME280 + DS18B20 sensors
  2.42" OLED SSD1309
  I2C rotary (https://github.com/Fattoresaimon/I2CEncoderV2)
  OpenTherm boiler interface (http://ihormelnyk.com/opentherm_adapter)
  OpenTherm library (https://github.com/ihormelnyk/opentherm_library/)
  Blynk service
  ESP Async webserver if Blynk not available (https://github.com/me-no-dev/ESPAsyncWebServer)
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
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

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
};

enum SETTING_SM {
  RADIATOR  = 0,
  FLOOR     = 1,
  HOLIDAY   = 2,
  SOFA      = 3,
  CHILD     = 4,
  THERMO_SET = 5,
};

enum ERROR_T {
  DS18B20_ERROR = 1,
  BME280_ERROR  = 2,
  TRANSM0_ERROR = 4,
  TRANSM1_ERROR = 8,
  OT_ERROR      = 16,
  ENCODER_ERROR = 32
};

//Defines
#define RELAYPIN1 32
#define RELAYPIN2 33
#define WATERPIN  18
#define OTPIN_IN 12
#define OTPIN_OUT 14
#define SERVERPORT 80
#define NTPSERVER "hu.pool.ntp.org"
#define TIMEOUT   5000  //5 sec
#define AFTERCIRCTIME 300000 //5min
#define BUTIMER   61
#define MAINTIMER 60013 //1min
#define OTTIMER 997 //1sec
#define HYSTERESIS 0.1
#define DS18B20_RESOLUTION 11
#define DS18B20_DEFREG 0x2A80
#define ENCODER_ADDRESS 0x02
#define RADIATOR_TEMP 60.0
#define FLOOR_TEMP 40.0
#define MAXWATERTEMP 36.0
#define NROFTRANSM 2


//Global variables
const float transmOffset[NROFTRANSM] = {8.0, -2.0};

float waterTemperature;
float actualTemperature;
float actualHumidity;
float actualPressure;
float transData[NROFTRANSM];
float bmeTemperature;
float setValue = 22.0;
float setFloorTemp = 22.0;
float kitchenTemp;
float temperatureRequest;

int setControlBase = 2;

bool boilerON = 0;
bool floorON = 0;
bool radiatorON = 0;
bool heatingON = 1;
bool disableMainTask = 0;
bool flameON = 0;
bool failSafe = 0;

//Init services
strDateTime dateTime;
Adafruit_BME280 bme;
U8G2_SSD1309_128X64_NONAME2_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL, SDA);
NTPtime NTPhu(NTPSERVER);   // Choose server pool as required
BlynkTimer timer;
WidgetTerminal terminal(V19);
OneWire oneWire(WATERPIN);
DallasTemperature sensor(&oneWire);
DeviceAddress sensorDeviceAddress;
i2cEncoderLibV2 Encoder(ENCODER_ADDRESS);
OpenTherm ot(OTPIN_IN, OTPIN_OUT);
AsyncWebServer server(SERVERPORT);
WiFiClient wclient;
HTTPClient hclient;

void GetWaterTemp()
{
  unsigned short tempRaw = DS18B20_DEFREG;
  static float lastvalidTemperature;
  static int ds18b20Errorcounter = 0;
  unsigned long DS18B20timeout = millis();

  while (DS18B20_DEFREG == tempRaw)
  {
    sensor.requestTemperaturesByAddress(sensorDeviceAddress);
    tempRaw = sensor.getTemp(sensorDeviceAddress);
    if (millis() - DS18B20timeout > TIMEOUT)
    {
      break;
    }
  }
  waterTemperature = tempRaw / 128.0;
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
  ErrorManager(DS18B20_ERROR, ds18b20Errorcounter, 5);
  Blynk.virtualWrite(V10, waterTemperature);
}

void ReadBME280()
{
  static float lastvalidTemperature;
  static float lastvalidHumidity;
  static float lastvalidPressure;
  static int bmeErrorcounter = 0;

  bme.takeForcedMeasurement();
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
  ErrorManager(BME280_ERROR, bmeErrorcounter, 5);
  Blynk.virtualWrite(V14, bmeTemperature);
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
          if (Encoder.readStatus(PUSHD)) {
            displayBox = SETTING;
          }
          else if (Encoder.readStatus(PUSHP)) {
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
        if (millis() - stateStartTime > TIMEOUT)
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
    case FAILED:
      {
        if (!Encoder.updateStatus()) {
          displayBox = INIT;
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
  u8g2.setContrast(0);
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_logisoso34_tf); // choose a suitable font
  u8g2.drawUTF8(8, 42, temperatureString); // write temperature
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

void Draw_Bitmap (int posx, int posy, int width, int height, unsigned char* varname, byte arrows)
{
  byte mask_right = B00000001;
  byte mask_left  = B00000010;

  u8g2.clearBuffer();
  u8g2.drawXBM(posx, posy, width, height, varname);
  if (arrows & mask_right)
  {
    u8g2.drawTriangle(107, 16, 123, 32, 107, 48);
  }
  if (arrows & mask_left)
  {
    u8g2.drawTriangle(21, 16, 5, 32, 21, 48);
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
    prevState = RADIATOR;
    stateEnterTime = millis();
  }
  if (millis() - stateEnterTime > 2 * TIMEOUT)
  {
    leaveMenu = 1;
  }
  switch (settingState)
  {
    case RADIATOR:
      {
        Draw_Bitmap(32, 0, radiator_width, radiator_height, radiator_bits, 1);
        if (Encoder.updateStatus()) {
          if (Encoder.readStatus(PUSHP)) {
            stateEnterTime = millis();
            settingState = CHILD;
          }
          if (Encoder.readStatus(RINC)) {
            stateEnterTime = millis();
            settingState = FLOOR;
          }
        }
        break;
      }
    case FLOOR:
      {
        Draw_Bitmap(32, 0, floor_width, floor_height, floor_bits, 3);
        if (Encoder.updateStatus()) {
          stateEnterTime = millis();
          if (Encoder.readStatus(RDEC)) {
            settingState = RADIATOR;
          }
          if (Encoder.readStatus(RINC)) {
            settingState = HOLIDAY;
          }
          if (Encoder.readStatus(PUSHP)) {
            prevState = FLOOR;
            settingState = THERMO_SET;
          }
        }
        break;
      }
    case HOLIDAY:
      {
        Draw_Bitmap(32, 0, holiday_width, holiday_height, holiday_bits, 2);
        if (Encoder.updateStatus()) {
          stateEnterTime = millis();
          if (Encoder.readStatus(RDEC)) {
            settingState = FLOOR;
          }
          if (Encoder.readStatus(PUSHP)) {
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
        Draw_Bitmap(32, 0, childroom_width, childroom_height, childroom_bits, 1);
        if (Encoder.updateStatus()) {
          if (Encoder.readStatus(PUSHP)) {
            stateEnterTime = millis();
            setControlBase = 2;
            Blynk.virtualWrite(V12, setControlBase);
            settingState = THERMO_SET;
            prevState = CHILD;
          }
          if (Encoder.readStatus(RINC)) {
            stateEnterTime = millis();
            settingState = SOFA;
          }
        }
        break;
      }
    case SOFA:
      {
        Draw_Bitmap(32, 0, sofa_width, sofa_height, sofa_bits, 2);
        if (Encoder.updateStatus()) {
          stateEnterTime = millis();
          if (Encoder.readStatus(RDEC)) {
            settingState = CHILD;
          }
          if (Encoder.readStatus(PUSHP)) {
            setControlBase = 1;
            Blynk.virtualWrite(V12, setControlBase);
            settingState = THERMO_SET;
            prevState = SOFA;
          }
        }
        break;
      }
    case THERMO_SET:
      {
        static unsigned int posCounter = 33;
        static bool initSet = 1;

        if (initSet)
        {
          if (FLOOR == prevState)
          {
            dtostrf(setFloorTemp, 4, 1, actualString);
            Encoder.writeCounter((int32_t)(setFloorTemp * 10));
          }
          else
          {
            dtostrf(setValue, 4, 1, actualString);
            Encoder.writeCounter((int32_t)(setValue * 10));
          }
          Encoder.writeMax((int32_t)255); /* Set the maximum  */
          Encoder.writeMin((int32_t)180); /* Set the minimum threshold */
          Encoder.writeStep((int32_t)5);
          strcat(actualString, "°C");
          initSet = 0;
        }
        while (posCounter > 0)
        {
          posCounter--;
          Draw_Bitmap(posCounter, 0, thermometer_width, thermometer_height, thermometer_bits, 0);
        }
        rotaryPosition = Encoder.readCounterInt();
        dtostrf(rotaryPosition / 10.0, 4, 1, setString);
        strcat(setString, "°C");

        Encoder.writeLEDR((rotaryPosition - 180) * 3);
        Encoder.writeLEDB((255 - rotaryPosition) * 3);

        u8g2.clearBuffer();
        u8g2.drawXBM(0, 0, thermometer_width, thermometer_height, thermometer_bits);
        u8g2.setFont(u8g2_font_t0_12_tf);
        u8g2.drawUTF8(60, 12, "Set:");
        u8g2.setFont(u8g2_font_logisoso18_tf); // choose a suitable font
        u8g2.drawUTF8(60, 35, setString);
        u8g2.setFont(u8g2_font_t0_12_tf);
        u8g2.drawUTF8(60, 52, "Current:");
        u8g2.drawUTF8(60, 64, actualString);
        u8g2.sendBuffer();
        if (Encoder.updateStatus()) {
          stateEnterTime = millis();
          if (Encoder.readStatus(PUSHP)) {
            if (FLOOR == prevState)
            {
              setFloorTemp = rotaryPosition / 10.0;
              Blynk.virtualWrite(V6, setFloorTemp);
            }
            else
            {
              setValue = rotaryPosition / 10.0;
              Blynk.virtualWrite(V5, setValue);
            }
            leaveMenu = 1;
          }
        }
        if (leaveMenu)
        {
          posCounter = 33;
          initSet = 1;
          MainTask();
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
  return (dateTime.valid);
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

  validdate = RefreshDateTime();
  if (lastRefresh == dateTime.second)
  {
    return;
  }

  if (validdate)
  {
    sprintf(dateChar, "%i-%02i-%02i", dateTime.year, dateTime.month, dateTime.day);
    sprintf(timeChar, "%02i:%02i:%02i", dateTime.hour, dateTime.minute, dateTime.second);
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
  u8g2.drawUTF8(0, 10, dateChar); // write date
  u8g2.drawUTF8(65, 10, timeChar); // write time
  u8g2.drawUTF8(0, 20, "LivingR:");
  u8g2.drawUTF8(65, 20, bme280String); // write living room temperature
  u8g2.drawUTF8(0, 30, "ChildR:");
  u8g2.drawUTF8(65, 30, transDataString0); // write child room temperature
  u8g2.drawUTF8(0, 40, "Kitchen:");
  u8g2.drawUTF8(65, 40, transDataString1); // write kitchen temperature
  u8g2.drawUTF8(0, 50, "Watertemp:");
  u8g2.drawUTF8(65, 50, watertempString); // write water temperature
  u8g2.sendBuffer();          // transfer internal memory to the display
  lastRefresh = dateTime.second;
}

void ReadTransmitter()
{
  static float lastvalidtransTemp[NROFTRANSM];
  static int transmErrorcounter[NROFTRANSM] = {0, 0};

  if (failSafe)
  {
    actualTemperature = bmeTemperature;
    kitchenTemp = bmeTemperature;
    return;
  }

  for (int i = 0; i < NROFTRANSM; i++)
  {
    hclient.begin(wclient, host[i]);
    hclient.setConnectTimeout(500);
    if (HTTP_CODE_OK == hclient.GET())
    {
      transData[i] = hclient.getString().toFloat();
    }
    else
    {
      transData[i] = 0.0;
    }
    hclient.end();
    if ((transData[i] < 10.0) || transData[i] > 84.0)
    {
      transData[i] = lastvalidtransTemp[i];
      transmErrorcounter[i]++;
    }
    else
    {
      transData[i] -= transmOffset[i];
      lastvalidtransTemp[i] = transData[i];
      transmErrorcounter[i] = 0;
    }
  }
  ErrorManager(TRANSM0_ERROR, transmErrorcounter[0], 8);
  ErrorManager(TRANSM1_ERROR, transmErrorcounter[1], 5);
  if (transmErrorcounter[1] < 5)
  {
    kitchenTemp = transData[1];
  }
  else
  {
    kitchenTemp = actualTemperature;
  }
  Blynk.virtualWrite(V11, kitchenTemp);
  if (2 == setControlBase)
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
      temperatureRequest = 0.0;
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
        if (laststate != OFF)
        {
          temperatureRequest = 0.0;
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
          temperatureRequest = CalculateBoilerTemp(heatstate);
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
          temperatureRequest = CalculateBoilerTemp(heatstate);
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
        temperatureRequest = CalculateBoilerTemp(heatstate);
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
          temperatureRequest = 0.0;
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
        if (flameON || (waterTemperature > MAXWATERTEMP))
        {
          digitalWrite(RELAYPIN1, 0);
          floorON = 0;
          Blynk.virtualWrite(V8, floorON);
          break;
        }
        if (!flameON)
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
        temperatureRequest = CalculateBoilerTemp(heatstate);
        if (actualTemperature < (setValue - HYSTERESIS))
        {
          heatstate = ALL_ON;
          digitalWrite(RELAYPIN2, 1);
          radiatorON = 1;
          laststate = FLOOR_ON;
          Blynk.virtualWrite(V9, radiatorON);
          break;
        }
        if ((kitchenTemp > (setFloorTemp + HYSTERESIS)) || (waterTemperature > MAXWATERTEMP))
        {
          temperatureRequest = 0.0;
          boilerON = 0;
          heatstate = PUMPOVERRUN;
          laststate = FLOOR_ON;
          Blynk.virtualWrite(V7, boilerON);
        }
        break;
      }
    case ALL_ON:
      {
        temperatureRequest = CalculateBoilerTemp(heatstate);
        if (actualTemperature > (setValue + HYSTERESIS))
        {
          heatstate = FLOOR_ON;
          digitalWrite(RELAYPIN2, 0);
          radiatorON = 0;
          laststate = ALL_ON;
          Blynk.virtualWrite(V9, radiatorON);
          break;
        }
        if ((kitchenTemp > (setFloorTemp + HYSTERESIS)) || (waterTemperature > MAXWATERTEMP))
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
        if (laststate != PUMPOVERRUN)
        {
          aftercirc = millis();
          laststate = PUMPOVERRUN;
          break;
        }
        if (millis() - aftercirc > AFTERCIRCTIME)
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
    EncoderDiag();
    tasktime = millis() - tic;
    if (tasktime > maxtask)
    {
      maxtask = tasktime;
      terminal.print("Max task time: ");
      terminal.println(maxtask);
      terminal.flush();
    }
    if (tasktime < mintask)
    {
      mintask = tasktime;
      terminal.print("Min task time: ");
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

  if (FLOOR_ON == controlState)
  {
    errorSignal = setFloorTemp + HYSTERESIS - kitchenTemp;
    boilerTemp = 30.0 + errorSignal * 50.0;
  }
  else
  {
    errorSignal = setValue + HYSTERESIS - actualTemperature;
    boilerTemp = FLOOR_TEMP + errorSignal * 100.0;
  }
  if (boilerTemp > RADIATOR_TEMP)
  {
    boilerTemp = RADIATOR_TEMP;
  }
  if (boilerTemp < 0.0)
  {
    boilerTemp = 0.0;
  }
}

void ProcessOpenTherm()
{
  unsigned long request;
  unsigned long response;
  int otErrorCounter = 0;

  if (!boilerON)
  {
    request = ot.buildSetBoilerStatusRequest(0, 1, 0, 0, 0);
  }
  else
  {
    request = ot.buildSetBoilerStatusRequest(1, 1, 0, 0, 0);
    response = ot.sendRequest(request);
    flameON = ot.isFlameOn(response);
    request = ot.buildSetBoilerTemperatureRequest(temperatureRequest);
  }
  response = ot.sendRequest(request);

  while (!ot.isValidResponse(response))
  {
    otErrorCounter++;
    response = ot.sendRequest(request);
    ErrorManager(OT_ERROR, otErrorCounter, 5);
    if (otErrorCounter >= 5)
    {
      terminal.print("Request: ");
      terminal.println(request);
      terminal.print("Response: ");
      terminal.println(response);
      terminal.print("Status: ");
      terminal.println(ot.getLastResponseStatus());
      terminal.flush();
      break;
    }
  }
}

void ErrorManager(ERROR_T errorID, int errorCounter, int errorLimit)
{
  static byte errorMask = B00000000;
  static byte prevErrorMask = B00000000;
  static unsigned long errorStart;
  static int prevControlBase;
  char errorTime[21];

  if ((errorCounter == 0) && (errorMask & errorID))
  {
    errorMask ^= errorID;
    prevErrorMask = errorMask;
    terminal.print("Error ID");
    terminal.print(errorID);
    terminal.print(" cleared after ");
    terminal.print((millis() - errorStart) / 1000);
    terminal.println(" seconds");
    terminal.flush();
    if (!errorMask)
    {
      Encoder.writeRGBCode(0x000000);
      if (prevControlBase > 0)
      {
        setControlBase = prevControlBase;
        Blynk.virtualWrite(V12, setControlBase);
      }
    }
  }

  if (errorCounter < errorLimit)
  {
    return;
  }
  Encoder.writeLEDR(0xFF);
  if (!errorMask)
  {
    errorStart = millis();
  }
  errorMask |= errorID;
  if (errorMask == prevErrorMask)
  {
    return;
  }
  else
  {
    prevErrorMask = errorMask;
  }
  if (RefreshDateTime())
  {
    sprintf(errorTime, "%i-%02i-%02i %02i:%02i:%02i", dateTime.year, dateTime.month, dateTime.day, dateTime.hour, dateTime.minute, dateTime.second);
    terminal.println(errorTime);
  }
  switch (errorID)
  {
    case DS18B20_ERROR:
      {
        terminal.println("DS18B20 error");
        break;
      }
    case BME280_ERROR:
      {
        prevControlBase = setControlBase;
        setControlBase = 2;
        Blynk.virtualWrite(V12, setControlBase);
        terminal.println("BME280 error");
        break;
      }
    case TRANSM0_ERROR:
      {
        prevControlBase = setControlBase;
        setControlBase = 1;
        Blynk.virtualWrite(V12, setControlBase);
        terminal.println("Transmitter0 error");
        break;
      }
    case TRANSM1_ERROR:
      {
        terminal.println("Transmitter1 error");
        break;
      }
    case OT_ERROR:
      {
        terminal.println("OpenTherm error");
        break;
      }
    case ENCODER_ERROR:
      {
        terminal.println("Encoder error");
        terminal.print("Counter: ");
        terminal.println(Encoder.readCounterInt());
        break;
      }
    default:
      {
        break;
      }
  }
  terminal.flush();
}

String processor(const String& var)
{
  String tempvar = var;
  tempvar.remove(2);
  int numberofPH = tempvar.toInt();
  switch (numberofPH) {
    case 1:
      return (String(actualHumidity, 0) + " %%");
      break;
    case 2:
      return (String(actualPressure, 0) + " mbar");
      break;
    case 3:
      return (String(actualTemperature, 1) + " °C");
      break;
    case 4:
      if (radiatorON)
      {
        return String("on");
      }
      else
      {
        return String("off");
      }
      break;
    case 5:
      if (boilerON)
      {
        return String("on");
      }
      else
      {
        return String("off");
      }
      break;
    case 6:
      if (floorON)
      {
        return String("on");
      }
      else
      {
        return String("off");
      }
      break;
    case 7:
      return (String(bmeTemperature, 1) + " °C");
      break;
    case 8:
      return (String(transData[0], 1) + " °C");
      break;
    case 9:
      return (String(kitchenTemp, 1) + " °C");
      break;
    case 10:
      return (String(waterTemperature, 1) + " °C");
      break;
    case 11:
      return (String(setValue, 1));
      break;
    case 12:
      return (String(setFloorTemp, 1));
      break;
    case 13:
      if (setControlBase == 1)
      {
        return String("checked");
      }
    case 14:
      if (setControlBase == 2)
      {
        return String("checked");
      }
    case 15:
      if (heatingON)
      {
        return String("checked");
      }
    default:
      return String();
      break;
  }
  return String();
}

void SetupWebServer ()
{
  static bool SPIFFSinited = 0;
  if (!SPIFFSinited)
  {
    SPIFFSinited = SPIFFS.begin(true);
  }
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    int paramsNr = request->params();
    if (4 == paramsNr)
    {
      for (int j = 0; j < paramsNr; j++)
      {
        AsyncWebParameter* p = request->getParam(j);
        if (String("p1") == p->name())
        {
          setValue = p->value().toFloat();
          Blynk.virtualWrite(V5, setValue);
        }
        if (String("p2") == p->name())
        {
          heatingON = (bool)(p->value().toInt());
          Blynk.virtualWrite(V2, heatingON);
        }
        if (String("p3") == p->name())
        {
          setFloorTemp  = p->value().toFloat();
          Blynk.virtualWrite(V6, setFloorTemp);
        }
        if (String("p4") == p->name())
        {
          setControlBase  = p->value().toInt();
          Blynk.virtualWrite(V12, setControlBase);
        }
      }
    }
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to load circle.css file
  server.on("/circle.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/circle.css", "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/script.js", "text/javascript");
  });
  server.begin();
}

void EncoderDiag()
{
  const int reg1 = 0xAAAAAAAA;
  const int reg2 = 0x55555555;
  static int encErrorcnt;
  int storedCounter;
  int counter2write;

  storedCounter = Encoder.readCounterLong();
  counter2write = ((storedCounter & reg1) < (storedCounter & reg2)) ? reg1 : reg2;
  Encoder.writeCounter(counter2write);
  storedCounter = Encoder.readCounterLong();
  if (storedCounter != counter2write)
  {
    encErrorcnt++;
  }
  else
  {
    encErrorcnt = 0;
  }
  ErrorManager(ENCODER_ERROR, encErrorcnt, 5);
}

void setup() {
  pinMode(RELAYPIN1, OUTPUT);
  pinMode(RELAYPIN2, OUTPUT);
  digitalWrite(RELAYPIN1, 0);
  digitalWrite(RELAYPIN2, 0);

  timer.setInterval(BUTIMER, ButtonCheck);
  timer.setInterval(MAINTIMER, MainTask);
  timer.setInterval(OTTIMER, ProcessOpenTherm);

  Wire.begin(SDA, SCL);
  Wire.setClock(400000);
  delay(100);
  bme.begin(BME280_ADDRESS_ALTERNATE);
  bme.setSampling(Adafruit_BME280::MODE_FORCED,  // mode
                  Adafruit_BME280::SAMPLING_X16, // temperature
                  Adafruit_BME280::SAMPLING_X1,  // pressure
                  Adafruit_BME280::SAMPLING_X1,  // humidity
                  Adafruit_BME280::FILTER_X16);  // filtering
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
  Encoder.writeRGBCode(0xFF0000);
  delay(200);
  Encoder.writeRGBCode(0x00FF00);
  delay(200);
  Encoder.writeRGBCode(0x0000FF);

  WiFi.mode(WIFI_STA);
  WiFi.begin (ssid, password);

  // Wait for connection
  unsigned long wifitimeout = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (millis() - wifitimeout > TIMEOUT)
    {
      failSafe = 1;
      break;
    }
  }

  if (!failSafe)
  {
    Encoder.writeRGBCode(0x000000);
    Blynk.config(auth);
    Blynk.connect();
    Blynk.syncAll();
    Blynk.virtualWrite(V7, boilerON);
    Blynk.virtualWrite(V8, floorON);
    Blynk.virtualWrite(V9, radiatorON);
    terminal.clear();
  }

  ot.begin(handleInterrupt);
  MainTask();
}

void loop() {
  static bool webserverIsRunning = 0;

  if (Blynk.connected())
  {
    if (webserverIsRunning)
    {
      server.end();
      webserverIsRunning = 0;
    }
    Blynk.run();
  }
  else
  {
    Blynk.disconnect();
    delay(1000);
    SetupWebServer();
    webserverIsRunning = 1;
    Blynk.connect();
  }
  timer.run();
}
