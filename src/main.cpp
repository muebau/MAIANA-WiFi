//------------------includes--------------------

// general
#include <Arduino.h>
#include <ArduinoJson.h>

// webserver
#include <WiFi.h>
#include <ESPmDNS.h>

#include <FS.h>
#include <SPIFFS.h>

//#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncUDP.h>
#include <AsyncTCP.h>

#define DEBUG(X)          \
  Serial.print(__LINE__); \
  Serial.print(": ");     \
  Serial.println(X)

//---------------global variables and definitions-------------

// general

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

// switch
#define SWITCH 4

//in seconds
#define CONFIG_TIMEOUT 300
bool configMode = false;
// timestamp of the last time the switch was pressed
unsigned long configStarted = 0;

#define BLINK_SEC 2
unsigned long blinkMillis = 0;
bool blinkState = false;
bool mDNSOK = false;
bool networkOK = false;
bool udpForwaredOK = false;
bool tcpForwaredOK = false;
static std::vector<AsyncClient *> clients;

// Set config WifI credentials
#define CONFIG_SSID "MAIANA"
#define CONFIG_PASS "MAIANA-AIS"

AsyncWebServer server(80);
AsyncUDP udp;
std::unique_ptr<AsyncServer> tcp;

#define NMEALEN 84
char nmeaLine[NMEALEN] = "";
uint8_t nmeaPos = 0;

const char *ssidWebserver = "MAIANA";
const char *passwordWebserver = "MAIANA-AIS";

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

// announce all functions
String getValue(String data, char separator, int index);
void gpsTimeToStruct(String input);
void stationDataToStruct(String input);
void systemDataToStruct(String input);
void txCfgToStruct(String input);
void txToStruct(String input);
void noiseFloorToStruct(String input);
void checkLine(String line);
void testParsing();
void handleInfo(AsyncWebServerRequest *request);
void handleTXstate(AsyncWebServerRequest *request);
void handleScan(AsyncWebServerRequest *request);
void handleWifi(AsyncWebServerRequest *request);
void handleProtocol(AsyncWebServerRequest *request);
void handleStation(AsyncWebServerRequest *request);
void handleSystem(AsyncWebServerRequest *request);
void notFound(AsyncWebServerRequest *request);
void setupWebServer();
void startWebServer();
void stopWebServer();
void setupNMEAForward();
void startNMEAForward();
void stopNMEAForward();
void setupClientWiFi();
void startClientWiFi();
void stopClientWiFi();
void setupAPWiFi();
void startAPWiFi();
void stopAPWiFi();
void setupConfigWiFi();
void startConfigWiFi();
void stopConfigWiFi();
void startConfigMode();
void stopConfigMode();
void stopWifi();
void stopNetwork();
void startNetwork();
void requestAISInfomation();
void configPoll();
void setupFileSystem();
void createAndHandleLine(char c);
void forwardIt(const char *line);
void safeWifiToFile();
void safeProtocolToFile();
void readWifiFromFile();
void readProtocolFromFile();

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

// GPRMC
void gpsTimeToStruct(String input)
{
  if (getValue(input, ',', 2) == "A")
  {
    infoState.time = getValue(getValue(input, ',', 1), '.', 0);
    infoState.date = getValue(input, ',', 9);
  }
}

// PAISTN
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

// PAISYS
void systemDataToStruct(String input)
{
  systemSettings.hardwareRevision = getValue(input, ',', 1);
  systemSettings.firmwareRevision = getValue(input, ',', 2);
  systemSettings.serialNumber = getValue(input, ',', 3);
  systemSettings.MCUtype = getValue(input, ',', 4);
  systemSettings.breakoutGeneration = getValue(input, ',', 5);
  systemSettings.bootloader = getValue(getValue(input, ',', 6), '*', 0);
}

