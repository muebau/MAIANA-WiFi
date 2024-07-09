//------------------includes--------------------

// general
#include <Arduino.h>
#include <ArduinoJson.h>

// webserver
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>

// #include <AsyncTCP.h>
#include <AsyncTCP.h>
#include <AsyncUDP.h>
#include <ESPAsyncWebServer.h>

#include "helperfunctions.h"

#ifdef FAKEAIS
#include "fakedata.h"
#endif

bool debug_logging = false;

#define DEBUG(X)            \
    Serial.print(__LINE__); \
    Serial.print(": ");     \
    Serial.println(X)

//---------------global variables and definitions-------------

// general

struct wifi {
    String type;
    String ssid;
    String password;
} wifiSettings;

struct protocol {
    bool tcp;
    int tcpPort;
    bool udp;
    int udpPort;
    bool websocket;
    int websocketPort;
} protocolSettings;

struct station {
    String mmsi;
    String callsign;
    String vesselname;
    int vesseltype;
    int loa;
    int beam;
    int portoffset;
    int bowoffset;
} stationSettings;

struct system {
    String hardwareRevision;
    String firmwareRevision;
    String serialNumber;
    String MCUtype;
    String breakoutGeneration;
    String bootloader;
} systemSettings;

struct info {
    String ip;
    String configTimeout;
    String time;
    String date;
} infoState;

