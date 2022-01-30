//------------------includes--------------------

//general
#include <Arduino.h>
#include <ArduinoJson.h>


//webserver

//---------------global variables and definitions-------------

//general

//webserver

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

void setup() {
  
  //general

  //webserver
}

void loop()
{

  //general

  //webserver
}
