#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22

// Objek DHT tetap dideklarasikan agar struktur kode asli tidak banyak berubah,
// namun pembacaan suhu nanti akan digantikan oleh fungsi mock.
DHT dht(DHTPIN, DHTTYPE);

const int N = 5;         // jumlah sampel moving average
float buffer[N];         // menyimpan N pembacaan terakhir
int indeks = 0;          // posisi penulisan berikutnya pada buffer
int jumlah = 0;          // jumlah sampel yang sudah terisi

float suhuMin = 1000.0;
float suhuMax = -1000.0;

// Mock function pengganti sensor DHT22 untuk pengujian simulasi
float bacaSuhuMock() {
    static unsigned long jumlahBaca = 0;
    jumlahBaca++;

    // Suhu dasar berubah secara bertahap menggunakan fungsi sinyal sinusoidal
    float suhuDasar = 28.0 + 2.0 * sin(jumlahBaca * 0.08);

    // Noise kecil untuk mensimulasikan fluktuasi pembacaan sensor asli
    float noise = random(-5, 6) * 0.1;

    float suhu = suhuDasar + noise;

    // Lonjakan suhu setiap 25 pembacaan untuk simulasi anomali
    if (jumlahBaca % 25 == 0) {
        suhu += 8.0;
    }

    return suhu;
}

float hitungMovingAverage(float nilaiBaru) {
    buffer[indeks] = nilaiBaru;
    indeks = (indeks + 1) % N;
    if (jumlah < N) jumlah++;

    float total = 0;
    for (int i = 0; i < jumlah; i++) {
        total += buffer[i];
    }
    return total / jumlah;
}

void setup() {
    Serial.begin(115200);
    dht.begin();
    // Menginisialisasi seed random generator agar nilai noise bervariasi setiap dijalankan
    randomSeed(analogRead(0)); 
}

void loop() {
    // Mengganti dht.readTemperature() dengan fungsi bacaSuhuMock()
    float suhu = bacaSuhuMock();

    if (isnan(suhu)) {
        Serial.println("Pembacaan gagal, data diabaikan");
    } else {
        float suhuFilter = hitungMovingAverage(suhu);

        if (suhu < suhuMin) suhuMin = suhu;
        if (suhu > suhuMax) suhuMax = suhu;

        Serial.print("Mentah: ");
        Serial.print(suhu);
        Serial.print(" | Filter: ");
        Serial.print(suhuFilter);
        Serial.print(" | Min: ");
        Serial.print(suhuMin);
        Serial.print(" | Max: ");
        Serial.println(suhuMax);
    }

    delay(2000);
}
