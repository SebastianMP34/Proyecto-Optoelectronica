// Ambiente de cueva estilo Minecraft (aproximación) - ESP32
// Bocina: GPIO 25 -> 470Ω -> base 2N2222 (ver esquema con transistor)
// Compatible con core ESP32 v2.x y v3.x

const int PIN_BOCINA = 25;
const int VOLUMEN = 100;   // duty de 0 a 127 (127 = máximo)

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  void iniciarBocina()     { ledcAttach(PIN_BOCINA, 2000, 8); }
  void sonar(int f)        { ledcWriteTone(PIN_BOCINA, f); }
  void ponerDuty(int d)    { ledcWrite(PIN_BOCINA, d); }
#else
  const int CANAL = 0;
  void iniciarBocina() {
    ledcSetup(CANAL, 2000, 8);
    ledcAttachPin(PIN_BOCINA, CANAL);
  }
  void sonar(int f)        { ledcWriteTone(CANAL, f); }
  void ponerDuty(int d)    { ledcWrite(CANAL, d); }
#endif

void silencio(int ms) {
  sonar(0);
  delay(ms);
}

// Nota que baja (o sube) lentamente de f1 a f2, con vibrato suave
void deslizar(int f1, int f2, int duracionMs) {
  int pasos = duracionMs / 15;
  for (int i = 0; i < pasos; i++) {
    float t = (float)i / pasos;
    float f = f1 + (f2 - f1) * t;
    f += 6 * sin(i * 0.5);                 // vibrato
    sonar((int)f);
    // fade in / fade out del volumen
    float env = (t < 0.2) ? t / 0.2 : (t > 0.7 ? (1 - t) / 0.3 : 1.0);
    ponerDuty((int)(VOLUMEN * env));
    delay(15);
  }
  sonar(0);
}

// Gota de agua / eco agudo
void gota() {
  for (int f = 1800; f > 900; f -= 100) {
    sonar(f);
    ponerDuty(VOLUMEN);
    delay(8);
  }
  sonar(0);
  delay(120);
  sonar(1200); ponerDuty(VOLUMEN / 3); delay(30);   // eco
  sonar(0);
}

// Retumbo grave irregular
void retumbo(int duracionMs) {
  unsigned long inicio = millis();
  while (millis() - inicio < duracionMs) {
    sonar(random(110, 200));
    ponerDuty(random(VOLUMEN / 2, VOLUMEN));
    delay(random(20, 60));
  }
  sonar(0);
}

void setup() {
  Serial.begin(115200);
  iniciarBocina();
  randomSeed(esp_random());
  Serial.println("Ambiente de cueva...");
}

void loop() {
  int efecto = random(0, 4);
  switch (efecto) {
    case 0: deslizar(420, 160, random(1500, 2500)); break;
    case 1: deslizar(200, 330, random(1200, 2000)); break;
    case 2: retumbo(random(800, 1500));             break;
    case 3: gota();                                 break;
  }
  silencio(random(1500, 4500));   // silencios largos, típicos de cueva
}