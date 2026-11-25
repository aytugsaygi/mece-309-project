// =====================================================================
//  MECE-309 Otonom Araç - ANA PROGRAM
//
//  Görev: rastgele yerlerdeki 3 kırmızı kutuyu bul, forkliftle al,
//  renkli işaretle belirtilmiş bırakma alanına yan yana bırak.
//  Saha sınırına (bant) ve ortadaki engele çarpma.
//
//  Dosyalar:
//    ayarlar.h       pinler, hızlar, eşikler, kalibrasyon sabitleri
//    otonom_arac.ino durum makinesi (bu dosya)
//    surus.ino       motorlar, enkoder, odometri, düz git / dön
//    sensorler.ino   TCRT5000 çizgi, HC-SR04 engel
//    kamera.ino      HuskyLens okuma, kutu/işaret seçimi
//    forklift.ino    servo
//    kalibrasyon.ino seri monitör test menüsü (CALISMA_MODU 1)
//
//  Gerekli kütüphaneler: HUSKYLENS (DFRobot), Servo, Wire
// =====================================================================
#include <Wire.h>
#include <Servo.h>
#include "HUSKYLENS.h"
#include "ayarlar.h"

// ---------------------------- Global durum ----------------------------
HUSKYLENS huskylens;
Servo catal;
int catalAci = FORK_UST_DERECE;

Durum durum = BASLANGIC;
int birakilanKutu = 0;
bool kutuTasiniyor = false;
unsigned long gorevBaslangicMs = 0;
unsigned long sonGorulmeMs = 0;
unsigned long sonIsaretYakinMs = 0;
int sonGorulenHata = 0;      // hedef en son görüntünün neresindeydi (+ sağ, - sol)
float hizaTeta = 0;          // alan çizgisine dik yön (bırakırken kullanılır)

// Enkoder
volatile long solSayac = 0, sagSayac = 0;
volatile int8_t solYon = 1, sagYon = 1;
volatile unsigned long solSonKesme = 0, sagSonKesme = 0;

// Odometri (başlangıç noktası = 0,0; başlangıç yönü = 0 rad; sola dönüş +)
float odoX = 0, odoY = 0, odoTeta = 0;
float odoYol = 0;            // toplam yol (ileri +, geri -)
float odoDonus = 0;          // toplam dönüş (sarmalanmamış, radyan)
long oncekiSol = 0, oncekiSag = 0;
unsigned long oncekiOdoMs = 0;
int solKomut = 0, sagKomut = 0;

// Alan hafızası: işaret her görüldüğünde konumu güncellenir
bool alanBiliniyor = false;
float alanX = 0, alanY = 0;

// Sensör önbellekleri
int irZeminSol = 0, irZeminSag = 0;
bool sonCizgiSol = false, sonCizgiSag = false;
int usSolCm = 999, usSagCm = 999;
unsigned long usSonOlcumMs = 0;
bool usSiradakiSol = true;

// Kamera
const uint8_t MAKS_KUTU = 6;
Blok kutular[MAKS_KUTU];
uint8_t kutuAdedi = 0;
Blok isaret;
Blok engelBlok;              // engel şeridi (görüntüdeki en büyüğü)
Blok hedef;                  // en son seçilen kutu veya işaret
unsigned long sonKameraMs = 0;

// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);

  motorBaslat();
  enkoderBaslat();
  sensorBaslat();
  catalBaslat();

  Wire.begin();
  int deneme = 0;
  while (!huskylens.begin(Wire)) {
    Serial.println(F("HuskyLens bekleniyor... (I2C modu ve kabloları kontrol edin)"));
    digitalWrite(PIN_LED, !digitalRead(PIN_LED));
    delay(300);
    if (CALISMA_MODU == 1 && ++deneme > 10) break;  // kalibrasyonda kamerasız da devam et
  }
  huskylens.writeAlgorithm(ALGORITHM_COLOR_RECOGNITION);
  digitalWrite(PIN_LED, LOW);

  delay(500);        // robot yerinde dursun, zemini okuyacağız
  irKalibre();

#if CALISMA_MODU == 1
  kalibrasyonYardim();
#else
  gorevBaslangicMs = millis();
  durumDegistir(KUTU_ARA);
#endif
}

