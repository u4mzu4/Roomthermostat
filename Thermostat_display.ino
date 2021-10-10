/*
  WT32-SC01 development board (http://www.wireless-tag.com/portfolio/wt32-sc01/)
  BME280 sensor
  InfluxDB database storage
*/

//Includes
#include <Adafruit_BME280.h>
#include <BlynkSimpleEsp32_SSL.h>
#include <FT6236.h>
#include <HTTPClient.h>
#include <icons_wt32.h>
#include <InfluxDbClient.h>
#include <TFT_eSPI.h>
#include <time.h>

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

#define TEST_MODE
#define I2C_SDA 18
#define I2C_SCL 19
#define TOUCH_THRESH 40

#define INT_PIN   39
#define NTPSERVER "hu.pool.ntp.org"
#define TIMEOUT   5000        //5 sec
#define MAINTIMER 60013       //1 min
#define SETTIMEOUT  50000     //50 sec
#define WRITE_PRECISION WritePrecision::S
#define MAX_BATCH_SIZE 6
#define WRITE_BUFFER_SIZE 12
#define NROFTRANSM 2
#define FONT_170  "Logisoso170"
#define FONT_50   "Logisoso50"
#define FONT_30   "Logisoso30"

//Global variables

float waterTemperature;
float actualTemperature;
float actualHumidity;
float actualPressure;
float transData[NROFTRANSM];
float bmeTemperature;
float setValue = 22.5;
float setFloorTemp = 22.5;
float kitchenTemp;
float outsideTemp;

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
TwoWire I2CWire = TwoWire(0);
Adafruit_BME280 bme;
BlynkTimer timer;
WiFiClient wclient;
HTTPClient hclient;
InfluxDBClient influxclient(influxdb_URL, influxdb_ORG, influxdb_BUCKET, influxdb_TOKEN);
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr1 = TFT_eSprite(&tft); //480x190
TFT_eSprite spr2 = TFT_eSprite(&tft); //64x64
TFT_eSprite spr3 = TFT_eSprite(&tft); //280x240
FT6236 touch = FT6236();

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
  /*
  Serial.print("BME temp: ");
  Serial.println(bmeTemperature);
  Serial.print("BME humidity: ");
  Serial.println(actualHumidity);
  Serial.print("BME pressure: ");
  Serial.println(actualPressure);
  */
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


void DrawFloatAsString(float floatToDraw, String unit, int posx, int posy)
{
  tft.drawString(String(floatToDraw, 1) + " " + unit, posx, posy);
}