struct txState {
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

// in seconds
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
bool websocketOk = false;
bool fakeAISData = false;

static std::vector<AsyncClient *> clients;

// Set config WifI credentials
#define CONFIG_SSID "MAIANA"
#define CONFIG_PASS "MAIANA-AIS"
#define WIFI_SETTINGS_FILE "/wifi.json"
#define PROTOCOL_SETTINGS_FILE "/protocol.json"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

AsyncUDP udp;
std::unique_ptr<AsyncServer> tcp;

#define NMEALEN 84
char nmeaLine[NMEALEN] = "";
uint8_t nmeaPos = 0;

#define WIFICONFIGLINELEN 100
char wifiConfigLine[WIFICONFIGLINELEN] = "";
uint8_t wifiConfigPos = 0;
bool wifiConfigLineReceiving = false;

const char *ssidWebserver = "MAIANA";
const char *passwordWebserver = "MAIANA-AIS";

const char *PARAM_TYPE = "type";
const char *PARAM_SSID = "ssid";
const char *PARAM_PASSWORD = "password";

const char *PARAM_TCP_ENABLE = "tcp";
const char *PARAM_TCP_PORT = "tcpport";
const char *PARAM_UDP_ENABLE = "udp";
const char *PARAM_UDP_PORT = "udpport";
const char *PARAM_WEBSOCKET_ENABLE = "websocket";
const char *PARAM_WEBSOCKET_PORT = "websocketport";

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
void initWebSocket();
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
bool readProtocolFromFile();
bool portIsValid(int port) { return port > 0 && port < 65536; }

String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// GPRMC
void gpsTimeToStruct(String input) {
    if (getValue(input, ',', 2) == "A") {
        infoState.time = getValue(getValue(input, ',', 1), '.', 0);
        infoState.date = getValue(input, ',', 9);
    }
}

// PAISTN
void stationDataToStruct(String input) {
    stationSettings.mmsi = getValue(input, ',', 1);
    stationSettings.vesselname = getValue(input, ',', 2);
    stationSettings.callsign = getValue(input, ',', 3);
    stationSettings.vesseltype = getValue(input, ',', 4).toInt();
    stationSettings.loa = getValue(input, ',', 5).toInt();
    stationSettings.beam = getValue(input, ',', 6).toInt();
    stationSettings.portoffset = getValue(input, ',', 7).toInt();
    stationSettings.bowoffset =
        getValue(getValue(input, ',', 8), '*', 0).toInt();
}

// PAISYS
void systemDataToStruct(String input) {
    systemSettings.hardwareRevision = getValue(input, ',', 1);
    systemSettings.firmwareRevision = getValue(input, ',', 2);
    systemSettings.serialNumber = getValue(input, ',', 3);
    systemSettings.MCUtype = getValue(input, ',', 4);
    systemSettings.breakoutGeneration = getValue(input, ',', 5);
    systemSettings.bootloader = getValue(getValue(input, ',', 6), '*', 0);
}

// PAITXCFG
void txCfgToStruct(String input) {
    txState.hardwarePresent = getValue(input, ',', 1);
    txState.hardwareSwitch = getValue(input, ',', 2);
    txState.softwareSwitch = getValue(input, ',', 3);
    txState.stationData = getValue(input, ',', 4);
    txState.status = getValue(getValue(input, ',', 5), '*', 0);
}

// PAITX,A,18*1C
void txToStruct(String input) {
    if (getValue(input, ',', 1).startsWith("A")) {
        txState.channelALast = getValue(getValue(input, ',', 2), '*', 0);
        txState.channelALastTime = infoState.time;
        txState.channelALastDate = infoState.date;
    }

    if (getValue(input, ',', 1).startsWith("B")) {
        txState.channelBLast = getValue(getValue(input, ',', 2), '*', 0);
        txState.channelBLastTime = infoState.time;
        txState.channelBLastDate = infoState.date;
    }
}

// PAINF,A,0x20*5B
void noiseFloorToStruct(String input) {
    if (getValue(input, ',', 1).startsWith("A")) {
        txState.channelANoise = getValue(getValue(input, ',', 2), '*', 0);
    }

    if (getValue(input, ',', 1).startsWith("B")) {
        txState.channelBNoise = getValue(getValue(input, ',', 2), '*', 0);
    }
}

void checkLine(String line) {
    if (line.startsWith("$PAI") || line.substring(3).startsWith("RMC")) {
        String msg = getValue(line, '*', 0);
        String msgchecksum = getValue(line, '*', 1);
        msgchecksum.toLowerCase();
        unsigned int len = msg.length() - 1;
        char checksum = 0;

        // skip the first character
        while (len > 0) {
            // XOR
            checksum ^= msg.charAt(len);
            len--;
        }

        String checksumStr = String(checksum, HEX);
        if (checksumStr.length() < 2) {
            checksumStr = "0" + checksumStr;
        }

        if (checksumStr == msgchecksum) {
            if (line.startsWith("$PAISTN")) {
                stationDataToStruct(line);
            }

            if (line.startsWith("$PAISYS")) {
                systemDataToStruct(line);
            }

            if (line.startsWith("$PAITXCFG")) {
                txCfgToStruct(line);
            }

            if (line.startsWith("$PAITX")) {
                txToStruct(line);
            }

            if (line.startsWith("$PAINF")) {
                noiseFloorToStruct(line);
            }

            if (line.startsWith("$GNRMC")) {
                gpsTimeToStruct(line);
            }
        }
    }
}

void testParsing() {
    checkLine("$PAINF,A,0x20*5B");
    checkLine("$PAINF,B,0x23*5B");
    checkLine("$PAITX,A,18*1C");
    checkLine("$PAITX,B,66*16");
    checkLine("$PAISYS,11.3.0,4.0.0,0537714,STM32L422,4,2*30");
    checkLine("$PAISTN,987654321,NAUT,CALLSIGN23,37,23,42,34,84*36");
    checkLine("$PAITXCFG,2,3,4,5,6*0C");
    checkLine(
        "$GNRMC,230121.000,A,5130.7862,N,00733.3069,E,0.09,117.11,010222,,,A,V*"
        "03");

    Serial.print("wifiSettings.type = ");
    Serial.println(wifiSettings.type);
    Serial.print("wifiSettings.ssid = ");
    Serial.println(wifiSettings.ssid);
    Serial.print("wifiSettings.password = ");
    Serial.println(wifiSettings.password);

    Serial.println("====================");

    Serial.print("protocolSettings.tcp = ");
    Serial.println(protocolSettings.tcp);
    Serial.print("protocolSettings.port = ");
    Serial.println(protocolSettings.tcpPort);
    Serial.print("protocolSettings.udp = ");
    Serial.println(protocolSettings.udp);
    Serial.print("protocolSettings.udpPort = ");
    Serial.println(protocolSettings.udpPort);
    Serial.print("protocolSettings.websocket = ");
    Serial.println(protocolSettings.websocket);

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
void handleInfo(AsyncWebServerRequest *request) {
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    DynamicJsonDocument json(1024);

    json["ip"] = infoState.ip;
    json["configTimeout"] = infoState.configTimeout;
    json["time"] = infoState.time;
    json["date"] = infoState.date;
    json["signal"] = WiFi.RSSI();

    serializeJson(json, *response);
    request->send(response);
}

void handleTXstate(AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_TOGGLE)) {
        if (txState.softwareSwitch) {
            Serial2.print("tx off\r\n");
        } else {
            Serial2.print("tx on\r\n");
        }

        Serial2.print("tx?\r\n");
    }
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
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

void handleScan(AsyncWebServerRequest *request) {
    String json = "[";
    int n = WiFi.scanComplete();
    if (n == -2) {
        WiFi.scanNetworks(true);
    } else if (n) {
        for (int i = 0; i < n; ++i) {
            if (i) {
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
        if (WiFi.scanComplete() == -2) {
            WiFi.scanNetworks(true);
        }
    }
    json += "]";
    request->send(200, "application/json", json);
    json = String();
}

void handleWifi(AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_TYPE) && request->hasParam(PARAM_SSID) &&
        request->hasParam(PARAM_PASSWORD)) {
        wifiSettings.type = request->getParam(PARAM_TYPE)->value();
        wifiSettings.ssid = request->getParam(PARAM_SSID)->value();
        wifiSettings.password = request->getParam(PARAM_PASSWORD)->value();
        startConfigWiFi();
        startWebServer();
    }

    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    DynamicJsonDocument json(1024);

    json[PARAM_TYPE] = wifiSettings.type;
    json[PARAM_SSID] = wifiSettings.ssid;
    json[PARAM_PASSWORD] = wifiSettings.password;

    serializeJson(json, *response);
    request->send(response);
    if (request->params() == 3) {
        writeJsonFile(WIFI_SETTINGS_FILE, json);
    }
}

void handleProtocol(AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_TCP_ENABLE) &&
        request->hasParam(PARAM_TCP_PORT)) {
        protocolSettings.tcp =
            request->getParam(PARAM_TCP_ENABLE)->value().equals("true");
        auto port = request->getParam(PARAM_TCP_PORT)->value().toInt();
        if (portIsValid(port)) {
            protocolSettings.tcpPort = port;
        }
    }
    if (request->hasParam(PARAM_UDP_ENABLE) &&
        request->hasParam(PARAM_UDP_PORT)) {
        protocolSettings.udp =
            request->getParam(PARAM_UDP_ENABLE)->value().equals("true");
        auto port = request->getParam(PARAM_UDP_PORT)->value().toInt();
        if (portIsValid(port)) {
            protocolSettings.udpPort = port;
        }
    }
    if (request->hasParam(PARAM_WEBSOCKET_ENABLE) &&
        request->hasParam(PARAM_WEBSOCKET_PORT)) {
        protocolSettings.websocket =
            request->getParam(PARAM_WEBSOCKET_ENABLE)->value().equals("true");
        auto port = request->getParam(PARAM_WEBSOCKET_PORT)->value().toInt();
        if (portIsValid(port)) {
            protocolSettings.websocketPort = port;
        }
    }

    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    DynamicJsonDocument json(1024);

    json[PARAM_TCP_ENABLE] = protocolSettings.tcp;
    json[PARAM_TCP_PORT] = protocolSettings.tcpPort;
    json[PARAM_UDP_ENABLE] = protocolSettings.udp;
    json[PARAM_UDP_PORT] = protocolSettings.udpPort;
    json[PARAM_WEBSOCKET_ENABLE] = protocolSettings.websocket;
    json[PARAM_WEBSOCKET_PORT] = protocolSettings.websocketPort;

    serializeJson(json, *response);
    request->send(response);
    if (request->params() == 6) {
        writeJsonFile(PROTOCOL_SETTINGS_FILE, json);
    }
}
void handleStation(AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_MMSI) && request->hasParam(PARAM_CALLSIGN) &&
        request->hasParam(PARAM_VESSELNAME) &&
        request->hasParam(PARAM_VESSELTYPE) && request->hasParam(PARAM_LOA) &&
        request->hasParam(PARAM_BEAM) && request->hasParam(PARAM_PORTOFFSET) &&
        request->hasParam(PARAM_BOWOFFSET)) {
        stationSettings.mmsi = request->getParam(PARAM_MMSI)->value();
        stationSettings.callsign = request->getParam(PARAM_CALLSIGN)->value();
        stationSettings.vesselname =
            request->getParam(PARAM_VESSELNAME)->value();
        stationSettings.vesseltype =
            request->getParam(PARAM_VESSELTYPE)->value().toInt();
        stationSettings.loa = request->getParam(PARAM_LOA)->value().toInt();
        stationSettings.beam = request->getParam(PARAM_BEAM)->value().toInt();
        stationSettings.bowoffset =
            request->getParam(PARAM_BOWOFFSET)->value().toInt();
        stationSettings.portoffset =
            request->getParam(PARAM_PORTOFFSET)->value().toInt();

        Serial2.println(
            "station " + stationSettings.mmsi + "," + stationSettings.mmsi +
            "," + stationSettings.callsign + "," + stationSettings.vesselname +
            "," + stationSettings.vesseltype + "," + stationSettings.loa + "," +
            stationSettings.beam + "," + stationSettings.bowoffset + "," +
            stationSettings.portoffset);
        Serial2.print("station?\r\n");
    }

    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
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

