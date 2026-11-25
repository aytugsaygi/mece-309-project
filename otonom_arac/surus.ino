// =====================================================================
//  SÜRÜŞ: L298N motorlar, enkoder, odometri, düz git / yerinde dön
//  Hızlar yüzde (-100..100). Açılar: sola (saat yönü tersi) dönüş +.
// =====================================================================

void motorBaslat() {
  pinMode(PIN_SOL_PWM, OUTPUT);
  pinMode(PIN_SOL_IN1, OUTPUT);
  pinMode(PIN_SOL_IN2, OUTPUT);
  pinMode(PIN_SAG_PWM, OUTPUT);
  pinMode(PIN_SAG_IN3, OUTPUT);
  pinMode(PIN_SAG_IN4, OUTPUT);
  dur();
}

void enkoderBaslat() {
#if ENKODER_VAR
  pinMode(PIN_ENK_SOL, INPUT_PULLUP);
  pinMode(PIN_ENK_SAG, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENK_SOL), solEnkoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENK_SAG), sagEnkoderISR, CHANGE);
#endif
  oncekiOdoMs = millis();
}

// Tek kanallı enkoder yönü bilemez; yönü son motor komutundan alıyoruz
void solEnkoderISR() {
  unsigned long t = micros();
  if (t - solSonKesme < ENK_DEBOUNCE_US) return;
  solSonKesme = t;
  solSayac += solYon;
}

void sagEnkoderISR() {
  unsigned long t = micros();
  if (t - sagSonKesme < ENK_DEBOUNCE_US) return;
  sagSonKesme = t;
  sagSayac += sagYon;
}

void motorYaz(uint8_t pinPwm, uint8_t pinA, uint8_t pinB, int hiz, bool ters) {
  hiz = constrain(hiz, -100, 100);
  if (ters) hiz = -hiz;
  if (hiz == 0) {
    digitalWrite(pinA, LOW);
    digitalWrite(pinB, LOW);
    analogWrite(pinPwm, 0);
    return;
  }
  int pwm = map(abs(hiz), 1, 100, PWM_MIN, PWM_MAX);
  digitalWrite(pinA, hiz > 0 ? HIGH : LOW);
  digitalWrite(pinB, hiz > 0 ? LOW : HIGH);
  analogWrite(pinPwm, pwm);
}

void motorlar(int sol, int sag) {
  odometriGuncelle();   // önceki komutla gidilen yolu hesaba kat (enkodersiz mod için şart)
  sol = constrain(sol, -100, 100);
  sag = constrain(sag, -100, 100);
  solKomut = sol;
  sagKomut = sag;
  if (sol != 0) solYon = sol > 0 ? 1 : -1;
  if (sag != 0) sagYon = sag > 0 ? 1 : -1;
  motorYaz(PIN_SOL_PWM, PIN_SOL_IN1, PIN_SOL_IN2, sol, SOL_MOTOR_TERS);
  motorYaz(PIN_SAG_PWM, PIN_SAG_IN3, PIN_SAG_IN4, (int)(sag * SAG_TRIM), SAG_MOTOR_TERS);
}

void dur() {
  motorlar(0, 0);
}

// Aktif fren: iki uç da HIGH -> motor kısa devre frenler. Daha az kayma, daha doğru mesafe.
void fren() {
  odometriGuncelle();
  solKomut = 0;
  sagKomut = 0;
  digitalWrite(PIN_SOL_IN1, HIGH);
  digitalWrite(PIN_SOL_IN2, HIGH);
  digitalWrite(PIN_SAG_IN3, HIGH);
  digitalWrite(PIN_SAG_IN4, HIGH);
  analogWrite(PIN_SOL_PWM, 255);
  analogWrite(PIN_SAG_PWM, 255);
  delay(60);
  dur();
  delay(40);
  odometriGuncelle();   // frenleme sırasında dönen tekeri de say
}

float aciNormalize(float a) {
  while (a > PI) a -= TWO_PI;
  while (a < -PI) a += TWO_PI;
  return a;
}

