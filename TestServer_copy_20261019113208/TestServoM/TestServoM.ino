// Prueba de servomotor con ESP32
// Libreria necesaria: "ESP32Servo" (Kevin Harrington / John K. Bennett)
// Instalar desde: Herramientas > Administrar bibliotecas

#include <ESP32Servo.h>

Servo miServo;

const int PIN_SERVO = 18;   // Señal (cable naranja/amarillo) al GPIO 18
const int PAUSA_MS  = 15;   // Menor = mas rapido

// Mueve el servo suavemente de un angulo a otro (sube o baja segun el caso)
void mover(int desde, int hasta) {
  if (desde < hasta) {
    for (int a = desde; a <= hasta; a++) {
      miServo.write(a);
      delay(PAUSA_MS);
    }
  } else {
    for (int a = desde; a >= hasta; a--) {
      miServo.write(a);
      delay(PAUSA_MS);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Reservar timers PWM del ESP32
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  miServo.setPeriodHertz(50);              // Servos estandar: 50 Hz
  miServo.attach(PIN_SERVO, 500, 2400);    // pin, pulso min, pulso max (us)

  Serial.println("Probando servo...");
  miServo.write(90);                       // Empieza en el centro
  delay(1000);
}

void loop() {
  // 90 -> 180 -> 90
  mover(90, 155);
  Serial.println("180");
  delay(500);
  mover(155, 90);
  Serial.println("90");
  delay(500);

  // 90 -> 0 -> 90
  mover(90, 45); 
  delay(500);
  mover(45, 90);
  Serial.println("90");
  delay(500);
}