// PAITXCFG
void txCfgToStruct(String input)
{
  txState.hardwarePresent = getValue(input, ',', 1);
  txState.hardwareSwitch = getValue(input, ',', 2);
  txState.softwareSwitch = getValue(input, ',', 3);
  txState.stationData = getValue(input, ',', 4);
  txState.status = getValue(getValue(input, ',', 5), '*', 0);
}

// PAITX,A,18*1C
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

// PAINF,A,0x20*5B
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

    // skip the first character
    while (len > 0)
    {
      // XOR
      checksum ^= msg.charAt(len);
      len--;
    }

    String checksumStr = String(checksum, HEX);
    if (checksumStr.length() < 2)
    {
      checksumStr = "0" + checksumStr;
    }

    if (checksumStr == msgchecksum)
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

  wifiSettings.type = "sta";
  wifiSettings.ssid = "MAIANA";
  wifiSettings.password = "MAIANA-AIS";

  Serial.print("wifiSettings.type = ");
  Serial.println(wifiSettings.type);
  Serial.print("wifiSettings.ssid = ");
  Serial.println(wifiSettings.ssid);
  Serial.print("wifiSettings.password = ");
  Serial.println(wifiSettings.password);

  Serial.println("====================");

  Serial.print("protocolSettings.type = ");
  Serial.println(protocolSettings.type);
  Serial.print("protocolSettings.port = ");
  Serial.println(protocolSettings.port);

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

// webserver
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
    if (txState.softwareSwitch)
    {
      Serial2.print("tx off\r\n");
    }
    else
    {
      Serial2.print("tx on\r\n");
    }

    Serial2.print("tx?\r\n");
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
}

void handleScan(AsyncWebServerRequest *request)
{
  String json = "[";
  int n = WiFi.scanComplete();
  if (n == -2)
  {
    WiFi.scanNetworks(true);
  }
  else if (n)
  {
    for (int i = 0; i < n; ++i)
    {
      if (i)
      {
        json += ",";
      }
      json += "{";
      json += "\"rssi\":" + String(WiFi.RSSI(i));
      json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
      json += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
      json += ",\"channel\":" + String(WiFi.channel(i));
      json += ",\"secure\":" + String(WiFi.encryptionType(i));
      json += "}";
    }
    WiFi.scanDelete();
    if (WiFi.scanComplete() == -2)
    {
      WiFi.scanNetworks(true);
    }
  }
  json += "]";
  request->send(200, "application/json", json);
  json = String();
}

