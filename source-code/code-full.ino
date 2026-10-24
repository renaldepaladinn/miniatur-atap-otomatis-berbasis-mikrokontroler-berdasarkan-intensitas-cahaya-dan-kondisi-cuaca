// ====== Blynk ======
#define BLYNK_TEMPLATE_ID "TMPL6jR3w75Nl"
#define BLYNK_TEMPLATE_NAME "Alat Pemberi Ikan Otomatis"
#define BLYNK_AUTH_TOKEN "xlo3142SNYAMrvYbcoQQ2em0UAPcQq-_"
// ====== Fuzzy ======
#include <Fuzzy.h>

// ====== Blynk ======
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// ====== Blynk ======
char ssid[] = "A16";
char pass[] = "Renal2707";
int ledOpen = V4;
int ledHalf = V0;
int ledClose = V7;

// ====== Pin Motor ======
#define ForwardPin 27
#define BackwardPin 26
#define EnablePin 14
#define EncoderPinA 32
#define EncoderPinB 33

// ====== Sensor ======
#define LIGHT_SENSOR_PIN 35
#define RAIN 34

// ====== Fuzzy Object ======
Fuzzy *fuzzy = new Fuzzy();

// FuzzyInputIntensitasCahaya
FuzzySet *gelap = new FuzzySet(0, 0, 1095, 1595);
FuzzySet *mendung = new FuzzySet(1095, 1595, 2595, 3095);
FuzzySet *terang = new FuzzySet(2595, 3095, 4095, 4095);

// FuzzyInputKondisiCuaca
FuzzySet *notHujan = new FuzzySet(0, 0, 1095, 1595);
FuzzySet *gerimis = new FuzzySet(1095, 1595, 2595, 3095);
FuzzySet *hujan = new FuzzySet(2595, 3095, 4095, 4095);

// FuzzyOutputBukaanAtap
FuzzySet *membuka = new FuzzySet(0, 0, 0, 30);
FuzzySet *sebagian = new FuzzySet(20, 50 ,50, 80);
FuzzySet *menutup = new FuzzySet(70, 100, 100, 100);

// ====== Variabel posisi atap ======
float posisiAtap = 0;  // 0 = membuka, 50 = sebagian, 100 = tertutup penuh

// ====== FUNGSI TAMBAHAN UNTUK MEMILIH KONDISI ======
String kondisiCahaya(float g, float m, float t) {
  if (g >= m && g >= t) return "Gelap";
  if (m >= g && m >= t) return "Mendung";
  return "Terang";
}

String kondisiHujan(float n, float g, float h) {
  if (n >= g && n >= h) return "Tidak Hujan";
  if (g >= n && g >= h) return "Gerimis";
  return "Hujan";
}

