
#include "fakedata.h"

#include <SPIFFS.h>

#include "Arduino.h"

long lastPos = 0;

String getDemoAISLine() {
    File file = SPIFFS.open("/ais.txt", "r");

    if (!file) {
        Serial.println("Failed to open file for reading");
        return "";
    }

    if (file.available()) {
        file.seek(lastPos);
        String line = file.readStringUntil('\n');
        lastPos = file.position();
        if (lastPos >= file.size() - 1) {
            lastPos = 0;
        }
        file.close();
        return line + "\n";
    } else {
        file.close();
        Serial.println("File not available");
        return "";
    }
}
