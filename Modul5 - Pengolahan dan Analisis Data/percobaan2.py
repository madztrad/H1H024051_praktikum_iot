import json
import time
import paho.mqtt.client as mqtt
import pandas as pd
import matplotlib.pyplot as plt

BROKER = "broker.hivemq.com"
PORT = 1883
TOPIC = "unsoed/tk245004/kelompok7/data"  # = topicData ESP32 (Modul 4 Percobaan 2A)
JUMLAH_SAMPEL = 30  # jumlah data

data = []


def on_connect(client, userdata, flags, reason_code, properties):
    print("Terhubung ke broker, kode:", reason_code)
    client.subscribe(TOPIC)


def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode())
        suhu = float(payload["suhu"])
    except (ValueError, KeyError, TypeError):
        print("Data tidak valid, diabaikan")
        return

    data.append({"waktu": time.strftime("%Y-%m-%d %H:%M:%S"), "suhu": suhu})

    print(f"[{len(data)}/{JUMLAH_SAMPEL}] suhu = {suhu}")

    if len(data) >= JUMLAH_SAMPEL:
        client.disconnect()


# --- Tahap 1: akuisisi data dari MQTT ---
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, PORT)
client.loop_forever()

# --- Tahap 2: simpan ke CSV ---
df = pd.DataFrame(data)
df.to_csv("data_suhu.csv", index=False)
print("Data tersimpan di data_suhu.csv")

# --- Tahap 3: analisis statistik dan deteksi anomali (z-score) ---
rata2 = df["suhu"].mean()
std = df["suhu"].std()

if std == 0:
    df["zscore"] = 0.0
else:
    df["zscore"] = (df["suhu"] - rata2) / std
df["anomali"] = df["zscore"].abs() > 2

print(f"Rata-rata : {rata2:.2f}")
print(f"Std deviasi: {std:.2f}")
print(f"Minimum   : {df['suhu'].min():.2f}")
print(f"Maksimum  : {df['suhu'].max():.2f}")
print(f"Jumlah anomali: {df['anomali'].sum()}")

# --- Modifikasi soal 5.6.4 no.4: moving average jendela 5 sebelum z-score ---
df["suhu_ma"] = df["suhu"].rolling(window=5, min_periods=1).mean()
rata2_ma = df["suhu_ma"].mean()
std_ma = df["suhu_ma"].std()

if std_ma == 0:
    df["zscore_ma"] = 0.0
else:
    df["zscore_ma"] = (df["suhu_ma"] - rata2_ma) / std_ma
df["anomali_ma"] = df["zscore_ma"].abs() > 2

print(f"Jumlah anomali (mentah)       : {df['anomali'].sum()}")
print(f"Jumlah anomali (moving avg 5) : {df['anomali_ma'].sum()}")

# --- Tahap 4: visualisasi ---
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 7))

ax1.plot(df.index, df["suhu"], marker="o", label="Suhu")
anom = df[df["anomali"]]
ax1.scatter(anom.index, anom["suhu"], color="red", s=100, label="Anomali", zorder=3)
ax1.axhline(rata2, color="gray", linestyle="--", label="Rata-rata")
ax1.set_xlabel("Urutan data")
ax1.set_ylabel("Suhu (°C)")
ax1.set_title("Data Suhu dan Deteksi Anomali (Mentah)")
ax1.legend()

ax2.plot(df.index, df["suhu_ma"], marker="o", color="green", label="Suhu MA-5")
anom_ma = df[df["anomali_ma"]]
ax2.scatter(anom_ma.index, anom_ma["suhu_ma"], color="red", s=100, label="Anomali", zorder=3)
ax2.axhline(rata2_ma, color="gray", linestyle="--", label="Rata-rata")
ax2.set_xlabel("Urutan data")
ax2.set_ylabel("Suhu (°C)")
ax2.set_title("Deteksi Anomali Setelah Moving Average (jendela 5)")
ax2.legend()

plt.tight_layout()
plt.savefig("grafik_suhu.png")
plt.show()
