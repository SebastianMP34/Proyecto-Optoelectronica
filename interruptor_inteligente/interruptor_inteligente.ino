// =====================================================
//  INTERRUPTOR INTELIGENTE - ESP32 ORIGINAL (RECEPTOR)
//  Core ESP32 2.0.16
//  Librerías: Adafruit SSD1306, Adafruit GFX,
//             DFRobotDFPlayerMini, ESP32Servo
// =====================================================
#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DFRobotDFPlayerMini.h>
#include <ESP32Servo.h>

// ---------------- RED PROPIA ----------------
const char* ap_ssid = "ESP32-David";
const char* ap_pass = "12345678";   // mínimo 8 caracteres
#define CANAL 1                     // Igual que en el emisor

// ---------------- PINES ----------------
#define PIN_SERVO  18
#define PIN_LDR    34   // ADC1 (el ADC2 no funciona con WiFi activo)
#define PIN_DF_RX  16   // RX2 de la ESP32  <- TX del MP3-TF-16P
#define PIN_DF_TX  17   // TX2 de la ESP32  -> RX del MP3-TF-16P (con resistencia de 1k)
// OLED: SDA = 21, SCL = 22

// ---------------- SERVO ----------------
const int ANGULO_REPOSO = 90;
const int ANGULO_ON     = 155;
const int ANGULO_OFF    = 45;
const int TIEMPO_IDA    = 500;   // ms para llegar al ángulo
const int TIEMPO_VUELTA = 500;   // ms para regresar al reposo

// ---------------- AUDIO (carpeta MP3 de la SD) ----------------
const int VOLUMEN               = 25;  // 0 a 30
const int AUDIO_AVISO           = 1;   // 0001.mp3 = Windows 10 USB
const int PRIMER_AUDIO_NOCHE    = 2;   // 0002.mp3 ... 0005.mp3
const int CANTIDAD_AUDIOS_NOCHE = 4;

// ---------------- FOTORRESISTENCIA ----------------
// Divisor de voltaje: 3.3V -> LDR -> pin 34 -> resistencia 10k -> GND
// Más luz = lectura MÁS ALTA. Calibra el umbral con el Monitor Serie.
const int UMBRAL_LUZ = 1500;                          // 0 a 4095
const unsigned long TIEMPO_CONFIRMAR_OSCURO = 10000;  // ms seguidos con poca luz

// ---------------- NOCHE ----------------
const int HORA_INICIO_NOCHE   = 23;     // 11 pm
const int HORA_FIN_NOCHE      = 10;     // 10 am
const int CADA_N_MINUTOS      = 5;      // parpadeo del switch
const int DURACION_PARPADEO   = 2000;   // ms que queda encendido

// =====================================================
Adafruit_SSD1306 display(128, 32, &Wire, -1);
WebServer server(80);
DFRobotDFPlayerMini audio;
Servo servo;

// Estado del sistema
String modo = "manual";          // "manual" o "ahorro"
bool encendido = false;          // estado actual del switch

// Reloj
unsigned long segundosBase = 10 * 3600UL + 30 * 60UL;
unsigned long millisBase = 0;

// Día / noche por comunicación (ESP-NOW)
struct Paquete {
  uint32_t clave;
};
const uint32_t CLAVE = 0xA5A5A5A5;
const unsigned long TIMEOUT = 5000;   // ms sin latido = noche
volatile unsigned long ultimoLatido = 0;
volatile bool huboLatido = false;
bool esDia = false;

// Control del modo ahorro
int faseAhorro = 0;                   // 0 = preparar, 1 = ya preparado
unsigned long inicioOscuro = 0;
long ultimoMinutoSusto = -1;
int indiceAudio = 0;

// =====================================================
//  AUDIO
// =====================================================
void reproducir(int numero) {
  audio.playMp3Folder(numero);        // /MP3/000N.mp3
  Serial.print("Audio: ");
  Serial.println(numero);
}

// =====================================================
//  SWITCH (servomotor)
// =====================================================
void moverSwitch(bool encender) {
  servo.write(encender ? ANGULO_ON : ANGULO_OFF);
  delay(TIEMPO_IDA);
  servo.write(ANGULO_REPOSO);
  delay(TIEMPO_VUELTA);
  encendido = encender;
  Serial.println(encender ? "Switch ON" : "Switch OFF");
}

void parpadearSwitch() {
  moverSwitch(true);
  delay(DURACION_PARPADEO);
  moverSwitch(false);
}