void handleSystem(AsyncWebServerRequest *request) {
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
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

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void setupWebServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", "text/html");
    });

    //    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    server.serveStatic("/js", SPIFFS, "/js/");

    // GET request /info
    server.on("/info", HTTP_GET,
              [](AsyncWebServerRequest *request) { handleInfo(request); });

    // GET request /txstate
    server.on("/txstate", HTTP_GET,
              [](AsyncWebServerRequest *request) { handleTXstate(request); });

    // GET request /scan
    server.on("/scan", HTTP_GET,
              [](AsyncWebServerRequest *request) { handleScan(request); });

    // GET request /wifi
    server.on("/wifi", HTTP_GET,
              [](AsyncWebServerRequest *request) { handleWifi(request); });

    // GET request /protocol
    server.on("/protocol", HTTP_GET,
              [](AsyncWebServerRequest *request) { handleProtocol(request); });

    // GET request /station
    server.on("/station", HTTP_GET,
              [](AsyncWebServerRequest *request) { handleStation(request); });

    // GET request /system
    server.on("/system", HTTP_GET,
              [](AsyncWebServerRequest *request) { handleSystem(request); });

    server.onNotFound(notFound);
}

void startWebServer() {
    if (networkOK) {
        setupWebServer();
        setupOTA(&server);
        server.begin();
        Serial.println("Webserver started");
        if (mDNSOK) {
            MDNS.addService("http", "tcp", 80);
        }
        initWebSocket();
    }
}