void loop() {
#if CALISMA_MODU == 1
  kalibrasyonDongusu();
#else
  odometriGuncelle();

  if (durum != BITTI && millis() - gorevBaslangicMs > GOREV_SURESI_MS) {
    durumDegistir(BITTI);
  }

  switch (durum) {
    case KUTU_ARA:      kutuAraDurumu();      break;
    case KUTUYA_YAKLAS: kutuyaYaklasDurumu(); break;
    case KUTU_AL:       kutuAlDurumu();       break;
    case ALAN_ARA:      alanAraDurumu();      break;
    case ALANA_YAKLAS:  alanaYaklasDurumu();  break;
    case ALAN_HIZALAN:  alanHizalanDurumu();  break;
    case KUTU_BIRAK:    kutuBirakDurumu();    break;
    case BITTI:         bittiDurumu();        break;
    default: break;
  }
#endif
}

// ---------------------------------------------------------------------
void durumDegistir(Durum yeni) {
  durum = yeni;
  sonGorulmeMs = millis();
  Serial.print(F("[DURUM] "));
  Serial.print((millis() - gorevBaslangicMs) / 1000.0, 1);
  Serial.print(F(" s -> "));
  Serial.println(durumAdi(yeni));
}

const __FlashStringHelper* durumAdi(Durum d) {
  switch (d) {
    case KUTU_ARA:      return F("KUTU_ARA");
    case KUTUYA_YAKLAS: return F("KUTUYA_YAKLAS");
    case KUTU_AL:       return F("KUTU_AL");
    case ALAN_ARA:      return F("ALAN_ARA");
    case ALANA_YAKLAS:  return F("ALANA_YAKLAS");
    case ALAN_HIZALAN:  return F("ALAN_HIZALAN");
    case KUTU_BIRAK:    return F("KUTU_BIRAK");
    case BITTI:         return F("BITTI");
    default:            return F("BASLANGIC");
  }
}

// ------------------------- KUTU_ARA ----------------------------------
// Yerinde adım adım dönerek kırmızı kutu ara. Tam turda bulamazsa ileri keşfe çık.
void kutuAraDurumu() {
  catalIndir();   // boşken çatal hep aşağıda, kutuya hazır
  if (tara(true, 360 - TARAMA_ADIM_DERECE, sonGorulenHata >= 0 ? -1 : 1)) {
    durumDegistir(KUTUYA_YAKLAS);
    return;
  }
  kesifYap();
}

// ------------------------- KUTUYA_YAKLAS -----------------------------
// Kutuyu görüntünün ortasında tutarak yaklaş.
void kutuyaYaklasDurumu() {
  kameraGuncelle();
  if (!enIyiKutuBul(hedef)) {
    dur();
    if (millis() - sonGorulmeMs > HEDEF_KAYIP_MS) durumDegistir(KUTU_ARA);
    return;
  }
  sonGorulmeMs = millis();
  int hata = hedef.x - KAMERA_ORTA_X;
  sonGorulenHata = hata;

  if (hedef.w >= KUTU_YAKIN_GENISLIK) {
    dur();
    durumDegistir(KUTU_AL);
    return;
  }

  bool yakin = hedef.w >= KUTU_US_KAPAT_GENISLIK;
  // Kutu yakındayken çizgiyi yoksay: kutu sınırın dibindeyse teker çizgiye değebilir (serbest)
  if (!yakin && cizgiVarMi()) {
    cizgidenKac();
    durumDegistir(KUTU_ARA);
    return;
  }
  // Kutu yakındayken ultrasonik kutunun kendisini görür, yoksay
  if (!yakin && engelVarMi()) {
    engeldenKac();
    durumDegistir(KUTU_ARA);
    return;
  }

  int taban = (hedef.w < KUTU_YAKIN_GENISLIK / 2) ? HIZ_NORMAL : HIZ_YAVAS;
  if (abs(hata) > 70) taban = 0;   // hedef çok yandaysa önce yerinde dön
  int fark = constrain((int)(hata * KP_HIZALAMA), -40, 40);
  motorlar(taban + fark, taban - fark);
}

