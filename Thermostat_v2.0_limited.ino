/*
  Lolin D32 Pro development board (https://docs.wemos.cc/en/latest/d32/d32_pro.html)
  BME280 + DS18B20 sensors
  2.42" OLED SSD1309
  I2C rotary (https://github.com/Fattoresaimon/I2CEncoderV2)
  OpenTherm boiler interface (http://ihormelnyk.com/opentherm_adapter)
  OpenTherm library (https://github.com/ihormelnyk/opentherm_library/)
  Blynk service
  InfluxDB database storage
*/

//Includes
#include <Adafruit_BME280.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>
#include <BlynkSimpleEsp32_SSL.h>
#include <HTTPClient.h>
#include <i2cEncoderLibV2.h>
#include <icons.h>
#include <OpenTherm.h>
#include <InfluxDbClient.h>
#include <Arduino_JSON.h>
#include <time.h>

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
#define OTPIN_IN  12
#define OTPIN_OUT 14
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
#define WRITE_PRECISION WritePrecision::S
#define MAX_BATCH_SIZE 15
#define WRITE_BUFFER_SIZE 30

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

int setControlBase = 2;

bool boilerON = 0;
bool floorON = 0;
bool radiatorON = 0;
bool heatingON = 1;
bool disableMainTask = 0;
bool flameON = 0;
bool failSafe = 0;
struct tm dateTime;

//Init services
Adafruit_BME280 bme;
U8G2_SSD1309_128X64_NONAME2_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL, SDA);
BlynkTimer timer;
WidgetTerminal terminal(V19);
OneWire oneWire(WATERPIN);
DallasTemperature sensor(&oneWire);
DeviceAddress sensorDeviceAddress;
i2cEncoderLibV2 Encoder(ENCODER_ADDRESS);
OpenTherm ot(OTPIN_IN, OTPIN_OUT, false);
WiFiClient wclient;
HTTPClient hclient;
InfluxDBClient influxclient(influxdb_URL, influxdb_ORG, influxdb_BUCKET, influxdb_TOKEN);

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
          if (Encoder.readStatus(i2cEncoderLibV2::PUSHD)) {
            displayBox = SETTING;
          }
          else if (Encoder.readStatus(i2cEncoderLibV2::PUSHP)) {
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
          if (Encoder.readStatus(i2cEncoderLibV2::PUSHP)) {
            stateEnterTime = millis();
            settingState = CHILD;
          }
          if (Encoder.readStatus(i2cEncoderLibV2::RINC)) {
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
          if (Encoder.readStatus(i2cEncoderLibV2::RDEC)) {
            settingState = RADIATOR;
          }
          if (Encoder.readStatus(i2cEncoderLibV2::RINC)) {
            settingState = HOLIDAY;
          }
          if (Encoder.readStatus(i2cEncoderLibV2::PUSHP)) {
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
          if (Encoder.readStatus(i2cEncoderLibV2::RDEC)) {
            settingState = FLOOR;
          }
          if (Encoder.readStatus(i2cEncoderLibV2::PUSHP)) {
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
          if (Encoder.readStatus(i2cEncoderLibV2::PUSHP)) {
            stateEnterTime = millis();
            setControlBase = 2;
            Blynk.virtualWrite(V12, setControlBase);
            settingState = THERMO_SET;
            prevState = CHILD;
          }
          if (Encoder.readStatus(i2cEncoderLibV2::RINC)) {
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
          if (Encoder.readStatus(i2cEncoderLibV2::RDEC)) {
            settingState = CHILD;
          }
          if (Encoder.readStatus(i2cEncoderLibV2::PUSHP)) {
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
          if (Encoder.readStatus(i2cEncoderLibV2::PUSHP)) {
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
  bool timeIsValid = getLocalTime(&dateTime);

  if (dateTime.tm_year > 135)
  {
    return 0;
  }
  return (timeIsValid);
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
  if (lastRefresh == dateTime.tm_sec)
  {
    return;
  }

  if (validdate)
  {
    sprintf(dateChar, "%i-%02i-%02i", dateTime.tm_year + 1900, dateTime.tm_mon, dateTime.tm_mday);
    sprintf(timeChar, "%02i:%02i:%02i", dateTime.tm_hour, dateTime.tm_min, dateTime.tm_sec);
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
  lastRefresh = dateTime.tm_sec;
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
    OpenWeatherRead();
    InfluxBatchWriter();
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
  Encoder.begin(i2cEncoderLibV2::INT_DATA | i2cEncoderLibV2::WRAP_DISABLE | i2cEncoderLibV2::DIRE_RIGHT | i2cEncoderLibV2::IPUP_DISABLE | i2cEncoderLibV2::RMOD_X1 | i2cEncoderLibV2::RGB_ENCODER);
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
