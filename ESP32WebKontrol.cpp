#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>  // WebSocket için

// WiFi bilgileri
const char* ssid = "SUPERONLINE-WiFi_3516";
const char* password = "AFL7CEPKETPF";

// Pin tanımlamaları
const int ledPin = 2;           // Dahili LED
const int pwmChannel = 0;       // PWM kanalı
const int pwmFreq = 5000;       // 5kHz
const int pwmResolution = 8;    // 8 bit (0-255)

// Değişkenler
bool ledState = false;
bool blinkMode = false;
unsigned long lastBlink = 0;
int blinkInterval = 500;        // ms cinsinden yanıp sönme hızı
int ledBrightness = 255;        // Parlaklık (0-255)
float minTemp = 100, maxTemp = -100;
unsigned long lastTempUpdate = 0;

// Web sunucu ve WebSocket
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Çalışma süresi
unsigned long startTime;

// HTML sayfası - TÜM ÖZELLİKLER BİR ARADA
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32 Ultimate Control Panel</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        * { box-sizing: border-box; transition: all 0.3s ease; }
        
        /* Temalar */
        body.light {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        body.dark {
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            color: #eee;
        }
        body {
            font-family: 'Segoe UI', Arial, sans-serif;
            margin: 0;
            padding: 20px;
            min-height: 100vh;
        }
        .container {
            max-width: 600px;
            margin: auto;
        }
        .card {
            background: rgba(255,255,255,0.15);
            backdrop-filter: blur(10px);
            padding: 20px;
            margin: 15px 0;
            border-radius: 20px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.1);
        }
        button {
            font-size: 20px;
            padding: 10px 20px;
            margin: 5px;
            border: none;
            border-radius: 50px;
            cursor: pointer;
            font-weight: bold;
            transition: transform 0.2s;
        }
        button:hover { transform: scale(1.05); }
        button:active { transform: scale(0.95); }
        
        .btn-primary { background: #4CAF50; color: white; }
        .btn-danger { background: #f44336; color: white; }
        .btn-warning { background: #ff9800; color: white; }
        .btn-info { background: #2196F3; color: white; }
        
        .led-status {
            font-size: 48px;
            font-weight: bold;
            margin: 10px 0;
        }
        .led-on { color: #ffeb3b; text-shadow: 0 0 20px rgba(255,235,59,0.5); }
        .led-off { color: #666; }
        
        .temp-bar {
            width: 100%;
            height: 30px;
            background: #333;
            border-radius: 15px;
            overflow: hidden;
            margin: 10px 0;
        }
        .temp-fill {
            height: 100%;
            width: 0%;
            background: linear-gradient(90deg, #00ff00, #ff0000);
            transition: width 0.3s;
            border-radius: 15px;
        }
        
        input[type="range"] {
            width: 100%;
            margin: 10px 0;
        }
        
        .stats {
            display: flex;
            justify-content: space-around;
            text-align: center;
            flex-wrap: wrap;
        }
        .stat-box {
            background: rgba(0,0,0,0.3);
            padding: 10px;
            border-radius: 10px;
            min-width: 100px;
            margin: 5px;
        }
        
        canvas {
            max-height: 200px;
            margin: 10px 0;
        }
        
        .theme-toggle {
            position: fixed;
            top: 20px;
            right: 20px;
            background: rgba(0,0,0,0.5);
            border-radius: 50px;
            padding: 10px 15px;
            cursor: pointer;
            z-index: 100;
        }
        
        @media (max-width: 600px) {
            .stats { flex-direction: column; }
            button { font-size: 16px; padding: 8px 16px; }
        }
    </style>
</head>
<body class="light">
    <div class="theme-toggle" onclick="toggleTheme()">🌓</div>
    
    <div class="container">
        <h1>🎛️ ESP32 Ultimate Panel</h1>
        
        <!-- LED Kontrol -->
        <div class="card">
            <h2>💡 LED Kontrol</h2>
            <div id="ledStatus" class="led-status led-off">⚫ KAPALI</div>
            <button class="btn-primary" onclick="toggleLED()">🔄 AÇ/KAPAT</button>
            <button id="blinkBtn" class="btn-warning" onclick="toggleBlinkMode()">✨ Yanıp Sönme Modu</button>
            
            <h3>🔆 Parlaklık: <span id="brightnessValue">255</span></h3>
            <input type="range" id="brightness" min="0" max="255" value="255" onchange="setBrightness(this.value)">
            
            <h3>⏱️ Yanıp Sönme Hızı: <span id="intervalValue">500</span> ms</h3>
            <input type="range" id="blinkInterval" min="100" max="2000" step="50" value="500" onchange="setBlinkInterval(this.value)">
        </div>
        
        <!-- CPU Sıcaklık -->
        <div class="card">
            <h3>🌡️ CPU Sıcaklık</h3>
            <div style="font-size: 32px; font-weight: bold;"><span id="temp">---</span> °C</div>
            <div class="temp-bar">
                <div id="tempFill" class="temp-fill" style="width: 0%"></div>
            </div>
            <canvas id="tempChart"></canvas>
        </div>
        
        <!-- İstatistikler -->
        <div class="card">
            <h3>📊 İstatistikler</h3>
            <div class="stats">
                <div class="stat-box">
                    <div>📈 Max Sıcaklık</div>
                    <div style="font-size: 24px; color: #ff6b6b;"><span id="maxTemp">---</span> °C</div>
                </div>
                <div class="stat-box">
                    <div>📉 Min Sıcaklık</div>
                    <div style="font-size: 24px; color: #4ecdc4;"><span id="minTemp">---</span> °C</div>
                </div>
                <div class="stat-box">
                    <div>📶 Wi-Fi Sinyal</div>
                    <div style="font-size: 24px;"><span id="rssi">---</span> dBm</div>
                </div>
                <div class="stat-box">
                    <div>⏱️ Çalışma Süresi</div>
                    <div style="font-size: 14px;" id="uptime">---</div>
                </div>
            </div>
        </div>
        
        <!-- Hızlı Ayarlar -->
        <div class="card">
            <h3>⚡ Hızlı Ayarlar</h3>
            <button class="btn-primary" onclick="setPreset(1)">💾 Normal Mod</button>
            <button class="btn-warning" onclick="setPreset(2)">✨ Parti Modu</button>
            <button class="btn-danger" onclick="setPreset(3)">⚠️ Alarm Modu</button>
            <button class="btn-info" onclick="resetStats()">🔄 İstatistikleri Sıfırla</button>
        </div>
    </div>
    
    <script>
        let chart;
        let tempData = [];
        let timeLabels = [];
        
        // Grafik başlatma
        function initChart() {
            const ctx = document.getElementById('tempChart').getContext('2d');
            chart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: timeLabels,
                    datasets: [{
                        label: 'CPU Sıcaklık (°C)',
                        data: tempData,
                        borderColor: '#ff6b6b',
                        backgroundColor: 'rgba(255,107,107,0.1)',
                        tension: 0.3,
                        fill: true
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: true,
                    plugins: {
                        legend: { labels: { color: document.body.classList.contains('dark') ? 'white' : 'black' } }
                    }
                }
            });
        }
        
        // WebSocket bağlantısı
        let ws;
        function connectWebSocket() {
            ws = new WebSocket(`ws://${window.location.hostname}:81`);
            ws.onmessage = (event) => {
                const data = JSON.parse(event.data);
                updateUI(data);
            };
        }
        
        // Arayüz güncelleme
        function updateUI(data) {
            document.getElementById('temp').innerText = data.temp;
            document.getElementById('rssi').innerHTML = data.rssi + ' dBm';
            document.getElementById('uptime').innerText = data.uptime;
            document.getElementById('maxTemp').innerText = data.maxTemp;
            document.getElementById('minTemp').innerText = data.minTemp;
            
            // Sıcaklık çubuğu (0-80°C arası)
            const percent = Math.min(100, (data.temp / 80) * 100);
            document.getElementById('tempFill').style.width = percent + '%';
            
            // LED durumu
            const ledStatusElem = document.getElementById('ledStatus');
            if(data.ledState) {
                ledStatusElem.innerHTML = '🟢 AÇIK';
                ledStatusElem.className = 'led-status led-on';
            } else {
                ledStatusElem.innerHTML = '⚫ KAPALI';
                ledStatusElem.className = 'led-status led-off';
            }
            
            // Grafik güncelleme
            if(tempData.length > 30) {
                tempData.shift();
                timeLabels.shift();
            }
            tempData.push(data.temp);
            const now = new Date();
            timeLabels.push(now.getHours() + ':' + now.getMinutes() + ':' + now.getSeconds());
            
            if(chart) {
                chart.data.datasets[0].data = [...tempData];
                chart.data.labels = [...timeLabels];
                chart.update();
            }
            
            // Sıcaklık rengi
            const tempElem = document.getElementById('temp');
            if(data.temp > 60) tempElem.style.color = '#ff4444';
            else if(data.temp > 45) tempElem.style.color = '#ffaa00';
            else tempElem.style.color = '#4caf50';
        }
        
        // LED kontrol
        function toggleLED() {
            ws.send(JSON.stringify({action: 'toggle'}));
        }
        
        function toggleBlinkMode() {
            ws.send(JSON.stringify({action: 'blinkMode'}));
        }
        
        function setBrightness(value) {
            document.getElementById('brightnessValue').innerText = value;
            ws.send(JSON.stringify({action: 'brightness', value: parseInt(value)}));
        }
        
        function setBlinkInterval(value) {
            document.getElementById('intervalValue').innerText = value;
            ws.send(JSON.stringify({action: 'blinkInterval', value: parseInt(value)}));
        }
        
        function setPreset(mode) {
            ws.send(JSON.stringify({action: 'preset', mode: mode}));
        }
        
        function resetStats() {
            ws.send(JSON.stringify({action: 'resetStats'}));
        }
        
        // Tema değiştirme
        function toggleTheme() {
            document.body.classList.toggle('dark');
            document.body.classList.toggle('light');
            if(chart) {
                chart.options.plugins.legend.labels.color = document.body.classList.contains('dark') ? 'white' : 'black';
                chart.update();
            }
        }
        
        // Başlatma
        window.onload = () => {
            initChart();
            connectWebSocket();
        };
    </script>
</body>
</html>
)rawliteral";

// ESP32 fonksiyonları
float getCPUTemperature() {
    return temperatureRead();
}

String getUptimeString() {
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

void updateLED() {
    if(blinkMode) {
        // Yanıp sönme modu - PWM ile yumuşak geçiş
        unsigned long now = millis();
        if(now - lastBlink >= (blinkInterval / 2)) {
            lastBlink = now;
            ledState = !ledState;
            ledcWrite(pwmChannel, ledState ? ledBrightness : 0);
        }
    } else {
        ledcWrite(pwmChannel, ledState ? ledBrightness : 0);
    }
}

// WebSocket olayları
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch(type) {
        case WStype_TEXT:
            String message = String((char*)payload);
            Serial.println("WebSocket: " + message);
            
            // JSON parse basit (ArduinoJson kütüphanesi olmadan)
            if(message.indexOf("toggle") > 0) {
                ledState = !ledState;
                if(!blinkMode) ledcWrite(pwmChannel, ledState ? ledBrightness : 0);
                Serial.println(ledState ? "LED ON" : "LED OFF");
            }
            else if(message.indexOf("blinkMode") > 0) {
                blinkMode = !blinkMode;
                if(!blinkMode) ledcWrite(pwmChannel, ledState ? ledBrightness : 0);
                Serial.println(blinkMode ? "Blink mode ON" : "Blink mode OFF");
            }
            else if(message.indexOf("brightness") > 0) {
                int start = message.indexOf("value") + 6;
                int end = message.indexOf("}", start);
                ledBrightness = message.substring(start, end).toInt();
                if(!blinkMode) ledcWrite(pwmChannel, ledState ? ledBrightness : 0);
                Serial.print("Brightness: ");
                Serial.println(ledBrightness);
            }
            else if(message.indexOf("blinkInterval") > 0) {
                int start = message.indexOf("value") + 6;
                int end = message.indexOf("}", start);
                blinkInterval = message.substring(start, end).toInt();
                Serial.print("Blink interval: ");
                Serial.println(blinkInterval);
            }
            else if(message.indexOf("resetStats") > 0) {
                minTemp = 100;
                maxTemp = -100;
                Serial.println("Stats reset");
            }
            else if(message.indexOf("preset") > 0) {
                int start = message.indexOf("mode") + 5;
                int mode = message.substring(start, start+1).toInt();
                switch(mode) {
                    case 1: // Normal
                        blinkMode = false;
                        ledBrightness = 255;
                        blinkInterval = 500;
                        ledcWrite(pwmChannel, ledState ? 255 : 0);
                        break;
                    case 2: // Parti modu
                        blinkMode = true;
                        ledBrightness = 255;
                        blinkInterval = 150;
                        break;
                    case 3: // Alarm modu
                        blinkMode = true;
                        ledBrightness = 255;
                        blinkInterval = 1000;
                        break;
                }
                Serial.print("Preset mode: ");
                Serial.println(mode);
            }
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // PWM ayarları
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(ledPin, pwmChannel);
    ledcWrite(pwmChannel, 0);
    
    startTime = millis();
    
    // WiFi bağlantısı
    Serial.println("\n🔌 ESP32 Ultimate Panel Başlatılıyor...");
    WiFi.begin(ssid, password);
    Serial.print("WiFi'ya bağlanıyor: ");
    Serial.println(ssid);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ Bağlandı!");
        Serial.print("📡 IP Adresi: http://");
        Serial.println(WiFi.localIP());
        Serial.print("📶 Sinyal: ");
        Serial.println(WiFi.RSSI());
    } else {
        Serial.println("\n❌ Bağlanamadı!");
        return;
    }
    
    // Web sunucu
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", index_html);
    });
    server.begin();
    
    // WebSocket başlat
    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);
    
    Serial.println("🚀 WebSocket ve Web sunucu hazır!");
    Serial.println("==================================");
}

void loop() {
    webSocket.loop();
    updateLED();
    
    // Saniyede bir veri gönder
    static unsigned long lastSend = 0;
    if(millis() - lastSend >= 1000) {
        lastSend = millis();
        
        float currentTemp = getCPUTemperature();
        if(currentTemp < minTemp) minTemp = currentTemp;
        if(currentTemp > maxTemp) maxTemp = currentTemp;
        
        String json = "{";
        json += "\"temp\":" + String(currentTemp, 1) + ",";
        json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
        json += "\"uptime\":\"" + getUptimeString() + "\",";
        json += "\"maxTemp\":" + String(maxTemp, 1) + ",";
        json += "\"minTemp\":" + String(minTemp, 1) + ",";
        json += "\"ledState\":" + String(ledState ? "true" : "false");
        json += "}";
        
        webSocket.broadcastTXT(json);
    }
    
    delay(10);
}
