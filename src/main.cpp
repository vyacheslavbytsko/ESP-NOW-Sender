#include <Arduino.h>

/*// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}*/

#include <WiFi.h>
#include <esp_now.h>
#include <Arduino.h>

uint8_t receiverMac[] = {0x74, 0x4D, 0xBD, 0x8A, 0x07, 0x64};  // Заменить на MAC приёмника

void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return;
    }

    esp_now_register_send_cb(onSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (!esp_now_add_peer(&peerInfo)) {
        Serial.println("Peer added");
    } else {
        Serial.println("Failed to add peer");
    }
}

void loop() {
    const char *msg = "Hello ESP-NOW!";
    esp_err_t result = esp_now_send(receiverMac, (uint8_t *)msg, strlen(msg) + 1);
    if (result != ESP_OK) {
        Serial.println("Send failed");
    }
    delay(5);
}