#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

// Program for ESP32 acting as an MQTT subscriber
// author: @anvaymayekar

// OLED constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // Initializes 128x64 OLED via I2C using Wire

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// MQTT Broker (e.g., HiveMQ Cloud)
const char* mqtt_server = "your-broker-url.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "your-mqtt-username";
const char* mqtt_pass = "your-mqtt-password";
const char* mqtt_topic = "telemetrix/stream";

// Credentials for testing only — use secure methods (e.g., header file) in production.

// GPIO pins
const int LED = 2;
const int BUZZER = 4;

// Clients
WiFiClientSecure net;
PubSubClient client(net);

Preferences prefs;  // Used to store and retrieve persistent data (message, timestamp) in flash memory

// Variables to persist the last received message and timestamp
String lastMessage = "";
String lastTimestamp = "";
bool messageReceivedOnce = false;

// helper function prototypes
void setup_WiFi();
void buzz(unsigned int period = 1000);
void showMessage(String title, String text = "", bool gap = true, int size = 1);
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void sendHeartbeat(unsigned long interval = 10000);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Initialize the OLED display with I2C address 0x3C
  // If initialization fails, print error and halt the program
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)  // Infinite loop to stop execution
      ;
  }

  // Wait for WiFi setup
  handle_WiFi();

  net.setInsecure();  // disables certificate verification

  // Set the MQTT broker address and port
  client.setServer(mqtt_server, mqtt_port);

  // Set the function to handle incoming MQTT messages
  client.setCallback(callback);
  delay(1000);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    sendHeartbeat();
  } else {
    String code = "Wifi disconnected!";
    String next = "Retrying...";
    showMessage(code, next);
    digitalWrite(LED, LOW);
    handle_WiFi();
  }
}

void handle_WiFi() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  showMessage("Connecting to Wifi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  showMessage("Connected to Wifi! ", WiFi.SSID(), true);
  Serial.println(WiFi.localIP());
  Serial.println();
  display.println(WiFi.localIP());
  display.display();
  digitalWrite(LED, HIGH);
  delay(1000);
}


void buzz(unsigned int period) {
  digitalWrite(BUZZER, HIGH);
  delay(period);
  digitalWrite(BUZZER, LOW);
}

void showMessage(String title, String text, bool gap, int size) {
  Serial.println(title);
  if (text.length() > 0) Serial.println(text + "\n");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(title);
  if (gap) display.println();
  display.setTextSize(size);
  display.println(text);
  display.setTextSize(1);
  display.display();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String fullMessage = "";
  for (unsigned int i = 0; i < length; i++) {
    fullMessage += (char)payload[i];
  }

  if (fullMessage.length() > 21) {
    lastTimestamp = fullMessage.substring(0, 20);  // First 20 = timestamp
    lastMessage = fullMessage.substring(21);       // After the space

    // Save message to flash
    prefs.putString("msg", lastMessage);
    prefs.putString("timestamp", lastTimestamp);
    prefs.putBool("has_msg", true);
    messageReceivedOnce = true;

    showMessage(lastTimestamp, lastMessage);
    buzz();
  } else {
    Serial.println("Invalid message format received...Ignored!");
  }
}



void reconnect() {
  String title;
  String subtitle;

  while (!client.connected()) {
    // Show initial connection attempt
    title = "Connecting to";
    subtitle = "MQTT broker...";
    showMessage(title, subtitle, false);

    // Sends periodic "online" heartbeat to MQTT broker
    sendHeartbeat();

    // Attempt connection
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      title = "Connected to";
      subtitle = String(mqtt_user);

      client.subscribe(mqtt_topic);
      client.publish("status/esp32", "online");

      showMessage(title, subtitle, false, 2);
      restoreMessage();
      delay(1000);
      break;
    } else {
      title = "Failed to connect to";
      subtitle = String(mqtt_user);
      showMessage(title, subtitle, false, 2);

      delay(5000);
    }
  }
}


// Loads and displays the last saved message from flash, or a default if none exists.
void restoreMessage() {
  prefs.begin("oled-msg", false);
  bool hasMessage = prefs.getBool("has_msg", false);

  if (hasMessage) {
    lastTimestamp = prefs.getString("timestamp", "");
    lastMessage = prefs.getString("msg", "");
    messageReceivedOnce = true;
  } else {
    lastTimestamp = "MQTT Ready!";
    lastMessage = "No message yet";
    messageReceivedOnce = false;
  }
  showMessage(lastTimestamp, lastMessage);
  buzz();
}

void sendHeartbeat(unsigned long interval) {
  static unsigned long lastSent = 0;
  if (millis() - lastSent >= interval) {
    client.publish("status/esp32", "online", true);  // `retain=true` to persist status
    lastSent = millis();
  }
}