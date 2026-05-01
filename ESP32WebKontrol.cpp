#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// WiFi bilgileri (KENDI BILGILERINIZI GIRIN)
const char* ssid = "SUPERONLINE-WiFi_3516";
const char* password = "AFL7CEPKETPF";

const int ledPin = 2;
bool ledState = false;
AsyncWebServer server(80);
unsigned long startTime;

float getInternalTemperature() {
  return temperatureRead();
}

String getUptime() {
  unsigned long seconds = millis() / 1000;
  int days = seconds / 86400;
  seconds %= 86400;
  int hours = seconds / 3600;
  seconds %= 3600;
  int minutes = seconds / 60;
  seconds %= 60;
  char buf[32];
  sprintf(buf, "%dg %02ds %02dd %02dsn", days, hours, minutes, seconds);
  return String(buf);
}

// HTML - Turkce karakter sorunu olmayan versiyon
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32 LED Control</title>
    <style>
        * { box-sizing: border-box; }
        body { 
            font-family: 'Segoe UI', Arial, sans-serif; 
            text-align: center; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            color: white;
            margin: 0;
            padding: 20px;
        }
        .container {
            max-width: 500px;
            margin: auto;
        }
        .card { 
            background: rgba(255,255,255,0.2); 
            backdrop-filter: blur(10px);
            padding: 25px; 
            margin: 20px 0; 
            border-radius: 20px; 
            box-shadow: 0 8px 32px rgba(0,0,0,0.1);
        }
        button { 
            font-size: 24px; 
            padding: 12px 30px; 
            border: none;
            border-radius: 50px;
            background: #4CAF50;
            color: white;
            cursor: pointer;
            transition: transform 0.2s;
            font-weight: bold;
        }
        button:hover { 
            transform: scale(1.05);
            background: #45a049;
        }
        button:active {
            transform: scale(0.95);
        }
        .status { 
            font-weight: bold; 
            font-size: 32px;
            margin: 15px 0;
        }
        .on { color: #ffeb3b; text-shadow: 0 0 10px rgba(255,235,59,0.5); }
        .off { color: #9e9e9e; }
        .info-text { 
            font-size: 18px; 
            margin: 12px 0;
            padding: 8px;
            background: rgba(0,0,0,0.2);
            border-radius: 10px;
        }
        h1 { font-size: 1.8em; margin-bottom: 10px; }
        h2 { font-size: 1.4em; margin: 0 0 15px 0; }
        h3 { font-size: 1.2em; margin: 0 0 15px 0; }
        .badge {
            background: rgba(0,0,0,0.3);
            padding: 5px 12px;
            border-radius: 20px;
            font-size: 12px;
            display: inline-block;
            margin-bottom: 10px;
        }
    </style>
    <script>
        function fetchData() {
            fetch("/data")
                .then(response => response.json())
                .then(data => {
                    document.getElementById("ledStatus").innerText = data.led ? "ON" : "OFF";
                    document.getElementById("ledStatus").className = data.led ? "status on" : "status off";
                    document.getElementById("temp").innerHTML = data.temp + " <span style='font-size:14px'>°C</span>";
                    document.getElementById("rssi").innerHTML = data.rssi + " <span style='font-size:14px'>dBm</span>";
                    document.getElementById("uptime").innerText = data.uptime;
                    
                    // Signal quality color
                    const rssiElem = document.getElementById("rssi");
                    if(data.rssi > -50) rssiElem.style.color = "#4caf50";
                    else if(data.rssi > -70) rssiElem.style.color = "#ffc107";
                    else rssiElem.style.color = "#f44336";
                });
        }
        
        function toggleLED() {
            fetch("/toggle")
                .then(() => fetchData());
        }
        
        setInterval(fetchData, 1000);
        window.onload = fetchData;
    </script>
</head>
<body>
    <div class="container">
        <h1>🎛️ ESP32 Control Panel</h1>
        <div class="badge">✨ No extra hardware needed ✨</div>
        
        <div class="card">
            <h2>💡 Built-in LED</h2>
            <div id="ledStatus" class="status off">OFF</div>
            <button onclick="toggleLED()">🔘 TOGGLE LED</button>
        </div>
        
        <div class="card">
            <h3>📊 System Info</h3>
            <div class="info-text">🌡️ <strong>CPU Temperature:</strong> <span id="temp">---</span></div>
            <div class="info-text">📶 <strong>Wi-Fi Signal:</strong> <span id="rssi">---</span></div>
            <div class="info-text">⏱️ <strong>Uptime:</strong> <span id="uptime">---</span></div>
        </div>
        
        <div class="card">
            <h3>🎯 What you can do (no extra hardware)</h3>
            <div style="font-size:13px; text-align:left;">
                ✓ Control built-in LED from anywhere<br>
                ✓ Monitor real CPU temperature<br>
                ✓ Check Wi-Fi signal strength<br>
                ✓ Track device uptime<br>
                ✓ Auto-refresh every second<br>
                ✓ OTA updates (wireless programming)
            </div>
        </div>
    </div>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  ledState = false;
  
  Serial.println("\n==================================");
  Serial.println("ESP32 Web Panel Starting...");
  Serial.println("==================================");
  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Connected!");
    Serial.print("📡 IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("📶 Signal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\n❌ Connection failed!");
    return;
  }
  
  startTime = millis();
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"led\":" + String(ledState ? "true" : "false") + ",";
    json += "\"temp\":" + String(getInternalTemperature(), 1) + ",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"uptime\":\"" + getUptime() + "\"";
    json += "}";
    request->send(200, "application/json", json);
  });
  
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
    ledState = !ledState;
    digitalWrite(ledPin, ledState ? HIGH : LOW);
    Serial.println(ledState ? "LED turned ON" : "LED turned OFF");
    request->send(200, "text/plain", "OK");
  });
  
  server.begin();
  Serial.println("\n🚀 Web server ready!");
  Serial.print("👉 Open browser at: http://");
  Serial.println(WiFi.localIP());
  Serial.println("==================================\n");
}

void loop() {
  delay(10);
}
