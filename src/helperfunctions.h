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

/// @brief Extracts a value from a string using a separator and index.
/// 
/// This function extracts a value from a string based on a separator and index. It splits the string into multiple parts using the separator,
/// and returns the value at the specified index.
/// 
/// @param data The string to extract the value from.
/// @param separator The character used as a separator.
/// @param index The index of the value to extract.
/// 
/// @return Returns the extracted value as a string.
String getValue(String data, char separator, int index);

#endif /* HELPERFUNCTIONS_H */
