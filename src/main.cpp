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
  String firmware = "1.1";
  String ip;
  String confogTimeout;
} infoSettings;

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
void scanWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();

  DynamicJsonDocument doc(1024);

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
  serializeJson(doc, Serial);
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

//PAISTN
void stationDataToStruct(String input)
{
  stationSettings.mmsi = getValue(input, ',', 1);
  stationSettings.vesselname = getValue(input, ',', 2);
  stationSettings.callsign = getValue(input, ',', 3);
  stationSettings.vesseltype = getValue(input, ',', 4);
  stationSettings.loa = getValue(input, ',', 5);
  stationSettings.beam = getValue(input, ',', 6);
  stationSettings.portoffset = getValue(input, ',', 7);
  stationSettings.bowoffset = getValue(getValue(input, '*', 0), ',', 8);
}

//PAISYS
void systemDataToStruct(String input)
{
  //TODO: check length to reflect versions
  systemSettings.hardwareRevision = getValue(input, ',', 1);
  systemSettings.firmwareRevision = getValue(input, ',', 2);
  systemSettings.serialNumber = getValue(input, ',', 3);
  ///-------------------
  systemSettings.MCUtype = getValue(input, ',', 4);
  systemSettings.breakoutGeneration = getValue(input, ',', 5);
  systemSettings.bootloader = getValue(getValue(input, ',', 6), '*', 0);
//SKdata.update({"MAIANA.hardwareRevision":data3[1],"MAIANA.firmwareRevision":data3[2],"MAIANA.serialNumber":data3[3],"MAIANA.MCUtype":data3[4],"MAIANA.breakoutGeneration":data3[5],"MAIANA.bootloader":data3[6]})
//SKdata.update({"MAIANA.hardwareRevision":data3[1],"MAIANA.firmwareRevision":data3[2],"MAIANA.serialNumber":data3[3],"MAIANA.MCUtype":data3[4],"MAIANA.breakoutGeneration":'',"MAIANA.bootloader":''})
//SKdata.update({"MAIANA.hardwareRevision":data3[1],"MAIANA.firmwareRevision":data3[2],"MAIANA.serialNumber":data3[3],"MAIANA.MCUtype":'',"MAIANA.breakoutGeneration":'',"MAIANA.bootloader":''})
}

//$PAITXCFG':
void txCfgToStruct(String input)
{
//hardwarePresent":int(data3[1])
//hardwareSwitch":int(data3[2])
//softwareSwitch":int(data3[3])
//stationData":int(data3[4])
//status":int(data3[5])
}

//#$PAITX,A,18*1C
void txToStruct(String input)
{
//channel"+data3[1]
//transmittedMessageType":data3[2]
}

//#$PAINF,A,0x20*5B
void noiseFloorToStruct(String input)
{
//channel"+data3[1]
//noiseValue = int(data3[2],16)
}

//webserver
void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void setup()
{

  //general
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  //webserver
  // Initialize spiffs
  if(!SPIFFS.begin()){
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
            {request->send(SPIFFS, "/index.html", "text/html");});

  // GET request /info
  server.on("/info", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String message;
              message = "info";
              request->send(200, "text/plain", "Hello, GET: " + message);
            });

  // GET request /wifi
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String message;
              if (request->hasParam(PARAM_TYPE) && request->hasParam(PARAM_SSID) && request->hasParam(PARAM_PASSWORD))
              {
                wifiSettings.type = request->getParam(PARAM_TYPE)->value();
                wifiSettings.ssid = request->getParam(PARAM_SSID)->value();
                wifiSettings.password = request->getParam(PARAM_PASSWORD)->value();
              }
              else
              {
                message = "wifi in leer";
              }
              request->send(200, "text/plain", "Hello, wifi: " + message);
            });

  // GET request /protocol
  server.on("/protocol", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String message;
              if (request->hasParam(PARAM_TYPE) && request->hasParam(PARAM_PORT))
              {
                protocolSettings.type = request->getParam(PARAM_TYPE)->value();
                protocolSettings.port = request->getParam(PARAM_PORT)->value().toInt();
              }
              else
              {
                message = "protocol in leer";
              }
              request->send(200, "text/plain", "Hello, GET: " + message);
            });

  // GET request /station
  server.on("/station", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String message;
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
              else
              {
                message = "station in leer";
              }
              request->send(200, "text/plain", "Hello, GET: " + message);
            });

  server.onNotFound(notFound);

  server.begin();
}

void loop()
{

  //general

  //webserver
}
