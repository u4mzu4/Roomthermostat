/*
  ESP32-S3-Wroom-1-N8R2 based development board (own design)
  DS18B20 sensors
  OpenTherm boiler interface (included into the devboard)
  OpenTherm library (https://github.com/ihormelnyk/opentherm_library/)
*/

//Includes
#include <DallasTemperature.h>
#include <HTTPClient.h>
#include <icons_wt32.h>
#include <OpenTherm.h>
#include <Ticker.h>
#include <WiFi.h>

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
#define TIMEOUT 5000          //5 sec
#define AFTERCIRCTIME 360000  //6 min
#define MAINTIMER 60013       //1 min
#define OTTIMER 997           //1 sec
#define HYSTERESIS 0.1
#define DS18B20_RESOLUTION 11
#define DS18B20_DEFREG 0x2A80
#define RADIATOR_TEMP 60.0
#define FLOOR_TEMP 40.0
#define MAXWATERTEMP 36.0

#define DEBUG_PRINT 1  // SET TO 0 OUT TO REMOVE TRACES

#if DEBUG_PRINT
#define D_SerialBegin(...) Serial.begin(__VA_ARGS__);
#define D_print(...) Serial.print(__VA_ARGS__)
#define D_println(...) Serial.println(__VA_ARGS__)
#else
#define D_SerialBegin(...)
#define D_print(...)
#define D_println(...)
#endif

//Global variables
const float transmOffset[NROFTRANSM] = { 8.0, -2.0, 0.0 };

float waterTemperature;
float actualTemperature = 22.5;
float transData[NROFTRANSM];
float setValue = 22.5;
float setFloorTemp = 22.5;
float kitchenTemp;
float temperatureRequest;

bool boilerON = 0;
bool floorON = 0;
bool radiatorON = 0;
bool flameON = 0;

//Init services
Ticker mainTimer;
Ticker otTimer;
OneWire oneWire(WATERPIN);
DallasTemperature sensor(&oneWire);
DeviceAddress sensorDeviceAddress;
OpenTherm ot(OTPIN_IN, OTPIN_OUT, false);

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
  D_print("Watertemp: ");
  D_println(waterTemperature);
  ErrorManager(DS18B20_ERROR, ds18b20Errorcounter, 5);
}

void ReadTransmitter() {
  HTTPClient hclient;
  static float lastvalidtransTemp[NROFTRANSM];
  static int transmErrorcounter[NROFTRANSM] = { 0, 0 };

  for (int i = 0; i < NROFTRANSM; i++) {
    hclient.begin(host[i]);
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
  if (transmErrorcounter[0] < 8) {
    actualTemperature = transData[0];
  } else {
    actualTemperature = kitchenTemp;
  }
  if (transmErrorcounter[2] < 5) {
    setValue = transData[2];
    setFloorTemp = transData[2];
  } else {
    setValue = 22.5;
    setFloorTemp = 22.5;
  }

  D_print("Kitchentemp: ");
  D_println(kitchenTemp);
  D_print("actualTemp: ");
  D_println(actualTemperature);
  D_print("setValue: ");
  D_println(setValue);
}

void ManageHeating() {
  static unsigned long aftercirc;
  static HEAT_SM heatstate = OFF;
  static HEAT_SM laststate;

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
  GetWaterTemp();
  ReadTransmitter();
  ManageHeating();
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
  D_print("boilerTemp: ");
  D_println(boilerTemp);
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

  if ((errorCounter == 0) && (errorMask & errorID)) {
    errorMask ^= errorID;
    prevErrorMask = errorMask;
  }

  if (errorCounter < errorLimit) {
    return;
  }

  errorMask |= errorID;
  if (errorMask == prevErrorMask) {
    return;
  } else {
    prevErrorMask = errorMask;
  }

  switch (errorID) {
    case DS18B20_ERROR:
      {
        break;
      }
    case TRANSM0_ERROR:
      {
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

void setup() {
  D_SerialBegin(115200)
    delay(100);
  pinMode(RELAYPIN1, OUTPUT);
  pinMode(RELAYPIN2, OUTPUT);
  pinMode(WATERPIN, INPUT);
  digitalWrite(RELAYPIN1, 0);
  digitalWrite(RELAYPIN2, 0);

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
      break;
    }
  }

  ot.begin(handleInterrupt);
  mainTimer.attach_ms(MAINTIMER, MainTask);
  otTimer.attach_ms(OTTIMER, ProcessOpenTherm);
  MainTask();
}

void loop() {
}