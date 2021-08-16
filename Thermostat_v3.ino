/*
  WT32-SC01 development board (http://www.wireless-tag.com/portfolio/wt32-sc01/)
  BME280 + DS18B20 sensors
  OpenTherm boiler interface (http://ihormelnyk.com/opentherm_adapter)
  OpenTherm library (https://github.com/ihormelnyk/opentherm_library/)
  Blynk service
  InfluxDB database storage
*/

//Includes

#include <Adafruit_BME280.h>
#include <Arduino_JSON.h>
#include <BlynkSimpleEsp32_SSL.h>
#include <DallasTemperature.h>
#include <FT6236.h>
#include <HTTPClient.h>
#include <icons_wt32.h>
#include <InfluxDbClient.h>
#include <OpenTherm.h>
#include <TFT_eSPI.h>
#include <time.h>

enum HEAT_SM {
  OFF         = 0,
  RADIATOR_ON = 1,
  FLOOR_ON    = 2,
  ALL_ON      = 3,
  PUMPOVERRUN = 4,
};

enum ERROR_T {
  DS18B20_ERROR = 1,
  BME280_ERROR  = 2,
  TRANSM0_ERROR = 4,
  TRANSM1_ERROR = 8,
  OT_ERROR      = 16,
  DISPLAY_ERROR = 32
};

enum DISPLAY_SM {
  INIT        = 0,
  MAIN        = 1,
  INFO        = 2,
  SETTINGS    = 3,
  THERMO_SET  = 4,
  SELECT_BASE = 5,
  HOLIDAY     = 6,
  RSRVD       = 9
};

//Defines
#define RELAYPIN1 32
#define RELAYPIN2 33
#define WATERPIN  18
#define OTPIN_IN  12
#define OTPIN_OUT 14
#define INT_PIN   39
#define NTPSERVER "hu.pool.ntp.org"
#define TIMEOUT   5000  		//5 sec
#define AFTERCIRCTIME 360000 	//6 min
#define MAINTIMER 60013 		//1 min
#define OTTIMER	997 			//1 sec
#define HYSTERESIS 0.1
#define DS18B20_RESOLUTION 11
#define DS18B20_DEFREG 0x2A80
#define RADIATOR_TEMP 60.0
#define FLOOR_TEMP 40.0
#define MAXWATERTEMP 36.0
#define NROFTRANSM 2
#define WRITE_PRECISION WritePrecision::S
#define MAX_BATCH_SIZE 15
#define WRITE_BUFFER_SIZE 30
#define FONT_170  "Logisoso170"
#define FONT_50   "Logisoso50"
#define FONT_30   "Logisoso30"

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
float outsideTemp;
float temperatureRequest;
float plotC = setValue;

int setControlBase = 2;

bool boilerON = 0;
bool floorON = 0;
bool radiatorON = 0;
bool heatingON = 1;
bool disableMainTask = 0;
bool flameON = 0;
bool failSafe = 0;
bool displayTouched;

struct tm dateTime;

//Init services
Adafruit_BME280 bme;
BlynkTimer timer;
WidgetTerminal terminal(V19);
OneWire oneWire(WATERPIN);
DallasTemperature sensor(&oneWire);
DeviceAddress sensorDeviceAddress;
OpenTherm ot(OTPIN_IN, OTPIN_OUT, false);
WiFiClient wclient;
HTTPClient hclient;
InfluxDBClient influxclient(influxdb_URL, influxdb_ORG, influxdb_BUCKET, influxdb_TOKEN);
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr1 = TFT_eSprite(&tft); //480x190
TFT_eSprite spr2 = TFT_eSprite(&tft); //64x64
TFT_eSprite spr3 = TFT_eSprite(&tft); //280x240
FT6236 touch = FT6236();

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
  waterTemperature = (float)tempRaw / 128.0;
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

