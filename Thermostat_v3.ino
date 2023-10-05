/*
  ESP32-S3-Wroom-1-N8R2 based development board (own design)
  DS18B20 sensors
  OpenTherm boiler interface (included into the devboard)
  OpenTherm library (https://github.com/ihormelnyk/opentherm_library/)
  InfluxDB database storage
*/

//Includes
#include <Arduino_JSON.h>
#include <SimpleTimer.h>
#include <DallasTemperature.h>
#include <HTTPClient.h>
#include <icons_wt32.h>
#include <InfluxDbClient.h>
#include <OpenTherm.h>
#include <time.h>

//Enums
enum HEAT_SM {
  OFF = 0,
  RADIATOR_ON = 1,
  FLOOR_ON = 2,
  ALL_ON = 3,
  PUMPOVERRUN = 4,
};

enum ERROR_T {
  DS18B20_ERROR = 1,
  BME280_ERROR = 2,
  TRANSM0_ERROR = 4,
  TRANSM1_ERROR = 8,
  OT_ERROR = 16,
  DISPLAY_ERROR = 32
};

//Defines
#define RELAYPIN1 10
#define RELAYPIN2 9
#define WATERPIN 18
#define OTPIN_IN 12
#define OTPIN_OUT 14
#define NTPSERVER "hu.pool.ntp.org"
#define TIMEOUT 5000          //5 sec
#define AFTERCIRCTIME 360000  //6 min
#define MAINTIMER 60013       //1 min
#define OTTIMER 997           //1 sec
#define SETTIMEOUT 50000      //50 sec
#define HYSTERESIS 0.1
#define DS18B20_RESOLUTION 11
#define DS18B20_DEFREG 0x2A80
#define RADIATOR_TEMP 60.0
#define FLOOR_TEMP 40.0
#define MAXWATERTEMP 36.0
#define NROFTRANSM 2
#define WRITE_PRECISION WritePrecision::S
#define MAX_BATCH_SIZE 9
#define WRITE_BUFFER_SIZE 18

//Global variables
const float transmOffset[NROFTRANSM] = { 8.0, -2.0 };

float waterTemperature;
float actualTemperature = 22.5;
float actualHumidity;
float actualPressure;
float transData[NROFTRANSM];
float bmeTemperature = 22.5;
float setValue = 22.5;
float setFloorTemp = 22.5;
float kitchenTemp;
float outsideTemp;
float temperatureRequest;

int setControlBase = 2;

bool boilerON = 0;
bool floorON = 0;
bool radiatorON = 0;
bool heatingON = 1;
bool flameON = 0;
bool failSafe = 0;

struct tm dateTime;

//Init services
SimpleTimer timer;
OneWire oneWire(WATERPIN);
DallasTemperature sensor(&oneWire);
DeviceAddress sensorDeviceAddress;
OpenTherm ot(OTPIN_IN, OTPIN_OUT, false);
WiFiClient wclient;
HTTPClient hclient;
InfluxDBClient influxclient(influxdb_URL, influxdb_ORG, influxdb_BUCKET, influxdb_TOKEN);

void GetWaterTemp() {
  unsigned short tempRaw = DS18B20_DEFREG;
  static float lastvalidTemperature;
  static int ds18b20Errorcounter = 0;
  unsigned long DS18B20timeout = millis();

  while (DS18B20_DEFREG == tempRaw) {
    sensor.requestTemperaturesByAddress(sensorDeviceAddress);
    tempRaw = sensor.getTemp(sensorDeviceAddress);
    if (millis() - DS18B20timeout > TIMEOUT) {
      break;
    }
  }
  waterTemperature = (float)tempRaw / 128.0;
  if (waterTemperature > 84.0) {
    waterTemperature = lastvalidTemperature;
    ds18b20Errorcounter++;
  } else {
    lastvalidTemperature = waterTemperature;
    ds18b20Errorcounter = 0;
  }
  ErrorManager(DS18B20_ERROR, ds18b20Errorcounter, 5);
}

bool RefreshDateTime() {
  bool timeIsValid = getLocalTime(&dateTime);

  if (dateTime.tm_year > 135) {
    return 0;
  }
  return (timeIsValid);
}

