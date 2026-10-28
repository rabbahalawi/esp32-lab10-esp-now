#include <WiFi.h>
#include <esp_now.h>

uint8_t broadcastAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
const int MAX_PEERS = 50;
uint8_t heard[MAX_PEERS][6];
int heardCount = 0;

String macToString(const uint8_t *mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  const uint8_t *mac = info->src_addr;
  for (int i = 0; i < heardCount; i++) {
    if (memcmp(heard[i], mac, 6) == 0) return;   // already known
  }
  if (heardCount < MAX_PEERS) {
    memcpy(heard[heardCount++], mac, 6);
    Serial.println("New board heard: " + macToString(mac));
    Serial.printf("Boards heard so far: %d\n", heardCount);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);          // station mode only, no WiFi.begin()
  delay(500);
  Serial.println("My MAC: " + WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcastAddress, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void loop() {
  const char *msg = "hello";
  esp_now_send(broadcastAddress, (const uint8_t *)msg, strlen(msg));
  delay(2000);
}
