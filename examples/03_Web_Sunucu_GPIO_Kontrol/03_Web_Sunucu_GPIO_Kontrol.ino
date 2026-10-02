/*
 * ESP32 Ethernet Devboard — 03 Web Sunucu ile GPIO Kontrolü
 *
 * Kart Ethernet üzerinden bir web sayfası yayınlar. Aynı ağdaki bir bilgisayar
 * ya da telefondan tarayıcıyla http://<kartın IP adresi>/ açın:
 *   - CIKIS_PIN'e bağlı LED/röle modülünü açıp kapatın,
 *   - BOOT düğmesinin (GPIO 0) durumunu, çalışma süresini ve ağ bilgisini görün.
 * /durum adresi aynı bilgiyi JSON olarak verir (otomasyon yazılımları için).
 *
 * Bağlantı: CIKIS_PIN (varsayılan GPIO 4) → LED (+ 330 Ω direnç) → GND
 *           ya da 3.3 V tetiklenen bir röle modülünün IN girişi.
 * Not: GPIO 34–39 yalnız giriştir; GPIO 0, 2, 5, 12, 15 açılış (strapping) pinleridir,
 *      çıkış için GPIO 4, 13, 14, 16, 32, 33 tercih edin.
 *
 * EN: Serves a web page over Ethernet to toggle an output pin and show the
 *     BOOT button state, uptime and network info. /durum returns JSON.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ETH.h>
#include <WebServer.h>

// ─────────── KULLANICI AYARLARI ───────────
#define CIKIS_PIN 4
// ──────────────────────────────────────────

#define BOOT_PIN 0

// Ethernet PHY (LAN8720) — kartın donanımına sabittir
#define ETH_ADDR       1
#define ETH_POWER_PIN  -1
#define ETH_MDC_PIN    23
#define ETH_MDIO_PIN   18
#define ETH_TYPE       ETH_PHY_LAN8720
#define ETH_CLK_MODE   ETH_CLOCK_GPIO17_OUT

WebServer sunucu(80);
bool cikisDurumu = false;

void agOlayi(WiFiEvent_t olay) {
  switch (olay) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname("ilimera-esp32-eth");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("Tarayicida acin: http://");
      Serial.println(ETH.localIP());
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH baglantisi kesildi");
      break;
    default:
      break;
  }
}

String durumJson() {
  String j = "{";
  j += "\"cikis\":" + String(cikisDurumu ? "true" : "false");
  j += ",\"boot_dugmesi\":" + String(digitalRead(BOOT_PIN) == LOW ? "true" : "false");
  j += ",\"calisma_suresi_s\":" + String(millis() / 1000);
  j += ",\"ip\":\"" + ETH.localIP().toString() + "\"";
  j += ",\"mac\":\"" + ETH.macAddress() + "\"";
  j += ",\"hiz_mbps\":" + String(ETH.linkSpeed());
  j += "}";
  return j;
}

void anaSayfa() {
  String s;
  s.reserve(1600);
  s += F("<!doctype html><html lang=tr><head><meta charset=utf-8>"
         "<meta name=viewport content='width=device-width,initial-scale=1'>"
         "<title>ILIMERA ESP32 Ethernet</title><style>"
         "body{font-family:system-ui,sans-serif;max-width:28rem;margin:2rem auto;padding:0 1rem;color:#111}"
         "a.b{display:inline-block;padding:.7rem 1.4rem;border-radius:.5rem;background:#111;color:#fff;text-decoration:none;margin:.2rem}"
         "a.k{background:#e5e5e5;color:#111}td{padding:.3rem .8rem .3rem 0}</style></head><body>"
         "<h1>ESP32 Ethernet</h1>");
  s += "<p>Cikis (GPIO " + String(CIKIS_PIN) + "): <b>" + String(cikisDurumu ? "ACIK" : "KAPALI") + "</b></p>";
  s += F("<p><a class=b href='/ac'>Ac</a><a class='b k' href='/kapat'>Kapat</a></p><table>");
  s += "<tr><td>BOOT dugmesi</td><td>" + String(digitalRead(BOOT_PIN) == LOW ? "basili" : "serbest") + "</td></tr>";
  s += "<tr><td>Calisma suresi</td><td>" + String(millis() / 1000) + " s</td></tr>";
  s += "<tr><td>IP</td><td>" + ETH.localIP().toString() + "</td></tr>";
  s += "<tr><td>MAC</td><td>" + ETH.macAddress() + "</td></tr>";
  s += "<tr><td>Baglanti</td><td>" + String(ETH.linkSpeed()) + " Mbps</td></tr>";
  s += F("</table><p><a href='/durum'>JSON</a> &middot; <a href='/'>Yenile</a></p></body></html>");
  sunucu.send(200, "text/html; charset=utf-8", s);
}

void cikisAyarla(bool acik) {
  cikisDurumu = acik;
  digitalWrite(CIKIS_PIN, acik ? HIGH : LOW);
  sunucu.sendHeader("Location", "/");
  sunucu.send(303);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(CIKIS_PIN, OUTPUT);
  digitalWrite(CIKIS_PIN, LOW);
  pinMode(BOOT_PIN, INPUT_PULLUP);

  WiFi.onEvent(agOlayi);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ETH.begin(ETH_TYPE, ETH_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLK_MODE);
#else
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
#endif

  sunucu.on("/", anaSayfa);
  sunucu.on("/ac", [] { cikisAyarla(true); });
  sunucu.on("/kapat", [] { cikisAyarla(false); });
  sunucu.on("/durum", [] { sunucu.send(200, "application/json", durumJson()); });
  sunucu.onNotFound([] { sunucu.send(404, "text/plain", "Bulunamadi"); });
  sunucu.begin();
  Serial.println("Web sunucu hazir, IP bekleniyor...");
}

void loop() {
  sunucu.handleClient();
}
