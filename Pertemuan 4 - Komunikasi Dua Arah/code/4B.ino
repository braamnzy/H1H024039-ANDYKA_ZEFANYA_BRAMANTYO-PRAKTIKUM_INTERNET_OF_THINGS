#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ==================== KONFIGURASI WIFI & MQTT ====================
const char* ssid = "Hazee";
const char* password = "abcd1234";
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;

const char* topicData     = "unsoed/tk245004/kelompok2/data";
const char* topicPerintah = "unsoed/tk245004/kelompok2/perintah";

// ==================== KONFIGURASI PIN ====================
const int ledPin = 4;   // D2 pada NodeMCU

WiFiClient espClient;
PubSubClient client(espClient);

// ==================== VARIABEL NON-BLOCKING TIMER ====================
unsigned long waktuTerakhirPublish = 0;
const long intervalPublish = 5000;  // publish data setiap 5 detik

// ==================== CALLBACK: MENERIMA PESAN MQTT ====================
void callback(char* topic, byte* payload, unsigned int length) {
  String pesan;
  for (unsigned int i = 0; i < length; i++) {
    pesan += (char)payload[i];
  }

  JsonDocument doc;
  if (deserializeJson(doc, pesan)) return;  // abaikan jika parsing gagal

  const char* perintah = doc["perintah"];
  digitalWrite(ledPin, String(perintah) == "ON" ? HIGH : LOW);

  Serial.print("Perintah diterima -> Aktuator: ");
  Serial.println(perintah);
}

// ==================== FUNGSI KONEKSI WIFI ====================
void hubungkanWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi berhasil terhubung!");
}

// ==================== FUNGSI KONEKSI MQTT ====================
void hubungkanMQTT() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke broker MQTT...");
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("berhasil!");
      client.subscribe(topicPerintah);
      Serial.print("Subscribe ke topic: ");
      Serial.println(topicPerintah);
    } else {
      Serial.print("gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 2 detik");
      delay(2000);
    }
  }
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  hubungkanWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

// ==================== LOOP UTAMA ====================
void loop() {
  if (!client.connected()) hubungkanMQTT();
  client.loop();  // memproses pesan masuk secara terus-menerus

  // Publish data dummy secara berkala
  if (millis() - waktuTerakhirPublish > intervalPublish) {
    waktuTerakhirPublish = millis();

    // ===== DATA DUMMY =====
    // Suhu acak antara 25.0 - 35.0 °C
    float suhu = 25.0 + (random(0, 1000) / 100.0);
    // Kelembapan acak antara 40.0 - 80.0 %
    float kelembapan = 40.0 + (random(0, 4000) / 100.0);

    JsonDocument doc;
    doc["suhu"] = suhu;
    doc["kelembapan"] = kelembapan;
    doc["sumber"] = "dummy";

    char buffer[128];
    serializeJson(doc, buffer);
    client.publish(topicData, buffer);

    Serial.print("Data dummy terkirim: ");
    Serial.println(buffer);
  }
}