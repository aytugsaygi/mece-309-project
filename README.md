# MECE-309 Otonom Araç

Robot rastgele bir noktadan başlar, 3 kırmızı kutuyu HuskyLens ile bulur, forkliftle alır ve renkli kartla işaretlenmiş bırakma alanına aralarında 5-10 cm boşlukla yan yana bırakır. Saha sınırını TCRT5000 sensörleriyle, ortadaki engeli HC-SR04 sensörleriyle algılar.

- **Kart:** Arduino Uno ya da Mega 2560. Aynı kod ikisinde de çalışır; tek fark HuskyLens kablolarının takıldığı pinler.
- **Kod:** [`otonom_arac/`](otonom_arac/) klasörü. Bütün ayarlar [`otonom_arac/ayarlar.h`](otonom_arac/ayarlar.h) içinde.

**Kurulum, bağlantılar, derleme, kalibrasyon ve sorun giderme için:** [KURULUM_REHBERI.md](KURULUM_REHBERI.md)