void handleWifi(AsyncWebServerRequest *request)
{
  if (request->hasParam(PARAM_TYPE) && request->hasParam(PARAM_SSID) && request->hasParam(PARAM_PASSWORD))
  {
    wifiSettings.type = request->getParam(PARAM_TYPE)->value();
    wifiSettings.ssid = request->getParam(PARAM_SSID)->value();
    wifiSettings.password = request->getParam(PARAM_PASSWORD)->value();
    startConfigWiFi();
    startWebServer();
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
    stopNMEAForward();

    auto port = request->getParam(PARAM_PORT)->value().toInt();
    if (port > 0 && port < 65536)
    {
      protocolSettings.port = port;
      startNMEAForward();
    }
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

    Serial2.println("station " + stationSettings.mmsi + "," + stationSettings.mmsi + "," + stationSettings.callsign + "," + stationSettings.vesselname + "," + stationSettings.vesseltype + "," + stationSettings.loa + "," + stationSettings.beam + "," + stationSettings.bowoffset + "," + stationSettings.portoffset);
    Serial2.print("station?\r\n");
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

void handleSystem(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonDocument json(1024);

  json["hardwareRevision"] = systemSettings.hardwareRevision;
  json["firmwareRevision"] = systemSettings.firmwareRevision;
  json["serialNumber"] = systemSettings.serialNumber;
  json["MCUtype"] = systemSettings.MCUtype;
  json["breakoutGeneration"] = systemSettings.breakoutGeneration;
  json["bootloader"] = systemSettings.bootloader;

  serializeJson(json, *response);
  request->send(response);
  Serial2.print("sys?\r\n");
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void setupWebServer()
{
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
  server.on("/station", HTTP_GET, [](AsyncWebServerRequest *request)
            { handleStation(request); });

  // GET request /system
  server.on("/system", HTTP_GET, [](AsyncWebServerRequest *request)
            { handleSystem(request); });

  server.onNotFound(notFound);
}

void startWebServer()
{
  if (networkOK)
  {
    setupWebServer();
    server.begin();
    Serial.println("Webserver started");
    if (mDNSOK)
    {
      MDNS.addService("http", "tcp", 80);
    }
  }
}

void stopWebServer()
{
  //  if (mDNSOK)
  //  {
  mdns_service_remove("_http", "_tcp");
  //  }
  server.end();
  Serial.println("Webserver stopped");
}

static void handleNewClient(void *arg, AsyncClient *client)
{
  clients.push_back(client);
}

void startTCPNMEAForward(uint16_t port)
{
  if (networkOK)
  {
    tcp.reset(new AsyncServer(protocolSettings.port));
    tcp.get()->onClient(&handleNewClient, tcp.get());
    tcp.get()->begin();
    tcpForwaredOK = true;
  }
}

void stopTCPNMEAForward()
{
  tcpForwaredOK = false;
  tcp.get()->end();
  tcp.release();
}

void startNMEAForward()
{
  // first stop everything
  stopNMEAForward();

  if (protocolSettings.port > 0 && protocolSettings.port < 65536)
  {
    if (protocolSettings.type == "tcp")
    {
      startTCPNMEAForward(protocolSettings.port);
      if (mDNSOK)
      {
        MDNS.addService("nmea-0183", "tcp", protocolSettings.port);
      }
    }

    if (protocolSettings.type == "udp")
    {
      udpForwaredOK = true;
      if (mDNSOK)
      {
        MDNS.addService("nmea-0183", "udp", protocolSettings.port);
      }
    }
  }
}

void stopNMEAForward()
{
  //  if (mDNSOK)
  //  {
  mdns_service_remove("_nmea-0183", "_tcp");
  mdns_service_remove("_nmea-0183", "_udp");
  //  }
  stopTCPNMEAForward();
  udpForwaredOK = false;
}

void setupClientWiFi()
{
  WiFi.mode(WIFI_STA);
}

void startClientWiFi()
{
  WiFi.begin(wifiSettings.ssid.c_str(), wifiSettings.password.c_str());
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed!\n");
    return;
  }
  Serial.print("Client IP is: ");
  Serial.println(WiFi.localIP());

  startNetwork();
}

void stopClientWiFi()
{
  stopWifi();
}

void setupAPWiFi()
{
  WiFi.mode(WIFI_AP);
}

void startAPWiFi()
{
  WiFi.softAP(CONFIG_SSID, CONFIG_PASS);
  startNetwork();
}

void stopAPWiFi()
{
  stopWifi();
}

void setupConfigWiFi()
{
  // WiFi.mode(WIFI_STA);
}

void startConfigWiFi()
{
  if (wifiSettings.type == "sta")
  {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(CONFIG_SSID, CONFIG_PASS);
    WiFi.begin(wifiSettings.ssid.c_str(), wifiSettings.password.c_str());
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
      Serial.printf("WiFi Failed!\n");
      return;
    }
    Serial.println("ConfigWiFi started as AP+STA");
  }
  // AP/none/undefined
  else
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(CONFIG_SSID, CONFIG_PASS);
    Serial.println("ConfigWiFi started as AP");
  }

  WiFi.scanNetworks(true);
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  infoState.ip = WiFi.localIP().toString();

  startNetwork();
}

void stopConfigWiFi()
{
  stopWifi();
}

