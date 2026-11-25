// =====================================================================
//  MECE-309 Otonom Araç - AYARLAR
//  Pinler, hızlar, eşikler ve kalibrasyon sabitleri bu dosyada.
//  "KALİBRE" yazan değerleri kalibrasyon modunda ölçüp güncelleyin.
// =====================================================================
#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------
// ÇALIŞMA MODU
//   0: Görev - robot açılınca kutuları toplamaya başlar
//   1: Kalibrasyon - Seri Monitör (115200 baud) üzerinden test komutları
// ---------------------------------------------------------------------
#define CALISMA_MODU 0

// Enkoder modülü (LM393 hız sensörü + 20 delikli disk) takılı mı?
//   1: evet, mesafe ve dönüş enkoderle ölçülür
//   0: hayır, mesafe ve dönüş süreyle tahmin edilir (yedek mod, daha az hassas)
#define ENKODER_VAR 1

// ---------------------------------------------------------------------
// PİNLER (Arduino Uno)
// ---------------------------------------------------------------------
// L298N motor sürücü (ENA/ENB üzerindeki jumper'ları çıkarın!)
const uint8_t PIN_SOL_PWM = 5;   // ENA
const uint8_t PIN_SOL_IN1 = 4;   // IN1
const uint8_t PIN_SOL_IN2 = 7;   // IN2
const uint8_t PIN_SAG_PWM = 6;   // ENB
const uint8_t PIN_SAG_IN3 = 8;   // IN3
const uint8_t PIN_SAG_IN4 = 12;  // IN4

// Enkoder (Uno'da kesme yalnızca 2 ve 3 numaralı pinlerde var)
const uint8_t PIN_ENK_SOL = 2;
const uint8_t PIN_ENK_SAG = 3;

// Forklift servosu (MG996R) - Servo kütüphanesi 9-10'da PWM'i kapatır
const uint8_t PIN_SERVO = 9;

// TCRT5000 çizgi sensörleri - modülün AO (analog) çıkışı
const uint8_t PIN_IR_SOL = A0;
const uint8_t PIN_IR_SAG = A1;

// HC-SR04 ultrasonik sensörler (ön sol ve ön sağ köşe)
const uint8_t PIN_US_SOL_TRIG = 10;
const uint8_t PIN_US_SOL_ECHO = 11;
const uint8_t PIN_US_SAG_TRIG = A2;
const uint8_t PIN_US_SAG_ECHO = A3;

// Durum LED'i (kart üstündeki LED): kutu taşınırken yanar
const uint8_t PIN_LED = 13;

// HuskyLens I2C: A4 = SDA (yeşil "T" kablosu), A5 = SCL (mavi "R" kablosu)

// ---------------------------------------------------------------------
// MOTORLAR
// ---------------------------------------------------------------------
const bool SOL_MOTOR_TERS = false;  // ileri komutunda geri dönüyorsa true yapın
const bool SAG_MOTOR_TERS = false;
const int  PWM_MIN = 90;            // KALİBRE: tekerin dönmeye başladığı PWM
const int  PWM_MAX = 255;           // 2S pil (7.4 V) için 255. 3S kullanırsanız ~170
const float SAG_TRIM = 1.00;        // enkodersiz modda düz gitmiyorsa sağ motor çarpanı

// Hızlar yüzde olarak (0-100)
const int HIZ_NORMAL = 70;
const int HIZ_YAVAS  = 45;
const int HIZ_INCE   = 35;   // ince hizalama ve çizgiye hizalanma
const int HIZ_DONUS  = 55;
const int HIZ_TARAMA = 50;

// Düz giderken yön tutma kazancı (radyan hata -> % hız farkı)
const float KP_DUZ = 150.0;

// ---------------------------------------------------------------------
// GEOMETRİ VE ENKODER
// ---------------------------------------------------------------------
const float TEKER_CAP_MM   = 65.0;   // KALİBRE: 500 mm ileri testinde düzeltin
const float TEKER_ARASI_MM = 150.0;  // KALİBRE: mekanik tasarımdan sonra ölçün,
                                      // 360° dönüş testinde tam tur atana kadar ayarlayın
const int   ENK_DELIK      = 20;
// CHANGE kesmesiyle her delik 2 sayım verir -> tur başına 40 sayım (~5 mm)
const float MM_PER_SAYIM   = (PI * TEKER_CAP_MM) / (ENK_DELIK * 2);
const unsigned long ENK_DEBOUNCE_US = 1500;

// %100 komutta tekerin hızı (mm/s). Enkoderli modda sadece zaman aşımı hesabında,
// enkodersiz modda mesafe tahmininde kullanılır.
const float TAM_HIZ_MM_S   = 450.0;  // KALİBRE (enkodersiz modda, dolu pille)

// ---------------------------------------------------------------------
// FORKLIFT (servo açıları)
// ---------------------------------------------------------------------
const int FORK_ALT_DERECE = 20;   // KALİBRE: çatal yere yakın, kutu kanalına girebilir
const int FORK_UST_DERECE = 70;   // KALİBRE: kutu yerden 1-2 cm kalkmış
const int FORK_ADIM_MS    = 8;    // her derece arası bekleme (yumuşak kaldırma)

