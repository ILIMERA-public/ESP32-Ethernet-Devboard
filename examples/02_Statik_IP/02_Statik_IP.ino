/*
 * ESP32 Ethernet Devboard — 02 Sabit (statik) IP
 *
 * DHCP yerine elle verilen IP adresiyle ağa bağlanır. DHCP sunucusu olmayan
 * endüstriyel ağlar, PLC/SCADA hatları veya cihazın hep aynı adreste
 * bulunması gereken kurulumlar için.
 *
 * Ayarlar: SABIT_IP, AG_GECIDI, ALT_AG, DNS — ağınıza göre değiştirin.
 * Bağlantıyı başka bir bilgisayardan "ping 192.168.1.50" ile sınayabilirsiniz.
 *
 * EN: Connects with a static IP instead of DHCP. Edit the addresses below to
 *     match your network, then ping the board from another computer.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ETH.h>

// ─────────── KULLANICI AYARLARI ───────────
IPAddress SABIT_IP (192, 168, 1, 50);
IPAddress AG_GECIDI(192, 168, 1, 1);
IPAddress ALT_AG   (255, 255, 255, 0);
IPAddress DNS      (8, 8, 8, 8);
// ──────────────────────────────────────────

// Ethernet PHY (LAN8720) — kartın donanımına sabittir
#define ETH_ADDR       1
#define ETH_POWER_PIN  -1
#define ETH_MDC_PIN    23
#define ETH_MDIO_PIN   18
#define ETH_TYPE       ETH_PHY_LAN8720
#define ETH_CLK_MODE   ETH_CLOCK_GPIO17_OUT

void agOlayi(WiFiEvent_t olay) {
  switch (olay) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname("ilimera-esp32-eth");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH kablo baglandi");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("IP: ");      Serial.println(ETH.localIP());
      Serial.print("Ag gecidi: "); Serial.println(ETH.gatewayIP());
      Serial.print("Alt ag: ");  Serial.println(ETH.subnetMask());
      Serial.print("MAC: ");     Serial.println(ETH.macAddress());
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH kablo cikarildi");
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== ESP32 Ethernet sabit IP ===");
  WiFi.onEvent(agOlayi);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ETH.begin(ETH_TYPE, ETH_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLK_MODE);
#else
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
#endif
  ETH.config(SABIT_IP, AG_GECIDI, ALT_AG, DNS);   // DHCP yerine sabit adres (ETH.begin'den sonra)
}

void loop() {}
