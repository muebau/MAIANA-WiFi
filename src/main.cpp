//------------------includes--------------------

//general
#include <Arduino.h>
#include <ArduinoJson.h>

//webserver
#include <WiFi.h>

#include <FS.h>
#include <SPIFFS.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define DEBUG(X)          \
  Serial.print(__LINE__); \
  Serial.print(": ");     \
  Serial.println(X)

//---------------global variables and definitions-------------

//general

struct wifi
{
  String type;
  String ssid;
  String password;
} wifiSettings;

struct protocol
{
  String type;
  int port;
} protocolSettings;

struct station
{
  String mmsi;
  String callsign;
  String vesselname;
  int vesseltype;
  int loa;
  int beam;
  int portoffset;
  int bowoffset;
} stationSettings;

struct system
{
  String hardwareRevision;
  String firmwareRevision;
  String serialNumber;
  String MCUtype;
  String breakoutGeneration;
  String bootloader;
} systemSettings;

struct info
{
  String ip;
  String configTimeout;
  String time;
  String date;
} infoState;

struct txState
{
  String hardwarePresent;
  String hardwareSwitch;
  String softwareSwitch;
  String stationData;
  String status;
  String channelALast;
  String channelALastTime;
  String channelALastDate;
  String channelBLast;
  String channelBLastTime;
  String channelBLastDate;
  String channelANoise;
  String channelBNoise;
} txState;

//taster

#define TASTER 4

//webserver

AsyncWebServer server(80);

const char *ssidWebserver = "ulfnet";
const char *passwordWebserver = "!=hierbinichmenschhierwillichsein44100$%";

const char *PARAM_TYPE = "type";
const char *PARAM_SSID = "ssid";
const char *PARAM_PASSWORD = "password";
const char *PARAM_PORT = "port";
const char *PARAM_MMSI = "mmsi";
const char *PARAM_CALLSIGN = "callsign";
const char *PARAM_VESSELNAME = "vesselname";
const char *PARAM_VESSELTYPE = "vesseltype";
const char *PARAM_LOA = "loa";
const char *PARAM_BEAM = "beam";
const char *PARAM_PORTOFFSET = "portoffset";
const char *PARAM_BOWOFFSET = "bowoffset";
const char *PARAM_TOGGLE = "softtxtoggle";

//---------------functions-----------------------------

//general

//read config
//store config

//start config mode
//start normal operation

//start TCP forward
//stop TCP forward

//start UDP forward
//stop UDP forward

//start WiFi AP
//start WiFi cli

//read from MAIANA
//set config in MAIANA

