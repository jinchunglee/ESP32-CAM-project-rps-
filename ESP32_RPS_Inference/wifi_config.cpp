#include "wifi_config.h"
#include <Arduino.h> // Include Arduino header to enable Serial communication

const char* ssid = "";// change to your own wifi  
const char* password = "";// change to your own password

void initWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected! IP address: ");
    Serial.println(WiFi.localIP());
}
