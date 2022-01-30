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

//webserver

AsyncWebServer server(80);

const char* ssidWebserver = "ulfnet";
const char* passwordWebserver = "!=hierbinichmenschhierwillichsein44100$%";

const char* PARAM_MESSAGE = "message";


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
    doc["networks"][0] = [];
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

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}


void setup() {
  
  //general
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  //webserver
  WiFi.begin(ssidWebserver, passwordWebserver);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Hello, world");
    });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam(PARAM_MESSAGE)) {
            message = request->getParam(PARAM_MESSAGE)->value();
        } else {
            message = "No message sent";
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