// =====================================================
//  LUZ
// =====================================================
bool hayLuzSuficiente() {
  long suma = 0;
  for (int i = 0; i < 10; i++) {
    suma += analogRead(PIN_LDR);
    delay(2);
  }
  int lectura = suma / 10;
  Serial.print("LDR: ");
  Serial.println(lectura);
  return lectura > UMBRAL_LUZ;
}

// =====================================================
//  ESP-NOW (callback de core 2.x)
// =====================================================
void alRecibir(const uint8_t *mac, const uint8_t *datos, int len) {
  if (len == sizeof(Paquete)) {
    Paquete p;
    memcpy(&p, datos, sizeof(p));
    if (p.clave == CLAVE) {
      ultimoLatido = millis();
      huboLatido = true;
    }
  }
}

// =====================================================
//  RELOJ Y PANTALLA
// =====================================================
unsigned long segundosActuales() {
  return (segundosBase + (millis() - millisBase) / 1000) % 86400UL;
}

bool enVentanaNoche() {
  int h = segundosActuales() / 3600;
  return (h >= HORA_INICIO_NOCHE || h < HORA_FIN_NOCHE);
}

void textoCentrado(String texto, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(texto, 0, y, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, y);
  display.print(texto);
}

void actualizarPantalla() {
  unsigned long total = segundosActuales();
  int h = total / 3600;
  int m = (total % 3600) / 60;

  char hora[6];
  sprintf(hora, "%02d:%02d", h, m);

  String textoModo = (modo == "ahorro") ? "MODO ECO" : "MODO MAN";

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  textoCentrado(textoModo, 0);
  textoCentrado(String(hora), 16);
  display.display();
}

// =====================================================
//  PÁGINA WEB
// =====================================================
String construirPagina() {
  unsigned long total = segundosActuales();
  int h = total / 3600;
  int m = (total % 3600) / 60;

  char horaValor[6];
  sprintf(horaValor, "%02d:%02d", h, m);

  bool ahorro = (modo == "ahorro");
  String dis = ahorro ? " disabled" : "";

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Interruptor inteligente</title>";
  html += "<style>";
  html += "body{font-family:Arial;max-width:400px;margin:20px auto;padding:0 15px;text-align:center}";
  html += "select,input,button{font-size:1.2em;padding:10px;margin:8px 0;width:100%;box-sizing:border-box}";
  html += "button{border:none;border-radius:8px;color:white;cursor:pointer}";
  html += "button:disabled{opacity:.35;cursor:not-allowed}";
  html += ".azul{background:#1565c0}.verde{background:#2e7d32}.rojo{background:#c62828}";
  html += "</style></head><body>";
  html += "<h1>Interruptor inteligente</h1>";

  html += "<form action='/set' method='GET'>";
  html += "<label>MODO:</label>";
  html += "<select name='modo'>";
  html += "<option value='manual'" + String(!ahorro ? " selected" : "") + ">Manual</option>";
  html += "<option value='ahorro'" + String(ahorro ? " selected" : "") + ">Ahorro Energético</option>";
  html += "</select>";
  html += "<label>Hora:</label>";
  html += "<input type='time' name='hora' value='" + String(horaValor) + "' required>";
  html += "<button class='azul' type='submit'>Enviar al display</button>";
  html += "</form>";

  html += "<hr>";
  html += "<p>Ahora es: <b>" + String(esDia ? "DÍA" : "NOCHE") + "</b></p>";
  html += "<p>Switch: <b>" + String(encendido ? "ON" : "OFF") + "</b></p>";
  html += "<form action='/on' method='GET'><button class='verde' type='submit'" + dis + ">ON</button></form>";
  html += "<form action='/off' method='GET'><button class='rojo' type='submit'" + dis + ">OFF</button></form>";
  if (ahorro) html += "<p><small>En Ahorro Energético los botones están desactivados.</small></p>";

  html += "</body></html>";
  return html;
}

void redirigirInicio() {
  server.sendHeader("Location", "/");
  server.send(303);
}

void paginaInicio() {
  server.send(200, "text/html", construirPagina());
}

void recibirDatos() {
  if (server.hasArg("modo")) {
    String nuevo = server.arg("modo");
    if ((nuevo == "manual" || nuevo == "ahorro") && nuevo != modo) {
      modo = nuevo;
      faseAhorro = 0;                // reinicia la lógica del modo ahorro
    }
  }

  if (server.hasArg("hora")) {
    String t = server.arg("hora");   // formato "HH:MM"
    int h = t.substring(0, 2).toInt();
    int m = t.substring(3, 5).toInt();
    if (h >= 0 && h < 24 && m >= 0 && m < 60) {
      segundosBase = h * 3600UL + m * 60UL;
      millisBase = millis();
    }
  }

  reproducir(AUDIO_AVISO);
  actualizarPantalla();
  redirigirInicio();
}