void ReadTransmitter() {
  static float lastvalidtransTemp[NROFTRANSM];
  static int transmErrorcounter[NROFTRANSM] = { 0, 0 };

  if (failSafe) {
    actualTemperature = bmeTemperature;
    kitchenTemp = bmeTemperature;
    return;
  }

  for (int i = 0; i < NROFTRANSM; i++) {
    hclient.begin(wclient, host[i]);
    hclient.setConnectTimeout(500);
    if (HTTP_CODE_OK == hclient.GET()) {
      transData[i] = hclient.getString().toFloat();
    } else {
      transData[i] = 0.0;
    }
    hclient.end();
    if ((transData[i] < 10.0) || transData[i] > 84.0) {
      transData[i] = lastvalidtransTemp[i];
      transmErrorcounter[i]++;
    } else {
      transData[i] -= transmOffset[i];
      lastvalidtransTemp[i] = transData[i];
      transmErrorcounter[i] = 0;
    }
  }
  ErrorManager(TRANSM0_ERROR, transmErrorcounter[0], 8);
  ErrorManager(TRANSM1_ERROR, transmErrorcounter[1], 5);
  if (transmErrorcounter[1] < 5) {
    kitchenTemp = transData[1];
  } else {
    kitchenTemp = actualTemperature;
  }
  if (2 == setControlBase) {
    actualTemperature = transData[0];
  } else {
    actualTemperature = bmeTemperature;
  }
}

void ManageHeating() {
  static unsigned long aftercirc;
  static HEAT_SM heatstate = OFF;
  static HEAT_SM laststate;

  if (!heatingON) {
    if (laststate = OFF) {
      return;
    } else {
      temperatureRequest = 0.0;
      digitalWrite(RELAYPIN1, 0);
      digitalWrite(RELAYPIN2, 0);
      boilerON = 0;
      radiatorON = 0;
      floorON = 0;
      laststate = OFF;
      return;
    }
  }

  switch (heatstate) {
    case OFF:
      {
        if (laststate != OFF) {
          temperatureRequest = 0.0;
          digitalWrite(RELAYPIN1, 0);
          digitalWrite(RELAYPIN2, 0);
          boilerON = 0;
          radiatorON = 0;
          floorON = 0;
          laststate = OFF;
          break;
        }
        if (actualTemperature < (setValue - HYSTERESIS)) {
          heatstate = RADIATOR_ON;
          temperatureRequest = CalculateBoilerTemp(heatstate);
          digitalWrite(RELAYPIN2, 1);
          boilerON = 1;
          radiatorON = 1;
          break;
        }
        if (kitchenTemp < (setFloorTemp - HYSTERESIS)) {
          heatstate = FLOOR_ON;
          temperatureRequest = CalculateBoilerTemp(heatstate);
          digitalWrite(RELAYPIN1, 1);
          boilerON = 1;
          floorON = 1;
          break;
        }
        break;
      }
    case RADIATOR_ON:
      {
        temperatureRequest = CalculateBoilerTemp(heatstate);
        if (kitchenTemp < (setFloorTemp - HYSTERESIS)) {
          digitalWrite(RELAYPIN1, 1);
          floorON = 1;
          heatstate = ALL_ON;
          laststate = RADIATOR_ON;
          break;
        }
        if (actualTemperature > (setValue + HYSTERESIS)) {
          temperatureRequest = 0.0;
          digitalWrite(RELAYPIN1, 1);
          digitalWrite(RELAYPIN2, 0);
          boilerON = 0;
          radiatorON = 0;
          floorON = 1;
          heatstate = PUMPOVERRUN;
          laststate = RADIATOR_ON;
          break;
        }
        if (flameON || (waterTemperature > MAXWATERTEMP)) {
          digitalWrite(RELAYPIN1, 0);
          floorON = 0;
          break;
        }
        if (!flameON) {
          digitalWrite(RELAYPIN1, 1);
          floorON = 1;
          break;
        }
        break;
      }
    case FLOOR_ON:
      {
        temperatureRequest = CalculateBoilerTemp(heatstate);
        if (actualTemperature < (setValue - HYSTERESIS)) {
          heatstate = ALL_ON;
          digitalWrite(RELAYPIN2, 1);
          radiatorON = 1;
          laststate = FLOOR_ON;
          break;
        }
        if ((kitchenTemp > (setFloorTemp + HYSTERESIS)) || (waterTemperature > MAXWATERTEMP)) {
          temperatureRequest = 0.0;
          boilerON = 0;
          heatstate = PUMPOVERRUN;
          laststate = FLOOR_ON;
        }
        break;
      }
    case ALL_ON:
      {
        temperatureRequest = CalculateBoilerTemp(heatstate);
        if (actualTemperature > (setValue + HYSTERESIS)) {
          heatstate = FLOOR_ON;
          digitalWrite(RELAYPIN2, 0);
          radiatorON = 0;
          laststate = ALL_ON;
          break;
        }
        if ((kitchenTemp > (setFloorTemp + HYSTERESIS)) || (waterTemperature > MAXWATERTEMP)) {
          digitalWrite(RELAYPIN1, 0);
          floorON = 0;
          heatstate = RADIATOR_ON;
          laststate = ALL_ON;
          break;
        }
        break;
      }
    case PUMPOVERRUN:
      {
        if (laststate != PUMPOVERRUN) {
          aftercirc = millis();
          laststate = PUMPOVERRUN;
          break;
        }
        if (millis() - aftercirc > AFTERCIRCTIME) {
          heatstate = OFF;
          laststate = PUMPOVERRUN;
          break;
        }
        break;
      }
  }
}

