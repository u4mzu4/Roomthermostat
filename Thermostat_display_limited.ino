/*
  WT32-SC01 development board (http://www.wireless-tag.com/portfolio/wt32-sc01/)
  BME280 sensor
*/

//Includes
#include <Adafruit_BME280.h>
#include <ESPAsyncWebServer.h>
#include <FT6236.h>
#include <icons_wt32.h>
#include <TFT_eSPI.h>
#include <Ticker.h>
#include <time.h>

enum DISPLAY_SM {
  INIT = 0,
  MAIN = 1,
  INFO = 2,
  SETTINGS = 3,
  THERMO_SET = 4,
  SELECT_BASE = 5,
  HOLIDAY = 6,
  RSRVD = 9
};

//Defines

#define I2C_SDA 18
#define I2C_SCL 19
#define TOUCH_THRESH 40
#define INT_PIN 39
#define NTPSERVER "hu.pool.ntp.org"
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"
#define TIMEOUT 5000      //5 sec
#define MAINTIMER 60013   //1 min
#define SETTIMEOUT 50000  //50 sec
#define FONT_170 "Logisoso170"
#define FONT_50 "Logisoso50"
#define FONT_30 "Logisoso30"
#define PORT 80  //HTTP

//Global variables

float actualTemperature;
float actualHumidity;
float actualPressure;
float bmeTemperature;
float setValue = 22.5;

const float BME_offset = 4.73;

bool disableMainTask = 0;
bool failSafe = 0;
bool displayTouched;

struct tm dateTime;

String message = String(setValue, 1);

//Init services
TwoWire I2CWire = TwoWire(0);
Adafruit_BME280 bme;
Ticker mainTimer;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr1 = TFT_eSprite(&tft);  //480x190
TFT_eSprite spr2 = TFT_eSprite(&tft);  //64x64
TFT_eSprite spr3 = TFT_eSprite(&tft);  //280x240
FT6236 touch = FT6236();
AsyncWebServer server(PORT);

void ReadBME280() {
  static float lastvalidTemperature;
  static float lastvalidHumidity;
  static float lastvalidPressure;
  static int bmeErrorcounter = 0;

  bme.takeForcedMeasurement();
  bmeTemperature = bme.readTemperature() - BME_offset;
  actualHumidity = bme.readHumidity();
  actualPressure = bme.readPressure() / 100.0F;

  if (bmeTemperature < 1.0 || bmeTemperature > 100.0) {
    bmeTemperature = lastvalidTemperature;
    bmeErrorcounter++;
  } else {
    lastvalidTemperature = bmeTemperature;
    actualTemperature = bmeTemperature;
    bmeErrorcounter = 0;
  }
  if (actualPressure > 1086.0 || actualPressure < 870.0) {
    actualPressure = lastvalidPressure;
  } else {
    lastvalidPressure = actualPressure;
  }
  if (actualHumidity > 99.0) {
    actualHumidity = lastvalidHumidity;
  } else {
    lastvalidHumidity = actualHumidity;
  }
}

bool RefreshDateTime() {
  bool timeIsValid = getLocalTime(&dateTime);

  if (dateTime.tm_year > 135) {
    return 0;
  }
  return (timeIsValid);
}


void DrawFloatAsString(float floatToDraw, String unit, int posx, int posy) {
  tft.drawString(String(floatToDraw, 1) + " " + unit, posx, posy);
}