void botonON() {
  if (modo == "manual") {            // en ahorro no hace nada
    reproducir(AUDIO_AVISO);
    moverSwitch(true);
  }
  redirigirInicio();
}

void botonOFF() {
  if (modo == "manual") {            // en ahorro no hace nada
    reproducir(AUDIO_AVISO);
    moverSwitch(false);
  }
  redirigirInicio();
}

// =====================================================
//  LÓGICA DEL MODO AHORRO ENERGÉTICO
// =====================================================
void ejecutarAhorro() {
  if (esDia) {
    // ---------- DÍA ----------
    if (faseAhorro == 0) {
      if (encendido) moverSwitch(false);   // igual que el botón OFF
      inicioOscuro = 0;
      faseAhorro = 1;
    }

    // Mientras el switch esté apagado, vigila la luz cada segundo.
    // Si hay poca luz sostenida, lo enciende y se queda encendido hasta la noche.
    if (!encendido) {
      static unsigned long ultimaLectura = 0;
      if (millis() - ultimaLectura >= 1000) {
        ultimaLectura = millis();
        if (hayLuzSuficiente()) {
          inicioOscuro = 0;
        } else {
          if (inicioOscuro == 0) inicioOscuro = millis();
          if (millis() - inicioOscuro >= TIEMPO_CONFIRMAR_OSCURO) {
            moverSwitch(true);             // igual que el botón ON
            inicioOscuro = 0;
          }
        }
      }
    }

  } else {
    // ---------- NOCHE ----------
    if (faseAhorro == 0) {
      if (encendido) moverSwitch(false);   // de noche el switch queda apagado
      ultimoMinutoSusto = -1;
      faseAhorro = 1;
    }

    if (enVentanaNoche()) {                // de 11 pm a 10 am
      long minutoTotal = segundosActuales() / 60;
      if (minutoTotal != ultimoMinutoSusto) {   // cambió el minuto
        ultimoMinutoSusto = minutoTotal;
        int minuto = minutoTotal % 60;

        // Un audio por minuto (0002 a 0005, en ciclo)
        reproducir(PRIMER_AUDIO_NOCHE + (indiceAudio % CANTIDAD_AUDIOS_NOCHE));
        indiceAudio++;

        // Cada 5 minutos el servo prende y apaga el switch
        if (minuto % CADA_N_MINUTOS == 0) parpadearSwitch();
      }
    } else {
      ultimoMinutoSusto = -1;
    }
  }
}

// =====================================================
//  SETUP Y LOOP
// =====================================================
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("No se encontro la pantalla OLED");
    while (true);
  }

  // Servo en reposo
  ESP32PWM::allocateTimer(0);
  servo.setPeriodHertz(50);
  servo.attach(PIN_SERVO, 500, 2400);
  servo.write(ANGULO_REPOSO);

  // MP3-TF-16P
  Serial2.begin(9600, SERIAL_8N1, PIN_DF_RX, PIN_DF_TX);
  audio.begin(Serial2, false);          // sin ACK: funciona mejor con módulos clon
  delay(500);
  audio.volume(VOLUMEN);

  // LDR
  pinMode(PIN_LDR, INPUT);

  // WiFi propio + ESP-NOW
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_pass, CANAL);
  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.softAPIP());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error al iniciar ESP-NOW");
  } else {
    esp_now_register_recv_cb(alRecibir);
  }

  millisBase = millis();

  server.on("/", paginaInicio);
  server.on("/set", recibirDatos);
  server.on("/on", botonON);
  server.on("/off", botonOFF);
  server.begin();

  actualizarPantalla();
}

void loop() {
  server.handleClient();

  // Decide DÍA o NOCHE según si hay comunicación con la otra ESP32
  bool hayComunicacion = huboLatido && (millis() - ultimoLatido < TIMEOUT);
  static int anterior = -1;
  int actual = hayComunicacion ? 1 : 0;

  if (actual != anterior) {
    anterior = actual;
    esDia = (actual == 1);
    faseAhorro = 0;                     // reinicia la lógica del modo ahorro
    Serial.println(esDia ? "DIA" : "NOCHE");
  }

  if (modo == "ahorro") {
    ejecutarAhorro();
  }

  // Refresca la pantalla cada medio segundo
  static unsigned long ultimo = 0;
  if (millis() - ultimo >= 500) {
    ultimo = millis();
    actualizarPantalla();
  }
}