bool RefreshDateTime()
{
  bool timeIsValid = getLocalTime(&dateTime);

  if (dateTime.tm_year > 135)
  {
    return 0;
  }
  return (timeIsValid);
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

void TouchCheck() {

  static DISPLAY_SM displayBox = MAIN;
  static unsigned long stateStartTime;
  static unsigned int sliderPos = 160;
  static char plotString[8];

  switch (displayBox)
  {
    case MAIN:
      {
        if (TouchInRange(0, 394, 64, 480))
        {
          disableMainTask = 1;
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, info_bits, info_width, info_height, TFT_BLUE);
          spr2.pushSprite(394, 246);
          delay(400);
          tft.fillScreen(TFT_BLACK);
          tft.loadFont(FONT_30);
          tft.drawString("2021-03-05 17:11:38", 10, 10);
          tft.drawString("Livingroom: ", 10, 50);
          tft.drawString("Childroom: ", 10, 90);
          tft.drawString("Kitchen: ", 10, 130);
          tft.drawString("Water: ", 10, 170);
          tft.drawString("24.8 °C", 160, 50);
          tft.drawString("23.8 °C", 160, 90);
          tft.drawString("22.9 °C", 160, 130);
          tft.drawString("29.7 °C", 160, 170);
          tft.loadFont(FONT_50);
          tft.drawString("RH: ", 10, 210);
          tft.drawString("39 %", 160, 210);
          tft.drawString("Press: ", 10, 270);
          tft.drawString("1005 mbar", 160, 270);
          delay(TIMEOUT);
          DrawMainPage();
          disableMainTask = 0;
        }
        if (TouchInRange(0, 298, 64, 384))
        {
          disableMainTask = 1;
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, settings_bits, settings_width, settings_height, TFT_GOLD);
          spr2.pushSprite(298, 246);
          delay(400);
          tft.fillScreen(TFT_BLACK);
          spr1.fillSprite(TFT_BLACK);
          spr1.drawXBitmap(0, 0, floor_160_bits, floor_160_width, floor_160_height, TFT_CYAN);
          spr1.drawXBitmap(160, 13, radiator_160_bits, radiator_160_width, radiator_160_height, TFT_WHITE);
          spr1.drawXBitmap(320, 0, holiday_bits, holiday_width, holiday_height, TFT_YELLOW);
          spr1.pushSprite(0, 80);
          displayBox = SETTINGS;
        }
        break;
      }
    case SETTINGS:
      {
        if (TouchInRange(80, 0, 160, 159))
        {
          DrawThermo();
          displayBox = THERMO_SET;
        }
        if (TouchInRange(80, 160, 160, 319))
        {
          tft.fillScreen(TFT_BLACK);
          spr1.fillSprite(TFT_BLACK);
          spr1.drawXBitmap(25, 0, bunkbed_bits, bunkbed_width, bunkbed_height, TFT_BROWN);
          spr1.drawXBitmap(265, 30, sofa_bits, sofa_width, sofa_height, TFT_RED);
          spr1.pushSprite(0, 65);
          displayBox = SELECT_BASE;
        }
        if (TouchInRange(80, 320, 160, 479))
        {
          tft.fillScreen(TFT_BLACK);
          spr1.loadFont(FONT_50);
          spr1.fillSprite(TFT_BLACK);
          spr1.drawString("HOLIDAY MODE!", 20, 20);
          spr1.drawString("ARE YOU SURE?", 20, 75);
          spr1.drawXBitmap(260, 30, holiday_bits, holiday_width, holiday_height, TFT_YELLOW);
          spr1.pushSprite(0, 20);
          spr2.fillSprite(TFT_BLACK);
          spr2.drawXBitmap(0, 0, cancel_bits, cancel_width, cancel_height, TFT_RED);
          spr2.pushSprite(280, 250);
          spr2.fillSprite(TFT_BLACK);
          spr2.drawXBitmap(0, 0, check_bits, check_width, check_height, TFT_GREEN);
          spr2.pushSprite(410, 250);
          displayBox = HOLIDAY;
        }
        break;
      }
    case THERMO_SET:
      {
        if (TouchInRange(191, 120, 255, 184))
        {
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, arrow_up_bits, arrow_up_width, arrow_up_height, TFT_RED);
          spr2.pushSprite(120, 40);
          plotC += 0.5;
          spr3.fillSprite(TFT_BLACK);
          spr3.drawString("Actual:", 0, 40);
          spr3.drawString(String(setValue, 1) + " °C", 130, 40);
          spr3.drawString("New:", 0, 135);
          spr3.setTextColor(TFT_BLACK, TFT_WHITE);
          spr3.drawString(String(plotC, 1), 130, 135);
          spr3.setTextColor(TFT_WHITE, TFT_BLACK);
          spr3.pushSprite(210, 0);
          spr2.fillSprite(TFT_BLACK);
          spr2.drawXBitmap(0, 0, arrow_up_bits, arrow_up_width, arrow_up_height, TFT_RED);
          spr2.pushSprite(120, 40);
        }
        if (TouchInRange(80, 120, 144, 184))
        {
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, arrow_down_bits, arrow_down_width, arrow_down_height, TFT_BLUE);
          spr2.pushSprite(120, 200);
          plotC -= 0.5;
          spr3.drawString("Actual:", 0, 40);
          spr3.drawString(String(setValue, 1) + " °C", 130, 40);
          spr3.drawString("New:", 0, 135);
          spr3.setTextColor(TFT_BLACK, TFT_WHITE);
          spr3.drawString(String(plotC, 1), 130, 135);
          spr3.setTextColor(TFT_WHITE, TFT_BLACK);
          spr3.pushSprite(210, 0);
          spr2.fillSprite(TFT_BLACK);
          spr2.drawXBitmap(0, 0, arrow_down_bits, arrow_down_width, arrow_down_height, TFT_BLUE);
          spr2.pushSprite(120, 200);
        }
        if (TouchInRange(0, 280, 64, 344))
        {
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, cancel_bits, cancel_width, cancel_height, TFT_RED);
          spr2.pushSprite(280, 250);
          delay(400);
          DrawMainPage();
          displayBox = MAIN;
          disableMainTask = 0;
        }
        if (TouchInRange(0, 410, 64, 474))
        {
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, check_bits, check_width, check_height, TFT_GREEN);
          spr2.pushSprite(410, 250);
          delay(400);
          DrawMainPage();
          displayBox = MAIN;
          disableMainTask = 0;
        }
        break;
      }
    case SELECT_BASE:
      {
        if (TouchInRange(65, 25, 255, 215))
        {
          spr1.fillSprite(TFT_BLACK);
          spr1.drawXBitmap(25, 0, bunkbed_bits, bunkbed_width, bunkbed_height, TFT_WHITE);
          spr1.drawXBitmap(265, 30, sofa_bits, sofa_width, sofa_height, TFT_RED);
          spr1.pushSprite(0, 65);
          DrawThermo();
          displayBox = THERMO_SET;
        }
        if (TouchInRange(65, 265, 255, 455))
        {
          spr1.fillSprite(TFT_BLACK);
          spr1.drawXBitmap(25, 0, bunkbed_bits, bunkbed_width, bunkbed_height, TFT_BROWN);
          spr1.drawXBitmap(265, 30, sofa_bits, sofa_width, sofa_height, TFT_WHITE);
          spr1.pushSprite(0, 65);
          DrawThermo();
          displayBox = THERMO_SET;
        }
        break;
      }
    case HOLIDAY:
      {
        if (TouchInRange(0, 280, 64, 344))
        {
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, cancel_bits, cancel_width, cancel_height, TFT_RED);
          spr2.pushSprite(280, 250);
          delay(400);
          DrawMainPage();
          displayBox = MAIN;
          disableMainTask = 0;
        }
        if (TouchInRange(0, 410, 64, 474))
        {
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, check_bits, check_width, check_height, TFT_GREEN);
          spr2.pushSprite(410, 250);
          delay(400);
          DrawMainPage();
          displayBox = MAIN;
          disableMainTask = 0;
        }
        break;
      }
  }
}

