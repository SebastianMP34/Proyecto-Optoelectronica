#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---- Configura estos datos ----
String modo = "X";          // Cámbialo cuando quieras: "1", "2", "A", etc.

int horaInicial    = 10;    // 0 a 23
int minutoInicial  = 30;    // 0 a 59
// --------------------------------

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
// ppipipipipipipipipipopopopoopop
unsigned long segundosBase;

// Dibuja un texto centrado horizontalmente en la fila y indicada
void textoCentrado(String texto, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(texto, 0, y, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, y);
  display.print(texto);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("No se encontro la pantalla OLED");
    while (true);
  }

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);

  segundosBase = horaInicial * 3600UL + minutoInicial * 60UL;
}

void loop() {
  unsigned long total = (segundosBase + millis() / 1000) % 86400UL;

  int h = total / 3600;
  int m = (total % 3600) / 60;

  char hora[6];
  sprintf(hora, "%02d:%02d", h, m);

  display.clearDisplay();
  textoCentrado("MODO " + modo, 0);   // renglón de arriba
  textoCentrado(String(hora), 16);    // renglón de abajo
  display.display();

  delay(500);
}