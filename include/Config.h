#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""
#define BLYNK_PRINT Serial

const char ssid[] = "";
const char pass[] = "";

const String BOT_TOKEN = ""; 
const String CHAT_ID = ""; 

const char* ap_ssid = "Earthquake_Monitor_AP"; 
const unsigned long AP_TIMEOUT = 20000; 

#define BUZZER_PIN       25
#define SD_CS_PIN        5
#define ULTRASONIC_PIN   32
#define BATTERY_PIN      34 
#define LED_PIN_R        26
#define LED_PIN_Y        27
#define LED_PIN_G        14
#define FLOAT_SENSOR_PIN 33 

#define BACKUP_LAT  5.147305
#define BACKUP_LNG  100.492161

#endif