void DrawThermo()
{
  tft.fillScreen(TFT_BLACK);
  spr1.fillSprite(TFT_BLACK);
  spr1.drawXBitmap(10, 15, thermometer_bits, thermometer_width, thermometer_height, TFT_WHITE);
  spr1.pushSprite(0, 40);
  spr2.fillSprite(TFT_BLACK);
  spr2.drawXBitmap(0, 0, arrow_up_bits, arrow_up_width, arrow_up_height, TFT_RED);
  spr2.pushSprite(120, 40);
  spr2.fillSprite(TFT_BLACK);
  spr2.drawXBitmap(0, 0, arrow_down_bits, arrow_down_width, arrow_down_height, TFT_BLUE);
  spr2.pushSprite(120, 200);
  spr2.fillSprite(TFT_BLACK);
  spr2.drawXBitmap(0, 0, cancel_bits, cancel_width, cancel_height, TFT_RED);
  spr2.pushSprite(280, 250);
  spr2.fillSprite(TFT_BLACK);
  spr2.drawXBitmap(0, 0, check_bits, check_width, check_height, TFT_GREEN);
  spr2.pushSprite(410, 250);
  spr3.fillSprite(TFT_BLACK);
  spr3.drawString("Actual:", 0, 40);
  spr3.drawString(String(setValue, 1) + " °C", 130, 40);
  spr3.drawString("New:", 0, 135);
  spr3.setTextColor(TFT_BLACK, TFT_WHITE);
  spr3.drawString(String(plotC, 1), 130, 135);
  spr3.setTextColor(TFT_WHITE, TFT_BLACK);
  spr3.pushSprite(210, 0);
}

