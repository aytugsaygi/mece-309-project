// =====================================================================
//  FORKLIFT: MG996R servo
//  MG996R anlık 2 A'ye kadar akım çeker. Arduino'nun 5V pininden
//  BESLEMEYİN; ayrı bir 5-6 V / 3 A voltaj düşürücü kullanın, GND ortak.
// =====================================================================

void catalBaslat() {
  catal.attach(PIN_SERVO);
  catal.write(catalAci);
  delay(300);
}

// Servoyu derece derece hareket ettir: kutu sarsılmaz, akım darbesi azalır
void catalGit(int hedefAci) {
  hedefAci = constrain(hedefAci, 0, 180);
  while (catalAci != hedefAci) {
    catalAci += (hedefAci > catalAci) ? 1 : -1;
    catal.write(catalAci);
    delay(FORK_ADIM_MS);
  }
}

void catalIndir()  { catalGit(FORK_ALT_DERECE); }
void catalKaldir() { catalGit(FORK_UST_DERECE); }
