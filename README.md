# 📟 IoT Pager: ESP32 & Python-Based Secure MQTT Pager

<br>

A compact ESP32-based pager system using **HiveMQ Cloud** for real-time message delivery. One ESP32 acts as a **subscriber**, receiving messages over MQTT and triggering a **buzzer + OLED notification**. A companion **Python script** acts as the **publisher**, enabling interactive message transmission with live feedback and heartbeat monitoring.

> 🔧 Developed during the **AICTE SAKEC IDEALAB Summer Intership Program 2025** as a side project, it demonstrates secure MQTT messaging, ESP32 flash memory usage, and robust cross-platform communication using TLS and state-aware embedded logic.

---

## 📌 Note

> ⚠️ Credentials shown here are **for demonstration only** — for production use, store sensitive data in a header/environment file.
>
> 💾 ESP32 uses `Preferences.h` (flash memory) to **persist the last received message**, even across reboots.
>
> 🔐 All MQTT traffic is sent via **TLS-secured MQTTS (port 8883)** using **HiveMQ Cloud**.
>
> ⚙️ The system includes built-in **WiFi checks**, **MQTT reconnection logic**, and **invalid message format handling** for reliability.

---

## 📦 Dependencies

### ✅ ESP32 (Arduino) Libraries

Install via **Arduino Library Manager**:

```
- Adafruit SSD1306          → OLED driver
- Adafruit GFX              → Graphics primitives
- PubSubClient              → MQTT client
- Preferences (built-in)    → ESP32 flash storage (NVS)
- WiFi.h / WiFiClientSecure → TLS communication
- Wire.h (built-in)         → I2C communication
```

> 💾 **`Preferences.h`** is used to **save and restore the last message and timestamp** received. This ensures the pager **retains state** across resets or power cycles, maintaining the most recent context.

---

## 🧠 System Reliability & Safety

The project includes multiple built-in **fault tolerance mechanisms**:

✅ **WiFi Auto-Reconnect**

* The ESP32 continuously monitors WiFi status and attempts automatic reconnection.
* If disconnected, the OLED shows `"Wifi disconnected!"` and retries silently.

✅ **MQTT Broker Reconnection**

* If the MQTT connection drops, the ESP32 tries reconnecting to HiveMQ in a loop.
* It displays the current retry status on the OLED and Serial Monitor.

✅ **Message Format Validation**

* Incoming messages must be at least 21 characters (timestamp + message).
* Invalid messages are **discarded safely** and logged as ignored.

✅ **Heartbeat Mechanism**

* ESP32 sends `status/esp32 = online` every 10 seconds (retained message).
* Python script monitors this to detect if ESP32 goes offline.

✅ **Flash Memory Restoration**

* Upon boot, the ESP32 retrieves stored message and timestamp from NVS.
* This avoids "blank screens" and keeps the user aware of the last message.

✅ **Visual + Audible Alerts**

* On message arrival, the **OLED displays content** and **buzzer beeps** for instant acknowledgment.

---

## 🛠️ Tools & Technologies