void startConfigMode()
{
  configStarted = millis();
  Serial.println("config interval resetted");
  if (!configMode)
  {
    configMode = true;
    startConfigWiFi();
    startWebServer();
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void stopConfigMode()
{
  if (configMode)
  {
    stopWebServer();
    stopConfigWiFi();
    Serial.println("Webserver + ConfigWiFi stopped");
    if (wifiSettings.type == "sta")
    {
      setupClientWiFi();
      startClientWiFi();
      Serial.println("ClientWiFi started");
    }
    else if (wifiSettings.type == "ap")
    {
      setupAPWiFi();
      startAPWiFi();
      Serial.println("APWiFi started");
    }
    digitalWrite(BUILTIN_LED, LOW);
    configMode = false;
  }
}

void stopWifi()
{
  stopNetwork();
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi stopped");
}

void stopNetwork()
{
  networkOK = false;
  mDNSOK = false;
  MDNS.end();
  Serial.println("Network stopped");
}

void startNetwork()
{
  mDNSOK = MDNS.begin("MAIANA");
  if (!mDNSOK)
  {
    Serial.println("Error setting up MDNS responder!");
  }
  networkOK = true;
  Serial.println("Network started");
}

void requestAISInfomation()
{
  Serial2.print("sys?\r\n");
  Serial2.print("station?\r\n");
  Serial2.print("tx?\r\n");
}

void configPoll()
{
  // switch

  // 0 = switch is pressed
  if (digitalRead(SWITCH) == LOW)
  {
    startConfigMode();
  }

  if (configMode)
  {
    if ((millis() - blinkMillis) / 1000 >= BLINK_SEC)
    {
      blinkState = !blinkState;
      digitalWrite(BUILTIN_LED, blinkState);
      requestAISInfomation();
      blinkMillis = millis();
    }

    uint16_t confSecSince = (millis() - configStarted) / 1000;
    infoState.configTimeout = CONFIG_TIMEOUT - confSecSince;
    if (confSecSince >= CONFIG_TIMEOUT)
    {
      stopConfigMode();
    }
  }
}

void setupFileSystem()
{
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS ready");
}

void createAndHandleLine(char c)
{
  nmeaLine[nmeaPos] = c;
  nmeaPos++;

  if (nmeaPos >= NMEALEN)
  {
    nmeaPos = 0;
  }

  if (nmeaLine[nmeaPos - 1] == '\r' && nmeaLine[nmeaPos] == '\n')
  {
    nmeaLine[nmeaPos - 1] = 0;
    checkLine(nmeaLine);
    forwardIt(nmeaLine);
  }
}

void forwardIt(const char *line)
{
  if (networkOK && protocolSettings.port > 0 && protocolSettings.port < 65536)
  {
    if (tcpForwaredOK)
    {
      String lineStr = String(line);
      std::for_each(clients.begin(), clients.end(), [&](AsyncClient *n)
                    {
                      if (n->space() > lineStr.length() && n->canSend())
                      {
                        n->add(line, lineStr.length());
                        n->send();
                      }
                    });
    }

    if (udpForwaredOK)
    {
      udp.broadcastTo(line, protocolSettings.port);
    }
  }
}

void safeWifiToFile()
{
}
void safeProtocolToFile()
{
}
void readWifiFromFile()
{
}
void readProtocolFromFile()
{
}

void setup()
{
  // general
  Serial.begin(38400);
  Serial2.begin(38400); //, SERIAL_8N1, RXD2, TXD2);

  // TODO: remove test call
  testParsing();

  requestAISInfomation();

  // GPIO for switch
  pinMode(SWITCH, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  setupFileSystem();
  // setupConfigWiFi();
  // startConfigWiFi();
  // setupWebServer();
  readWifiFromFile();
  readProtocolFromFile();
  if (wifiSettings.type.equals("sta"))
  {
    setupClientWiFi();
    startClientWiFi();
  }
  else if (wifiSettings.type.equals("ap"))
  {
    setupAPWiFi();
    startAPWiFi();
  }
}

void loop()
{
  // general
  if (Serial.available())
  {
    Serial2.write(Serial.read());
  }
  if (Serial2.available())
  {
    char c = Serial2.read();
    createAndHandleLine(c);
    Serial.write(c);
  }
  configPoll();
}
