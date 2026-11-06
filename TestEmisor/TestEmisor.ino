#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define CANAL 1   // Debe ser igual al de la ESP32 original

struct Paquete {
  uint32_t clave;
};

const uint32_t CLAVE = 0xA5A5A5A5;   // Misma clave en las dos ESP32
uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(CANAL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error al iniciar ESP-NOW");
    while (true);
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcastAddr, 6);
  peer.channel = 0;      // 0 = usa el canal actual
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void loop() {
  Paquete p;
  p.clave = CLAVE;
  esp_now_send(broadcastAddr, (uint8_t*)&p, sizeof(p));
  Serial.println("Latido enviado");
  delay(1000);
}