//scanWiFi
void scanWiFi(AsyncResponseStream *response)
{
  //  WiFi.mode(WIFI_STA);
  //  WiFi.disconnect();
  //  delay(100);
  int n = WiFi.scanNetworks();

  DynamicJsonDocument doc(4096);

  if (n == 0)
  {
    doc.createNestedArray("networks");
  }
  else
  {
    for (int i = 0; i < n; ++i)
    {
      doc["networks"][i][0] = WiFi.SSID(i);
      doc["networks"][i][1] = WiFi.RSSI(i);
      doc["networks"][i][2] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
    }
  }
  serializeJson(doc, *response);
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

//GPRMC
void gpsTimeToStruct(String input)
{
  if (getValue(input, ',', 2) == "A")
  {
    infoState.time = getValue(getValue(input, ',', 1), '.', 0);
    infoState.date = getValue(input, ',', 9);
  }
}

//PAISTN
void stationDataToStruct(String input)
{
  stationSettings.mmsi = getValue(input, ',', 1);
  stationSettings.vesselname = getValue(input, ',', 2);
  stationSettings.callsign = getValue(input, ',', 3);
  stationSettings.vesseltype = getValue(input, ',', 4).toInt();
  stationSettings.loa = getValue(input, ',', 5).toInt();
  stationSettings.beam = getValue(input, ',', 6).toInt();
  stationSettings.portoffset = getValue(input, ',', 7).toInt();
  stationSettings.bowoffset = getValue(getValue(input, ',', 8), '*', 0).toInt();
}

//PAISYS
void systemDataToStruct(String input)
{
  systemSettings.hardwareRevision = getValue(input, ',', 1);
  systemSettings.firmwareRevision = getValue(input, ',', 2);
  systemSettings.serialNumber = getValue(input, ',', 3);
  systemSettings.MCUtype = getValue(input, ',', 4);
  systemSettings.breakoutGeneration = getValue(input, ',', 5);
  systemSettings.bootloader = getValue(getValue(input, ',', 6), '*', 0);
}

//PAITXCFG
void txCfgToStruct(String input)
{
  txState.hardwarePresent = getValue(input, ',', 1);
  txState.hardwareSwitch = getValue(input, ',', 2);
  txState.softwareSwitch = getValue(input, ',', 3);
  txState.stationData = getValue(input, ',', 4);
  txState.status = getValue(getValue(input, ',', 5), '*', 0);
}

//PAITX,A,18*1C
void txToStruct(String input)
{
  if (getValue(input, ',', 1).startsWith("A"))
  {
    txState.channelALast = getValue(getValue(input, ',', 2), '*', 0);
    txState.channelALastTime = infoState.time;
    txState.channelALastDate = infoState.date;
  }

  if (getValue(input, ',', 1).startsWith("B"))
  {
    txState.channelBLast = getValue(getValue(input, ',', 2), '*', 0);
    txState.channelBLastTime = infoState.time;
    txState.channelBLastDate = infoState.date;
  }
}

//PAINF,A,0x20*5B
void noiseFloorToStruct(String input)
{
  if (getValue(input, ',', 1).startsWith("A"))
  {
    txState.channelANoise = getValue(getValue(input, ',', 2), '*', 0);
  }

  if (getValue(input, ',', 1).startsWith("B"))
  {
    txState.channelBNoise = getValue(getValue(input, ',', 2), '*', 0);
  }
}

void checkLine(String line)
{
  if (line.startsWith("$PAI") || line.substring(3).startsWith("RMC"))
  {
    String msg = getValue(line, '*', 0);
    String msgchecksum = getValue(line, '*', 1);
    msgchecksum.toLowerCase();
    unsigned int len = msg.length() - 1;
    char checksum = 0;

    //skip the first character
    while (len > 0)
    {
      //XOR
      checksum ^= msg.charAt(len);
      len--;
    }

    String checksumStr = String(checksum, HEX);
    if (checksumStr.length() < 2)
    {
      checksumStr = "0" + checksumStr;
    }

    if (checksumStr.equals(msgchecksum))
    {
      if (line.startsWith("$PAISTN"))
      {
        stationDataToStruct(line);
      }

      if (line.startsWith("$PAISYS"))
      {
        systemDataToStruct(line);
      }

      if (line.startsWith("$PAITXCFG"))
      {
        txCfgToStruct(line);
      }

      if (line.startsWith("$PAITX"))
      {
        txToStruct(line);
      }

      if (line.startsWith("$PAINF"))
      {
        noiseFloorToStruct(line);
      }

      if (line.startsWith("$GNRMC"))
      {
        gpsTimeToStruct(line);
      }
    }
  }
}

void testParsing()
{
  checkLine("$PAINF,A,0x20*5B");
  checkLine("$PAINF,B,0x23*5B");
  checkLine("$PAITX,A,18*1C");
  checkLine("$PAITX,B,66*16");
  checkLine("$PAISYS,11.3.0,4.0.0,0537714,STM32L422,4,2*30");
  checkLine("$PAISTN,987654321,NAUT,CALLSIGN23,37,23,42,34,84*36");
  checkLine("$PAITXCFG,2,3,4,5,6*0C");
  checkLine("$GNRMC,230121.000,A,5130.7862,N,00733.3069,E,0.09,117.11,010222,,,A,V*03");
  /*
  Serial.print("wifiSettings.type = ");
  Serial.println(wifiSettings.type);
  Serial.print("wifiSettings.ssid = ");
  Serial.println(wifiSettings.ssid);
  Serial.print("wifiSettings.password = ");
  Serial.println(wifiSettings.password);

  Serial.print("protocolSettings.type = ");
  Serial.println(protocolSettings.type);
  Serial.print("protocolSettings.port = ");
  Serial.println(protocolSettings.port);
*/
  Serial.println("====================");

  Serial.print("infoState.ip = ");
  Serial.println(infoState.ip);
  Serial.print("infoState.configTimeout = ");
  Serial.println(infoState.configTimeout);
  Serial.print("infoState.time = ");
  Serial.println(infoState.time);
  Serial.print("infoState.date = ");
  Serial.println(infoState.date);

  Serial.println("====================");

  Serial.print("stationSettings.mmsi = ");
  Serial.println(stationSettings.mmsi);
  Serial.print("stationSettings.callsign = ");
  Serial.println(stationSettings.callsign);
  Serial.print("stationSettings.vesselname = ");
  Serial.println(stationSettings.vesselname);
  Serial.print("stationSettings.vesseltype = ");
  Serial.println(stationSettings.vesseltype);
  Serial.print("stationSettings.loa = ");
  Serial.println(stationSettings.loa);
  Serial.print("stationSettings.beam = ");
  Serial.println(stationSettings.beam);
  Serial.print("stationSettings.portoffset = ");
  Serial.println(stationSettings.portoffset);
  Serial.print("stationSettings.bowoffset = ");
  Serial.println(stationSettings.bowoffset);

  Serial.println("====================");

  Serial.print("systemSettings.hardwareRevision = ");
  Serial.println(systemSettings.hardwareRevision);
  Serial.print("systemSettings.firmwareRevision = ");
  Serial.println(systemSettings.firmwareRevision);
  Serial.print("systemSettings.serialNumber = ");
  Serial.println(systemSettings.serialNumber);
  Serial.print("systemSettings.MCUtype = ");
  Serial.println(systemSettings.MCUtype);
  Serial.print("systemSettings.breakoutGeneration = ");
  Serial.println(systemSettings.breakoutGeneration);
  Serial.print("systemSettings.bootloader = ");
  Serial.println(systemSettings.bootloader);

  Serial.println("====================");

  Serial.print("txState.hardwarePresent = ");
  Serial.println(txState.hardwarePresent);
  Serial.print("txState.hardwareSwitch = ");
  Serial.println(txState.hardwareSwitch);
  Serial.print("txState.softwareSwitch = ");
  Serial.println(txState.softwareSwitch);
  Serial.print("txState.stationData = ");
  Serial.println(txState.stationData);
  Serial.print("txState.status = ");
  Serial.println(txState.status);
  Serial.print("txState.channelALast = ");
  Serial.println(txState.channelALast);
  Serial.print("txState.channelBLast = ");
  Serial.println(txState.channelBLast);
  Serial.print("txState.channelANoise = ");
  Serial.println(txState.channelANoise);
  Serial.print("txState.channelBNoise = ");
  Serial.println(txState.channelBNoise);
}

//webserver

void handleInfo(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonDocument json(1024);

  json["ip"] = infoState.ip;
  json["configTimeout"] = infoState.configTimeout;
  json["time"] = infoState.time;
  json["date"] = infoState.date;

  serializeJson(json, *response);
  request->send(response);
}

void handleTXstate(AsyncWebServerRequest *request)
{
  if (request->hasParam(PARAM_TOGGLE))
  {
    //TODO: send to AIS
    //->getParam(PARAM_TOGGLE)->value();
  }
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonDocument json(1024);

  json["hardwarePresent"] = txState.hardwarePresent;
  json["hardwareSwitch"] = txState.hardwareSwitch;
  json["softwareSwitch"] = txState.softwareSwitch;
  json["stationData"] = txState.stationData;
  json["status"] = txState.status;
  json["channelALast"] = txState.channelALast;
  json["channelALastTime"] = txState.channelALastTime;
  json["channelALastDate"] = txState.channelALastDate;
  json["channelBLast"] = txState.channelBLast;
  json["channelBLastTime"] = txState.channelBLastTime;
  json["channelBLastDate"] = txState.channelBLastDate;
  json["channelANoise"] = txState.channelANoise;
  json["channelBNoise"] = txState.channelBNoise;

  serializeJson(json, *response);
  request->send(response);

  Serial.print("DebugGPIO: ");
  Serial.println(digitalRead(TASTER));
}

void handleScan(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("application/json");

  scanWiFi(response);

  request->send(response);
}

void handleWifi(AsyncWebServerRequest *request)
{
  if (request->hasParam(PARAM_TYPE) && request->hasParam(PARAM_SSID) && request->hasParam(PARAM_PASSWORD))
  {
    wifiSettings.type = request->getParam(PARAM_TYPE)->value();
    wifiSettings.ssid = request->getParam(PARAM_SSID)->value();
    wifiSettings.password = request->getParam(PARAM_PASSWORD)->value();
  }

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonDocument json(1024);

  json["type"] = wifiSettings.type;
  json["ssid"] = wifiSettings.ssid;
  json["password"] = wifiSettings.password;

  serializeJson(json, *response);
  request->send(response);
}

void handleProtocol(AsyncWebServerRequest *request)
{
  if (request->hasParam(PARAM_TYPE) && request->hasParam(PARAM_PORT))
  {
    protocolSettings.type = request->getParam(PARAM_TYPE)->value();
    protocolSettings.port = request->getParam(PARAM_PORT)->value().toInt();
  }

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonDocument json(1024);

  json["type"] = protocolSettings.type;
  json["port"] = protocolSettings.port;

  serializeJson(json, *response);
  request->send(response);
}

void handleStation(AsyncWebServerRequest *request)
{

  if (request->hasParam(PARAM_MMSI) && request->hasParam(PARAM_CALLSIGN) && request->hasParam(PARAM_VESSELNAME) && request->hasParam(PARAM_VESSELTYPE) && request->hasParam(PARAM_LOA) && request->hasParam(PARAM_BEAM) && request->hasParam(PARAM_PORTOFFSET) && request->hasParam(PARAM_BOWOFFSET))
  {
    stationSettings.mmsi = request->getParam(PARAM_MMSI)->value();
    stationSettings.callsign = request->getParam(PARAM_CALLSIGN)->value();
    stationSettings.vesselname = request->getParam(PARAM_VESSELNAME)->value();
    stationSettings.vesseltype = request->getParam(PARAM_VESSELTYPE)->value().toInt();
    stationSettings.loa = request->getParam(PARAM_LOA)->value().toInt();
    stationSettings.beam = request->getParam(PARAM_BEAM)->value().toInt();
    stationSettings.bowoffset = request->getParam(PARAM_BOWOFFSET)->value().toInt();
    stationSettings.portoffset = request->getParam(PARAM_PORTOFFSET)->value().toInt();
  }

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonDocument json(1024);

  json["mmsi"] = stationSettings.mmsi;
  json["callsign"] = stationSettings.callsign;
  json["vesselname"] = stationSettings.vesselname;
  json["vesseltype"] = stationSettings.vesseltype;
  json["loa"] = stationSettings.loa;
  json["beam"] = stationSettings.beam;
  json["portoffset"] = stationSettings.portoffset;
  json["bowoffset"] = stationSettings.bowoffset;

  serializeJson(json, *response);
  request->send(response);
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void setup()
{
  //general
  Serial.begin(115200);
  Serial2.begin(115200); //, SERIAL_8N1, RXD2, TXD2);
  testParsing();
  WiFi.mode(WIFI_STA);

  //GPIO for taster
  pinMode(TASTER, INPUT_PULLDOWN);

  //webserver
  // Initialize spiffs
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.begin(ssidWebserver, passwordWebserver);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed!\n");
    return;
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  // GET request /info
  server.on("/info", HTTP_GET, [](AsyncWebServerRequest *request)
            { handleInfo(request); });

  // GET request /txstate
  server.on("/txstate", HTTP_GET, [](AsyncWebServerRequest *request)
            { handleTXstate(request); });

  // GET request /scan
  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request)
            { handleScan(request); });

  // GET request /wifi
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request)
            { handleWifi(request); });

  // GET request /protocol
  server.on("/protocol", HTTP_GET, [](AsyncWebServerRequest *request)
            { handleProtocol(request); });

  // GET request /station
  server.on("/station", HTTP_GET, [](AsyncWebServerRequest *request) {handleStation(request);});

  server.onNotFound(notFound);

  server.begin();
}

void loop()
{

  //general
  if (Serial.available())
  {
    Serial2.write(Serial.read());
  }
  if (Serial2.available())
  {
    Serial.write(Serial2.read());
  }

  //webserver
}
