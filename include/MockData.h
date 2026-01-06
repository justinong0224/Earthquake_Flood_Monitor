#ifndef MOCKDATA_H
#define MOCKDATA_H

#include <Arduino.h>

void initSD_Mock();
void saveToSD_Mock(float vib, float rise, String status, int batt);
String getSmartHistory_Mock(String type);

#endif