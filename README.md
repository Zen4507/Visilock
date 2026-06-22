# 🔐 VisiLock: Smart Door Lock System

## Description
VisiLock is a low-cost, real-time, edge-computing Internet of Things (IoT) access control system. Built on the ESP32-CAM microcontroller, it deploys machine learning models for low-latency facial recognition and integrates a Telegram Bot API to construct an automated notification and remote-control pipeline.

This project was engineered to replace traditional, vulnerable key-based systems, achieving high facial recognition accuracy while transmitting security alerts and real-time photo logs directly to the user's smartphone

## 🛠️ Technology Stack

- Language: C++ (Arduino IDE Framework)

- Hardware:
  
  - ESP32-CAM Module (AI Thinker)
  - Relay Module (5V)
  - 12V Solenoid Lock
  - Arduino UNO (Utilized as a 3.3V to 5V step-up interface for the relay)
  - Buck Converter (Steps down 12V to 5V for the microcontrollers)
  - 12V Power Supply (3x 3.7V rechargeable Li-ion cells)
  - Red & Green Status LEDs

- APIs & Protocols: Telegram REST API, HTTP/HTTPS, Wi-Fi 802.11 b/g/n

## ✨ Key Features

- Edge-Computing Facial Recognition: Runs ML models directly on the ESP32-CAM hardware for fast, localized access control without relying on heavy cloud processing.

- Interactive Telegram Bot: Complete remote control via Telegram commands (unlock, lock, capture photo, check status).

- Automated Intruder Alerts: If an unrecognized person is detected while the door is locked, the system activates the camera flash, captures a photo, and sends an immediate 🚨 INTRUDER ALERT to the owner.

- Auto-Lock Mechanism: Automatically secures the door 10 seconds after an authorized unlock.

- Visual Status Indicators: Physical Red (Locked) and Green (Unlocked) LEDs for immediate visual feedback at the door.

## 🏗️ System Architecture
- Capture Phase: The ESP32-CAM continuously monitors the video stream. The onboard flash LED (GPIO 4) assists in low-light conditions.
  
- Inference Phase: Upon face detection, the on-board ML model extracts features and compares them against enrolled/authorized faces.
  
- Actuation Phase:
  - Authorized: The ESP32 triggers the relay (GPIO 14) and Green LED (GPIO 12), unlocking the physical door for 10 seconds.
  - Unauthorized: Access is denied, the Red LED (GPIO 13) remains on, and the system captures a high-resolution snapshot.

- Notification Phase: Snapshots, status updates, and intruder alerts are transmitted securely to the configured Telegram chat.

## 🚀 Getting Started

### Prerequisites

- Arduino IDE (with ESP32 board manager installed)

- Required Libraries: esp_camera.h, WiFi.h, WiFiClientSecure.h, UniversalTelegramBot.h, ArduinoJson.h

- A Telegram account and a registered Bot Token (via BotFather).

### Hardware Wiring

- Relay Signal: GPIO 14

- Red LED (Locked): GPIO 13

- Green LED (Unlocked): GPIO 12

- Flash LED: GPIO 4 (Onboard)

### Installation & Configuration

- Clone this repository:

    git clone [https://github.com/Zen4507/Visilock.git](https://github.com/Zen4507/Visilock.git)
    cd VisiLock


- Open the .ino sketch in the Arduino IDE.

- Update the configuration variables in the code with your specific credentials:

  `const char* ssid = "YOUR_WIFI_SSID";
  const char* password = "YOUR_WIFI_PASSWORD";
  String BOT_TOKEN = "YOUR_TELEGRAM_BOT_TOKEN";
  String CHAT_ID   = "YOUR_TELEGRAM_CHAT_ID";`


- Compile and upload the code to your ESP32-CAM.

## 📱 Telegram Bot Commands

Once the system is online, you can control it remotely via your Telegram Bot using the following commands:

- /start - Shows the command description and verifies the system is online.

- /photo - Turns on the flash, captures a live photo, and sends it to the chat.

- /unlock - Unlocks the door remotely for 10 seconds.

- /lock - Overrides the timer and locks the door immediately.

- /status - Returns live system diagnostics (WiFi IP, Door state, Face Detection state, Intruder mode).

## 🎥 Project Demo

See VisiLock in action! The video below demonstrates real-time face recognition, automated door unlocking, and remote control via the Telegram Bot.
![VisiLock Demo Video](https://img.youtube.com/vi/xCXm79TkUHc/maxresdefault.jpg)](https://youtu.be/xCXm79TkUHc)

## 🧠 What I Learned

Building VisiLock provided hands-on experience bridging the gap between hardware engineering and software integration:

- Asynchronous API Polling: Mastered integrating third-party REST APIs (Telegram Bot) using C++ WiFiClientSecure to handle commands and send photo payloads without blocking the main hardware control loop.

- Hardware Interfacing: Gained practical knowledge of voltage regulation and level shifting, specifically using a Buck Converter to step down 12V to 5V, and utilizing an Arduino UNO to step up 3.3V logic signals to safely trigger a 5V relay.

## 🔮 Future Improvements

Based on testing observations, here are planned upgrades for the next iteration:

- Low-Light Optimization: Facial recognition accuracy drops in very low luminance. We plan to integrate a dedicated IR (Infrared) illuminator module to replace the harsh white LED flash for better nighttime operation.

- Offline Logging (SD Card): Currently, photo transmission can be delayed or fail in weak Wi-Fi network conditions. Adding local SD-card logging will ensure a backup of intruder photos even if the network drops.

- Secondary Authentication: Implementing a fallback authentication method (like an RFID reader or a simple physical keypad) to ensure access is still possible during power outages or if the camera lens is obstructed.

## 🎓 Acknowledgments

This project was developed during August 2025 -- November 2025 by Rahul Kumar Das, Trishan Roy, and Umer Sagri under the guidance of Dr. Prasenjit Chanak at the Indian Institute of Technology (BHU) Varanasi.
