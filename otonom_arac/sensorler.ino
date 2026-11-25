// =====================================================================
//  SENSÖRLER: TCRT5000 çizgi (analog), HC-SR04 ultrasonik
// =====================================================================

void sensorBaslat() {
  pinMode(PIN_US_SOL_TRIG, OUTPUT);
  pinMode(PIN_US_SOL_ECHO, INPUT);
  pinMode(PIN_US_SAG_TRIG, OUTPUT);
  pinMode(PIN_US_SAG_ECHO, INPUT);
  digitalWrite(PIN_US_SOL_TRIG, LOW);
  digitalWrite(PIN_US_SAG_TRIG, LOW);
}

// Açılışta robot düz zemin üzerindeyken zemin değerini ölç.
// Sınır bandı siyah da olsa beyaz da olsa zeminden farklı okunur.
void irKalibre() {
  long s = 0, g = 0;
  for (int i = 0; i < 50; i++) {
    s += analogRead(PIN_IR_SOL);
    g += analogRead(PIN_IR_SAG);
    delay(5);
  }
  irZeminSol = s / 50;
  irZeminSag = g / 50;
  Serial.print(F("Zemin IR sol/sag: "));
  Serial.print(irZeminSol);
  Serial.print(F(" / "));
  Serial.println(irZeminSag);
}

bool solCizgide() {
  int v = (analogRead(PIN_IR_SOL) + analogRead(PIN_IR_SOL)) / 2;
  return abs(v - irZeminSol) > IR_ESIK_FARK;
}

bool sagCizgide() {
  int v = (analogRead(PIN_IR_SAG) + analogRead(PIN_IR_SAG)) / 2;
  return abs(v - irZeminSag) > IR_ESIK_FARK;
}

// Hangi sensörün gördüğünü kaçınma manevrası için saklar
bool cizgiVarMi() {
  sonCizgiSol = solCizgide();
  sonCizgiSag = sagCizgide();
  return sonCizgiSol || sonCizgiSag;
}

int usOlc(uint8_t trig, uint8_t echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long sure = pulseIn(echo, HIGH, US_ZAMAN_ASIMI_US);
  if (sure == 0) return 999;    // menzilde bir şey yok
  return sure / 58;
}

// Her çağrıda en fazla bir sensör ölçülür, sensörler sırayla ve 30 ms arayla
// tetiklenir; böylece birbirlerinin yankısını duymazlar ve döngü yavaşlamaz.
void usGuncelle() {
  if (millis() - usSonOlcumMs < 30) return;
  usSonOlcumMs = millis();
  if (usSiradakiSol) usSolCm = usOlc(PIN_US_SOL_TRIG, PIN_US_SOL_ECHO);
  else               usSagCm = usOlc(PIN_US_SAG_TRIG, PIN_US_SAG_ECHO);
  usSiradakiSol = !usSiradakiSol;
}

// Engel ya da duvar: ultrasonik yakın bir şey ölçtüyse veya kamera engel şeridini yolda gördüyse
bool engelVarMi() {
  usGuncelle();
  int alt = kutuTasiniyor ? TASIMA_KOR_CM : 2;
  bool sol = usSolCm > alt && usSolCm < ENGEL_MESAFE_CM;
  bool sag = usSagCm > alt && usSagCm < ENGEL_MESAFE_CM;
  return sol || sag || kameraEngelYakin();
}