// Diferansiyel sürüş odometrisi: robotun (x, y, yön) tahmini
void odometriGuncelle() {
  float dSol, dSag;
#if ENKODER_VAR
  noInterrupts();
  long s = solSayac;
  long g = sagSayac;
  interrupts();
  dSol = (s - oncekiSol) * MM_PER_SAYIM;
  dSag = (g - oncekiSag) * MM_PER_SAYIM;
  oncekiSol = s;
  oncekiSag = g;
#else
  unsigned long simdi = millis();
  float dt = (simdi - oncekiOdoMs) / 1000.0;
  oncekiOdoMs = simdi;
  dSol = solKomut / 100.0 * TAM_HIZ_MM_S * dt;
  dSag = sagKomut / 100.0 * TAM_HIZ_MM_S * dt;
#endif
  float ds = (dSol + dSag) / 2.0;
  float dTeta = (dSag - dSol) / TEKER_ARASI_MM;
  odoX += ds * cos(odoTeta + dTeta / 2.0);
  odoY += ds * sin(odoTeta + dTeta / 2.0);
  odoTeta = aciNormalize(odoTeta + dTeta);
  odoYol += ds;
  odoDonus += dTeta;
}

// Düz git. mm > 0 ileri, mm < 0 geri.
// cizgiKontrol / engelKontrol: ileri giderken çizgi veya engel görürse durur.
HareketSonuc git(int mm, int hiz, bool cizgiKontrol, bool engelKontrol) {
  odometriGuncelle();
  float basYol = odoYol;
  float hedefTeta = odoTeta;
  int yon = mm >= 0 ? 1 : -1;
  float beklenenSn = abs(mm) / (TAM_HIZ_MM_S * hiz / 100.0);
  unsigned long zamanAsimi = 1000UL + (unsigned long)(beklenenSn * 2500.0);
  unsigned long bas = millis();
  HareketSonuc sonuc = TAMAM;

  while (fabs(odoYol - basYol) < abs(mm)) {
    if (millis() - bas > zamanAsimi) break;
    if (cizgiKontrol && yon > 0 && cizgiVarMi()) { sonuc = CIZGI; break; }
    // Kameraya 100 ms'de bir bak ki engel şeridi de kontrol edilsin
    if (engelKontrol && yon > 0 && millis() - sonKameraMs > 100) kameraGuncelle();
    if (engelKontrol && yon > 0 && engelVarMi()) { sonuc = ENGEL; break; }
    float hata = aciNormalize(hedefTeta - odoTeta);   // + ise sola düzeltmeli
    int duz = (int)(hata * KP_DUZ);
    motorlar(yon * hiz - duz, yon * hiz + duz);
    delay(5);
  }
  fren();
  return sonuc;
}

// Yerinde dön. derece > 0 sola, < 0 sağa.
void don(float derece, int hiz) {
  if (fabs(derece) < 2) return;
  odometriGuncelle();
  float basDonus = odoDonus;
  float hedef = fabs(derece) * DEG_TO_RAD;
  int yon = derece > 0 ? 1 : -1;
  float beklenenSn = (hedef * TEKER_ARASI_MM / 2.0) / (TAM_HIZ_MM_S * hiz / 100.0);
  unsigned long zamanAsimi = 1000UL + (unsigned long)(beklenenSn * 2500.0);
  unsigned long bas = millis();

  while (true) {
    float donulen = fabs(odoDonus - basDonus);
    if (donulen >= hedef) break;
    if (millis() - bas > zamanAsimi) break;
    // Son 20 derecede yavaşla, hedefi aşmasın
    int h = (hedef - donulen < 0.35) ? max(HIZ_INCE, hiz / 2) : hiz;
    motorlar(-yon * h, yon * h);
    delay(5);
  }
  fren();
}

// Mutlak yöne dön (odometri yönüne göre)
void yoneDon(float hedefTeta, int hiz) {
  odometriGuncelle();
  don(aciNormalize(hedefTeta - odoTeta) * RAD_TO_DEG, hiz);
}