bool TouchInRange(uint16_t xmin, uint16_t ymin, uint16_t xmax, uint16_t ymax)
{
  bool touchInRange = false;

  TS_Point p = touch.getPoint();
  if ((p.x >= xmin) && (p.y >= ymin))
  {
    if ((p.x <= xmax) && (p.y <= ymax))
    {
      touchInRange = true;
    }
  }
  return touchInRange;
}

void DrawMainPage()
{
  tft.fillScreen(TFT_BLACK);

  spr1.loadFont(FONT_170);
  spr1.fillSprite(TFT_BLACK);
  spr1.drawString("22.4 °C", 20, 20);
  spr1.pushSprite(0, 20);

  spr2.fillSprite(TFT_BLACK);
  spr2.drawXBitmap(0, 0, radiator_bits, radiator_width, radiator_height, TFT_WHITE);
  spr2.pushSprite(10, 246);
  spr2.fillSprite(TFT_BLACK);
  spr2.drawXBitmap(0, 0, floor_bits, floor_width, floor_height, TFT_WHITE);
  spr2.pushSprite(202, 246);
  spr2.fillSprite(TFT_BLACK);
  spr2.drawXBitmap(0, 0, flame_bits, flame_width, flame_height, TFT_RED);
  spr2.pushSprite(106, 246);
  spr2.fillSprite(TFT_BLACK);
  spr2.drawXBitmap(0, 0, settings_bits, settings_width, settings_height, TFT_GOLD);
  spr2.pushSprite(298, 246);
  spr2.fillSprite(TFT_BLACK);
  spr2.drawXBitmap(0, 0, info_bits, info_width, info_height, TFT_BLUE);
  spr2.pushSprite(394, 246);
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
    OpenWeatherRead();
    InfluxBatchWriter();
    DrawMainPage();

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


void IRAM_ATTR HandleTouchISR()
{
  displayTouched = 1;
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
  return boilerTemp;
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
      terminal.println("Request: 0x" + String(request, HEX));
      terminal.println("Response: 0x" + String(response, HEX));
      terminal.println("Status: " + String(ot.statusToString(ot.getLastResponseStatus())));
      terminal.flush();
      return;
    }
  }
  ErrorManager(OT_ERROR, 0, 5);
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
    sprintf(errorTime, "%i-%02i-%02i %02i:%02i:%02i", dateTime.tm_year + 1900, dateTime.tm_mon, dateTime.tm_mday, dateTime.tm_hour, dateTime.tm_min, dateTime.tm_sec);
    terminal.println(errorTime);
    terminal.flush();
  }
  else
  {
    terminal.println(String(millis()));
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
    case DISPLAY_ERROR:
      {
        terminal.println("Display error");
        break;
      }
    default:
      {
        break;
      }
  }
  terminal.flush();
}