void stopWebServer() {
    if (mDNSOK) {
        mdns_service_remove("_http", "_tcp");
    }
    server.end();
    Serial.println("Webserver stopped");
}

static void handleNewClient(void *arg, AsyncClient *client) {
    clients.push_back(client);
}

void startTCPNMEAForward(uint16_t port) {
    if (networkOK) {
        tcp.reset(new AsyncServer(protocolSettings.tcpPort));
        tcp.get()->onClient(&handleNewClient, tcp.get());
        tcp.get()->begin();
        tcpForwaredOK = true;
    }
}

void stopTCPNMEAForward() {
    tcpForwaredOK = false;
    tcp.get()->end();
    tcp.release();
}

void startNMEAForward() {
    // first stop everything
    // stopNMEAForward();

    if (protocolSettings.tcp) {
        startTCPNMEAForward(protocolSettings.tcpPort);
        if (mDNSOK) {
            MDNS.addService("nmea-0183", "tcp", protocolSettings.tcpPort);
        }
    }

    if (protocolSettings.udp) {
        udpForwaredOK = true;
        if (mDNSOK) {
            MDNS.addService("nmea-0183", "udp", protocolSettings.udpPort);
        }
    }

    if (protocolSettings.websocket) {
        websocketOk = true;
        if (mDNSOK) {
            MDNS.addService("nmea-0183", "ws", protocolSettings.websocketPort);
        }
    }
}

