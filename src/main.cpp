#include <Arduino.h>
#include "Config.h" 
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_MPU6050.h>
#include <Wire.h>
#include "time.h"
#include <esp_now.h> 
#include <WebServer.h> 
#include <DNSServer.h> 
#include "WebPage.h"
#include "MockData.h"

Adafruit_MPU6050 mpu;
BlynkTimer timer;
WidgetTerminal terminal(V5);
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
WebServer server(80); 
DNSServer dnsServer; 
SemaphoreHandle_t sdMutex; 

#define BUZZER_CHANNEL 0 

float vibrationThreshold = 0.15;   
float waterRiseThreshold = 30.0;  
bool forceAlarm = false;
bool isSDReady = true; 
bool rescueTriggered = false; 
float estimatedMagnitude = 0.0;
float baseDistance = 0.0; 
float baselineG = 9.8; 
float vibrationG = 0.0; 
float waterRise = 0.0;  
int batteryLevel = 0;   
bool isAlertActive = false; 

unsigned long wifiLostTime = 0;
bool isApMode = false;

TaskHandle_t Task1;
String globalTelegramMsg = "";   
bool newTelegramMsg = false;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800; 

typedef struct struct_message {
  bool isRescue;
} struct_message;
struct_message incomingData;

String getLocalTimeStr() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return "NTP...";
  char timeString[25];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo); 
  return String(timeString);
}

String getFullDateTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return "2000-01-01 00:00:00";
  char timeString[25];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo); 
  return String(timeString);
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float calculateRichter(float gForce) {
  if (gForce < 0.05) return 0.0;
  if (gForce < 0.2) return mapFloat(gForce, 0.05, 0.2, 3.0, 4.5);
  if (gForce < 1.0) return mapFloat(gForce, 0.2, 1.0, 4.5, 7.0);
  return mapFloat(gForce, 1.0, 3.0, 7.0, 9.5);
}

int readBattery() {
  int analogVal = analogRead(BATTERY_PIN);
  if (analogVal < 500) return 100; 
  float voltage = analogVal * (3.3 / 4095.0) * 2; 
  int percentage = map(voltage * 100, 340, 420, 0, 100);
  return constrain(percentage, 0, 100);
}