![ESP32](https://img.shields.io/badge/ESP32-MCU-323232.svg?style=for-the-badge\&logo=espressif\&logoColor=white)
![Arduino IDE](https://img.shields.io/badge/Arduino_IDE-Programming-00979D.svg?style=for-the-badge\&logo=arduino\&logoColor=white)
![C++](https://img.shields.io/badge/C++-Firmware-00599C?style=for-the-badge\&logo=cplusplus\&logoColor=white)
![Python](https://img.shields.io/badge/Python-Scripting-3776AB.svg?style=for-the-badge\&logo=python\&logoColor=white)
![MQTT](https://img.shields.io/badge/MQTT-Protocol-FF6600.svg?style=for-the-badge)
![HiveMQ Cloud](https://img.shields.io/badge/HiveMQ_Cloud-Broker-FFCB05.svg?style=for-the-badge\&logo=hive\&logoColor=black)
![Adafruit SSD1306](https://img.shields.io/badge/Adafruit_SSD1306-Display-blue?style=for-the-badge\&logo=adafruit\&logoColor=white)
![PubSubClient](https://img.shields.io/badge/PubSubClient-MQTT_Lib-orange?style=for-the-badge)
![WiFi](https://img.shields.io/badge/WiFi-Connectivity-29ABE2?style=for-the-badge\&logo=wifi\&logoColor=white)

---

## 📁 File Structure

```
iot-pager/
├── ESP32_MQTT_SUBSCRIBER/
│   └── ESP32_MQTT_SUBSCRIBER.ino     // 🔻 Subscriber code for ESP32
├── mqtt_publisher.py                 // 🖥️ Python MQTT Publisher Script
├── images/                           // 🖼️ Output screenshots / circuit diagrams
└── README.md                         // 📘 This file
```

---

## 🧠 System Overview

### 🔻 ESP32 Subscriber Node

* Connects to WiFi and subscribes to `telemetrix/stream` topic via HiveMQ Cloud
* Displays incoming message on 128x64 OLED with timestamp
* Triggers **buzzer (GPIO 4)** and **onboard LED (GPIO 2)** on new message
* Persists last message using ESP32's internal flash via `Preferences`
* Sends periodic heartbeat (`status/esp32`) to broker every 10 seconds

---

### 🖥️ Python Publisher

* Publishes messages to the same MQTT topic with real-time timestamp
* Monitors ESP32 heartbeat and prints warnings when offline
* Beautifully logs all states (TX, Sync, Confirm, Lost, etc.) with ANSI colors
* Interactive CLI — type your message and hit enter!
* TLS-secured connection via port `8883` (MQTTS)

---

## ⚙️ Hardware Requirements

🔌 **1× ESP32 Dev Board**

🖥️ **1× OLED Display (128x64 I2C, SSD1306)**

🔔 **1× Buzzer connected to GPIO 4**

💡 **Internal LED (GPIO 2)**

📶 **WiFi Access (2.4 GHz only)**

---

## 🧾 ESP32 Configuration

| Component    | Pin Used                   |
| ------------ | -------------------------- |
| OLED (I2C)   | SDA: GPIO 21, SCL: GPIO 22 |
| Buzzer       | GPIO 4                     |
| Internal LED | GPIO 2                     |
| Baud Rate    | 115200                     |

---

## ☁️ HiveMQ Cloud Setup

1. 👉 Visit [HiveMQ Cloud Console](https://console.hivemq.cloud)
2. 🆕 Create a **Free Cluster**
3. 🔐 Go to **Access Management → Credentials** and generate:

   * MQTT Username
   * MQTT Password
4. 🔄 Go to **Topics** and ensure `telemetrix/stream` and `status/esp32` are allowed
5. 🔗 Use the following settings in your code:

   * Broker: `your-cluster-url.hivemq.cloud`
   * Port: `8883`
   * TLS: ✅ Enabled (with `client.setInsecure()` on ESP)
6. 🎯 Replace placeholders in `.ino` and `.py` with your actual credentials

---

## 🧪 Running the System

### ESP32 (Subscriber)

1. Open `ESP32_MQTT_SUBSCRIBER.ino` in Arduino IDE
2. Install dependencies from Library Manager:

   ```plaintext
   - Adafruit SSD1306
   - Adafruit GFX
   - WiFi
   - PubSubClient
   ```
3. Replace WiFi + MQTT credentials at the top
4. Select **ESP32 Dev Module**, choose COM port, and upload
5. Open Serial Monitor @ 115200 baud

### Python (Publisher)

> Requires Python ≥ 3.7

1. Install dependencies:

   ```bash
   pip install paho-mqtt
   ```
2. Run the script:

   ```bash
   python mqtt_publisher.py
   ```
3. Type any message and press Enter — ESP32 will buzz and show it on OLED.

---

### Terminal Logs (Python Publisher)

```
[SYNC] Connected to broker at c2d6...hivemq.cloud
[READY] ESP32 interface ready — enter message or type 'exit' to terminate.

>> Hello IoT!
[TXLOG] [05 Jul 25 19:45:21] Message published successfully!
[CNFRM] MQTT message acknowledged by broker.
...
```

### Serial Monitor (ESP32)

```
Connected to Wifi!
192.168.1.67

05 Jul 25 19:45:21
Hello IoT!
```

---

## 🔐 Security Note

Credentials shown are **test/demo** only.
For production deployments:

* Store credentials in `secrets.h` or `.env` file
* Avoid hardcoding sensitive keys in public repositories

---

## ⚖️ License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT).
Feel free to modify and share with attribution.

---

## 👨‍💻 Author

> **Anvay Mayekar**
> B.Tech in Electronics & Computer Science — SAKEC
>
>[![GitHub](https://img.shields.io/badge/GitHub-181717.svg?style=for-the-badge\&logo=GitHub\&logoColor=white)](https://github.com/anvaymayekar)
[![LinkedIn](https://img.shields.io/badge/LinkedIn-0A66C2.svg?style=for-the-badge\&logo=LinkedIn\&logoColor=white)](https://linkedin.com/in/anvaymayekar)
[![Instagram](https://img.shields.io/badge/Instagram-%23E4405F.svg?style=for-the-badge\&logo=Instagram\&logoColor=white)](https://instagram.com/anvaymayekar)