// ---------------------------------------------------------------------
// ÇİZGİ SENSÖRLERİ
// Açılışta zemin okunur. Okuma zeminden bu kadar saparsa çizgi sayılır.
// Böylece sınır bandı siyah da olsa beyaz da olsa çalışır.
// ---------------------------------------------------------------------
const int IR_ESIK_FARK = 150;     // KALİBRE: 0-1023 ölçeğinde

// ---------------------------------------------------------------------
// ULTRASONİK
// ---------------------------------------------------------------------
const int ENGEL_MESAFE_CM = 25;            // bundan yakın = engel
const int TASIMA_KOR_CM   = 12;            // kutu taşırken bundan yakını yoksay (kutunun yankısı)
const unsigned long US_ZAMAN_ASIMI_US = 6000;  // ~1 m

// ---------------------------------------------------------------------
// HUSKYLENS (Color Recognition modu, "Learn Multiple" açık)
// ---------------------------------------------------------------------
const uint8_t KUTU_ID   = 1;     // ID1 = kırmızı kutu
const uint8_t ISARET_ID = 2;     // ID2 = bırakma alanındaki renkli kart (mavi)
const uint8_t ENGEL_ID  = 3;     // ID3 = engelin üstündeki renkli şerit (yeşil)
const int KAMERA_ORTA_X = 160;   // görüntü 320x240

// Engel şeridi: alt kenarı görüntüde bu y değerinden aşağıdaysa engel yakın sayılır
const int ENGEL_YAKIN_ALT_Y = 170;      // KALİBRE: engel 25 cm öndeyken şeridin alt kenarı
const int ENGEL_YOL_YARIM_PX = 70;      // görüntü ortasında robotun yolu sayılan şeridin yarı genişliği
const unsigned long KAMERA_ENGEL_GECERLILIK_MS = 300;  // bundan eski kamera verisi kullanılmaz

const int KUTU_MIN_GENISLIK      = 12;   // daha küçük kırmızı lekeler gürültü sayılır
const int KUTU_YAKIN_GENISLIK    = 110;  // KALİBRE: kutu bu genişlikteyken son yaklaşmaya geç
const int KUTU_US_KAPAT_GENISLIK = 60;   // kutu bu kadar büyükse ultrasoniği yoksay (kutunun kendisi)
const int ISARET_YAKIN_GENISLIK  = 90;   // KALİBRE: robot alan çizgisine yaklaştığında işaretin genişliği

const float KP_HIZALAMA = 0.25;          // piksel hatası -> % hız farkı
const int HIZALAMA_TOLERANS_PX = 10;
const unsigned long HEDEF_KAYIP_MS = 400;

// Mesafe tahmini: mesafe_mm = K / genislik_px
// KALİBRE: nesneyi bilinen mesafeye koyun, K = genislik_px * mesafe_mm
const float KUTU_MESAFE_K   = 12000.0;
const float ISARET_MESAFE_K = 18000.0;
const float ISARET_GENISLIK_MM = 150.0;   // bırakma işaretinin gerçek genişliği
const float KAMERA_DERECE_PIKSEL = 0.19;  // yaklaşık görüş açısı / 320

// ---------------------------------------------------------------------
// GÖREV MANEVRALARI
// ---------------------------------------------------------------------
const int KUTU_SAYISI = 3;
const int SON_ITIS_MM = 130;          // KALİBRE: çatal kutunun altına tam girene kadar ileri
// Kutu merkezleri arası: 10 cm kutu + 4 cm boşluk. Kural: boşluk 1-7 cm -> 110-170 arası,
// ortadaki 140 iki yönde de 3 cm hata payı bırakır.
const int BIRAKMA_ARALIK_MM = 140;
const int BIRAKMA_GERI_MM = 150;      // yana kaymadan önce geri çekilme
const int BIRAKMA_ILERI_EK_MM = 0;    // kutuyu alanın daha içine itmek için ek ileri
const int BIRAKMA_SONRASI_GERI_MM = 200;
// Bırakma sırası: +1 sağ, -1 sol, 0 orta. Orta en sona kalır; böylece
// robot her seferinde boş olan ortadan çizgiye hizalanabilir.
const int8_t BIRAKMA_SIRASI[KUTU_SAYISI] = {1, -1, 0};

// Engel 1 m uzunluğunda bir çubuk: yanından geçmek için geniş dönüş ve uzun ilerleme
const int KACINMA_GERI_MM  = 100;
const int KACINMA_DONUS_DERECE = 70;
const int KACINMA_ILERI_MM = 350;
const int KESIF_ILERI_MM   = 500;     // tarama sonuç vermezse bu kadar ileri git
const int TARAMA_ADIM_DERECE = 30;
const int TARAMA_BEKLEME_MS  = 150;   // her adımdan sonra kameranın yeni kare alması için
const float YERLESMIS_YARICAP_MM = 350.0;  // alana bu kadar yakın kırmızılar bırakılmış kutudur

const unsigned long GOREV_SURESI_MS = 600000UL;  // 10 dakika

// ---------------------------------------------------------------------
// TİPLER
// ---------------------------------------------------------------------
struct Blok {
  bool var;
  int16_t x, y, w, h;
};

enum Durum {
  BASLANGIC,
  KUTU_ARA,
  KUTUYA_YAKLAS,
  KUTU_AL,
  ALAN_ARA,
  ALANA_YAKLAS,
  ALAN_HIZALAN,
  KUTU_BIRAK,
  BITTI
};

enum HareketSonuc { TAMAM, CIZGI, ENGEL };