void InfluxBatchWriter() {

  unsigned long tnow;
  float boilerON_f = (float)boilerON;
  float floorON_f = (float)floorON;
  float radiatorON_f = (float)radiatorON;
  float flameON_f = (float)flameON;
  float setControlBase_f = (float)setControlBase;

  String influxDataType[MAX_BATCH_SIZE] = {"meas", "meas", "meas", "meas", "meas", "meas", "meas", "meas", "set", "set", "set", "status", "status", "status", "status"};
  String influxDataUnit[MAX_BATCH_SIZE] = {"Celsius", "Celsius", "%", "mbar", "Celsius", "Celsius", "Celsius", "Celsius", "Celsius", "Celsius", "bool", "bool", "bool", "bool", "bool"};
  String influxFieldName[MAX_BATCH_SIZE] = {"waterTemperature", "actualTemperature", "actualHumidity", "actualPressure", "bmeTemperature", "kitchenTemp", "childRoomTemp", "outsideTemp", "setValue", "setFloorTemp", "setControlBase", "boilerON", "floorON", "radiatorON", "flameON"};
  float* influxFieldValue[MAX_BATCH_SIZE] = {&waterTemperature, &actualTemperature, &actualHumidity, &actualPressure, &bmeTemperature, &kitchenTemp, &transData[0], &outsideTemp, &setValue, &setFloorTemp, &setControlBase_f, &boilerON_f, &floorON_f, &radiatorON_f, &flameON_f};

  if (influxclient.isBufferEmpty()) {
    tnow = GetEpochTime();
    for (int i = 0; i < MAX_BATCH_SIZE; i++)
    {
      Point influxBatchPoint("thermostat");
      influxBatchPoint.addTag("data_type", influxDataType[i]);
      influxBatchPoint.addTag("data_unit", influxDataUnit[i]);
      influxBatchPoint.addField(influxFieldName[i], *(influxFieldValue[i]));
      influxBatchPoint.setTime(tnow);
      influxclient.writePoint(influxBatchPoint);
    }
    influxclient.flushBuffer();
  }
}

void OpenWeatherRead() {
  JSONVar myJSONObject;
  String jsonBuffer = "{}";

  hclient.begin(wclient, openweatherURL);
  hclient.setConnectTimeout(500);
  if (HTTP_CODE_OK == hclient.GET())
  {
    jsonBuffer = hclient.getString();
    myJSONObject = JSON.parse(jsonBuffer);
    outsideTemp = (float)(double)(myJSONObject["main"]["temp"]);
    outsideTemp -= 273.15;
    Blynk.virtualWrite(V15, outsideTemp);
  }
  hclient.end();
}

unsigned long GetEpochTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return (0);
  }
  time(&now);
  return now;
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAYPIN1, OUTPUT);
  pinMode(RELAYPIN2, OUTPUT);
  pinMode(INT_PIN, INPUT);
  digitalWrite(RELAYPIN1, 0);
  digitalWrite(RELAYPIN2, 0);

  attachInterrupt(INT_PIN, HandleTouchISR, FALLING);

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
  sensor.begin();
  delay(100);
  sensor.getAddress(sensorDeviceAddress, 0);
  delay(100);
  sensor.setResolution(sensorDeviceAddress, DS18B20_RESOLUTION);
  delay(100);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, 127);
  delay(300);

  SPIFFS.begin();
  tft.begin();
  touch.begin(128, 18, 19);
  tft.setRotation(1);
  spr1.createSprite(480, 190);
  spr2.createSprite(64, 64);
  spr3.createSprite(280, 240);
  spr1.setColorDepth(16);
  spr1.loadFont(FONT_170);
  spr1.setTextColor(TFT_WHITE, TFT_BLACK);
  spr3.setColorDepth(16);
  spr3.loadFont(FONT_50);
  spr3.setTextColor(TFT_WHITE, TFT_BLACK);
  DrawMainPage();

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
    configTime(3600, 3600, NTPSERVER);
    influxclient.setWriteOptions(WriteOptions().writePrecision(WRITE_PRECISION).batchSize(MAX_BATCH_SIZE).bufferSize(WRITE_BUFFER_SIZE));
    influxclient.validateConnection();
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
  if (displayTouched)
  {
    TouchCheck();
    displayTouched = 0;
  }
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