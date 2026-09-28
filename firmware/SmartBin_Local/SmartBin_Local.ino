// ====================================================================
// Project: SmartBin Container Level Monitor (SmartBin-1)
// Mode: Pure Local Access Point (Standalone Network)
// Institution: VIT BHOPAL UNIVERSITY
// Course/Group: MEE2014 - Group 3
// Authors: SHIVAM SINGH & ARYAN SINGH
// Board: ESP32 DevKit V1
// ====================================================================

#include <WiFi.h>
#include <WebServer.h>

// --------------------------------------------------------------------
// Local Standalone Hotspot Credentials
// --------------------------------------------------------------------
const char* AP_SSID = "SmartBin-WiFi";
const char* AP_PASS = "12345678";

// --------------------------------------------------------------------
// Hardware Pin Mappings
// --------------------------------------------------------------------
const int TRIG_PIN       = 5;    // GPIO5 / D5
const int ECHO_PIN       = 18;   // GPIO18 / D18 (via Voltage Divider)
const int GREEN_LED_PIN  = 14;  // GPIO14 / D14
const int YELLOW_LED_PIN = 27;  // GPIO27 / D27
const int RED_LED_PIN    = 26;  // GPIO26 / D26
const int BUZZER_PIN     = 25;  // GPIO25 / D25

// --------------------------------------------------------------------
// Container Configuration (Calibrated 14.4cm Depth)
// --------------------------------------------------------------------
const float BIN_DEPTH_CM      = 14.4; // Depth of empty bin
const float FULL_THRESHOLD_CM = 4.0;  // Threshold distance for FULL state

// Telemetry Variables
float distance_cm = 14.4;
int fill_percentage = 0;
String status_msg = "OK 🟢";

WebServer server(80);