void stopNMEAForward() {
    udpForwaredOK = false;

    stopTCPNMEAForward();
    if (mDNSOK) {
        mdns_service_remove("_nmea-0183", "_tcp");
        mdns_service_remove("_nmea-0183", "_udp");
    }
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("Disconnected from WiFi access point");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    Serial.println("Trying to Reconnect");
    WiFi.begin(wifiSettings.ssid.c_str(), wifiSettings.password.c_str());
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("Connected to WiFi access point");
}

void setupClientWiFi() { WiFi.mode(WIFI_STA); }

void WiFiStationGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("Connected to WiFi access point");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    infoState.ip = WiFi.localIP().toString();
}
void startClientWiFi() {
    WiFi.onEvent(WiFiStationDisconnected,
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(WiFiStationGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

    WiFi.begin(wifiSettings.ssid.c_str(), wifiSettings.password.c_str());

    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }
    Serial.print("Client IP is: ");
    Serial.println(WiFi.localIP());

    startNetwork();
    startWebServer();
    startNMEAForward();
}

void stopClientWiFi() { stopWifi(); }

void setupAPWiFi() { WiFi.mode(WIFI_AP); }

void startAPWiFi() {
    WiFi.softAP(CONFIG_SSID, CONFIG_PASS);
    startNetwork();
    startWebServer();
}

void stopAPWiFi() { stopWifi(); }

void setupConfigWiFi() {
    // WiFi.mode(WIFI_STA);
}

void startConfigWiFi() {
    if (wifiSettings.type == "sta") {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(CONFIG_SSID, CONFIG_PASS);
        WiFi.begin(wifiSettings.ssid.c_str(), wifiSettings.password.c_str());
        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            Serial.printf("WiFi Failed!\n");
            return;
        }
        Serial.println("ConfigWiFi started as AP+STA");
    }
    // AP/none/undefined
    else {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(CONFIG_SSID, CONFIG_PASS);
        Serial.println("ConfigWiFi started as AP");
    }

    WiFi.scanNetworks(true);
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    infoState.ip = WiFi.localIP().toString();

    startNetwork();
    startWebServer();
}

void stopConfigWiFi() { stopWifi(); }

void startConfigMode() {
    configStarted = millis();
    Serial.println("config interval resetted");
    if (!configMode) {
        configMode = true;
        startConfigWiFi();
        startWebServer();
        digitalWrite(LED_BUILTIN, HIGH);
    }
}

void stopConfigMode() {
    if (configMode) {
        stopWebServer();
        stopConfigWiFi();
        Serial.println("Webserver + ConfigWiFi stopped");
        if (wifiSettings.type == "sta") {
            setupClientWiFi();
            startClientWiFi();
            Serial.println("ClientWiFi started");
        } else if (wifiSettings.type == "ap") {
            setupAPWiFi();
            startAPWiFi();
            Serial.println("APWiFi started");
        }
        digitalWrite(BUILTIN_LED, LOW);
        configMode = false;
    }
}

void stopWifi() {
    stopNetwork();
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi stopped");
}

void stopNetwork() {
    networkOK = false;
    mDNSOK = false;
    MDNS.end();
    Serial.println("Network stopped");
}

void startNetwork() {
    mDNSOK = MDNS.begin("MAIANA");
    if (!mDNSOK) {
        Serial.println("Error setting up MDNS responder!");
    }
    networkOK = true;
    Serial.println("Network started");
}

bool loadWifiSettings() {
    DynamicJsonDocument doc(1024);
    if (!loadJsonFile(WIFI_SETTINGS_FILE, doc)) {
        Serial.println("Failed to load WiFi settings");
        return false;
    }
    if (doc.containsKey(PARAM_SSID)) {
        wifiSettings.ssid = doc[PARAM_SSID].as<String>();
    }
    if (doc.containsKey(PARAM_TYPE)) {
        wifiSettings.type = doc[PARAM_TYPE].as<String>();
    }
    if (doc.containsKey(PARAM_PASSWORD)) {
        wifiSettings.password = doc[PARAM_PASSWORD].as<String>();
    }

    return true;
}

