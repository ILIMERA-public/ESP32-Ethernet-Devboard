/*
 * ESP32 Ethernet Devboard — 01 Ethernet Bağlantı Testi (DHCP + HTTP GET)
 *
 * LAN8720 Ethernet PHY'yi başlatır, yerel ağdan otomatik IP alır (DHCP) ve her
 * 10 saniyede bir Google'ın bağlantı test adresine HTTP GET isteği atar.
 * Beklenen sonuç: "HTTP kodu: 204" (internet erişimi var).
 *
 * Kurulum: RJ45'e modem/switch'e bağlı bir Ethernet kablosu takın, kartı USB
 * Type-C ile bağlayın. Seri Monitör: 115200 baud.
 * Arduino IDE kartı: "ESP32 Dev Module". ESP32 Arduino çekirdeği 2.x ve 3.x için yazılmıştır.
 *
 * EN: Starts the LAN8720 Ethernet PHY, gets an IP over DHCP and sends an HTTP GET
 *     to Google's connectivity check every 10 s. Expected: "HTTP kodu: 204".
 *     Written for ESP32 Arduino core 2.x and 3.x.
 */

#include <Arduino.h>
#include <WiFi.h>         // ağ olayları (WiFi.onEvent) için
#include <ETH.h>
#include <HTTPClient.h>

// ── Ethernet PHY (LAN8720) — kartın donanımına sabittir, değiştirmeyin ──
#define ETH_ADDR       1
#define ETH_POWER_PIN  -1
#define ETH_MDC_PIN    23
#define ETH_MDIO_PIN   18
#define ETH_TYPE       ETH_PHY_LAN8720
#define ETH_CLK_MODE   ETH_CLOCK_GPIO17_OUT

const char* TEST_URL = "http://clients3.google.com/generate_204";
const unsigned long ARALIK_MS = 10000;

volatile bool ethHazir = false;
unsigned long sonIstek = 0;

void ethernetBaslat() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ETH.begin(ETH_TYPE, ETH_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLK_MODE);
#else
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
#endif
}

void agOlayi(WiFiEvent_t olay) {
  switch (olay) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH baslatildi");
      ETH.setHostname("ilimera-esp32-eth");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH kablo baglandi");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("IP adresi: ");
      Serial.print(ETH.localIP());
      Serial.printf("  |  %d Mbps, %s\n", ETH.linkSpeed(), ETH.fullDuplex() ? "full duplex" : "half duplex");
      ethHazir = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH baglantisi kesildi!");
      ethHazir = false;
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== ESP32 Ethernet baglanti testi ===");
  WiFi.onEvent(agOlayi);
  ethernetBaslat();
}

void loop() {
  if (ethHazir && millis() - sonIstek >= ARALIK_MS) {
    sonIstek = millis();
    HTTPClient http;
    http.begin(TEST_URL);
    int kod = http.GET();
    Serial.printf("HTTP kodu: %d%s\n", kod, kod == 204 ? "  (internet erisimi var)" : "");
    http.end();
  }
}
