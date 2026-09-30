#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ====== GANTI SESUAI KEBUTUHAN ======
const char* ssid = "Atharva";
const char* password = "starlink";
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* topicData = "unsoed/tk245004/kelompok7/data";
const char* topicPerintah = "unsoed/tk245004/kelompok7/perintah";          // LED
const char* topicBuzzer = "unsoed/tk245004/kelompok7/perintah/buzzer";     // Buzzer
// ====================================

#define DHTPIN 4        // GPIO4 = D2 (jangan pakai GPIO6-11 di ESP8266)
#define DHTTYPE DHT22   // ganti ke DHT11 jika memakai sensor DHT11

const int ledPin = 2;                // GPIO2 = D4 (LED bawaan)
const bool LED_ACTIVE_LOW = true;    // LED bawaan: LOW = menyala
const int buzzerPin = 14;            // GPIO14 = D5 (buzzer aktif)

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long waktuTerakhirPublish = 0;
const long intervalPublish = 5000;

void setLed(bool on) {
  digitalWrite(ledPin, (on != LED_ACTIVE_LOW) ? HIGH : LOW);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String pesan;
  for (unsigned int i = 0; i < length; i++) pesan += (char)payload[i];

  JsonDocument doc;
  if (deserializeJson(doc, pesan)) {
    Serial.println("Gagal parsing JSON");
    return;
  }

  const char* perintah = doc["perintah"];
  if (perintah == nullptr) return;
  bool on = (String(perintah) == "ON");

  // Bedakan aktuator berdasarkan topic yang menerima pesan
  if (String(topic) == topicPerintah) {
    setLed(on);
    Serial.print("[LED] ");
    Serial.println(perintah);
  } else if (String(topic) == topicBuzzer) {
    digitalWrite(buzzerPin, on ? HIGH : LOW);
    Serial.print("[Buzzer] ");
    Serial.println(perintah);
  }
}

void hubungkanWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("WiFi berhasil terhubung!");
}

void hubungkanMQTT() {
  while (!client.connected()) {
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      client.subscribe(topicPerintah);
      client.subscribe(topicBuzzer);
      Serial.println("Terhubung dan subscribe topic LED & buzzer");
    } else {
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  setLed(false);                   // LED mati saat awal
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);    // buzzer mati saat awal
  dht.begin();
  hubungkanWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) hubungkanMQTT();
  client.loop();

  if (millis() - waktuTerakhirPublish > intervalPublish) {
    waktuTerakhirPublish = millis();
    float suhu = dht.readTemperature();
    if (!isnan(suhu)) {
      JsonDocument doc;
      doc["suhu"] = suhu;
      char buffer[128];
      serializeJson(doc, buffer);
      client.publish(topicData, buffer);
      Serial.print("Data terkirim: ");
      Serial.println(buffer);
    }
  }
}