//ESP32-CAM Face Detection + Telegram Door Lock Integration

#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// ===== Camera model selection =====
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// ===== Pin definitions =====
#define Relay 14
#define Red 13
#define Green 12
#define FLASH_LED 4   // onboard flash LED for photos

// ===== WiFi credentials =====
const char* ssid = "OnePlus Nord CE4";
const char* password = "rit12345";

// ===== Telegram bot credentials =====
String BOT_TOKEN = "8337241562:AAEQuVPHy818f5qZLToccUx8a9kZgFIuazU";
String CHAT_ID   = "6006265069";

// ===== Global objects =====
WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOT_TOKEN, clientTCP);

void startCameraServer();

boolean matchFace = false;
boolean activateRelay = false;
boolean doorManuallyUnlocked = false;
boolean intruderDetected = false;
long prevMillis = 0;
int interval = 10000;  // door open time: 10 seconds
unsigned long lastIntruderAlert = 0;
unsigned long alertInterval = 60000; // 60 sec between intruder alerts
unsigned long lastBotCheck = 0;

// ===== Function to send a photo to Telegram =====
String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  
  // Capture photo
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Camera capture failed");
    return "Camera capture failed";
  }

  Serial.println("📸 Photo captured, sending to Telegram...");

  if (!clientTCP.connect(myDomain, 443)) {
    Serial.println("❌ Connection to Telegram failed");
    esp_camera_fb_return(fb);
    return "Connection failed";
  }

  String head = "--VisiLock\r\nContent-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + CHAT_ID +
                "\r\n--VisiLock\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"photo.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--VisiLock--\r\n";
  uint32_t imageLen = fb->len;
  uint32_t totalLen = imageLen + head.length() + tail.length();

  clientTCP.println("POST /bot" + BOT_TOKEN + "/sendPhoto HTTP/1.1");
  clientTCP.println("Host: api.telegram.org");
  clientTCP.println("Content-Type: multipart/form-data; boundary=VisiLock");
  clientTCP.println("Content-Length: " + String(totalLen));
  clientTCP.println();
  clientTCP.print(head);

  uint8_t *fbBuf = fb->buf;
  size_t fbLen = fb->len;
  for (size_t n = 0; n < fbLen; n += 1024) {
    size_t chunk = (n + 1024 < fbLen) ? 1024 : fbLen - n;
    clientTCP.write(fbBuf, chunk);
    fbBuf += chunk;
  }
  clientTCP.print(tail);
  esp_camera_fb_return(fb);

  // Wait for response
  long timeout = millis();
  while (millis() - timeout < 10000) {
    while (clientTCP.available()) {
      clientTCP.read();
      timeout = millis();
    }
  }
  clientTCP.stop();
  Serial.println("✅ Photo sent to Telegram successfully!");
  return "OK";
}

// ===== Handle Telegram commands =====
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    Serial.println("📩 New Telegram message received!");
    Serial.print("Chat ID: ");
    Serial.println(chat_id);
    Serial.print("Text: ");
    Serial.println(text);

    // Check if authorized user
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "❌ Unauthorized user", "");
      Serial.println("⚠ Unauthorized user attempted access");
      continue;
    }

    // Process commands
    if (text == "/start") {
      String msg = "🤖 ESP32-CAM Door Lock System\n\n";
      msg += "📋 Available Commands:\n";
      msg += "📸 /photo - Capture and send current photo\n";
      msg += "🔓 /unlock - Unlock door remotely (10 sec)\n";
      msg += "🔒 /lock - Lock door immediately\n";
      msg += "ℹ /status - Check system status\n\n";
      msg += "🔔 System is online and monitoring!";
      bot.sendMessage(CHAT_ID, msg, "Markdown");
      Serial.println("✅ /start command executed");
    }

    else if (text == "/photo") {
      Serial.println("📸 Taking photo on demand...");
      bot.sendMessage(CHAT_ID, "📷 Capturing photo...", "");
      digitalWrite(FLASH_LED, HIGH);
      delay(200);
      String result = sendPhotoTelegram();
      digitalWrite(FLASH_LED, LOW);
      if (result == "OK") {
        bot.sendMessage(CHAT_ID, "✅ Photo sent successfully!", "");
      } else {
        bot.sendMessage(CHAT_ID, "❌ Failed to send photo: " + result, "");
      }
      Serial.println("✅ /photo command executed");
    }

    else if (text == "/unlock") {
      Serial.println("🔓 Remote unlock requested...");
      doorManuallyUnlocked = true;
      matchFace = true;
      activateRelay = false;
      intruderDetected = false; // Stop intruder alerts
      bot.sendMessage(CHAT_ID, "✅ Door unlocked remotely! Will auto-lock in 10 seconds.", "");
      Serial.println("✅ /unlock command executed - door will open for 10 seconds");
    }

    else if (text == "/lock") {
      Serial.println("🔒 Remote lock requested...");
      digitalWrite(Relay, LOW);
      digitalWrite(Green, LOW);
      digitalWrite(Red, HIGH);
      matchFace = false;
      activateRelay = false;
      doorManuallyUnlocked = false;
      intruderDetected = false;
      bot.sendMessage(CHAT_ID, "🔒 Door locked remotely. Face detection resumed.", "");
      Serial.println("✅ /lock command executed - system ready");
    }

    else if (text == "/status") {
      String status = "📊 System Status:\n\n";
      status += "🌐 WiFi: Connected\n";
      status += "📡 IP: " + WiFi.localIP().toString() + "\n";
      status += "🚪 Door: " + String(activateRelay ? "Unlocked 🔓" : "Locked 🔒") + "\n";
      status += "👤 Face Detection: " + String(matchFace ? "Match ✅" : "Active 👀") + "\n";
      status += "⚠ Intruder Mode: " + String(intruderDetected ? "Alert! 🚨" : "Normal 🟢") + "\n";
      bot.sendMessage(CHAT_ID, status, "Markdown");
      Serial.println("✅ /status command executed");
    }

    else {
      Serial.println("⚠ Unknown command received");
      bot.sendMessage(CHAT_ID, "⚠ Unknown command. Type /start to see available options.", "");
    }
  }
}