// ------------------------- KUTU_AL -----------------------------------
void kutuAlDurumu() {
  // 1) İnce hizalama: kutu tam ortada olsun ki çatal 60 mm'lik kanala girsin
  unsigned long bas = millis();
  while (millis() - bas < 1500) {
    kameraGuncelle();
    Blok b;
    if (!enIyiKutuBul(b)) break;
    int hata = b.x - KAMERA_ORTA_X;
    if (abs(hata) <= HIZALAMA_TOLERANS_PX) break;
    int h = hata > 0 ? HIZ_INCE : -HIZ_INCE;
    motorlar(h, -h);
    delay(40);
    dur();
    delay(60);
  }
  dur();

  // 2) Çatal aşağı, körlemesine ileri, kaldır
  catalIndir();
  git(SON_ITIS_MM, HIZ_YAVAS, false, false);
  catalKaldir();

  kutuTasiniyor = true;
  digitalWrite(PIN_LED, HIGH);
  durumDegistir(ALAN_ARA);
}

// ------------------------- ALAN_ARA ----------------------------------
void alanAraDurumu() {
  // Alanı daha önce gördüysek önce o yöne dön ve çevresine bak
  if (alanBiliniyor) {
    float hedefAci = atan2(alanY - odoY, alanX - odoX);
    don(aciNormalize(hedefAci - odoTeta) * RAD_TO_DEG, HIZ_DONUS);
    don(-60, HIZ_TARAMA);
    if (tara(false, 120, 1)) {
      durumDegistir(ALANA_YAKLAS);
      return;
    }
  }
  if (tara(false, 360 - TARAMA_ADIM_DERECE, sonGorulenHata >= 0 ? -1 : 1)) {
    durumDegistir(ALANA_YAKLAS);
    return;
  }
  kesifYap();
}

// ------------------------- ALANA_YAKLAS ------------------------------
void alanaYaklasDurumu() {
  kameraGuncelle();
  bool gorunuyor = isaret.var;
  if (gorunuyor && isaret.w >= ISARET_YAKIN_GENISLIK) sonIsaretYakinMs = millis();
  bool alanYakin = sonIsaretYakinMs != 0 && (millis() - sonIsaretYakinMs) < 1500;

  if (cizgiVarMi()) {
    if (alanYakin) {           // işaret yakındayken görülen çizgi = bırakma alanının çizgisi
      dur();
      durumDegistir(ALAN_HIZALAN);
    } else {                   // işaret uzaktayken görülen çizgi = saha sınırı
      cizgidenKac();
      durumDegistir(ALAN_ARA);
    }
    return;
  }

  if (!gorunuyor) {
    if (alanYakin) {           // işaret çok yakınken görüş alanından çıkabilir, düz devam
      motorlar(HIZ_INCE, HIZ_INCE);
      return;
    }
    dur();
    if (millis() - sonGorulmeMs > HEDEF_KAYIP_MS) durumDegistir(ALAN_ARA);
    return;
  }
  sonGorulmeMs = millis();

  // Alana yakınken ultrasonik daha önce bırakılmış kutuları görür, yoksay
  if (!alanYakin && engelVarMi()) {
    engeldenKac();
    durumDegistir(ALAN_ARA);
    return;
  }

  int hata = isaret.x - KAMERA_ORTA_X;
  sonGorulenHata = hata;
  int taban = alanYakin ? HIZ_YAVAS : HIZ_NORMAL;
  if (abs(hata) > 70) taban = 0;
  int fark = constrain((int)(hata * KP_HIZALAMA), -40, 40);
  motorlar(taban + fark, taban - fark);
}

// ------------------------- ALAN_HIZALAN ------------------------------
// İki çizgi sensörü de çizgiyi görene kadar her teker ayrı ilerler.
// Sonuçta robot çizgiye dik durur; böylece kutular aynı açıyla bırakılır.
void alanHizalanDurumu() {
  cizgiyeHizalan();
  hizaTeta = odoTeta;
  durumDegistir(KUTU_BIRAK);
}

// Çizgiye dik hizalan: her teker kendi sensörü çizgiyi görene kadar ilerler.
// Her bırakmadan önce bu adım, yana kayma sırasında biriken açı ve mesafe hatasını sıfırlar.
void cizgiyeHizalan() {
  unsigned long bas = millis();
  while (millis() - bas < 2500) {
    bool sol = solCizgide();
    bool sag = sagCizgide();
    if (sol && sag) break;
    motorlar(sol ? 0 : HIZ_INCE, sag ? 0 : HIZ_INCE);
    delay(5);
  }
  dur();
  odometriGuncelle();
}

