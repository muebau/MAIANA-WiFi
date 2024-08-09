#include "aisstore.h"

#ifdef AISMEMORY

#include <AIS.h>
#include <Arduino.h>

#include <map>

#include "helperfunctions.h"

#define AIS_CLEANLOOP_INTERVAL 60000 // in ms
#define AIS_MAX_AGE 7 * 60000 // in ms

std::map<std::string, std::string> ais_messages;
std::map<std::string, long> ais_messages_timer;

long next_age_check = millis() + AIS_CLEANLOOP_INTERVAL;
bool debug_logging_ais = true;

std::map<std::string, std::string> getAISMessages() { return ais_messages; }

void storeAIS(const char* line) {
    String ais_value_only = getValue(line, ',', 5);
    AIS ais_msg(ais_value_only.c_str());
    unsigned long mmsi = ais_msg.get_mmsi();
    unsigned int msg_type = ais_msg.get_numeric_type();

    std::string keyStr =
        std::to_string(mmsi) + "-" +
        std::to_string(ais_msg.get_numeric_type()) +
        (msg_type == 24 ? (ais_msg.get_partno() == 0 ? "A" : "B") : "");

    ais_messages[keyStr.c_str()] = line;
    ais_messages_timer[std::to_string(mmsi)] = millis();

    if (debug_logging_ais) {
        Serial.print("AIS message stored key: ");
        Serial.println(keyStr.c_str());
        Serial.print("We have now ");
        Serial.print(ais_messages.size());
        Serial.println(" AIS message stored");
    }
}

void clear_aged_AIS() {
     long age_limit = long(millis()) - AIS_MAX_AGE;
    for (auto it = ais_messages_timer.begin();
         it != ais_messages_timer.end();) {
        if (debug_logging_ais) {
            Serial.print("mssi: ");
            Serial.print(it->first.c_str());
            Serial.print(" age: ");
            Serial.print((it->second - age_limit) / 1000.0);
        }

        if (it->second < age_limit) {
            Serial.println(" *Remove*");
            for (auto msg = ais_messages.begin(); msg != ais_messages.end();) {
                // locate mssi in ais_messages
                if (msg->first.find(it->first + "-") == 0) {
                    if (debug_logging_ais) {
                        Serial.print("AIS message removed key: ");
                        Serial.println(msg->first.c_str());
                    }
                    msg = ais_messages.erase(msg);
                } else {
                    ++msg;
                }
            }
            it = ais_messages_timer.erase(it);
        } else {
            Serial.println(" *Keep*");
            ++it;
        }
    }
}

void ais_store_cleanup_loop() {
    // This function is called from the main loop
    // It is responsible for cleaning up the AIS store
    // It should be called at least once every minute
    if (millis() > next_age_check) {
        if (debug_logging_ais) {
            Serial.print("Scheduled cleanup. We have ");
            Serial.print(ais_messages_timer.size());
            Serial.print(" unique MMSIs with ");
            Serial.print(ais_messages.size());
            Serial.println(" AIS message stored.");
        }
        next_age_check = millis() + AIS_CLEANLOOP_INTERVAL;
        clear_aged_AIS();
    }
}
#endif
