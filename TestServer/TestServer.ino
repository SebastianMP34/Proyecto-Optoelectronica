#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---- Red propia del ESP32 ----
const char* ap_ssid = "ESP32-David";
const char* ap_pass = "12345678";   // mínimo 8 caracteres
// ------------------------------

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);

// Estado del sistema
String modo = "manual";          // "manual" o "auto"
bool encendido = false;

// Reloj
unsigned long segundosBase = 10 * 3600UL + 30 * 60UL;  // hora inicial 10:30
unsigned long millisBase = 0;

// ---------- Subrutinas de ON y OFF (vacías por ahora) ----------
void accionON() {
  // Aquí irá lo que debe hacer el botón ON
}

void accionOFF() {
  // Aquí irá lo que debe hacer el botón OFF
}
// ----------------------------------------------------------------

unsigned long segundosActuales() {
  return (segundosBase + (millis() - millisBase) / 1000) % 86400UL;
}

void textoCentrado(String texto, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(texto, 0, y, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, y);
  display.print(texto);
}

void actualizarPantalla() {
  unsigned long total = segundosActuales();
  int h = total / 3600;
  int m = (total % 3600) / 60;

  char hora[6];
  sprintf(hora, "%02d:%02d", h, m);

  String textoModo = (modo == "auto") ? "MODO AUTO" : "MODO MAN";

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  textoCentrado(textoModo, 0);
  textoCentrado(String(hora), 16);
  display.display();
}

String construirPagina() {
  unsigned long total = segundosActuales();
  int h = total / 3600;
  int m = (total % 3600) / 60;

  char horaValor[6];
  sprintf(horaValor, "%02d:%02d", h, m);

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 David</title>";
  html += "<style>";
  html += "body{font-family:Arial;max-width:400px;margin:20px auto;padding:0 15px;text-align:center}";
  html += "select,input,button{font-size:1.2em;padding:10px;margin:8px 0;width:100%;box-sizing:border-box}";
  html += "button{border:none;border-radius:8px;color:white;cursor:pointer}";
  html += ".azul{background:#1565c0}.verde{background:#2e7d32}.rojo{background:#c62828}";
  html += "</style></head><body>";
  html += "<h1>Control ESP32</h1>";

  html += "<form action='/set' method='GET'>";
  html += "<label>MODO:</label>";
  html += "<select name='modo'>";
  html += "<option value='manual'" + String(modo == "manual" ? " selected" : "") + ">Manual</option>";
  html += "<option value='auto'" + String(modo == "auto" ? " selected" : "") + ">Automático</option>";
  html += "</select>";
  html += "<label>Hora:</label>";
  html += "<input type='time' name='hora' value='" + String(horaValor) + "' required>";
  html += "<button class='azul' type='submit'>Enviar al display</button>";
  html += "</form>";

  html += "<hr>";
  html += "<p>Estado: <b>" + String(encendido ? "ON" : "OFF") + "</b></p>";
  html += "<form action='/on' method='GET'><button class='verde' type='submit'>ON</button></form>";
  html += "<form action='/off' method='GET'><button class='rojo' type='submit'>OFF</button></form>";

  html += "</body></html>";
  return html;
}

void paginaInicio() {
  server.send(200, "text/html", construirPagina());
}

void recibirDatos() {
  if (server.hasArg("modo")) {
    String m = server.arg("modo");
    if (m == "manual" || m == "auto") modo = m;
  }

  if (server.hasArg("hora")) {
    String t = server.arg("hora");           // formato "HH:MM"
    int h = t.substring(0, 2).toInt();
    int m = t.substring(3, 5).toInt();
    if (h >= 0 && h < 24 && m >= 0 && m < 60) {
      segundosBase = h * 3600UL + m * 60UL;
      millisBase = millis();
    }
  }

  actualizarPantalla();
  server.sendHeader("Location", "/");
  server.send(303);
}

void botonON() {
  encendido = true;
  accionON();
  server.sendHeader("Location", "/");
  server.send(303);
}

void botonOFF() {
  encendido = false;
  accionOFF();
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("No se encontro la pantalla OLED");
    while (true);
  }

  WiFi.softAP(ap_ssid, ap_pass);
  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.softAPIP());   // normalmente 192.168.4.1

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

  // Refresca la pantalla cada medio segundo para que la hora avance
  static unsigned long ultimo = 0;
  if (millis() - ultimo >= 500) {
    ultimo = millis();
    actualizarPantalla();
  }
}