void TouchCheck() {

  static DISPLAY_SM displayBox = MAIN;
  static char plotString[12];
  static unsigned long stateStartTime;
  static bool radiatorToBeSet;
  static float setOffset = 0.0f;
  static float actualValue;


  switch (displayBox)
  {
    case MAIN:
      {
        if (TouchInRange(0, 394, 64, 480))
        {
          disableMainTask = 1;
          RefreshDateTime();
          InfluxBatchReaderInfo();
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, info_bits, info_width, info_height, TFT_BLUE);
          spr2.pushSprite(394, 246);
          delay(400);
          tft.fillScreen(TFT_BLACK);
          tft.loadFont(FONT_30);
          sprintf(plotString, "%i-%02i-%02i", dateTime.tm_year + 1900, dateTime.tm_mon, dateTime.tm_mday);
          tft.drawString(plotString, 10, 10);
          sprintf(plotString, "%02i:%02i:%02i", dateTime.tm_hour, dateTime.tm_min, dateTime.tm_sec);
          tft.drawString(plotString, 300, 10);
          tft.drawString("Livingroom: ", 10, 50);
          DrawFloatAsString(bmeTemperature, "°C", 160, 50);
          tft.drawString("Childroom: ", 10, 90);
          DrawFloatAsString(transData[0], "°C", 160, 90);
          tft.drawString("Kitchen: ", 10, 130);
          DrawFloatAsString(kitchenTemp, "°C", 160, 130);
          tft.drawString("Water: ", 10, 170);
          DrawFloatAsString(waterTemperature, "°C", 160, 170);
          tft.unloadFont();
          tft.loadFont(FONT_50);
          tft.drawString("RH: ", 10, 210);
          DrawFloatAsString(actualHumidity, "%", 160, 210);
          tft.drawString("Press: ", 10, 270);
          DrawFloatAsString(actualPressure, "mbar", 160, 270);
          tft.unloadFont();
          delay(TIMEOUT);
          DrawMainPage(1);
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
          stateStartTime = millis();
          displayBox = SETTINGS;
        }
        break;
      }
    case SETTINGS:
      {
        if (millis() - stateStartTime > SETTIMEOUT)
        {
          DrawMainPage(1);
          displayBox = MAIN;
          disableMainTask = 0;
        }

        if (TouchInRange(80, 0, 160, 159))
        {
          radiatorToBeSet = 0;
          DrawThermo(radiatorToBeSet);
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
          spr1.unloadFont();
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
        if (millis() - stateStartTime > SETTIMEOUT)
        {
          DrawMainPage(1);
          displayBox = MAIN;
          disableMainTask = 0;
        }
        if (radiatorToBeSet)
        {
          actualValue = setValue;
        }
        else
        {
          actualValue = setFloorTemp;
        }
        if (TouchInRange(191, 120, 255, 184))
        {
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, arrow_up_bits, arrow_up_width, arrow_up_height, TFT_RED);
          spr2.pushSprite(120, 40);
          setOffset += 0.5;
          spr3.loadFont(FONT_50);
          spr3.fillSprite(TFT_BLACK);
          spr3.drawString("Actual:", 0, 40);
          spr3.drawString(String(actualValue, 1) + " °C", 130, 40);
          spr3.drawString("New:", 0, 135);
          spr3.setTextColor(TFT_BLACK, TFT_WHITE);
          spr3.drawString(String(actualValue + setOffset, 1) + " °C", 130, 135);
          spr3.setTextColor(TFT_WHITE, TFT_BLACK);
          spr3.pushSprite(210, 0);
          spr3.unloadFont();
          spr2.fillSprite(TFT_BLACK);
          spr2.drawXBitmap(0, 0, arrow_up_bits, arrow_up_width, arrow_up_height, TFT_RED);
          spr2.pushSprite(120, 40);
        }
        if (TouchInRange(80, 120, 144, 184))
        {
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, arrow_down_bits, arrow_down_width, arrow_down_height, TFT_BLUE);
          spr2.pushSprite(120, 200);
          setOffset -= 0.5;
          spr3.loadFont(FONT_50);
          spr3.fillSprite(TFT_BLACK);
          spr3.drawString("Actual:", 0, 40);
          spr3.drawString(String(actualValue, 1) + " °C", 130, 40);
          spr3.drawString("New:", 0, 135);
          spr3.setTextColor(TFT_BLACK, TFT_WHITE);
          spr3.drawString(String(actualValue + setOffset, 1) + " °C", 130, 135);
          spr3.setTextColor(TFT_WHITE, TFT_BLACK);
          spr3.pushSprite(210, 0);
          spr3.unloadFont();
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
          DrawMainPage(1);
          displayBox = MAIN;
          disableMainTask = 0;
        }
        if (TouchInRange(0, 410, 64, 474))
        {
          if (radiatorToBeSet)
          {
            setValue = actualValue + setOffset;
            //Blynk.virtualWrite(V2, setValue);
          }
          else
          {
            setFloorTemp = actualValue + setOffset;
            //Blynk.virtualWrite(V3, setFloorTemp);
          }
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, check_bits, check_width, check_height, TFT_GREEN);
          spr2.pushSprite(410, 250);
          delay(400);
          DrawMainPage(1);
          displayBox = MAIN;
          disableMainTask = 0;
        }
        break;
      }
    case SELECT_BASE:
      {
        if (millis() - stateStartTime > SETTIMEOUT)
        {
          DrawMainPage(1);
          displayBox = MAIN;
          disableMainTask = 0;
        }
        if (TouchInRange(65, 25, 255, 215))
        {
          setControlBase = 2;
          //Blynk.virtualWrite(V0, setControlBase);
          spr1.fillSprite(TFT_BLACK);
          spr1.drawXBitmap(25, 0, bunkbed_bits, bunkbed_width, bunkbed_height, TFT_WHITE);
          spr1.drawXBitmap(265, 30, sofa_bits, sofa_width, sofa_height, TFT_RED);
          spr1.pushSprite(0, 65);
          radiatorToBeSet = 1;
          DrawThermo(radiatorToBeSet);
          displayBox = THERMO_SET;
        }
        if (TouchInRange(65, 265, 255, 455))
        {
          setControlBase = 1;
          //Blynk.virtualWrite(V0, setControlBase);
          spr1.fillSprite(TFT_BLACK);
          spr1.drawXBitmap(25, 0, bunkbed_bits, bunkbed_width, bunkbed_height, TFT_BROWN);
          spr1.drawXBitmap(265, 30, sofa_bits, sofa_width, sofa_height, TFT_WHITE);
          spr1.pushSprite(0, 65);
          radiatorToBeSet = 1;
          DrawThermo(radiatorToBeSet);
          displayBox = THERMO_SET;
        }
        break;
      }
    case HOLIDAY:
      {
        if (millis() - stateStartTime > SETTIMEOUT)
        {
          DrawMainPage(1);
          displayBox = MAIN;
          disableMainTask = 0;
        }
        if (TouchInRange(0, 280, 64, 344))
        {
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, cancel_bits, cancel_width, cancel_height, TFT_RED);
          spr2.pushSprite(280, 250);
          delay(400);
          DrawMainPage(1);
          displayBox = MAIN;
          disableMainTask = 0;
        }
        if (TouchInRange(0, 410, 64, 474))
        {
          setValue = 20.0;
          setFloorTemp = 20.0;
          //Blynk.virtualWrite(V2, setValue);
          //Blynk.virtualWrite(V3, setFloorTemp);
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, check_bits, check_width, check_height, TFT_GREEN);
          spr2.pushSprite(410, 250);
          delay(400);
          DrawMainPage(1);
          displayBox = MAIN;
          disableMainTask = 0;
        }
        break;
      }
  }
}

