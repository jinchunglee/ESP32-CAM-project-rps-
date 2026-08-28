#include "wifi_config.h"
#include <Arduino.h> // 

const char* ssid = ""; //change to your wifi
const char* password = "";//change to your password

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


