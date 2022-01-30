//------------------includes--------------------

//general
#include <Arduino.h>
#include <ArduinoJson.h>

//webserver
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

//---------------global variables and definitions-------------

//general

struct wifi
{
  String type;
  String ssid;
  String password;
};

struct protocol
{
  String type;
  int port;
};

struct station
{
  String mmsi;
  String callsing;
  int vesseltype;
  int loa;
  int beam;
  int portoffset;
  int bowoffset;
};

struct info
{
  String firmware = "1.1";
  String ip;
  String confogTimeout;
};



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
  WiFi.begin(ssidWebserver, passwordWebserver);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed!\n");
    return;
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hello, world"); });

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
                message = request->getParam(PARAM_TYPE)->value() + request->getParam(PARAM_SSID)->value() + request->getParam(PARAM_PASSWORD)->value();
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
                message = request->getParam(PARAM_TYPE)->value() + request->getParam(PARAM_PORT)->value();
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
                message = request->getParam(PARAM_MMSI)->value() + request->getParam(PARAM_CALLSIGN)->value() + request->getParam(PARAM_VESSELNAME)->value() + request->getParam(PARAM_VESSELTYPE)->value() + request->getParam(PARAM_LOA)->value() + request->getParam(PARAM_BEAM)->value() + request->getParam(PARAM_PORTOFFSET)->value() + request->getParam(PARAM_BOWOFFSET)->value();
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