float readDistance() {
  pinMode(ULTRASONIC_PIN, OUTPUT);
  digitalWrite(ULTRASONIC_PIN, LOW); delayMicroseconds(2);
  digitalWrite(ULTRASONIC_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(ULTRASONIC_PIN, LOW);
  pinMode(ULTRASONIC_PIN, INPUT);
  long duration = pulseIn(ULTRASONIC_PIN, HIGH, 30000); 
  float dist = duration * 0.034 / 2;
  return (dist == 0 || dist > 400) ? -1 : dist;
}

void setTrafficLight(String color) {
  digitalWrite(LED_PIN_R, LOW);
  digitalWrite(LED_PIN_Y, LOW);
  digitalWrite(LED_PIN_G, LOW);
  if (color == "RED") digitalWrite(LED_PIN_R, HIGH);
  else if (color == "YELLOW") digitalWrite(LED_PIN_Y, HIGH);
  else if (color == "GREEN") digitalWrite(LED_PIN_G, HIGH);
}

void playSirenEffect() {
  setTrafficLight("RED"); 
  for (int freq = 300; freq <= 2600; freq += 100) { 
    ledcWriteTone(BUZZER_CHANNEL, freq); delay(10); 
  }
  digitalWrite(LED_PIN_R, LOW); delay(50); digitalWrite(LED_PIN_R, HIGH);
  for (int freq = 2600; freq >= 300; freq -= 100) { 
    ledcWriteTone(BUZZER_CHANNEL, freq); delay(10); 
  }
  ledcWriteTone(BUZZER_CHANNEL, 0);
}

void playSimpleTone(int freq, int duration) {
  ledcWriteTone(BUZZER_CHANNEL, freq);
  delay(duration);
  ledcWriteTone(BUZZER_CHANNEL, 0);
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingDataPtr, int len) {
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  if (incomingData.isRescue) {
    rescueTriggered = true; 
    Serial.println(">>> SOS SIGNAL RECEIVED <<<");
  }
}

void sendTelegramAlert(String message) {
  if (newTelegramMsg == false) {
    globalTelegramMsg = message;
    newTelegramMsg = true;
  }
}

void handleCallback(String query_id, String from_id, String callback_data) {
  if (callback_data == "MENU_HISTORY") {
    String keyboardJson = "[[{\"text\":\"🌋 Earthquake History\",\"callback_data\":\"TYPE_EARTHQUAKE\"}],";
    keyboardJson += "[{\"text\":\"🌊 Flood History\",\"callback_data\":\"TYPE_FLOOD\"}]]";
    bot.sendMessageWithInlineKeyboard(from_id, "Select History Type:", "", keyboardJson);
  }
  else if (callback_data == "TYPE_EARTHQUAKE") {
    bot.sendMessage(from_id, "🔍 Scanning log for Earthquake events...", "");
    String result = getSmartHistory_Mock("EARTHQUAKE");
    bot.sendMessage(from_id, result, "");
  }
  else if (callback_data == "TYPE_FLOOD") {
    bot.sendMessage(from_id, "🔍 Scanning log for Flood events...", "");
    String result = getSmartHistory_Mock("FLOOD");
    bot.sendMessage(from_id, result, "");
  }
}

void TelegramCode( void * pvParameters ) {
  Serial.println("Telegram Task Started");
  for(;;) { 
    if (newTelegramMsg) {
       if (WiFi.status() == WL_CONNECTED) {
          bot.sendMessage(CHAT_ID, globalTelegramMsg, "");
          newTelegramMsg = false;
       }
    }
    if (WiFi.status() == WL_CONNECTED) {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while (numNewMessages) {
        for (int i = 0; i < numNewMessages; i++) {
          if (bot.messages[i].type == "callback_query") {
             handleCallback(bot.messages[i].query_id, String(bot.messages[i].chat_id), bot.messages[i].text);
          } else if (bot.messages[i].text == "/start" || bot.messages[i].text == "/menu") {
             String keyboardJson = "[[{\"text\":\"📜 Show History\",\"callback_data\":\"MENU_HISTORY\"}]]";
             bot.sendMessageWithInlineKeyboard(String(bot.messages[i].chat_id), "Tap to view logs:", "", keyboardJson);
          }
        }
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
    }
    vTaskDelay(500);
  }
}

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleData() {
  String json = "{";
  json += "\"mag\":\"" + String(estimatedMagnitude, 1) + "\",";
  json += "\"vib\":\"" + String(vibrationG, 2) + "\",";
  json += "\"water\":\"" + String(waterRise, 1) + "\",";
  json += "\"batt\":\"" + String(batteryLevel) + "\",";
  json += "\"alert\":" + String(isAlertActive ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

BLYNK_WRITE(V3) { vibrationThreshold = param.asFloat(); }
BLYNK_WRITE(V4) { forceAlarm = (param.asInt() == 1); }

void runSensors() {
  static unsigned long lastTelegramTime = 0;

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float totalAccel = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));
  vibrationG = abs(totalAccel - baselineG) / 9.8; 
  if (vibrationG < 0.08) vibrationG = 0.0;

  estimatedMagnitude = calculateRichter(vibrationG);
  bool isEarthquake = (vibrationG > vibrationThreshold);

  bool isFloatTriggered = (digitalRead(FLOAT_SENSOR_PIN) == LOW);
  bool isFlood = isFloatTriggered; 

  waterRise = 0.0;
  if (isFlood) {
    float currentDistance = readDistance();
    if (currentDistance > 0 && baseDistance > 0) {
      waterRise = baseDistance - currentDistance;
    }
    if (waterRise < 0) waterRise = 0.0;
  }

  batteryLevel = readBattery(); 

  if (WiFi.status() == WL_CONNECTED) {
      Blynk.virtualWrite(V0, vibrationG);
      Blynk.virtualWrite(V7, estimatedMagnitude);
      Blynk.virtualWrite(V8, waterRise); 
      Blynk.virtualWrite(V1, batteryLevel);
      Blynk.virtualWrite(V9, isFlood ? 255 : 0); 
  }

  isAlertActive = (isEarthquake || isFlood || forceAlarm || rescueTriggered);

  if (isAlertActive) {
    String alertType = "";
    if (rescueTriggered) alertType = "🆘 RESCUE REQUEST!"; 
    else if (isEarthquake) alertType = "🚨 EARTHQUAKE!";
    else if (isFlood) alertType = "🌊 FLOOD ALERT!";
    else alertType = "🔴 REMOTE TEST";
    
    if (WiFi.status() == WL_CONNECTED) {
        Blynk.virtualWrite(V2, alertType);
        terminal.print("\n!!! "); terminal.print(alertType); terminal.print(" !!!");
        
        if (millis() - lastTelegramTime > 15000) {
          String msg = "⚠️ EMERGENCY ALERT ⚠️\n";
          msg += "Time: " + getFullDateTime() + "\n";
          msg += "Type: " + alertType + "\n";
          if (rescueTriggered) msg += "Status: User pressed panic button!\nACTION: RESCUE NEEDED IMMEDIATELY!\n";
          if (isFlood) msg += "Water Level: " + String(waterRise) + " cm\n"; 
          if (isEarthquake) msg += "Vib: " + String(vibrationG) + " g (Mag: " + String(estimatedMagnitude, 1) + ")\n";
          msg += "\n📍 Location:\nhttps://maps.google.com/?q=" + String(BACKUP_LAT, 6) + "," + String(BACKUP_LNG, 6);
          sendTelegramAlert(msg);
          lastTelegramTime = millis();
        }
    }
    setTrafficLight("RED");
    playSirenEffect();
    if (rescueTriggered) rescueTriggered = false; 

  } else {
    ledcWriteTone(BUZZER_CHANNEL, 0); // 修复
    setTrafficLight("GREEN");
    if (WiFi.status() == WL_CONNECTED) Blynk.virtualWrite(V2, "Safe");
  }

  if (WiFi.status() == WL_CONNECTED) {
    terminal.print("\nTime: "); terminal.print(getLocalTimeStr());
    terminal.print(" | Vib: "); terminal.print(vibrationG, 2);
    terminal.print("g | Mag: "); terminal.print(estimatedMagnitude, 1);
    if (isFlood) terminal.print(" | Lvl: "); terminal.print(waterRise, 1); terminal.print("cm");
    terminal.print(" | Batt: "); terminal.print(batteryLevel); terminal.print("%");
    terminal.flush(); 
  }
  
  Serial.print("Time: "); Serial.print(getLocalTimeStr());
  Serial.print(" | Vib: "); Serial.print(vibrationG, 2);
  Serial.print(" | Batt: "); Serial.print(batteryLevel); 
  Serial.print("% | WiFi: "); Serial.println(WiFi.status() == WL_CONNECTED ? "OK" : "LOST");

  saveToSD_Mock(vibrationG, waterRise, isAlertActive?"ALARM":"OK", batteryLevel);
}

void setup() {
  Serial.begin(115200);
  sdMutex = xSemaphoreCreateMutex(); 
  client.setInsecure();
  
  pinMode(LED_PIN_R, OUTPUT); pinMode(LED_PIN_Y, OUTPUT); pinMode(LED_PIN_G, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT); pinMode(BATTERY_PIN, INPUT); pinMode(FLOAT_SENSOR_PIN, INPUT_PULLUP);

  setTrafficLight("YELLOW");

  ledcSetup(BUZZER_CHANNEL, 2000, 10);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  
  if (!mpu.begin()) Serial.println("MPU Fail");
  else { mpu.setAccelerometerRange(MPU6050_RANGE_4_G); mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); }

  initSD_Mock(); 
  
  Serial.println("Connecting to Blynk...");
  WiFi.begin(ssid, pass);
  Blynk.config(BLYNK_AUTH_TOKEN); 
  
  configTime(gmtOffset_sec, 0, ntpServer);
  
  if (esp_now_init() != ESP_OK) { Serial.println("Error initializing ESP-NOW"); }
  esp_now_register_recv_cb((esp_now_recv_cb_t)OnDataRecv);
  
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.onNotFound(handleRoot); 

  float totalG = 0;
  for(int i=0; i<20; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    totalG += sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));
    delay(5); 
  }
  baselineG = totalG / 20.0; 

  float totalD = 0; int valid = 0;
  for(int i=0; i<5; i++) {
    float d = readDistance();
    if(d > 0) { totalD += d; valid++; }
    delay(50);
  }
  if(valid > 0) baseDistance = totalD / valid;
  
  xTaskCreatePinnedToCore(TelegramCode, "TelegramTask", 10000, NULL, 1, &Task1, 0);

  playSimpleTone(2000, 150); 
  setTrafficLight("GREEN");
  Blynk.virtualWrite(V9, 0);

  String bootMsg = "✅ SYSTEM ONLINE\nMode: VSCode Final Demo (V5.3)\n💾 SD Card: ✅ OK\n";
  bootMsg += "📏 Base Dist: " + String(baseDistance) + " cm\n🔋 Battery: " + String(readBattery()) + "%\n";
  sendTelegramAlert(bootMsg); 

  timer.setInterval(1000L, runSensors);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }
  timer.run(); 

  if (isApMode) {
    dnsServer.processNextRequest(); 
    server.handleClient();          
  }

  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 1000) {
    lastCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiLostTime == 0) wifiLostTime = millis();
      unsigned long waitTime = millis() - wifiLostTime;
      Serial.print("Offline: "); Serial.println(waitTime / 1000); 

      if (!isApMode && (waitTime > AP_TIMEOUT)) {
        Serial.println("🚨 Starting AP Mode...");
        WiFi.disconnect(); delay(100);
        WiFi.mode(WIFI_AP); WiFi.softAP(ap_ssid, NULL); 
        dnsServer.start(53, "*", WiFi.softAPIP());
        server.begin();
        isApMode = true;
      }
    } else {
      if (isApMode) {
        Serial.println("✅ WiFi Back...");
        ESP.restart(); 
      }
      wifiLostTime = 0;
    }
  }
}