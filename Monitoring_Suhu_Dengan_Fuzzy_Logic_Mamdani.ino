#define BLYNK_TEMPLATE_ID "TMPL63y8N5y6Q"
#define BLYNK_TEMPLATE_NAME "Derik Dwi Heavyanto UTS"
#define BLYNK_AUTH_TOKEN "L9T7Y6JbOO7LxTXrRssSisjPY2NurEWT"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT11.h>

// ================= WIFI & BLYNK =================
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "vivo Y22";
char pass[] = "12345678";

// ================= SENSOR =================
#define DHTPIN 32
DHT11 dht11(DHTPIN);

// ================= TIMER =================
BlynkTimer timer;

// ================= FUNGSI FUZZY =================
float naik(float x, float a, float b) {
  if (x <= a) return 0;
  if (x >= b) return 1;
  return (x - a) / (b - a);
}

float turun(float x, float a, float b) {
  if (x <= a) return 1;
  if (x >= b) return 0;
  return (b - x) / (b - a);
}

float segitiga(float x, float a, float b, float c) {
  if (x <= a || x >= c) return 0;
  if (x == b) return 1;
  if (x < b) return (x - a) / (b - a);
  return (c - x) / (c - b);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Serial.println("=== Sistem Mamdani Manual Mulai ===");

  Blynk.begin(auth, ssid, pass);

  timer.setInterval(1000L, prosesFuzzy);
}

// ================= LOOP =================
void loop() {
  Blynk.run();
  timer.run();
}

// ================= PROSES FUZZY =================
void prosesFuzzy() {

  int suhu = dht11.readTemperature();
  int hum = dht11.readHumidity();

  if (suhu == -1 || hum == -1) {
    Serial.println("Sensor error");
    return;
  }

  // Kirim ke Blynk
  Blynk.virtualWrite(V0, suhu);
  Blynk.virtualWrite(V1, hum);

  // ========== FUZZIFIKASI ==========
  float suhuDingin = turun(suhu, 18, 23);
  float suhuNormal = segitiga(suhu, 22, 26, 30);
  float suhuPanas = naik(suhu, 28, 33);


  float humKering = turun(hum, 35, 45);
  float humNormal = segitiga(hum, 45, 55, 65);
  float humLembab = naik(hum, 60, 75);


  // ========== INFERENSI (MAMDANI - MIN) ==========
  float r1 = suhuDingin;                 // dingin → mati
  float r2 = suhuNormal;                 // normal → pelan
  float r3 = min(suhuPanas, humKering);  // panas & kering → pelan
  float r4 = min(suhuPanas, humNormal);  // panas & normal → cepat
  float r5 = min(suhuPanas, humLembab);  // panas & lembab → cepat

  // ========== AGREGASI (MAX) ==========
  float outMati = r1;
  float outPelan = max(r2, r3);
  float outCepat = max(r4, r5);

  // ========== DEFUZZIFIKASI ==========
  float zMati = 0;
  float zPelan = 45;
  float zCepat = 100;

  float total = outMati + outPelan + outCepat;
  if (total == 0) return;

  float z = (outMati * zMati + outPelan * zPelan + outCepat * zCepat) / total;

  // ========== OUTPUT ==========
  if (z > 60) {
    Blynk.virtualWrite(V3, "KIPAS CEPAT");
  } else if (z > 30) {
    Blynk.virtualWrite(V3, "KIPAS PELAN");
  } else {
    Blynk.virtualWrite(V3, "KIPAS MATI");
  }

  // ========== DEBUG SERIAL ==========
  Serial.print("Suhu: ");
  Serial.print(suhu);
  Serial.print(" | Hum: ");
  Serial.print(hum);
  Serial.print(" | Z: ");
  Serial.print(z);
  Serial.print(" | Output: ");

  if (z > 60) Serial.println("CEPAT");
  else if (z > 30) Serial.println("PELAN");
  else Serial.println("MATI");
}