// Embedded Web Dashboard Interface
const char HTML_DASHBOARD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>SmartBin-1 Control Center</title>
  <style>
    :root {
      --bg-color: #0b1329;
      --card-bg: #16203a;
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --accent-green: #22c55e;
      --accent-yellow: #eab308;
      --accent-red: #ef4444;
      --border-color: #2a3756;
    }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: var(--bg-color);
      color: var(--text-main);
      margin: 0;
      padding: 16px;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      box-sizing: border-box;
    }
    .app-card {
      background: var(--card-bg);
      width: 100%;
      max-width: 480px;
      padding: 24px;
      border-radius: 24px;
      box-shadow: 0 20px 40px rgba(0,0,0,0.6);
      border: 1px solid var(--border-color);
    }
    .header { text-align: center; margin-bottom: 16px; }
    .header h1 { font-size: 26px; margin: 0; color: #38bdf8; letter-spacing: 0.5px; }
    .tag-badge {
      display: inline-block;
      background: #1e293b;
      color: #cbd5e1;
      font-size: 11px;
      padding: 4px 10px;
      border-radius: 12px;
      margin-top: 6px;
      font-weight: 600;
      border: 1px solid var(--border-color);
    }
    .location-info {
      font-size: 12px;
      color: var(--text-muted);
      margin-top: 6px;
      font-weight: 500;
    }
    .bin-container {
      width: 120px;
      height: 190px;
      border: 4px solid #475569;
      border-radius: 0 0 18px 18px;
      margin: 16px auto;
      position: relative;
      overflow: hidden;
      background: #090d16;
    }
    .bin-fill-level {
      width: 100%;
      position: absolute;
      bottom: 0;
      height: 0%;
      background: var(--accent-green);
      transition: height 0.5s ease, background-color 0.5s ease;
    }
    .metrics-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
      margin: 16px 0;
    }
    .metric-card {
      background: #0d1527;
      padding: 12px;
      border-radius: 12px;
      border: 1px solid var(--border-color);
      text-align: center;
    }
    .metric-label { font-size: 10px; color: var(--text-muted); font-weight: 700; text-transform: uppercase; }
    .metric-val { font-size: 20px; font-weight: 700; margin-top: 4px; }
    .status-pill {
      text-align: center;
      padding: 8px 16px;
      border-radius: 16px;
      font-size: 15px;
      font-weight: 700;
      background: #1e293b;
      margin: 10px 0;
      border: 1px solid var(--border-color);
    }
    .info-box {
      background: #0d1527;
      border: 1px solid var(--border-color);
      border-radius: 12px;
      padding: 12px;
      margin-top: 12px;
      font-size: 12px;
    }
    .info-row { display: flex; justify-content: space-between; margin-bottom: 6px; }
    .info-row:last-child { margin-bottom: 0; }
    .history-log {
      max-height: 90px;
      overflow-y: auto;
      background: #090d16;
      border: 1px solid var(--border-color);
      border-radius: 8px;
      padding: 8px;
      margin-top: 8px;
      font-size: 11px;
      color: #cbd5e1;
    }
    .history-item { padding: 3px 0; border-bottom: 1px solid #1e293b; }
    .history-item:last-child { border-bottom: none; }
    .clean-btn {
      width: 100%;
      background: #0284c7;
      color: #fff;
      border: none;
      padding: 10px;
      border-radius: 10px;
      font-weight: 700;
      cursor: pointer;
      margin-top: 10px;
      transition: background 0.2s;
    }
    .clean-btn:active { background: #0369a1; }
    .footer {
      text-align: center;
      margin-top: 18px;
      padding-top: 12px;
      border-top: 1px solid var(--border-color);
      font-size: 11px;
      color: var(--text-muted);
    }
    .authors { font-weight: 700; color: #38bdf8; margin-top: 4px; }
  </style>
</head>
<body>

<div class="app-card">
  <div class="header">
    <h1>SmartBin-1</h1>
    <div class="tag-badge">MEE2014 — GROUP 3</div>
    <div class="location-info">📍 VIT BHOPAL UNIVERSITY</div>
  </div>

  <div class="bin-container">
    <div id="fillVisual" class="bin-fill-level"></div>
  </div>

  <div id="pillStatus" class="status-pill">STATUS: CONNECTED</div>

  <div class="metrics-grid">
    <div class="metric-card">
      <div class="metric-label">Distance</div>
      <div id="valDistance" class="metric-val">-- cm</div>
    </div>
    <div class="metric-card">
      <div class="metric-label">Fill Level</div>
      <div id="valFill" class="metric-val">-- %</div>
    </div>
  </div>

  <div class="info-box">
    <div class="info-row">
      <span>Collection Priority:</span>
      <strong id="valPriority" style="color: var(--accent-green);">LOW</strong>
    </div>
    <div class="info-row">
      <span>Hardware Status:</span>
      <strong style="color: var(--accent-green);">ONLINE (LOCAL AP)</strong>
    </div>
    <div class="info-row">
      <span>Last Cleaned:</span>
      <strong id="valLastCleaned">Not recorded yet</strong>
    </div>
  </div>

  <div class="info-box">
    <div class="info-row">
      <span><strong>Cleaning History Log:</strong></span>
    </div>
    <div id="historyLog" class="history-log">
      <div class="history-item">System booted & monitoring initialized.</div>
    </div>
    <button class="clean-btn" onclick="logManualCleaning()">🧹 Mark as Cleaned Now</button>
  </div>

  <div class="footer">
    Copyright © 2026 SmartBin Systems
    <div class="authors">SHIVAM SINGH & ARYAN SINGH</div>
  </div>
</div>

<script>
  let lastWasFull = false;
  let lastCleanedTime = "On Initialization";

  function updateTimestamp() {
    const d = new Date();
    return d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
  }

  function logManualCleaning() {
    lastCleanedTime = updateTimestamp();
    document.getElementById('valLastCleaned').innerText = lastCleanedTime;
    
    const log = document.getElementById('historyLog');
    const item = document.createElement('div');
    item.className = 'history-item';
    item.style.color = 'var(--accent-green)';
    item.innerText = `[${lastCleanedTime}] Manual cleaning verified.`;
    log.prepend(item);
  }

  setInterval(async () => {
    try {
      const res = await fetch('/data');
      const data = await res.json();

      document.getElementById('valDistance').innerText = data.distance.toFixed(1) + ' cm';
      document.getElementById('valFill').innerText = data.fill + ' %';

      const visual = document.getElementById('fillVisual');
      const pill = document.getElementById('pillStatus');
      const priority = document.getElementById('valPriority');

      visual.style.height = data.fill + '%';

      if (data.fill >= 80) {
        visual.style.backgroundColor = 'var(--accent-red)';
        pill.innerText = 'STATUS: FULL 🚨';
        pill.style.color = 'var(--accent-red)';
        priority.innerText = 'HIGH (ACTION REQ.)';
        priority.style.color = 'var(--accent-red)';
        lastWasFull = true;
      } else if (data.fill >= 50) {
        visual.style.backgroundColor = 'var(--accent-yellow)';
        pill.innerText = 'STATUS: HALF FULL 🟡';
        pill.style.color = 'var(--accent-yellow)';
        priority.innerText = 'MEDIUM';
        priority.style.color = 'var(--accent-yellow)';
      } else {
        visual.style.backgroundColor = 'var(--accent-green)';
        pill.innerText = 'STATUS: OK 🟢';
        pill.style.color = 'var(--accent-green)';
        priority.innerText = 'LOW';
        priority.style.color = 'var(--accent-green)';

        if (lastWasFull) {
          lastWasFull = false;
          lastCleanedTime = updateTimestamp();
          document.getElementById('valLastCleaned').innerText = lastCleanedTime;
          
          const log = document.getElementById('historyLog');
          const item = document.createElement('div');
          item.className = 'history-item';
          item.style.color = 'var(--accent-green)';
          item.innerText = `[${lastCleanedTime}] Auto-Detected: Bin emptied by staff.`;
          log.prepend(item);
        }
      }
    } catch (e) {
      console.log('Polling telemetry...');
    }
  }, 500);
</script>

</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", HTML_DASHBOARD);
}

void handleData() {
  String json = "{";
  json += "\"distance\":" + String(distance_cm, 1) + ",";
  json += "\"fill\":" + String(fill_percentage) + ",";
  json += "\"status\":\"" + status_msg + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configure Pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Hardware Boot Test
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(YELLOW_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, HIGH);
  delay(1200);
  
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  noTone(BUZZER_PIN);

  // Start Local Access Point Mode directly
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.println("\n==================================================");
  Serial.println(" SmartBin Standalone Hotspot Active ");
  Serial.println(" Connect Phone Wi-Fi to: SmartBin-WiFi");
  Serial.println(" Password: " + String(AP_PASS));
  Serial.print(" Open Browser URL: http://");
  Serial.println(WiFi.softAPIP()); // http://192.168.4.1
  Serial.println("==================================================");

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();

  // 1. Ultrasonic Distance Measurement
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    distance_cm = BIN_DEPTH_CM;
    fill_percentage = 0;
  } else {
    distance_cm = duration * 0.0343 / 2.0;

    if (distance_cm > BIN_DEPTH_CM) distance_cm = BIN_DEPTH_CM;
    if (distance_cm < 2.0) distance_cm = 2.0;

    fill_percentage = round(((BIN_DEPTH_CM - distance_cm) / (BIN_DEPTH_CM - FULL_THRESHOLD_CM)) * 100.0);
    fill_percentage = constrain(fill_percentage, 0, 100);
  }

  // 2. Hardware Alert State Logic
  if (fill_percentage >= 80) {
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, HIGH);
    tone(BUZZER_PIN, 2500);
    status_msg = "FULL 🚨";
  } else if (fill_percentage >= 50) {
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, LOW);
    noTone(BUZZER_PIN);
    status_msg = "HALF FULL 🟡";
  } else {
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
    noTone(BUZZER_PIN);
    status_msg = "OK 🟢";
  }

  delay(200);
}