void DrawThermo(bool setPoint)
{
  String thermoString;

  if (setPoint)
  {
    thermoString = String(setValue, 1) + " °C";
  }
  else
  {
    thermoString = String(setFloorTemp, 1) + " °C";
  }
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
  spr3.loadFont(FONT_50);
  spr3.fillSprite(TFT_BLACK);
  spr3.drawString("Actual:", 0, 40);
  spr3.drawString(thermoString, 130, 40);
  spr3.drawString("New:", 0, 135);
  spr3.setTextColor(TFT_BLACK, TFT_WHITE);
  spr3.drawString(thermoString, 130, 135);
  spr3.setTextColor(TFT_WHITE, TFT_BLACK);
  spr3.pushSprite(210, 0);
  spr3.unloadFont();
}


void DrawMainPage(bool forcedDraw)
{
  static int lastDrawnTemp;
  static int lastDrawnIcons;
  int actualIcons;

  if (forcedDraw)
  {
    lastDrawnTemp = -273.15;
    lastDrawnIcons = 8;
  }
  actualIcons = (int)radiatorON + 2 * (int)floorON + 4 * (int)flameON;
  if ((round(actualTemperature * 10.0) == lastDrawnTemp) && (lastDrawnIcons == actualIcons))
  {
    return;
  }
  else
  {
    lastDrawnIcons = actualIcons;
    lastDrawnTemp = round(actualTemperature * 10.0);
  }

  tft.fillScreen(TFT_BLACK);

  spr1.loadFont(FONT_170);
  spr1.fillSprite(TFT_BLACK);
  spr1.drawString(String(actualTemperature, 1) + " °C", 20, 20);
  spr1.pushSprite(0, 20);
  spr1.unloadFont();
  spr2.fillSprite(TFT_BLACK);
  if (radiatorON)
  {
    spr2.drawXBitmap(0, 0, radiator_bits, radiator_width, radiator_height, TFT_WHITE);
  }
  spr2.pushSprite(10, 246);
  spr2.fillSprite(TFT_BLACK);
  if (floorON)
  {
    spr2.drawXBitmap(0, 0, floor_bits, floor_width, floor_height, TFT_WHITE);
  }
  spr2.pushSprite(202, 246);
  spr2.fillSprite(TFT_BLACK);
  if (flameON)
  {
    spr2.drawXBitmap(0, 0, flame_bits, flame_width, flame_height, TFT_RED);
  }
  spr2.pushSprite(106, 246);
  spr2.fillSprite(TFT_BLACK);
  spr2.drawXBitmap(0, 0, settings_bits, settings_width, settings_height, TFT_GOLD);
  spr2.pushSprite(298, 246);
  spr2.fillSprite(TFT_BLACK);
  spr2.drawXBitmap(0, 0, info_bits, info_width, info_height, TFT_BLUE);
  spr2.pushSprite(394, 246);
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

void MainTask()
{
  unsigned long tic = millis();
  static unsigned int mintask = 4000;
  static unsigned int maxtask = 0;
  unsigned int tasktime;

  if (!disableMainTask)
  {
    ReadBME280();
    InfluxBatchReaderMain();
    DrawMainPage(0);
    InfluxBatchWriter();

    tasktime = millis() - tic;
    if (tasktime > maxtask)
    {
      maxtask = tasktime;
    }
    if (tasktime < mintask)
    {
      mintask = tasktime;
    }
  }
}

void IRAM_ATTR HandleTouchISR()
{
  displayTouched = 1;
}

void InfluxBatchReaderMain() {
  String query1 = "from(bucket: \"thermo_data\") |> range(start: -1m, stop:now()) |> filter(fn: (r) => r[\"_measurement\"] == \"thermostat\" and r[\"_field\"] == \"actualTemperature\") |> last()";
  String query2 = "from(bucket: \"thermo_data\") |> range(start: -1m, stop:now()) |> filter(fn: (r) => r[\"_measurement\"] == \"thermostat\" and r[\"_field\"] == \"flameON\") |> last()";
  String query3 = "from(bucket: \"thermo_data\") |> range(start: -1m, stop:now()) |> filter(fn: (r) => r[\"_measurement\"] == \"thermostat\" and r[\"_field\"] == \"floorON\") |> last()";
  String query4 = "from(bucket: \"thermo_data\") |> range(start: -1m, stop:now()) |> filter(fn: (r) => r[\"_measurement\"] == \"thermostat\" and r[\"_field\"] == \"radiatorON\") |> last()";
  
  FluxQueryResult result = influxclient.query(query1);
  while (result.next()) {
    actualTemperature = result.getValueByName("_value").getDouble();
  }
  result.close();
  result = influxclient.query(query2);
  while (result.next()) {
    flameON = (bool)(result.getValueByName("_value").getDouble());
  }
  result.close();
  result = influxclient.query(query3);
  while (result.next()) {
    floorON = (bool)(result.getValueByName("_value").getDouble());
  }
  result.close();
  result = influxclient.query(query4);
  while (result.next()) {
    radiatorON = (bool)(result.getValueByName("_value").getDouble());
  }
  result.close();
}

void InfluxBatchReaderInfo() {
  String query1 = "from(bucket: \"thermo_data\") |> range(start: -1m, stop:now()) |> filter(fn: (r) => r[\"_measurement\"] == \"thermostat\" and r[\"_field\"] == \"kitchenTemp\") |> last()";
  String query2 = "from(bucket: \"thermo_data\") |> range(start: -1m, stop:now()) |> filter(fn: (r) => r[\"_measurement\"] == \"thermostat\" and r[\"_field\"] == \"childRoomTemp\") |> last()";
  String query3 = "from(bucket: \"thermo_data\") |> range(start: -1m, stop:now()) |> filter(fn: (r) => r[\"_measurement\"] == \"thermostat\" and r[\"_field\"] == \"waterTemperature\") |> last()";
  
  FluxQueryResult result = influxclient.query(query1);
  while (result.next()) {
    kitchenTemp = result.getValueByName("_value").getDouble();
  }
  result.close();
  result = influxclient.query(query2);
  while (result.next()) {
    transData[0] = result.getValueByName("_value").getDouble();
  }
  result.close();
  result = influxclient.query(query3);
  while (result.next()) {
    waterTemperature = result.getValueByName("_value").getDouble();
  }
  result.close();
}

void InfluxBatchWriter() {

  unsigned long tnow;
  float setControlBase_f = (float)setControlBase;

  String influxDataType[MAX_BATCH_SIZE] = {"meas", "meas", "meas", "set", "set", "set"};
  String influxDataUnit[MAX_BATCH_SIZE] = {"%", "mbar", "Celsius", "Celsius", "Celsius", "bool"};
  String influxFieldName[MAX_BATCH_SIZE] = {"actualHumidity", "actualPressure", "bmeTemperature", "setValue", "setFloorTemp", "setControlBase"};
  float* influxFieldValue[MAX_BATCH_SIZE] = {&actualHumidity, &actualPressure, &bmeTemperature, &setValue, &setFloorTemp, &setControlBase_f};

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
  pinMode(INT_PIN, INPUT);
  attachInterrupt(INT_PIN, HandleTouchISR, FALLING);
  timer.setInterval(MAINTIMER, MainTask);

  I2CWire.begin(I2C_SDA, I2C_SCL, 400000);
  delay(100);
  bme.begin(BME280_ADDRESS_ALTERNATE, &I2CWire);
  delay(100);
  bme.setSampling(Adafruit_BME280::MODE_FORCED,  // mode
                  Adafruit_BME280::SAMPLING_X16, // temperature
                  Adafruit_BME280::SAMPLING_X1,  // pressure
                  Adafruit_BME280::SAMPLING_X1,  // humidity
                  Adafruit_BME280::FILTER_X16);  // filtering
  delay(100);
  touch.begin(TOUCH_THRESH, I2C_SDA, I2C_SCL);
  delay(100);
  ledcSetup(0, 5000, 3);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, 2);
  delay(100);
  SPIFFS.begin();
  tft.begin();
  tft.setRotation(1);
  spr1.createSprite(480, 190);
  spr2.createSprite(64, 64);
  spr3.createSprite(280, 240);
  spr1.setColorDepth(16);
  spr1.setTextColor(TFT_WHITE, TFT_BLACK);
  spr3.setColorDepth(16);
  spr3.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);

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
  }
  MainTask();
}

void loop() {
    if (displayTouched)
    {
    TouchCheck();
    displayTouched = 0;
    }
  timer.run();
}