void MainTask() {
  unsigned long tic = millis();
  static unsigned int mintask = 4000;
  static unsigned int maxtask = 0;
  unsigned int tasktime;

  GetWaterTemp();
  ReadTransmitter();
  ManageHeating();
  OpenWeatherRead();
  InfluxBatchReader();
  InfluxBatchWriter();

  tasktime = millis() - tic;
  if (tasktime > maxtask) {
    maxtask = tasktime;
  }
  if (tasktime < mintask) {
    mintask = tasktime;
  }
}

void IRAM_ATTR handleInterrupt() {
  ot.handleInterrupt();
}

float CalculateBoilerTemp(HEAT_SM controlState) {
  float boilerTemp;
  float errorSignal;

  if (FLOOR_ON == controlState) {
    errorSignal = setFloorTemp + HYSTERESIS - kitchenTemp;
    boilerTemp = 30.0 + errorSignal * 50.0;
  } else {
    errorSignal = setValue + HYSTERESIS - actualTemperature;
    boilerTemp = FLOOR_TEMP + errorSignal * 100.0;
  }
  if (boilerTemp > RADIATOR_TEMP) {
    boilerTemp = RADIATOR_TEMP;
  }
  if (boilerTemp < 0.0) {
    boilerTemp = 0.0;
  }
  return boilerTemp;
}

void ProcessOpenTherm() {
  unsigned long request;
  unsigned long response;
  int otErrorCounter = 0;

  if (!boilerON) {
    request = ot.buildSetBoilerStatusRequest(0, 1, 0, 0, 0);
    flameON = 0;
  } else {
    request = ot.buildSetBoilerStatusRequest(1, 1, 0, 0, 0);
    response = ot.sendRequest(request);
    flameON = ot.isFlameOn(response);
    request = ot.buildSetBoilerTemperatureRequest(temperatureRequest);
  }
  response = ot.sendRequest(request);

  while (!ot.isValidResponse(response)) {
    otErrorCounter++;
    response = ot.sendRequest(request);
    ErrorManager(OT_ERROR, otErrorCounter, 5);
    if (otErrorCounter >= 5) {
      return;
    }
  }
  ErrorManager(OT_ERROR, 0, 5);
}

void ErrorManager(ERROR_T errorID, int errorCounter, int errorLimit) {
  static byte errorMask = B00000000;
  static byte prevErrorMask = B00000000;
  static unsigned long errorStart;
  static int prevControlBase;
  char errorTime[21];

  if ((errorCounter == 0) && (errorMask & errorID)) {
    errorMask ^= errorID;
    prevErrorMask = errorMask;
    if (!errorMask) {
      if (prevControlBase > 0) {
        setControlBase = prevControlBase;
      }
    }
  }

  if (errorCounter < errorLimit) {
    return;
  }
  if (!errorMask) {
    errorStart = millis();
  }
  errorMask |= errorID;
  if (errorMask == prevErrorMask) {
    return;
  } else {
    prevErrorMask = errorMask;
  }
  if (RefreshDateTime()) {
    sprintf(errorTime, "%i-%02i-%02i %02i:%02i:%02i", dateTime.tm_year + 1900, dateTime.tm_mon, dateTime.tm_mday, dateTime.tm_hour, dateTime.tm_min, dateTime.tm_sec);
  }
  switch (errorID) {
    case DS18B20_ERROR:
      {
        break;
      }
    case BME280_ERROR:
      {
        prevControlBase = setControlBase;
        setControlBase = 2;
        break;
      }
    case TRANSM0_ERROR:
      {
        prevControlBase = setControlBase;
        setControlBase = 1;
        break;
      }
    case TRANSM1_ERROR:
      {
        break;
      }
    case OT_ERROR:
      {
        break;
      }
    case DISPLAY_ERROR:
      {
        break;
      }
    default:
      {
        break;
      }
  }
}

