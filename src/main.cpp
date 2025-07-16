#include <WiFi.h>
#include <esp_now.h>
#include <Arduino.h>

// load this code to SER=5909...
uint8_t receiverMac[] = {0x74, 0x4D, 0xBD, 0x8A, 0x07, 0x64}; // ESP32 S3 - SER=5735...
//uint8_t receiverMac[] = {0x28, 0x37, 0x2F, 0x6A, 0xC0, 0xA4} // ESP32 C3 SuperMini

struct Data {
  uint16_t heartbeat;
  float temperature;
  float oxygen;
  uint32_t timestamp;
};

uint8_t i = 0;
uint8_t sendBuffer[15];
Data currentData;
Data prevData;

int generateHeartbeat() {
  return random(60, 101);  // [60, 100]
}

float generateTemperature() {
  return random(359, 377) / 10.0f; // [35.9, 37.6]
}

float generateOxygen() {
  return random(100, 201) / 100.0f; // [1.00, 2.00]
}

void collectData(void *parameter) {
    while (true) {
        // Сохраняем предыдущие значения
        prevData = currentData;

        // Генерируем новые
        currentData.heartbeat = generateHeartbeat();
        currentData.temperature = generateTemperature();
        currentData.oxygen = generateOxygen();
        currentData.timestamp = millis();
        
        // Собираем пакет
        memcpy(&sendBuffer[1], &currentData.timestamp, sizeof(currentData.timestamp));           // 4 байта
        memcpy(&sendBuffer[5], &currentData.heartbeat, sizeof(currentData.heartbeat));           // 2 байта
        memcpy(&sendBuffer[7], &currentData.oxygen, sizeof(currentData.oxygen));                 // 4 байта
        memcpy(&sendBuffer[11], &currentData.temperature, sizeof(currentData.temperature));      // 4 байта

        // Для отладки принтуем информацию
        Serial.printf("Heartbeat: %d | Temp: %.1f | O2: %.2f | Timestamp: %lu\n",
                    currentData.heartbeat, currentData.temperature,
                    currentData.oxygen, currentData.timestamp);
        
        // Ждём 1с
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void sendData(void *parameter) {
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    while (true) {
        // Устанавливаем номер пакета
        sendBuffer[0] = i;

        // Отправляем пакет
        esp_err_t result = esp_now_send(receiverMac, sendBuffer, sizeof(sendBuffer));
        if (result != ESP_OK) {
        Serial.println("Send failed");
        }

        // Увеличиваем счётчик пакетов
        i = (i + 1) % 128;

        // Ждём 0.1 с
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
    Serial.begin(115200);

    // Включаем ESP NOW
    WiFi.mode(WIFI_STA);
    Serial.println(WiFi.macAddress());
    WiFi.disconnect();
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return;
    }

    // Добавляем главное устройство в список пиров ESP NOW
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (!esp_now_add_peer(&peerInfo)) {
        Serial.println("Peer added");
    } else {
        Serial.println("Failed to add peer");
    }

    // Регистрируем коллбэк на отправку
    esp_now_register_send_cb(onSent);

    // Регистрируем задачи собирать и отправлять информацию
    xTaskCreatePinnedToCore(collectData, "Collect Data Task", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(sendData, "ESP NOW Send Data Task", 2048, NULL, 1, NULL, 1);
}

void loop() {}