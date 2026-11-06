// Lectura de fotoresistencia (LDR) con ESP32

const int PIN_LDR = 34;   // Pin ADC (solo entrada)

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);        // 0 - 4095
  analogSetAttenuation(ADC_11db);  // Rango hasta ~3.3V
}

void loop() {
  int valor = analogRead(PIN_LDR);
  float voltaje = valor * (3.3 / 4095.0);
  int porcentaje = map(valor, 0, 4095, 0, 100);

  Serial.print("Valor ADC: ");
  Serial.print(valor);
  Serial.print(" | Voltaje: ");
  Serial.print(voltaje, 2);
  Serial.print(" V | Luz: ");
  Serial.print(porcentaje);
  Serial.println(" %");

  delay(500);
}