void setup() {
  Serial.begin(115200);
  pinMode(ForwardPin, OUTPUT);
  pinMode(BackwardPin, OUTPUT);
  pinMode(EnablePin, OUTPUT);
  analogSetAttenuation(ADC_11db);

  // ====== Koneksi WiFi dan Blynk ======
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // ====== FUZZY LOGIC  ======

  // FuzzyInputIntensitasCahaya
  FuzzyInput *cahaya = new FuzzyInput(1);

  cahaya->addFuzzySet(gelap);
  cahaya->addFuzzySet(mendung);
  cahaya->addFuzzySet(terang);
  fuzzy->addFuzzyInput(cahaya);

  // FuzzyInputKondisiCuaca
  FuzzyInput *cuaca = new FuzzyInput(2);

  cuaca->addFuzzySet(notHujan);
  cuaca->addFuzzySet(gerimis);
  cuaca->addFuzzySet(hujan);
  fuzzy->addFuzzyInput(cuaca);

  // FuzzyOutputBukaanAtap
  FuzzyOutput *atap = new FuzzyOutput(1);

  atap->addFuzzySet(membuka);
  atap->addFuzzySet(sebagian);
  atap->addFuzzySet(menutup);
  fuzzy->addFuzzyOutput(atap);

  // Building FuzzyRule1
  FuzzyRuleAntecedent *ifTerangAndTidakHujan = new FuzzyRuleAntecedent();
  ifTerangAndTidakHujan->joinWithAND(terang, notHujan);
  FuzzyRuleConsequent *thenAtapMembuka = new FuzzyRuleConsequent();
  thenAtapMembuka->addOutput(membuka);
  FuzzyRule *fuzzyRule1 = new FuzzyRule(1, ifTerangAndTidakHujan, thenAtapMembuka);
  fuzzy->addFuzzyRule(fuzzyRule1);

  // Building FuzzyRule2
  FuzzyRuleAntecedent *ifTerangAndGerimis = new FuzzyRuleAntecedent();
  ifTerangAndGerimis->joinWithAND(terang, gerimis);
  FuzzyRuleConsequent *thenAtapMenutup1 = new FuzzyRuleConsequent();
  thenAtapMenutup1->addOutput(menutup);
  FuzzyRule *fuzzyRule2 = new FuzzyRule(1, ifTerangAndGerimis, thenAtapMenutup1);
  fuzzy->addFuzzyRule(fuzzyRule2);

  // Building FuzzyRule3
  FuzzyRuleAntecedent *ifTerangAndHujan = new FuzzyRuleAntecedent();
  ifTerangAndHujan->joinWithAND(terang, hujan);
  FuzzyRuleConsequent *thenAtapMenutup2 = new FuzzyRuleConsequent();
  thenAtapMenutup2->addOutput(menutup);
  FuzzyRule *fuzzyRule3 = new FuzzyRule(1, ifTerangAndHujan, thenAtapMenutup2);
  fuzzy->addFuzzyRule(fuzzyRule3);
  
  // Building FuzzyRule4
  FuzzyRuleAntecedent *ifMendungAndTidakHujan = new FuzzyRuleAntecedent();
  ifMendungAndTidakHujan->joinWithAND(mendung, notHujan);
  FuzzyRuleConsequent *thenAtapSebagian = new FuzzyRuleConsequent();
  thenAtapSebagian->addOutput(sebagian);
  FuzzyRule *fuzzyRule4 = new FuzzyRule(1, ifMendungAndTidakHujan, thenAtapSebagian);
  fuzzy->addFuzzyRule(fuzzyRule4);

  // Building FuzzyRule5
  FuzzyRuleAntecedent *ifMendungAndGerimis = new FuzzyRuleAntecedent();
  ifMendungAndGerimis->joinWithAND(mendung, gerimis);
  FuzzyRuleConsequent *thenAtapMenutup3 = new FuzzyRuleConsequent();
  thenAtapMenutup3->addOutput(menutup);
  FuzzyRule *fuzzyRule5 = new FuzzyRule(1, ifMendungAndGerimis, thenAtapMenutup3);
  fuzzy->addFuzzyRule(fuzzyRule5);

  // Building FuzzyRule6
  FuzzyRuleAntecedent *ifMendungAndHujan = new FuzzyRuleAntecedent();
  ifMendungAndHujan->joinWithAND(mendung, hujan);
  FuzzyRuleConsequent *thenAtapMenutup4 = new FuzzyRuleConsequent();
  thenAtapMenutup4->addOutput(menutup);
  FuzzyRule *fuzzyRule6 = new FuzzyRule(1, ifMendungAndHujan, thenAtapMenutup4);
  fuzzy->addFuzzyRule(fuzzyRule6);

  // Building FuzzyRule7
  FuzzyRuleAntecedent *ifGelapAndTidakHujan = new FuzzyRuleAntecedent();
  ifGelapAndTidakHujan->joinWithAND(gelap, notHujan);
  FuzzyRuleConsequent *thenAtapMenutup5 = new FuzzyRuleConsequent();
  thenAtapMenutup5->addOutput(menutup);
  FuzzyRule *fuzzyRule7 = new FuzzyRule(1, ifGelapAndTidakHujan, thenAtapMenutup5);
  fuzzy->addFuzzyRule(fuzzyRule7);

  // Building FuzzyRule8
  FuzzyRuleAntecedent *ifGelapAndGerimis = new FuzzyRuleAntecedent();
  ifGelapAndGerimis->joinWithAND(gelap, gerimis);
  FuzzyRuleConsequent *thenAtapMenutup6 = new FuzzyRuleConsequent();
  thenAtapMenutup6->addOutput(menutup);
  FuzzyRule *fuzzyRule8 = new FuzzyRule(1, ifGelapAndGerimis, thenAtapMenutup6);
  fuzzy->addFuzzyRule(fuzzyRule8);

  // Building FuzzyRule9
  FuzzyRuleAntecedent *ifGelapAndHujan = new FuzzyRuleAntecedent();
  ifGelapAndHujan->joinWithAND(gelap, hujan);
  FuzzyRuleConsequent *thenAtapMenutup7 = new FuzzyRuleConsequent();
  thenAtapMenutup7->addOutput(menutup);
  FuzzyRule *fuzzyRule9 = new FuzzyRule(1, ifGelapAndHujan, thenAtapMenutup7);
  fuzzy->addFuzzyRule(fuzzyRule9);
}

