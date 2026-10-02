/*
 * ESP32 Ethernet Devboard — 04 Ethernet üzerinden MQTT
 *
 * Kablolu ağdan bir MQTT sunucusuna (broker) bağlanır:
 *   - her 10 saniyede <KONU_KOK>/telemetri konusuna JSON yayınlar
 *     (çalışma süresi, boş bellek, analog giriş, IP),
 *   - <KONU_KOK>/komut konusunu dinler: "AC" / "KAPAT" ile CIKIS_PIN'i sürer,
 *   - bağlantı koparsa kendiliğinden yeniden bağlanır.
 *
 * Gereken kütüphane: PubSubClient (Nick O'Leary) — Arduino IDE Kütüphane
 * Yöneticisi'nden "PubSubClient" aratıp kurun.
 *
 * Deneme için herkese açık test sunucusu kullanılır (test.mosquitto.org:1883).
 * Kendi sunucunuzu (Mosquitto, EMQX, HiveMQ…) ve kullanıcı adı/şifresini yazın;
 * herkese açık sunucuya gizli veri göndermeyin.
 * Bilgisayardan izlemek için:
 *   mosquitto_sub -h test.mosquitto.org -t "ilimera/esp32eth/#" -v
 *   mosquitto_pub -h test.mosquitto.org -t "ilimera/esp32eth/<MAC>/komut" -m AC
 *
 * EN: Publishes JSON telemetry over Ethernet to an MQTT broker every 10 s and
 *     listens for "AC"/"KAPAT" (on/off) commands. Requires the PubSubClient library.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ETH.h>
#include <PubSubClient.h>

// ─────────── KULLANICI AYARLARI ───────────
const char* MQTT_SUNUCU   = "test.mosquitto.org";
const uint16_t MQTT_PORT  = 1883;
const char* MQTT_KULLANICI = "";        // gerekmiyorsa boş bırakın
const char* MQTT_SIFRE     = "";
#define CIKIS_PIN   4                    // komutla sürülecek çıkış
#define ANALOG_PIN  34                   // ölçülecek analog giriş (GPIO 32–39)
const unsigned long YAYIN_ARALIGI_MS = 10000;
// ──────────────────────────────────────────

// Ethernet PHY (LAN8720) — kartın donanımına sabittir
#define ETH_ADDR       1
#define ETH_POWER_PIN  -1
#define ETH_MDC_PIN    23
#define ETH_MDIO_PIN   18
#define ETH_TYPE       ETH_PHY_LAN8720
#define ETH_CLK_MODE   ETH_CLOCK_GPIO17_OUT

WiFiClient tcp;              // TCP soketi; ağ arayüzünden bağımsızdır, Ethernet üzerinden çalışır
PubSubClient mqtt(tcp);
volatile bool ethHazir = false;
String konuKok;              // ilimera/esp32eth/<MAC>
unsigned long sonYayin = 0, sonDeneme = 0;

void agOlayi(WiFiEvent_t olay) {
  switch (olay) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname("ilimera-esp32-eth");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("IP: "); Serial.println(ETH.localIP());
      ethHazir = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH baglantisi kesildi");
      ethHazir = false;
      break;
    default:
      break;
  }
}

void mesajGeldi(char* konu, byte* veri, unsigned int uzunluk) {
  String mesaj;
  for (unsigned int i = 0; i < uzunluk; i++) mesaj += (char)veri[i];
  mesaj.trim();
  mesaj.toUpperCase();
  Serial.printf("[%s] %s\n", konu, mesaj.c_str());

  if (mesaj == "AC" || mesaj == "1" || mesaj == "ON")         digitalWrite(CIKIS_PIN, HIGH);
  else if (mesaj == "KAPAT" || mesaj == "0" || mesaj == "OFF") digitalWrite(CIKIS_PIN, LOW);
  mqtt.publish((konuKok + "/cikis").c_str(), digitalRead(CIKIS_PIN) ? "ACIK" : "KAPALI", true);
}

bool mqttBaglan() {
  String istemciId = "ilimera-" + konuKok.substring(konuKok.lastIndexOf('/') + 1);
  String durumKonusu = konuKok + "/durum";
  Serial.printf("MQTT'ye baglaniliyor: %s:%u ... ", MQTT_SUNUCU, MQTT_PORT);
  // Bağlantı koparsa sunucu "durum" konusuna "cevrimdisi" yazar (LWT)
  bool ok = mqtt.connect(istemciId.c_str(),
                         strlen(MQTT_KULLANICI) ? MQTT_KULLANICI : nullptr,
                         strlen(MQTT_SIFRE) ? MQTT_SIFRE : nullptr,
                         durumKonusu.c_str(), 1, true, "cevrimdisi");
  if (!ok) {
    Serial.printf("basarisiz (durum %d)\n", mqtt.state());
    return false;
  }
  Serial.println("baglandi");
  mqtt.publish(durumKonusu.c_str(), "cevrimici", true);
  mqtt.subscribe((konuKok + "/komut").c_str());
  Serial.println("Komut konusu: " + konuKok + "/komut");
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(CIKIS_PIN, OUTPUT);
  digitalWrite(CIKIS_PIN, LOW);

  WiFi.onEvent(agOlayi);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ETH.begin(ETH_TYPE, ETH_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLK_MODE);
#else
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
#endif

  String mac = ETH.macAddress();
  mac.replace(":", "");
  konuKok = "ilimera/esp32eth/" + mac;
  Serial.println("Konu koku: " + konuKok);

  mqtt.setServer(MQTT_SUNUCU, MQTT_PORT);
  mqtt.setCallback(mesajGeldi);
}

void loop() {
  if (!ethHazir) return;

  if (!mqtt.connected()) {
    if (millis() - sonDeneme > 5000) {       // 5 saniyede bir yeniden dene
      sonDeneme = millis();
      mqttBaglan();
    }
    return;
  }
  mqtt.loop();

  if (millis() - sonYayin >= YAYIN_ARALIGI_MS) {
    sonYayin = millis();
    char json[160];
    snprintf(json, sizeof(json),
             "{\"calisma_suresi_s\":%lu,\"bos_bellek\":%u,\"analog_mv\":%u,\"ip\":\"%s\"}",
             millis() / 1000, (unsigned)ESP.getFreeHeap(),
             (unsigned)analogReadMilliVolts(ANALOG_PIN), ETH.localIP().toString().c_str());
    mqtt.publish((konuKok + "/telemetri").c_str(), json);
    Serial.println(json);
  }
}
