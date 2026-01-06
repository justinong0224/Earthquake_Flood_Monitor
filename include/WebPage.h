#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1" charset="utf-8">
    <title>Disaster Monitor Local</title>
    <style>
        body { font-family: -apple-system, sans-serif; background: #121212; color: #e0e0e0; margin: 0; padding: 20px; text-align: center; }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; max-width: 400px; margin: auto; }
        .card { background: #1e1e1e; padding: 15px; border-radius: 15px; border: 1px solid #333; box-shadow: 0 4px 10px rgba(0,0,0,0.5); }
        .full { grid-column: span 2; }
        .value { display: block; font-size: 32px; font-weight: 800; margin-top: 5px; color: #00ff99; }
        .label { font-size: 11px; color: #888; text-transform: uppercase; letter-spacing: 1px; }
        .emergency { border-color: #ff4444 !important; background: #2a1111 !important; animation: blink 1s infinite; }
        @keyframes blink { 0% { opacity: 1; } 50% { opacity: 0.7; } 100% { opacity: 1; } }
        .emergency .value { color: #ff4444; }
        h2 { margin-bottom: 25px; font-weight: 300; letter-spacing: 2px; }
    </style>
</head>
<body>
    <h2>📡 LOCAL STATION (OFFLINE)</h2>
    <div class="grid">
        <div class="card full" id="status_card">
            <span class="label">SYSTEM SAFETY STATUS</span>
            <span class="value" id="status">CONNECTING...</span>
        </div>
        <div class="card">
            <span class="label">EST. MAGNITUDE</span>
            <span class="value" id="mag">0.0</span>
        </div>
        <div class="card">
            <span class="label">VIBRATION (g)</span>
            <span class="value" id="vib">0.00</span>
        </div>
        <div class="card">
            <span class="label">WATER RISE (cm)</span>
            <span class="value" id="water">0.0</span>
        </div>
        <div class="card">
            <span class="label">BATTERY</span>
            <span class="value" id="batt">--%</span>
        </div>
    </div>
    <script>
        function update() {
            fetch('/data').then(r => r.json()).then(d => {
                document.getElementById('mag').innerText = d.mag;
                document.getElementById('vib').innerText = d.vib;
                document.getElementById('water').innerText = d.water;
                document.getElementById('batt').innerText = d.batt + '%';
                let s = document.getElementById('status');
                let sc = document.getElementById('status_card');
                if(d.alert) {
                    s.innerText = "🚨 EMERGENCY";
                    sc.classList.add('emergency');
                } else {
                    s.innerText = "✅ SYSTEM SAFE";
                    sc.classList.remove('emergency');
                }
            }).catch(e => console.log("Station Offline"));
        }
        setInterval(update, 2000);
        update();
    </script>
</body>
</html>
)rawliteral";

#endif