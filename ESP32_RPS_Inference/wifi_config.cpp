#include "wifi_config.h"
#include <Arduino.h> // 記得加這一行才能用 Serial

const char* ssid = "JamesLee";
const char* password = "920501fu";

void initWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("連線中 WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi 已連線！ IP 位址為: ");
    Serial.println(WiFi.localIP());
}