// ------------------------- KUTU_BIRAK --------------------------------
void kutuBirakDurumu() {
  int8_t taraf = BIRAKMA_SIRASI[min(birakilanKutu, KUTU_SAYISI - 1)];

  if (taraf != 0) {
    // Geri çekil, yana kay, tekrar çizgiye dik dön, aynı mesafe ileri git
    git(-BIRAKMA_GERI_MM, HIZ_YAVAS, false, false);
    yoneDon(hizaTeta + (taraf > 0 ? -HALF_PI : HALF_PI), HIZ_DONUS);
    git(BIRAKMA_ARALIK_MM, HIZ_YAVAS, false, false);
    yoneDon(hizaTeta, HIZ_DONUS);
    cizgiyeHizalan();   // yana kayarken açı kayabilir, çizgiyi sensörle bulup yeniden dik dur
  }
  if (BIRAKMA_ILERI_EK_MM > 0) git(BIRAKMA_ILERI_EK_MM, HIZ_INCE, false, false);

  catalIndir();
  delay(200);
  git(-BIRAKMA_SONRASI_GERI_MM, HIZ_YAVAS, false, false);

  kutuTasiniyor = false;
  digitalWrite(PIN_LED, LOW);
  birakilanKutu++;
  Serial.print(F("Birakilan kutu: "));
  Serial.println(birakilanKutu);

  if (birakilanKutu >= KUTU_SAYISI) {
    durumDegistir(BITTI);
  } else {
    don(180, HIZ_DONUS);   // alana sırtını dön, yeni kutu ara
    durumDegistir(KUTU_ARA);
  }
}

// ------------------------- BITTI -------------------------------------
void bittiDurumu() {
  dur();
  digitalWrite(PIN_LED, (millis() / 500) % 2);
}

// ------------------------- Yardımcı davranışlar ----------------------

// Adım adım dönerek hedef ara. kutuAraniyor: true = kırmızı kutu, false = alan işareti.
// yon: +1 sola, -1 sağa. Bulursa true döner ve 'hedef' doldurulur.
bool tara(bool kutuAraniyor, int toplamDerece, int yon) {
  for (int donulen = 0; donulen <= toplamDerece; donulen += TARAMA_ADIM_DERECE) {
    if (donulen > 0) don(yon * TARAMA_ADIM_DERECE, HIZ_TARAMA);
    delay(TARAMA_BEKLEME_MS);
    if (hedefGoruluyor(kutuAraniyor)) return true;
  }
  return false;
}

bool hedefGoruluyor(bool kutuAraniyor) {
  if (!kameraGuncelle()) return false;
  if (kutuAraniyor) return enIyiKutuBul(hedef);
  if (isaret.var) {
    hedef = isaret;
    return true;
  }
  return false;
}

// Hiçbir şey görünmüyorsa ileri git; sınıra veya engele gelirse kaç
void kesifYap() {
  HareketSonuc s = git(KESIF_ILERI_MM, HIZ_NORMAL, true, true);
  if (s == CIZGI) cizgidenKac();
  else if (s == ENGEL) engeldenKac();
}

void cizgidenKac() {
  bool sol = sonCizgiSol, sag = sonCizgiSag;
  dur();
  git(-KACINMA_GERI_MM, HIZ_NORMAL, false, false);
  float aci = 180;
  if (sol && !sag) aci = -135;        // çizgi solda -> sağa dön
  else if (sag && !sol) aci = 135;    // çizgi sağda -> sola dön
  don(aci, HIZ_DONUS);
}

void engeldenKac() {
  // Kamera şeridi görüyorsa görüntüde daha çok boşluk kalan tarafa dön (çubuğun yakın ucu),
  // görmüyorsa ultrasoniğin daha uzak ölçtüğü tarafa dön
  bool solaDon;
  if (engelBlok.var && millis() - sonKameraMs < KAMERA_ENGEL_GECERLILIK_MS) {
    int solBosluk = engelBlok.x - engelBlok.w / 2;
    int sagBosluk = 2 * KAMERA_ORTA_X - (engelBlok.x + engelBlok.w / 2);
    solaDon = solBosluk > sagBosluk;
  } else {
    solaDon = usSolCm > usSagCm;
  }
  dur();
  git(-KACINMA_GERI_MM, HIZ_NORMAL, false, false);
  don(solaDon ? KACINMA_DONUS_DERECE : -KACINMA_DONUS_DERECE, HIZ_DONUS);
  HareketSonuc s = git(KACINMA_ILERI_MM, HIZ_NORMAL, true, true);
  if (s == CIZGI) cizgidenKac();
}
