#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ====== GANTI SESUAI KEBUTUHAN ======
const char* ssid = "Atharva";
const char* password = "starlink";
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* topicPerintah = "unsoed/tk245004/kelompok7/perintah";
// ====================================

const int ledPin = 2;                 // GPIO2 (D4) = LED bawaan NodeMCU
const bool LED_ACTIVE_LOW = true;     // true untuk LED bawaan; false untuk LED eksternal ke GND

// Pengaturan PWM ESP8266
const int pwmFreq = 5000;             // frekuensi PWM 5 kHz
const int pwmRange = 255;             // rentang nilai 0-255

WiFiClient espClient;
PubSubClient client(espClient);

// Menulis kecerahan 0-255 ke LED (menangani logika terbalik jika active-low)
void setKecerahan(int nilai) {
  nilai = constrain(nilai, 0, 255);
  analogWrite(ledPin, LED_ACTIVE_LOW ? (255 - nilai) : nilai);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String pesan;
  for (unsigned int i = 0; i < length; i++) {
    pesan += (char)payload[i];
  }

  Serial.print("Pesan diterima [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(pesan);

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, pesan);
  if (error) {
    Serial.print("Gagal parsing JSON: ");
    Serial.println(error.c_str());
    return;
  }

  const char* perintah = doc["perintah"];
  if (perintah == nullptr) {
    Serial.println("Key 'perintah' tidak ditemukan");
    return;
  }

  // Ambil intensitas; jika tidak ada di JSON, default 255 (terang penuh)
  int intensitas = doc["intensitas"] | 255;
  intensitas = constrain(intensitas, 0, 255);

  if (String(perintah) == "ON") {
    setKecerahan(intensitas);
    Serial.print("Aktuator: ON, intensitas = ");
    Serial.println(intensitas);
  } else if (String(perintah) == "OFF") {
    setKecerahan(0);
    Serial.println("Aktuator: OFF");
  }
}

void hubungkanWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi berhasil terhubung!");
}

void hubungkanMQTT() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke broker MQTT...");
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("berhasil terhubung!");
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

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  analogWriteRange(pwmRange);   // nilai PWM 0-255
  analogWriteFreq(pwmFreq);     // frekuensi 5 kHz
  setKecerahan(0);              // LED mati saat awal

  hubungkanWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    hubungkanMQTT();
  }
  client.loop();
}