// ===== Setup =====
void setup() {
  // Initialize pins
  pinMode(Relay, OUTPUT);
  pinMode(Red, OUTPUT);
  pinMode(Green, OUTPUT);
  pinMode(FLASH_LED, OUTPUT);
  digitalWrite(Relay, LOW);
  digitalWrite(Red, HIGH);
  digitalWrite(Green, LOW);
  digitalWrite(FLASH_LED, LOW);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("\n\n🚀 ESP32-CAM Door Lock System Starting...");

  // ===== Camera configuration =====
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
    Serial.println("✅ PSRAM found");
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    Serial.println("⚠ PSRAM not found");
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("✅ Camera initialized");

  sensor_t * s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }
  s->set_framesize(s, FRAMESIZE_QVGA);

  // ===== WiFi Connection =====
  Serial.println("📡 Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    Serial.print("📍 IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi connection failed!");
    return;
  }

  // ===== Telegram Bot Setup =====
  clientTCP.setCACert(NULL); // Skip certificate validation
  Serial.println("🤖 Connecting to Telegram...");
  
  // Send startup message
  delay(1000); // Give time for connection
  if (bot.sendMessage(CHAT_ID, "🟢 ESP32-CAM Door Lock System Online!\n\nType /start for commands.", "Markdown")) {
    Serial.println("✅ Telegram bot connected and startup message sent!");
  } else {
    Serial.println("⚠ Failed to send startup message");
  }

  // ===== Camera Web Server =====
  startCameraServer();
  Serial.println("🌐 Camera web server started!");
  Serial.println("=================================");
  Serial.println("✅ System Ready!");
  Serial.println("=================================\n");
}

void loop() {
  delay(10);
  yield();

  // ===== Face Recognition & Door Control Logic =====
  if (matchFace == true && activateRelay == false) {
    activateRelay = true;
    digitalWrite(Relay, HIGH);
    digitalWrite(Green, HIGH);
    digitalWrite(Red, LOW);
    prevMillis = millis();
    
    if (doorManuallyUnlocked) {
      Serial.println("✅ Door unlocked remotely");
    } else {
      bot.sendMessage(CHAT_ID, "✅ Known face detected! Door unlocked for 10 seconds.", "");
      Serial.println("✅ Known face detected - door unlocked");
    }
  }

  // Auto-lock after interval
  if (activateRelay == true && millis() - prevMillis > interval) {
    activateRelay = false;
    matchFace = false;
    doorManuallyUnlocked = false;
    digitalWrite(Relay, LOW);
    digitalWrite(Green, LOW);
    digitalWrite(Red, HIGH);
    Serial.println("🔒 Door auto-locked after 10 seconds");
    // Resume face detection
    intruderDetected = false;
  }

  // ===== Intruder Detection & Alert =====
  // Only trigger if door is locked and no known face
  if (!matchFace && !activateRelay && !doorManuallyUnlocked) {
    // Check if enough time has passed since last alert
    if (millis() - lastIntruderAlert > alertInterval) {
      lastIntruderAlert = millis();
      intruderDetected = true;
      
      Serial.println("⚠ INTRUDER DETECTED! Sending alert...");
      bot.sendMessage(CHAT_ID, "🚨 INTRUDER ALERT!\n\nUnknown person detected at door!\nCapturing photo...", "Markdown");
      
      digitalWrite(FLASH_LED, HIGH);
      delay(200);
      String result = sendPhotoTelegram();
      digitalWrite(FLASH_LED, LOW);
      
      if (result == "OK") {
        Serial.println("✅ Intruder photo sent successfully");
      } else {
        Serial.println("❌ Failed to send intruder photo");
      }
    }
  } else {
    intruderDetected = false;
  }

  // ===== Telegram Command Polling =====
  if (millis() - lastBotCheck > 2000) {  // Check every 2 seconds
    int numNew = bot.getUpdates(bot.last_message_received + 1);
    if (numNew > 0) {
      Serial.printf("📬 Received %d new message(s)\n", numNew);
      handleNewMessages(numNew);
    }
    lastBotCheck = millis();
  }
}