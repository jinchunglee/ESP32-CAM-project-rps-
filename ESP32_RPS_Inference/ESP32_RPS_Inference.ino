#include "camera_config.h"
#include "wifi_config.h"
#include "ai_inference.h"
#include "web_server.h"

String global_ai_result = "AI Booting...";
bool is_ai_running = false;

// ==========================================
// Optimized AI Task: Precise OS Scheduling Control
// ==========================================
void aiTask(void *pvParameters) {
    while(1) {
        // 1. Countdown phase
        for (int countdown = 5; countdown > 0; countdown--) {
            global_ai_result = "⏱️ Time until next recognition: " + String(countdown) + " sec";
            // Yield CPU for 1 second after each count to keep the web page streaming smoothly
            vTaskDelay(1000 / portTICK_PERIOD_MS); 
        }

        // 2. Prepare for recognition
        if (!is_ai_running) {
            is_ai_running = true;
            
            // Switch state instantly to prevent Core 0 from double-counting
            global_ai_result = "📸 AI processing recognition...";
            vTaskDelay(10 / portTICK_PERIOD_MS); // Brief yield to force Core 0 to output this line

            camera_fb_t* fb = esp_camera_fb_get();
            if (fb) {
                unsigned long start_time = millis();
                const char* result = runInference(fb); // Enters 8.7s inference freeze
                unsigned long elapsed_time = millis() - start_time;
                float elapsed_seconds = elapsed_time / 1000.0f;

                // 3. Recognition successful, print terminal report
                Serial.println("\n========================================");
                Serial.printf("✅ Recognition successful! Result: %s (Time: %.1f sec)\n", result, elapsed_seconds);
                Serial.printf("🧠 Internal RAM remaining: %d Bytes | 💾 PSRAM remaining: %d Bytes\n", ESP.getFreeHeap(), ESP.getFreePsram());
                Serial.println("========================================");
                
                // Update status for web interface
                global_ai_result = "✅ Success! Result: " + String(result);
                
                esp_camera_fb_return(fb);
            } else {
                global_ai_result = "⚠️ Warning: Failed to capture camera frame, retrying...";
            }
            is_ai_running = false;
        }

        // Hold result on screen for 2 seconds before starting the next cycle
        vTaskDelay(2000 / portTICK_PERIOD_MS); 
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- Entering OS Dual-Core Multitasking Mode ---");

    initCamera();        
    initWiFi();          
    initAI();            
    startCameraServer(); 

    // 🎯 [Optimization]: Increased priority (5th parameter) from 1 to 5!
    // Ensures Core 1 executes this task immediately when scheduled over lower-priority tasks.
    xTaskCreatePinnedToCore(
        aiTask, 
        "AI_WorkerTask", 
        8192, 
        NULL, 
        5,  // 👈 Increased priority (Priority 5) to fix rapid countdown scheduling issues
        NULL, 
        1
    );
    
    Serial.println("🚀 Dual-core architecture deployed successfully!");
}

void loop() {
    delay(1000);
}
