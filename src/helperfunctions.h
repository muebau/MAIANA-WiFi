#ifndef HELPERFUNCTIONS_H /* include guards */
#define HELPERFUNCTIONS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ElegantOTA.h>


void setupOTA( AsyncWebServer *server);

void onOTAProgress(size_t current, size_t final);

void onOTAEnd(bool success);

void onOTAStart();

void otaLoop();

/// @brief Loads a JSON file from SPIFFS.
/// @param filename The name of the JSON file to load.
/// @param doc The JsonDocument object to store the loaded JSON data.
/// @return Returns true if the JSON file was successfully loaded, false otherwise.
bool loadJsonFile(String filename, JsonDocument& doc);

/// @brief Writes a JSON file to SPIFFS.
/// @param filename The name of the JSON file to write.
/// @param doc The JsonDocument object containing the JSON data to write.
/// @return Returns true if the JSON file was successfully written, false otherwise.
bool writeJsonFile(String filename, JsonDocument &doc);

#endif /* HELPERFUNCTIONS_H */