void loop() {
Blynk.run();

  // ====== Baca sensor ======
int cahayaVal = analogRead(LIGHT_SENSOR_PIN);

int rawRain = analogRead(RAIN);
int hujanVal = 4095 - rawRain;   // kering=0, basah=4095

Serial.print("Cahaya: "); Serial.print(cahayaVal);
Serial.print(" | Rain RAW: "); Serial.print(rawRain);
Serial.print(" | Rain INV: "); Serial.println(hujanVal);

  // ====== Hitung membership untuk ditampilkan ke Blynk ======
float g  = gelap->getPertinence();
float m  = mendung->getPertinence();
float t  = terang->getPertinence();

float nh = notHujan->getPertinence();
float gr = gerimis->getPertinence();
float hj = hujan->getPertinence();

  // Tentukan kondisi
  String kondisiC = kondisiCahaya(g, m, t);
  String kondisiH = kondisiHujan(nh, gr, hj);
  String kondisiGabungan = kondisiC + " | " + kondisiH;

  // TAMPILKAN KE BLYNK (V5)
  Blynk.virtualWrite(V5, kondisiGabungan);
  Serial.println("Kondisi Fuzzy: " + kondisiGabungan);
  
  // ====== Fuzzifikasi ======
  fuzzy->setInput(1, cahayaVal);
  fuzzy->setInput(2, hujanVal);
  fuzzy->fuzzify();
  float outputAtap = fuzzy->defuzzify(1);

  Serial.print("Output Fuzzy: ");
  Serial.println(outputAtap);

  // ====== Kendali Motor Berdasarkan Output + Posisi Terakhir ======
  if (outputAtap >= 0 && outputAtap <= 20) {  // Atap membuka penuh
    if (posisiAtap == 0) {
      posisiAtap = 0;
    } else if (posisiAtap == 100) {
      delay(1500);
      Serial.println("Atap Membuka...");
      digitalWrite(ForwardPin, LOW);
      digitalWrite(BackwardPin, HIGH);
      analogWrite(EnablePin, 150);
      posisiAtap = 0;
      delay(7500);
      digitalWrite(ForwardPin, LOW);
      digitalWrite(BackwardPin, LOW);
      analogWrite(EnablePin, 0);
    } else if (posisiAtap == 50) {
      delay(1500);
      Serial.println("Atap Membuka...");
      digitalWrite(ForwardPin, LOW);
      digitalWrite(BackwardPin, HIGH);
      analogWrite(EnablePin, 150);
      posisiAtap = 0;
      delay(3500);
      digitalWrite(ForwardPin, LOW);
      digitalWrite(BackwardPin, LOW);
      analogWrite(EnablePin, 0);
    }
    Blynk.virtualWrite(ledOpen, 255);
    Blynk.virtualWrite(ledHalf, 0);
    Blynk.virtualWrite(ledClose, 0);
  }
  else if (outputAtap >= 30 && outputAtap <= 70) {  // Atap membuka sebagian
    if (posisiAtap == 0) {
      delay(1500);
      Serial.println("Atap Menutup ke Posisi Sebagian...");
      digitalWrite(ForwardPin, HIGH);
      digitalWrite(BackwardPin, LOW);
      analogWrite(EnablePin, 150);
      posisiAtap = 50;
      delay(3500);
      digitalWrite(ForwardPin, LOW);
      digitalWrite(BackwardPin, LOW);
      analogWrite(EnablePin, 0);
    } else if (posisiAtap == 100) {
      delay(1500);
      Serial.println("Atap Membuka ke Posisi Sebagian...");
      digitalWrite(ForwardPin, LOW);
      digitalWrite(BackwardPin, HIGH);
      analogWrite(EnablePin, 150);
      posisiAtap = 50;
      delay(3500);
      digitalWrite(ForwardPin, LOW);
      digitalWrite(BackwardPin, LOW);
      analogWrite(EnablePin, 0);
    } else if (posisiAtap == 50) {
      posisiAtap = 50;
    }
    Blynk.virtualWrite(ledOpen, 0);
    Blynk.virtualWrite(ledHalf, 255);
    Blynk.virtualWrite(ledClose, 0);
  }
  else if (outputAtap >= 80 && outputAtap <= 100){  // Atap menutup penuh
    if (posisiAtap == 0) {
      delay(1500);
      Serial.println("Atap Menutup...");
      digitalWrite(ForwardPin, HIGH);
      digitalWrite(BackwardPin, LOW);
      analogWrite(EnablePin, 150);
      posisiAtap = 100;
      delay(7500);
      digitalWrite(ForwardPin, LOW);
      digitalWrite(BackwardPin, LOW);
      analogWrite(EnablePin, 0);
    } else if (posisiAtap == 50) {
      delay(1500);
      Serial.println("Atap Menutup...");
      digitalWrite(ForwardPin, HIGH);
      digitalWrite(BackwardPin, LOW);
      analogWrite(EnablePin, 150);
      posisiAtap = 100;
      delay(3500);
      digitalWrite(ForwardPin, LOW);
      digitalWrite(BackwardPin, LOW);
      analogWrite(EnablePin, 0);
    } else if (posisiAtap == 100) {
      posisiAtap = 100;
    }
    Blynk.virtualWrite(ledOpen, 0);
    Blynk.virtualWrite(ledHalf, 0);
    Blynk.virtualWrite(ledClose, 255);
  }

  delay(5000);
}