void TouchCheck() {

  static DISPLAY_SM displayBox = MAIN;
  static char plotString[12];
  static unsigned long stateStartTime;
  static float setOffset = 0.0f;
  static float actualValue;


  switch (displayBox) {
    case MAIN:
      {
        if (TouchInRange(0, 394, 64, 480)) {
          disableMainTask = 1;
          RefreshDateTime();
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
          tft.drawString("Nappali: ", 10, 50);
          DrawFloatAsString(bmeTemperature, " °C", 160, 50);
          tft.unloadFont();
          tft.loadFont(FONT_50);
          tft.drawString("Para: ", 10, 210);
          DrawFloatAsString(actualHumidity, "%", 160, 210);
          tft.drawString("Nyomas: ", 10, 270);
          DrawFloatAsString(actualPressure, "mbar", 160, 270);
          tft.unloadFont();
          delay(TIMEOUT);
          DrawMainPage(1);
          disableMainTask = 0;
        }
        if (TouchInRange(0, 298, 64, 384)) {
          disableMainTask = 1;
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, settings_bits, settings_width, settings_height, TFT_GOLD);
          spr2.pushSprite(298, 246);
          delay(400);
          tft.fillScreen(TFT_BLACK);
          spr1.fillSprite(TFT_BLACK);
          DrawThermo();
          spr1.pushSprite(0, 80);
          stateStartTime = millis();
          displayBox = THERMO_SET;
        }
        break;
      }
    case THERMO_SET:
      {
        if (millis() - stateStartTime > SETTIMEOUT) {
          DrawMainPage(1);
          displayBox = MAIN;
          disableMainTask = 0;
        }
        actualValue = setValue;
        if (TouchInRange(191, 120, 255, 184)) {
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, arrow_up_bits, arrow_up_width, arrow_up_height, TFT_RED);
          spr2.pushSprite(120, 40);
          setOffset += 0.5;
          spr3.loadFont(FONT_50);
          spr3.fillSprite(TFT_BLACK);
          spr3.drawString("Actual:", 0, 40);
          spr3.drawString(String(actualValue, 1) + "  °C", 130, 40);
          spr3.drawString("New:", 0, 135);
          spr3.setTextColor(TFT_BLACK, TFT_WHITE);
          spr3.drawString(String(actualValue + setOffset, 1) + "  °C", 130, 135);
          spr3.setTextColor(TFT_WHITE, TFT_BLACK);
          spr3.pushSprite(210, 0);
          spr3.unloadFont();
          spr2.fillSprite(TFT_BLACK);
          spr2.drawXBitmap(0, 0, arrow_up_bits, arrow_up_width, arrow_up_height, TFT_RED);
          spr2.pushSprite(120, 40);
        }
        if (TouchInRange(80, 120, 144, 184)) {
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, arrow_down_bits, arrow_down_width, arrow_down_height, TFT_BLUE);
          spr2.pushSprite(120, 200);
          setOffset -= 0.5;
          spr3.loadFont(FONT_50);
          spr3.fillSprite(TFT_BLACK);
          spr3.drawString("Actual:", 0, 40);
          spr3.drawString(String(actualValue, 1) + "  °C", 130, 40);
          spr3.drawString("New:", 0, 135);
          spr3.setTextColor(TFT_BLACK, TFT_WHITE);
          spr3.drawString(String(actualValue + setOffset, 1) + "  °C", 130, 135);
          spr3.setTextColor(TFT_WHITE, TFT_BLACK);
          spr3.pushSprite(210, 0);
          spr3.unloadFont();
          spr2.fillSprite(TFT_BLACK);
          spr2.drawXBitmap(0, 0, arrow_down_bits, arrow_down_width, arrow_down_height, TFT_BLUE);
          spr2.pushSprite(120, 200);
        }
        if (TouchInRange(0, 280, 64, 344)) {
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, cancel_bits, cancel_width, cancel_height, TFT_RED);
          spr2.pushSprite(280, 250);
          delay(400);
          DrawMainPage(1);
          displayBox = MAIN;
          disableMainTask = 0;
        }
        if (TouchInRange(0, 410, 64, 474)) {
          setValue = actualValue + setOffset;
          if (setValue < 18.0) {
            setValue = 18.0;
          }
          if (setValue > 25.5) {
            setValue = 25.5;
          }
          message = String(setValue, 1);
          setOffset = 0.0f;
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
    case HOLIDAY:
      {
        if (millis() - stateStartTime > SETTIMEOUT) {
          DrawMainPage(1);
          displayBox = MAIN;
          disableMainTask = 0;
        }
        if (TouchInRange(0, 280, 64, 344)) {
          spr2.fillSprite(TFT_WHITE);
          spr2.drawXBitmap(0, 0, cancel_bits, cancel_width, cancel_height, TFT_RED);
          spr2.pushSprite(280, 250);
          delay(400);
          DrawMainPage(1);
          displayBox = MAIN;
          disableMainTask = 0;
        }
        if (TouchInRange(0, 410, 64, 474)) {
          setValue = 20.0f;
          message = "20.0";
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

void DrawThermo() {
  String thermoString;

  thermoString = String(setValue, 1) + "  °C";
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


void DrawMainPage(bool forcedDraw) {
  static int lastDrawnTemp;
  int actualIcons;

  if (forcedDraw) {
    lastDrawnTemp = -273;
  }
  if (round(actualTemperature * 10.0) == lastDrawnTemp) {
    return;
  } else {
    lastDrawnTemp = round(actualTemperature * 10.0);
  }

  tft.fillScreen(TFT_BLACK);
  spr1.loadFont(FONT_170);
  spr1.fillSprite(TFT_BLACK);
  spr1.drawString(String(actualTemperature, 1) + "  °C", 20, 20);
  spr1.pushSprite(0, 20);
  spr1.unloadFont();
  spr2.fillSprite(TFT_BLACK);
  spr2.drawXBitmap(0, 0, settings_bits, settings_width, settings_height, TFT_GOLD);
  spr2.pushSprite(298, 246);
  spr2.fillSprite(TFT_BLACK);
  spr2.drawXBitmap(0, 0, info_bits, info_width, info_height, TFT_BLUE);
  spr2.pushSprite(394, 246);
}

bool TouchInRange(uint16_t xmin, uint16_t ymin, uint16_t xmax, uint16_t ymax) {
  bool touchInRange = false;

  TS_Point p = touch.getPoint();
  if ((p.x >= xmin) && (p.y >= ymin)) {
    if ((p.x <= xmax) && (p.y <= ymax)) {
      touchInRange = true;
    }
  }
  return touchInRange;
}

void MainTask() {
  if (!disableMainTask) {
    ReadBME280();
    DrawMainPage(0);
  }
}

void IRAM_ATTR HandleTouchISR() {
  displayTouched = 1;
}

void setup() {
  pinMode(INT_PIN, INPUT);
  attachInterrupt(INT_PIN, HandleTouchISR, FALLING);

  I2CWire.begin(I2C_SDA, I2C_SCL, 400000);
  delay(100);
  bme.begin(BME280_ADDRESS_ALTERNATE, &I2CWire);
  delay(100);
  bme.setSampling(Adafruit_BME280::MODE_FORCED,   // mode
                  Adafruit_BME280::SAMPLING_X16,  // temperature
                  Adafruit_BME280::SAMPLING_X1,   // pressure
                  Adafruit_BME280::SAMPLING_X1,   // humidity
                  Adafruit_BME280::FILTER_X16);   // filtering
  delay(100);
  touch.begin(TOUCH_THRESH, I2C_SDA, I2C_SCL);
  delay(100);
  ledcAttach(TFT_BL, 5000, 3);
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
  WiFi.begin(ssid, password);

  // Wait for connection
  unsigned long wifitimeout = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (millis() - wifitimeout > TIMEOUT) {
      failSafe = 1;
      break;
    }
  }
  if (!failSafe) {
    configTzTime(TIMEZONE, NTPSERVER);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", message);
    });
    server.begin();
  }
  MainTask();
  mainTimer.attach_ms(MAINTIMER, MainTask);
}

void loop() {
  if (displayTouched) {
    TouchCheck();
    displayTouched = 0;
  }
}