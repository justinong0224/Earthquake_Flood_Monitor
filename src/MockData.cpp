#include "MockData.h"

void initSD_Mock() {
  Serial.println("(MOCK) SD Card check bypassed.");
  Serial.println("✅ SD Card Initialized (FAKE)");
}

void saveToSD_Mock(float vib, float rise, String status, int batt) {
}

String getSmartHistory_Mock(String type) {
  delay(600); 

  String report = "📜 " + type + " HISTORY LOGS\n";
  report += "---------------------\n";

  if (type == "EARTHQUAKE") {
    report += "1️⃣ 📅 2026-01-05\n⏰ Time: 14:30:22\n⏳ Dur: 12s | 📉 Avg: 1.10g\n📈 Max: 1.85g (Mag: 8.2)\n\n";
    report += "2️⃣ 📅 2026-01-04\n⏰ Time: 09:15:05\n⏳ Dur: 4s  | 📉 Avg: 0.15g\n📈 Max: 0.22g (Mag: 4.8)\n\n";
    report += "3️⃣ 📅 2026-01-02\n⏰ Time: 23:11:40\n⏳ Dur: 2s  | 📉 Avg: 0.06g\n📈 Max: 0.09g (Mag: 3.2)\n\n";
    report += "4️⃣ 2025-12-30\n⏰ Time: 10:00:00\n⏳ Dur: 1s  | 📉 Avg: 0.05g\n📈 Max: 0.06g (Mag: 2.5)\n\n";
    report += "5️⃣ 2025-12-15\n⏰ Time: 15:45:10\n⏳ Dur: 8s  | 📉 Avg: 0.30g\n📈 Max: 0.45g (Mag: 5.5)\n";
  } 
  else if (type == "FLOOD") {
    report += "1️⃣ 📅 2026-01-05\n⏰ Time: 16:00:00\n🌊 Max: 42.5cm | 📏 Avg: 28.1cm\n📈 Rate: 0.94 cm/min\n\n";
    report += "2️⃣ 📅 2026-01-03\n⏰ Time: 20:10:00\n🌊 Max: 5.2cm | 📏 Avg: 3.5cm\n📈 Rate: 0.52 cm/min\n\n";
    report += "3️⃣ 📅 2026-01-01\n⏰ Time: 08:00:00\n🌊 Max: 2.1cm | 📏 Avg: 1.2cm\n📈 Rate: 0.42 cm/min\n\n";
    report += "4️⃣ 2025-12-28\n⏰ Time: 14:22:00\n🌊 Max: 1.5cm | 📏 Avg: 0.8cm\n📈 Rate: 0.75 cm/min\n\n";
    report += "5️⃣ 2025-12-10\n⏰ Time: 03:30:00\n🌊 Max: 15.0cm | 📏 Avg: 10.0cm\n📈 Rate: 0.25 cm/min\n";
  }
  return report;
}