void InfluxBatchReader() {
  String query1 = "from(bucket: \"thermo_data\") |> range(start: -1m, stop:now()) |> filter(fn: (r) => r[\"_measurement\"] == \"thermostat\" and r[\"_field\"] == \"bmeTemperature\") |> last()";
  String query2 = "from(bucket: \"thermo_data\") |> range(start: -1m, stop:now()) |> filter(fn: (r) => r[\"_measurement\"] == \"thermostat\" and r[\"_field\"] == \"setValue\") |> last()";
  String query3 = "from(bucket: \"thermo_data\") |> range(start: -1m, stop:now()) |> filter(fn: (r) => r[\"_measurement\"] == \"thermostat\" and r[\"_field\"] == \"setFloorTemp\") |> last()";
  String query4 = "from(bucket: \"thermo_data\") |> range(start: -1m, stop:now()) |> filter(fn: (r) => r[\"_measurement\"] == \"thermostat\" and r[\"_field\"] == \"setControlBase\") |> last()";

  FluxQueryResult result = influxclient.query(query1);
  while (result.next()) {
    bmeTemperature = result.getValueByName("_value").getDouble();
  }
  result.close();
  result = influxclient.query(query2);
  while (result.next()) {
    setValue = result.getValueByName("_value").getDouble();
  }
  result.close();
  result = influxclient.query(query3);
  while (result.next()) {
    setFloorTemp = result.getValueByName("_value").getDouble();
  }
  result.close();
  result = influxclient.query(query4);
  while (result.next()) {
    setControlBase = (int)(result.getValueByName("_value").getDouble());
  }
  result.close();
}

void InfluxBatchWriter() {

  unsigned long tnow;
  float boilerON_f = (float)boilerON;
  float floorON_f = (float)floorON;
  float radiatorON_f = (float)radiatorON;
  float flameON_f = (float)flameON;
  float setControlBase_f = (float)setControlBase;

  String influxDataType[MAX_BATCH_SIZE] = { "meas", "meas", "meas", "meas", "meas", "status", "status", "status", "status" };
  String influxDataUnit[MAX_BATCH_SIZE] = { "Celsius", "Celsius", "Celsius", "Celsius", "Celsius", "bool", "bool", "bool", "bool" };
  String influxFieldName[MAX_BATCH_SIZE] = { "waterTemperature", "actualTemperature", "kitchenTemp", "childRoomTemp", "outsideTemp", "boilerON", "floorON", "radiatorON", "flameON" };
  float* influxFieldValue[MAX_BATCH_SIZE] = { &waterTemperature, &actualTemperature, &kitchenTemp, &transData[0], &outsideTemp, &boilerON_f, &floorON_f, &radiatorON_f, &flameON_f };

  if (influxclient.isBufferEmpty()) {
    tnow = GetEpochTime();
    for (int i = 0; i < MAX_BATCH_SIZE; i++) {
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
  if (HTTP_CODE_OK == hclient.GET()) {
    jsonBuffer = hclient.getString();
    myJSONObject = JSON.parse(jsonBuffer);
    outsideTemp = (float)(double)(myJSONObject["main"]["temp"]);
    outsideTemp -= 273.15;
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
  pinMode(RELAYPIN1, OUTPUT);
  pinMode(RELAYPIN2, OUTPUT);
  pinMode(WATERPIN, INPUT);
  digitalWrite(RELAYPIN1, 0);
  digitalWrite(RELAYPIN2, 0);

  timer.setInterval(MAINTIMER, MainTask);
  timer.setInterval(OTTIMER, ProcessOpenTherm);

  sensor.begin();
  delay(100);
  sensor.getAddress(sensorDeviceAddress, 0);
  delay(100);
  sensor.setResolution(sensorDeviceAddress, DS18B20_RESOLUTION);
  delay(100);

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
    configTime(3600, 3600, NTPSERVER);
    influxclient.setWriteOptions(WriteOptions().writePrecision(WRITE_PRECISION).batchSize(MAX_BATCH_SIZE).bufferSize(WRITE_BUFFER_SIZE));
    influxclient.validateConnection();
  }

  ot.begin(handleInterrupt);
  MainTask();
}

void loop() {

  timer.run();
}