void requestAISInfomation() {
    Serial2.print("sys?\r\n");
    Serial2.print("station?\r\n");
    Serial2.print("tx?\r\n");
}

void configPoll() {
    // switch

    // 0 = switch is pressed
    if (digitalRead(SWITCH) == LOW) {
        startConfigMode();
    }

    if (configMode) {
        if ((millis() - blinkMillis) / 1000 >= BLINK_SEC) {
            blinkState = !blinkState;
            digitalWrite(BUILTIN_LED, blinkState);
            requestAISInfomation();
            blinkMillis = millis();
        }

        uint16_t confSecSince = (millis() - configStarted) / 1000;
        infoState.configTimeout = CONFIG_TIMEOUT - confSecSince;
        if (confSecSince >= CONFIG_TIMEOUT) {
            stopConfigMode();
        }
    }
}

void setupFileSystem() {
    if (!SPIFFS.begin()) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    Serial.println("SPIFFS ready");
}

bool handleWifiConfigLine(char c) {
    if (c == '{') {
        wifiConfigPos = 0;
        wifiConfigLineReceiving = true;
    }
    if (wifiConfigLineReceiving) {
        wifiConfigLine[wifiConfigPos] = c;
        wifiConfigPos++;
    }
    if (c == '}') {
        wifiConfigLine[wifiConfigPos] = 0;
        wifiConfigLineReceiving = false;
        Serial.print("Configure wifi settings:");
        Serial.println(&wifiConfigLine[0]);
        if (wifiConfigPos == 0) {
            Serial.println("wifi config line empty");
            return true;
        }
        wifiConfigPos = 0;
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, wifiConfigLine);
        if (error) {
            Serial.print(F("wifi config line deserializeJson() failed: "));
            Serial.println(error.f_str());
            return true;
        }
        if (!doc.containsKey(PARAM_TYPE)) {
            doc[PARAM_TYPE] = "sta";
        }
        if (doc.containsKey(PARAM_SSID) && doc.containsKey(PARAM_PASSWORD)) {
            writeJsonFile(WIFI_SETTINGS_FILE, doc);
            stopWifi();
            loadWifiSettings();
            startConfigWiFi();
        } else {
            Serial.println("wifi config line missing ssid or password");
        }
        return true;
    }
    if (wifiConfigPos >= WIFICONFIGLINELEN) {
        wifiConfigPos = 0;
        wifiConfigLineReceiving = false;
        Serial.println("wifiConfigLine too long");
    }
    return wifiConfigLineReceiving;
}

void createAndHandleLine(char c) {
    nmeaLine[nmeaPos] = c;

    if (nmeaPos >= NMEALEN || c == '$' || c == '!') {
        nmeaPos = 0;
        nmeaLine[nmeaPos] = c;
    }

    if (nmeaLine[nmeaPos - 1] == '\r' && nmeaLine[nmeaPos] == '\n') {
        nmeaLine[nmeaPos + 1] = 0;
        Serial.println(&nmeaLine[0]);

        checkLine(nmeaLine);
        forwardIt(nmeaLine);
    }
    nmeaPos++;
}

void forwardIt(const char *line) {
    // if (networkOK && protocolSettings.port > 0 &&
    //    protocolSettings.port < 65536) {
    if (tcpForwaredOK) {
        String lineStr = String(line);
        std::for_each(clients.begin(), clients.end(), [&](AsyncClient *n) {
            if (n->space() > lineStr.length() && n->canSend()) {
                n->add(line, lineStr.length());
                n->send();
            }
        });
    }

    if (udpForwaredOK) {
        udp.broadcastTo(line, protocolSettings.udpPort);
        if (debug_logging) {
            Serial.print("UDP send: ");
            Serial.println(line);
        };
    }

    if (websocketOk) {
        ws.textAll(line);
        if (debug_logging) {
            Serial.print("Websocket send: ");
            Serial.println(line);
        }
    }
}

void safeWifiToFile() {}
void safeProtocolToFile() {}
void readWifiFromFile() {}

