/*
 * ESP32 WROOM 433MHz RF Transmitter with Web Config
 *
 * Transmits configurable data via a 433MHz RF module. Control from a phone
 * by connecting to the ESP32's WiFi AP and opening the web interface.
 *
 * Uses software OOK (no RadioHead) to avoid ESP32 watchdog/timer issues.
 * Board: ESP32 Dev Module (DOIT ESP32 DEVKIT V1 or similar ESP32 WROOM board)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ============== PIN CONFIGURATION (ESP32 WROOM → 433MHz module) ==============
#define RF_TX_PIN  5   // Connect to 433MHz module DATA pin (VCC→3.3V, GND→GND)

// ============== WIFI AP (phone connects here) ==============
#define WIFI_AP_SSID      "ESP32-Xmitter-1"
#define WIFI_AP_PASSWORD  "xmitter1"   // Min 8 characters
#define WEB_SERVER_PORT   80

// ============== RF & TRANSMISSION DEFAULTS ==============
#define PAYLOAD_MAX_LEN   60
#define BURST_DURATION_MS  5000
#define BURST_INTERVAL_MS  2000

// Software OOK timing (no timer; avoids watchdog reset)
#define OOK_BIT_US        500   // microseconds per bit (~2000 bps)
#define OOK_HIGH_US       250
#define OOK_PREAMBLE_PULSES 16

// ============== GLOBALS ==============
WebServer server(WEB_SERVER_PORT);
Preferences prefs;

char payloadText[PAYLOAD_MAX_LEN + 1] = "Hello 433";
unsigned long burstDurationMs = BURST_DURATION_MS;
unsigned long burstIntervalMs = BURST_INTERVAL_MS;
bool transmitting = false;
unsigned long burstStartMs = 0;
unsigned long lastBurstEndMs = 0;

// ============== HTML PAGE (embedded) ==============
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 433MHz Xmitter</title>
  <style>
    *{box-sizing:border-box}
    body{font-family:sans-serif;max-width:420px;margin:20px auto;padding:12px;background:#1a1a2e;color:#eee}
    h1{font-size:1.3rem;color:#0f0}
    label{display:block;margin:10px 0 4px;color:#aaa}
    input,textarea,button{width:100%;padding:10px;margin-bottom:8px;border:1px solid #333;border-radius:6px;background:#16213e;color:#fff;font-size:1rem}
    textarea{min-height:80px;resize:vertical}
    button{cursor:pointer;font-weight:bold;border:none}
    .btn-start{background:#0a0;color:#000}
    .btn-stop{background:#a00;color:#fff}
    .status{padding:8px;border-radius:6px;margin:10px 0;font-weight:bold}
    .status-idle{background:#333;color:#888}
    .status-tx{background:#0a0;color:#000}
    .saved{color:#0f0;font-size:0.9rem;margin-top:4px}
  </style>
</head>
<body>
  <h1>433MHz Transmitter 1 Config</h1>
  <div id="status" class="status status-idle">Status: Idle</div>
  <form action="/save" method="POST">
    <label>Payload (text to send, max 60 chars)</label>
    <textarea name="payload" id="payload" maxlength="60" placeholder="Enter text to transmit">%PAYLOAD%</textarea>
    <label>Burst duration (seconds)</label>
    <input type="number" name="burst_sec" id="burst_sec" min="1" max="3600" value="%BURST_SEC%" step="1">
    <label>Interval between bursts (milliseconds)</label>
    <input type="number" name="interval_ms" id="interval_ms" min="100" max="60000" value="%INTERVAL_MS%" step="100">
    <button type="submit">Save settings</button>
  </form>
  <p id="saved" class="saved" style="display:none">Settings saved.</p>
  <p>
    <button type="button" id="btnToggle" class="btn-start" onclick="toggleTx()">Start transmission</button>
  </p>
  <script>
    function toggleTx(){
      var btn = document.getElementById('btnToggle');
      var isStart = btn.textContent.indexOf('Start') >= 0;
      fetch(isStart ? '/start' : '/stop', { method: 'POST' })
        .then(function(){ updateStatus(); setTimeout(updateStatus, 400); })
        .catch(function(){ updateStatus(); });
    }
    function updateStatus(){
      fetch('/status?t=' + Date.now()).then(r=>r.json()).then(function(d){
        var el = document.getElementById('status');
        var btn = document.getElementById('btnToggle');
        if(d.transmitting){
          el.className = 'status status-tx'; el.textContent = 'Status: Transmitting';
          btn.className = 'btn-stop'; btn.textContent = 'Stop transmission';
        } else {
          el.className = 'status status-idle'; el.textContent = 'Status: Idle';
          btn.className = 'btn-start'; btn.textContent = 'Start transmission';
        }
      }).catch(function(){});
    }
    setInterval(updateStatus, 1500);
    updateStatus();
    var params = new URLSearchParams(window.location.search);
    if(params.get('saved')){ document.getElementById('saved').style.display='block'; setTimeout(function(){ document.getElementById('saved').style.display='none'; }, 3000); }
  </script>
</body>
</html>
)rawliteral";

// ============== HELPERS ==============
void replacePlaceholder(String& html, const String& key, const String& value) {
  html.replace("%" + key + "%", value);
}

String escapeHtml(const String& s) {
  String out;
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    if (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '"') out += "&quot;";
    else out += c;
  }
  return out;
}

void loadSettings() {
  prefs.begin("xmitter", true);
  String p = prefs.getString("payload", "Hello 433");
  p.toCharArray(payloadText, sizeof(payloadText));
  burstDurationMs = prefs.getULong("burstMs", BURST_DURATION_MS);
  burstIntervalMs = prefs.getULong("intervalMs", BURST_INTERVAL_MS);
  prefs.end();
}

void saveSettings() {
  prefs.begin("xmitter", false);
  prefs.putString("payload", String(payloadText));
  prefs.putULong("burstMs", burstDurationMs);
  prefs.putULong("intervalMs", burstIntervalMs);
  prefs.end();
}

// ============== WEB HANDLERS ==============
void handleRoot() {
  String html = FPSTR(INDEX_HTML);
  replacePlaceholder(html, "PAYLOAD", escapeHtml(String(payloadText)));
  replacePlaceholder(html, "BURST_SEC", String(burstDurationMs / 1000));
  replacePlaceholder(html, "INTERVAL_MS", String(burstIntervalMs));
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("payload")) {
    String p = server.arg("payload");
    p.trim();
    if (p.length() > 0) {
      p.toCharArray(payloadText, sizeof(payloadText));
    }
  }
  if (server.hasArg("burst_sec")) {
    long sec = server.arg("burst_sec").toInt();
    if (sec >= 1 && sec <= 3600) burstDurationMs = (unsigned long)sec * 1000;
  }
  if (server.hasArg("interval_ms")) {
    long ms = server.arg("interval_ms").toInt();
    if (ms >= 100 && ms <= 60000) burstIntervalMs = (unsigned long)ms;
  }
  saveSettings();
  server.sendHeader("Location", "/?saved=1");
  server.send(302, "text/plain", "");
}

void handleStart() {
  transmitting = true;
  burstStartMs = millis();
  lastBurstEndMs = 0;
  Serial.println(F("Start transmission"));
  server.send(200, "text/plain", "OK");
}

void handleStop() {
  transmitting = false;
  Serial.println(F("Stop transmission"));
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  String json = "{\"transmitting\":";
  json += transmitting ? "true" : "false";
  json += "}";
  server.send(200, "application/json", json);
}

// ============== SOFTWARE OOK TRANSMIT (no RadioHead; no timer = no watchdog reset) ==============
static void sendOOKBit(bool one) {
  digitalWrite(RF_TX_PIN, HIGH);
  delayMicroseconds(OOK_HIGH_US);
  digitalWrite(RF_TX_PIN, LOW);
  delayMicroseconds(one ? (OOK_BIT_US - OOK_HIGH_US) : (OOK_BIT_US * 3 - OOK_HIGH_US));
}

static void sendOOKByte(uint8_t b) {
  for (int i = 0; i < 8; i++) {
    sendOOKBit(b & 1);
    b >>= 1;
  }
}

void doRfTransmit() {
  size_t len = strnlen(payloadText, PAYLOAD_MAX_LEN);
  if (len == 0) return;
  // Preamble
  for (int i = 0; i < OOK_PREAMBLE_PULSES; i++) {
    digitalWrite(RF_TX_PIN, HIGH);
    delayMicroseconds(OOK_HIGH_US);
    digitalWrite(RF_TX_PIN, LOW);
    delayMicroseconds(OOK_BIT_US - OOK_HIGH_US);
  }
  sendOOKByte((uint8_t)len);
  for (size_t i = 0; i < len; i++)
    sendOOKByte((uint8_t)payloadText[i]);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("ESP32 WROOM 433MHz Xmitter"));

  loadSettings();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
  IPAddress ip = WiFi.softAPIP();
  Serial.print(F("AP IP: "));
  Serial.println(ip);
  Serial.print(F("Connect your phone to WiFi: "));
  Serial.println(WIFI_AP_SSID);
  Serial.println(F("Then open: http://192.168.4.1"));

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/start", handleStart);   // accept GET or POST so Start button works
  server.on("/stop", handleStop);
  server.on("/status", handleStatus);
  server.begin();
  pinMode(RF_TX_PIN, OUTPUT);
  digitalWrite(RF_TX_PIN, LOW);
  Serial.println(F("Web server started."));
}

void loop() {
  server.handleClient();  // Always handle web requests so Stop button works

  if (!transmitting) {
    delay(10);
    return;
  }

  unsigned long now = millis();

  // After a burst, wait for the interval before next burst
  if (lastBurstEndMs != 0 && (now - lastBurstEndMs) < burstIntervalMs) {
    delay(20);
    return;
  }
  if (lastBurstEndMs != 0 && (now - lastBurstEndMs) >= burstIntervalMs) {
    burstStartMs = now;
    lastBurstEndMs = 0;
  }

  // During burst: send payload repeatedly until burst duration elapsed
  if ((now - burstStartMs) < burstDurationMs) {
    doRfTransmit();
    delay(50);  // Small delay between packets to avoid flooding
  } else {
    lastBurstEndMs = now;
  }
}
