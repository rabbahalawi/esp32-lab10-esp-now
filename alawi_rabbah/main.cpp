#include <esp_now.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <string.h>

uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Same struct as the instructor's sketches
typedef struct struct_message {
  char label[32];
  int count;
  float temperature;
  bool alarm;
} struct_message;

struct_message outgoing;

const int MAX_PEERS = 20;
uint8_t heardMacs[MAX_PEERS][6];
volatile int heardCount = 0;
uint8_t myMac[6];

String macToString(const uint8_t *m) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           m[0], m[1], m[2], m[3], m[4], m[5]);
  return String(buf);
}

void onReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  const uint8_t* srcMac = info->src_addr;
  for (int i = 0; i < heardCount; i++) {
    if (memcmp(heardMacs[i], srcMac, 6) == 0) return;   // already counted
  }
  if (heardCount < MAX_PEERS) {
    memcpy(heardMacs[heardCount], srcMac, 6);
    heardCount++;
  }
}

void printSummary() {
  Serial.println("---------- SUMMARY ----------");
  Serial.println("My MAC: " + macToString(myMac));
  Serial.printf("Boards heard: %d\n", heardCount);
  for (int i = 0; i < heardCount; i++) {
    Serial.println("  " + macToString(heardMacs[i]));
  }
  Serial.println("-----------------------------");
}

void setup() {
  Serial.begin(115200);
  unsigned long t = millis();
  while (!Serial && millis() - t < 3000) delay(10);   // wait for native USB

  WiFi.mode(WIFI_STA);
  delay(500);
  esp_read_mac(myMac, ESP_MAC_WIFI_STA);              // reliable MAC read

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, broadcast, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  strncpy(outgoing.label, macToString(myMac).c_str(), sizeof(outgoing.label) - 1);
  outgoing.label[sizeof(outgoing.label) - 1] = '\0';
}

void loop() {
  static unsigned long lastSend = 0, lastPrint = 0;

  if (millis() - lastSend >= 1000) {
    lastSend = millis();
    outgoing.count++;
    outgoing.temperature = 25.0 + (outgoing.count % 5) * 0.5;
    outgoing.alarm = false;
    esp_now_send(broadcast, (uint8_t*)&outgoing, sizeof(outgoing));
  }

  if (millis() - lastPrint >= 5000) {
    lastPrint = millis();
    printSummary();
  }
}
