#include "helperfunctions.h"

#include <Arduino.h> /* we need arduino functions to implement this */
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

#include <ElegantOTA.h>



// OTA helper functions

unsigned long ota_progress_millis = 0;

void setupOTA( AsyncWebServer *server) {
    ElegantOTA.begin(server);  // Start ElegantOTA
    ElegantOTA.setAutoReboot(false);

    // ElegantOTA callbacks
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onProgress(onOTAProgress);
    ElegantOTA.onEnd(onOTAEnd);
}

void onOTAStart() {
    // Log when OTA has started
    Serial.println("OTA update started!");
    // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
    // Log every 1 second
    if (millis() - ota_progress_millis > 1000) {
        ota_progress_millis = millis();
        Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n",
                      current, final);
    }
}

void onOTAEnd(bool success) {
    // Log when OTA has finished
    if (success) {
        Serial.println("OTA update finished successfully!");
    } else {
        Serial.println("There was an error during OTA update!");
    }
    // <Add your own code here>
}

void otaLoop() {
    ElegantOTA.loop();
}

// JSON file helper functions

bool loadJsonFile(String filename, JsonDocument& doc) {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount file system");
        return false;
    }
    if (!SPIFFS.exists(filename)) {
        Serial.println("File not found");
        return false;
    }
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.println("Failed to open file");
        return false;
    }
    size_t size = file.size();
    std::unique_ptr<char[]> buf(new char[size]);
    file.readBytes(buf.get(), size);
    file.close();
    DeserializationError error = deserializeJson(doc, buf.get());
    if (error) {
        Serial.print("Failed to parse file: ");
        Serial.println(error.c_str());
        return false;
    }
    return true;
};

// File helper functions

bool writeJsonFile(String filename, JsonDocument &doc) {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount file system");
        return false;
    }

    File file = SPIFFS.open(filename, "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write to file");
        file.close();
        return false;
    }

    file.close();
    Serial.println("File saved successfully: " + filename);
    serializeJson(doc, Serial);
    Serial.println();
    return true;
};


