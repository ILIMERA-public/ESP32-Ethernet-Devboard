/*
 * ESP32 Ethernet Devboard — 05 Ethernet öncelikli, Wi-Fi yedekli bağlantı
 *
 * Kart her zaman kablolu ağı tercih eder. Ethernet kablosu çıkarılırsa ya da
 * ağ geçidine ulaşılamazsa Wi-Fi'ye geçer; kablo geri takılıp IP alınınca
 * Wi-Fi'yi kapatıp tekrar Ethernet'e döner. Her 15 saniyede hangi arayüzün
 * kullanıldığını ve internet erişimini seri ekrana yazar.
 *
 * Neden iki arayüz aynı anda açık değil?  ESP32 trafiği varsayılan olarak en son
 * IP alan arayüze yönlendirmeye eğilimlidir (teknik dokümanda "Ethernet ve Wi-Fi
 * çakışmaları"). Bu örnek her an tek arayüzü açık tutarak bu belirsizliği ortadan kaldırır.
 *
 * Ayarlar: WIFI_AG ve WIFI_SIFRE.
 * Besleme: Wi-Fi ve Ethernet birlikte çalışırken en az 1 A veren bir USB kaynağı kullanın.
 *
 * EN: Prefers Ethernet; falls back to Wi-Fi when the cable is unplugged and returns
 *     to Ethernet once it gets an IP again. Only one interface is active at a time
 *     to avoid default-route ambiguity. Set WIFI_AG / WIFI_SIFRE.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ETH.h>
#include <HTTPClient.h>

// ─────────── KULLANICI AYARLARI ───────────
const char* WIFI_AG    = "AgAdi";
const char* WIFI_SIFRE = "Sifre";
// ──────────────────────────────────────────

// Ethernet PHY (LAN8720) — kartın donanımına sabittir
#define ETH_ADDR       1
#define ETH_POWER_PIN  -1
#define ETH_MDC_PIN    23
#define ETH_MDIO_PIN   18
#define ETH_TYPE       ETH_PHY_LAN8720
#define ETH_CLK_MODE   ETH_CLOCK_GPIO17_OUT

const char* TEST_URL = "http://clients3.google.com/generate_204";

volatile bool ethIpVar = false;
volatile bool wifiIpVar = false;
bool wifiAcik = false;
unsigned long sonKontrol = 0;

void agOlayi(WiFiEvent_t olay) {
  switch (olay) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname("ilimera-esp32-eth");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("[ETH] IP: "); Serial.println(ETH.localIP());
      ethIpVar = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("[ETH] baglanti kesildi");
      ethIpVar = false;
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("[WiFi] IP: "); Serial.println(WiFi.localIP());
      wifiIpVar = true;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      wifiIpVar = false;
      break;
    default:
      break;
  }
}

void wifiAc() {
  if (wifiAcik) return;
  Serial.println("Ethernet yok, Wi-Fi'ye geciliyor...");
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_AG, WIFI_SIFRE);
  wifiAcik = true;
}

void wifiKapat() {
  if (!wifiAcik) return;
  Serial.println("Ethernet geri geldi, Wi-Fi kapatiliyor.");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiAcik = false;
  wifiIpVar = false;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== Ethernet oncelikli, Wi-Fi yedekli baglanti ===");
  WiFi.onEvent(agOlayi);
  WiFi.mode(WIFI_OFF);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ETH.begin(ETH_TYPE, ETH_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLK_MODE);
#else
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
#endif
  sonKontrol = millis();   // açılışta Ethernet'e IP alması için zaman tanı
}

void loop() {
  // Ethernet varsa Wi-Fi'yi kapat; 10 sn içinde Ethernet IP alamazsa Wi-Fi'yi aç
  if (ethIpVar) {
    wifiKapat();
  } else if (!wifiAcik && millis() > 10000) {
    wifiAc();
  }

  if (millis() - sonKontrol >= 15000) {
    sonKontrol = millis();
    const char* arayuz = ethIpVar ? "Ethernet" : (wifiIpVar ? "Wi-Fi" : "yok");
    int kod = -1;
    if (ethIpVar || wifiIpVar) {
      HTTPClient http;
      http.setConnectTimeout(5000);
      http.begin(TEST_URL);
      kod = http.GET();
      http.end();
    }
    Serial.printf("Aktif arayuz: %-8s | internet testi: %d\n", arayuz, kod);
  }
}
