#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define BLYNK_TEMPLATE_ID "TMPL6EBt6e2g8"
#define BLYNK_TEMPLATE_NAME "Earthquake Monitor"
#define BLYNK_AUTH_TOKEN "4GLXLNAX86O8OWCJuJe0CbfQIfaWN223"
#define BLYNK_PRINT Serial

const char ssid[] = "umobile@wifi";
const char pass[] = "Happybanana@";

const String BOT_TOKEN = "8552373725:AAHiNRd4VF6wQKqz0b-RVo3HyHd4Yhxdxio"; 
const String CHAT_ID = "-1003133159576"; 

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