int validPort(int port, int defaultPort) {
    if (portIsValid(port)) {
        return port;
    } else {
        Serial.print("port setting ");
        Serial.print(port);
        Serial.print("invalid. Using default value:");
        Serial.println(defaultPort);
        return defaultPort;
    };
}

bool readProtocolFromFile() {
    // defaults used when no file or errors/missing values
    protocolSettings.tcp = true;
    protocolSettings.tcpPort = 10110;
    protocolSettings.udp = true;
    protocolSettings.udpPort = 2000;
    protocolSettings.websocket = true;
    protocolSettings.websocketPort = 80;

    StaticJsonDocument<512> doc;
    if (!loadJsonFile(PROTOCOL_SETTINGS_FILE, doc)) {
        Serial.println("Failed to load Protocol settings. Using defaults");
        return false;
    }
    Serial.print("Protocol settings loaded from ");
    Serial.println(PROTOCOL_SETTINGS_FILE);
    if (doc.containsKey(PARAM_TCP_ENABLE)) {
        protocolSettings.tcp = doc[PARAM_TCP_ENABLE].as<bool>();
    } else {
        Serial.println("tcp not found. using default value");
    }
    if (doc.containsKey(PARAM_TCP_PORT)) {
        protocolSettings.tcpPort =
            validPort(doc[PARAM_TCP_PORT].as<int>(), protocolSettings.tcpPort);
    } else {
        Serial.println("tcpPort not found. using default value");
    }
    if (doc.containsKey(PARAM_UDP_ENABLE)) {
        protocolSettings.udp = doc[PARAM_UDP_ENABLE].as<bool>();
    } else {
        Serial.println("udp not found. using default value");
    }
    if (doc.containsKey(PARAM_UDP_PORT)) {
        protocolSettings.udpPort =
            validPort(doc[PARAM_UDP_PORT].as<int>(), protocolSettings.udpPort);
    } else {
        Serial.println("udpPort not found. using default value");
    }
    if (doc.containsKey(PARAM_WEBSOCKET_ENABLE)) {
        protocolSettings.websocket = doc[PARAM_WEBSOCKET_ENABLE].as<bool>();
    } else {
        Serial.println("websocket not found. using default value");
    }
    if (doc.containsKey(PARAM_WEBSOCKET_PORT)) {
        protocolSettings.websocketPort =
            validPort(doc[PARAM_WEBSOCKET_PORT].as<int>(),
                      protocolSettings.websocketPort);
    } else {
        Serial.println("websocketPort not found. using default value");
    }
    return false;
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n",
                          client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            // handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void initWebSocket() {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

void setup() {
    // general
    Serial.begin(38400);
    Serial2.begin(38400);  //, SERIAL_8N1, RXD2, TXD2);

    Serial.println("Maiana AIS Transponder forwarder started");

    // GPIO for switch
    pinMode(SWITCH, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);

    setupFileSystem();
    // setupConfigWiFi();
    // startConfigWiFi();
    // setupWebServer();
    readWifiFromFile();
    bool wifidetails = loadWifiSettings();
    readProtocolFromFile();
    if (wifidetails && wifiSettings.type.equals("sta")) {
        setupClientWiFi();
        startClientWiFi();
    } else if (!wifidetails || wifiSettings.type.equals("ap")) {
        setupAPWiFi();
        startAPWiFi();
    }
    // TODO: remove test call
    testParsing();

    requestAISInfomation();
}

long timerloop = 0;

void loop() {
    // general
    if (Serial.available()) {
        char s = Serial.read();
        if (!handleWifiConfigLine(s)) {
            Serial2.write(s);
        }
    }
    if (Serial2.available()) {
        char c = Serial2.read();
        createAndHandleLine(c);
        Serial.write(c);
    }
    configPoll();
    otaLoop();
    ws.cleanupClients();

#ifdef FAKEAIS
    if (fakeAISData && millis() - timerloop > 5000) {
        timerloop = millis();
        String line = getRandomAISLine();
        for (int i = 0; i < line.length(); i++) {
            createAndHandleLine(line[i]);
        }
    }
#endif
}
