// =====================================================================
//  KAMERA: HuskyLens (Color Recognition) okuma ve hedef seçimi
//  ID1 = kırmızı kutu, ID2 = bırakma alanı kartı, ID3 = engel şeridi
//  Görüntü 320x240 piksel; x,y blok merkezi, w,h blok boyutu
// =====================================================================

// Kameradan yeni bir kare iste. Kutuları, en büyük işareti ve en büyük engel şeridini sakla.
bool kameraGuncelle() {
  kutuAdedi = 0;
  isaret.var = false;
  engelBlok.var = false;
  if (!huskylens.request()) return false;
  sonKameraMs = millis();

  while (huskylens.available()) {
    HUSKYLENSResult r = huskylens.read();
    if (r.command != COMMAND_RETURN_BLOCK) continue;
    Blok b;
    b.var = true;
    b.x = r.xCenter;
    b.y = r.yCenter;
    b.w = r.width;
    b.h = r.height;
    if (r.ID == KUTU_ID && kutuAdedi < MAKS_KUTU) {
      kutular[kutuAdedi++] = b;
    } else if (r.ID == ISARET_ID && (!isaret.var || b.w > isaret.w)) {
      isaret = b;
    } else if (r.ID == ENGEL_ID && (!engelBlok.var || b.w > engelBlok.w)) {
      engelBlok = b;
    }
  }

  if (isaret.var) alanKonumunuGuncelle();
  return true;
}

// Engel şeridi robotun önündeki yolda ve yakın mı?
// Kamera aşağı baktığı için yakın nesneler görüntünün alt kısmında görünür.
bool kameraEngelYakin() {
  if (!engelBlok.var || millis() - sonKameraMs > KAMERA_ENGEL_GECERLILIK_MS) return false;
  int alt = engelBlok.y + engelBlok.h / 2;
  int sol = engelBlok.x - engelBlok.w / 2;
  int sag = engelBlok.x + engelBlok.w / 2;
  bool yolda = sag > KAMERA_ORTA_X - ENGEL_YOL_YARIM_PX && sol < KAMERA_ORTA_X + ENGEL_YOL_YARIM_PX;
  return yolda && alt >= ENGEL_YAKIN_ALT_Y;
}

// Görülen bloğun saha üzerindeki yaklaşık konumu (odometri çerçevesinde)
void blokKonumu(const Blok &b, float k, float &x, float &y) {
  float mesafe = k / max((int)b.w, 1);
  float aci = -(b.x - KAMERA_ORTA_X) * KAMERA_DERECE_PIKSEL * DEG_TO_RAD;  // sağdaki nesne -> eksi açı
  x = odoX + mesafe * cos(odoTeta + aci);
  y = odoY + mesafe * sin(odoTeta + aci);
}

// Basit haritalama: işaret her görüldüğünde alanın yerini hatırla
void alanKonumunuGuncelle() {
  blokKonumu(isaret, ISARET_MESAFE_K, alanX, alanY);
  alanBiliniyor = true;
}

// Alana daha önce bırakılmış kutu mu? (onları tekrar almaya çalışmayalım)
bool yerlesmisKutuMu(const Blok &b) {
  if (birakilanKutu == 0) return false;

  // 1) Aynı karede işaret görünüyorsa: işaretin altında ve kutu sırası
  //    genişliği içinde kalan kırmızılar alandaki kutulardır.
  if (isaret.var) {
    float pikselPerMm = isaret.w / ISARET_GENISLIK_MM;
    float siraYariGenislikPx = (BIRAKMA_ARALIK_MM + 100) * pikselPerMm;
    if (abs(b.x - isaret.x) < siraYariGenislikPx && b.y > isaret.y) return true;
  }

  // 2) Hafızadaki alan konumuna çok yakınsa
  if (alanBiliniyor) {
    float kx, ky;
    blokKonumu(b, KUTU_MESAFE_K, kx, ky);
    float dx = kx - alanX, dy = ky - alanY;
    if (sqrt(dx * dx + dy * dy) < YERLESMIS_YARICAP_MM) return true;
  }
  return false;
}

bool kutuGecerli(const Blok &b) {
  if (b.w < KUTU_MIN_GENISLIK || b.h < KUTU_MIN_GENISLIK) return false;
  float oran = (float)b.h / b.w;
  if (oran < 0.4 || oran > 2.5) return false;   // çok ince/uzun kırmızılar kutu değil
  return !yerlesmisKutuMu(b);
}

// Geçerli kutulardan en yakını (en geniş görüneni) seç
bool enIyiKutuBul(Blok &sonuc) {
  int enIyi = -1;
  for (uint8_t i = 0; i < kutuAdedi; i++) {
    if (!kutuGecerli(kutular[i])) continue;
    if (enIyi < 0 || kutular[i].w > kutular[enIyi].w) enIyi = i;
  }
  if (enIyi < 0) return false;
  sonuc = kutular[enIyi];
  return true;
}
