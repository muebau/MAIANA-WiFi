#ifndef AISSTORE_H /* include guards */
#define AISSTORE_H

#ifdef AISMEMORY

#include <Arduino.h>
#include <map>


void storeAIS(const char* line);

std::map<std::string, std::string> getAISMessages();

void ais_store_cleanup_loop();

#endif /* AISMEMORY */

#endif /* AISSTORE_H */
