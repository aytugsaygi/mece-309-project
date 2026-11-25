// =====================================================================
//  KALİBRASYON MENÜSÜ  (ayarlar.h içinde CALISMA_MODU 1 yapın)
//  Seri Monitör: 115200 baud, satır sonu "Yok" ya da "Yeni satır"
// =====================================================================

void kalibrasyonYardim() {
  Serial.println(F("\n=== KALIBRASYON MENUSU ==="));
  Serial.println(F("k : kamera bloklarini yazdir (5 s)"));
  Serial.println(F("i : cizgi sensorleri (5 s)"));
  Serial.println(F("u : ultrasonik (5 s)"));
  Serial.println(F("m : motor yonu testi (sol ileri/geri, sag ileri/geri)"));
  Serial.println(F("f : 500 mm ileri git     g : 500 mm geri git"));
  Serial.println(F("d : 360 derece don       r : 90 derece sola don"));
  Serial.println(F("a : catal asagi          y : catal yukari"));
  Serial.println(F("s<aci> : servoyu aciya gotur (ornek: s45)"));
  Serial.println(F("o : odometriyi yazdir    h : bu menu"));
}

void kalibrasyonDongusu() {
  if (!Serial.available()) return;
  char c = Serial.read();
  unsigned long bas;

  switch (c) {
    case 'k':
      bas = millis();
      while (millis() - bas < 5000) {
        if (kameraGuncelle()) {
          for (uint8_t i = 0; i < kutuAdedi; i++) {
            Blok &b = kutular[i];
            Serial.print(F("KUTU x=")); Serial.print(b.x);
            Serial.print(F(" y=")); Serial.print(b.y);
            Serial.print(F(" w=")); Serial.print(b.w);
            Serial.print(F(" h=")); Serial.println(b.h);
          }
          if (isaret.var) {
            Serial.print(F("ISARET x=")); Serial.print(isaret.x);
            Serial.print(F(" y=")); Serial.print(isaret.y);
            Serial.print(F(" w=")); Serial.print(isaret.w);
            Serial.print(F(" h=")); Serial.println(isaret.h);
          }
          if (engelBlok.var) {
            Serial.print(F("ENGEL x=")); Serial.print(engelBlok.x);
            Serial.print(F(" w=")); Serial.print(engelBlok.w);
            Serial.print(F(" alt_y=")); Serial.print(engelBlok.y + engelBlok.h / 2);
            Serial.print(F(" yakin=")); Serial.println(kameraEngelYakin());
          }
          if (kutuAdedi == 0 && !isaret.var && !engelBlok.var) Serial.println(F("(blok yok)"));
        } else {
          Serial.println(F("HuskyLens cevap vermiyor"));
        }
        delay(200);
      }
      break;

    case 'i':
      bas = millis();
      while (millis() - bas < 5000) {
        Serial.print(F("IR sol=")); Serial.print(analogRead(PIN_IR_SOL));
        Serial.print(F(" (zemin ")); Serial.print(irZeminSol);
        Serial.print(F(")  sag=")); Serial.print(analogRead(PIN_IR_SAG));
        Serial.print(F(" (zemin ")); Serial.print(irZeminSag);
        Serial.print(F(")  cizgi: ")); Serial.print(solCizgide());
        Serial.print(F(" ")); Serial.println(sagCizgide());
        delay(200);
      }
      break;

    case 'u':
      bas = millis();
      while (millis() - bas < 5000) {
        Serial.print(F("US sol=")); Serial.print(usOlc(PIN_US_SOL_TRIG, PIN_US_SOL_ECHO));
        delay(30);
        Serial.print(F(" cm  sag=")); Serial.print(usOlc(PIN_US_SAG_TRIG, PIN_US_SAG_ECHO));
        Serial.println(F(" cm"));
        delay(200);
      }
      break;

    case 'm':
      Serial.println(F("Sol ileri")); motorlar(60, 0); delay(1000); dur(); delay(500);
      Serial.println(F("Sol geri"));  motorlar(-60, 0); delay(1000); dur(); delay(500);
      Serial.println(F("Sag ileri")); motorlar(0, 60); delay(1000); dur(); delay(500);
      Serial.println(F("Sag geri"));  motorlar(0, -60); delay(1000); dur();
      break;

    case 'f': git(500, HIZ_NORMAL, false, false);  odometriYazdir(); break;
    case 'g': git(-500, HIZ_NORMAL, false, false); odometriYazdir(); break;
    case 'd': don(360, HIZ_DONUS); odometriYazdir(); break;
    case 'r': don(90, HIZ_DONUS);  odometriYazdir(); break;
    case 'a': catalIndir();  Serial.println(F("Catal asagi"));  break;
    case 'y': catalKaldir(); Serial.println(F("Catal yukari")); break;

    case 's': {
      int aci = Serial.parseInt();
      catalGit(aci);
      Serial.print(F("Servo: ")); Serial.println(catalAci);
      break;
    }

    case 'o': odometriYazdir(); break;
    case 'h': kalibrasyonYardim(); break;
    default: break;
  }
}

void odometriYazdir() {
  odometriGuncelle();
  Serial.print(F("x=")); Serial.print(odoX, 0);
  Serial.print(F(" mm  y=")); Serial.print(odoY, 0);
  Serial.print(F(" mm  yon=")); Serial.print(odoTeta * RAD_TO_DEG, 1);
  Serial.print(F(" der  yol=")); Serial.print(odoYol, 0);
  Serial.println(F(" mm"));
}
