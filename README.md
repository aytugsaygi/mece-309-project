# MECE-309 Otonom Araç

Robot rastgele bir noktadan başlar, 3 kırmızı kutuyu HuskyLens ile bulur, forkliftle alır ve mavi kartla işaretlenmiş ~50×50 cm'lik bırakma alanına aralarında 1-7 cm boşlukla yan yana bırakır. Saha sınırındaki siyah bandı TCRT5000 sensörleriyle algılar. Duvarı ve ortadaki 1 m'lik engeli HC-SR04 sensörleri ve engelin üstündeki yeşil şerit sayesinde kamera algılar.

- **Kart:** Arduino Uno ya da Mega 2560. Aynı kod ikisinde de çalışır; tek fark HuskyLens kablolarının takıldığı pinler.
- **Kod:** [`otonom_arac/`](otonom_arac/) klasörü. Bütün ayarlar [`otonom_arac/ayarlar.h`](otonom_arac/ayarlar.h) içinde.

**Kurulum, bağlantılar, derleme, kalibrasyon ve sorun giderme için:** [KURULUM_REHBERI.md](KURULUM_REHBERI.md)
