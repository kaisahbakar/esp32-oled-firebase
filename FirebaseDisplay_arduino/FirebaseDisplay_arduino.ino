#include <WiFi.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Firebase_ESP_Client.h>
#include <WebServer.h>

// Firebase credentials
#define API_KEY "AIzaSyAh2cMDej1uDLpLLAFlsShipkkRJCrBu64"
#define DATABASE_URL "https://iot-display-project-31818-default-rtdb.asia-southeast1.firebasedatabase.app"

// EEPROM addresses
#define EEPROM_SIZE 100
#define WIFI_SSID_ADDR 0
#define WIFI_PASS_ADDR 32
#define DEVICE_ID_ADDR 64

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Web server
WebServer server(80);

// Globals
String wifiSSID, wifiPASS, deviceID;
bool apMode = false;

// Pins
#define CONFIG_BUTTON 0
#define LED_INDICATOR 2

// OLED display helper
void showStatus(String line1, String line2 = "", String line3 = "") {
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.println(line1);
  if (line2 != "") oled.println(line2);
  if (line3 != "") oled.println(line3);
  oled.display();
}

// EEPROM read
void readEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  char s[32], p[32], d[32];
  for (int i = 0; i < 32; i++) {
    s[i] = EEPROM.read(WIFI_SSID_ADDR + i);
    p[i] = EEPROM.read(WIFI_PASS_ADDR + i);
    d[i] = EEPROM.read(DEVICE_ID_ADDR + i);
  }
  wifiSSID = String(s).c_str();
  wifiPASS = String(p).c_str();
  deviceID = String(d).c_str();
  EEPROM.end();
  wifiSSID.trim(); wifiPASS.trim(); deviceID.trim();
}

// EEPROM write
void writeEEPROM(String s, String p, String d) {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < 32; i++) {
    EEPROM.write(WIFI_SSID_ADDR + i, i < s.length() ? s[i] : 0);
    EEPROM.write(WIFI_PASS_ADDR + i, i < p.length() ? p[i] : 0);
    EEPROM.write(DEVICE_ID_ADDR + i, i < d.length() ? d[i] : 0);
  }
  EEPROM.commit();
  EEPROM.end();
}

// WiFi connect
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  return WiFi.status() == WL_CONNECTED;
}

// Web config page
void launchWebConfig() {
  String networkList = "<ul><li>Scanning...</li></ul>";
  int n = WiFi.scanNetworks();
  if (n > 0) {
    networkList = "<ul>";
    for (int i = 0; i < n; i++) {
      networkList += "<li>" + WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)</li>";
    }
    networkList += "</ul>";
  }

  server.on("/", HTTP_GET, [=]() {
    String page = "<html><body><h2>ESP32 WiFi Config</h2>"
                  "<form action='/save'>"
                  "SSID: <input name='ssid'><br>"
                  "Password: <input name='pass'><br>"
                  "Device ID: <input name='id'><br>"
                  "<input type='submit' value='Save'></form>"
                  "<h3>Available Networks:</h3>" + networkList + "</body></html>";
    server.send(200, "text/html", page);
  });

  server.on("/save", HTTP_GET, []() {
    wifiSSID = server.arg("ssid");
    wifiPASS = server.arg("pass");
    deviceID = server.arg("id");
    writeEEPROM(wifiSSID, wifiPASS, deviceID);
    server.send(200, "text/html", "<h2>Saved. Restarting...</h2>");
    delay(2000);
    ESP.restart();
  });

  server.begin();
  while (true) {
    server.handleClient();
    delay(10);
  }
}

// Start AP mode
void startAPMode() {
  digitalWrite(LED_INDICATOR, HIGH);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP32_Setup", "87654321");
  IPAddress apIP = WiFi.softAPIP();
  showStatus("AP Mode:", "ESP32_Setup", apIP.toString());
  launchWebConfig();
}

// Setup
void setup() {
  pinMode(CONFIG_BUTTON, INPUT_PULLUP);
  pinMode(LED_INDICATOR, OUTPUT);
  digitalWrite(LED_INDICATOR, LOW);
  Serial.begin(115200);

  EEPROM.begin(EEPROM_SIZE);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  showStatus("Booting...");

  readEEPROM();
  delay(2000);

  if (digitalRead(CONFIG_BUTTON) == LOW || wifiSSID.length() == 0) {
    apMode = true;
    startAPMode();
  }

  if (!connectWiFi()) {
    startAPMode();
  }

  digitalWrite(LED_INDICATOR, LOW);
  showStatus("WiFi Connected", "ID: " + deviceID);

  // Firebase setup
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("✅ Firebase signUp successful");
  } else {
    Serial.printf("❌ SignUp failed: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  server.on("/reconfigure", HTTP_GET, []() {
    server.send(200, "text/plain", "Reconfiguring...");
    delay(1000);
    startAPMode();
  });

  server.begin();
}

// Main loop
void loop() {
  server.handleClient();

  if (WiFi.status() != WL_CONNECTED) {
    showStatus("WiFi: Disconnected");
    connectWiFi();
    return;
  }

  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();

    Serial.println("➡️ Loop running...");
    Serial.println("Checking Firebase...");

    if (!Firebase.ready()) {
      Serial.println("⚠️ Firebase not ready");
      showStatus("Firebase Not Ready");
      return;
    }

    Serial.println("✅ Firebase is ready");

    if (Firebase.RTDB.getString(&fbdo, "/deviceText")) {
      String msg = fbdo.stringData();
      Serial.println("📩 Message from Firebase: " + msg);
      showStatus("Connected", msg, "ID: " + deviceID);
    } else {
      Serial.println("❌ Firebase error: " + fbdo.errorReason());
      showStatus("FB Error", fbdo.errorReason());
    }
  }
}
