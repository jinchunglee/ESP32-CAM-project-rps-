#include "camera_config.h"
#include "wifi_config.h"
#include "ai_inference.h"
#include "web_server.h"

String global_ai_result = "AI開機中...";
bool is_ai_running = false;

// ==========================================
// 優化版 AI 任務：精準掌控 OS 排程
// ==========================================
void aiTask(void *pvParameters) {
    while(1) {
        // 1. 倒數計時階段
        for (int countdown = 5; countdown > 0; countdown--) {
            global_ai_result = "⏱️ 距離下次辨識還有: " + String(countdown) + " 秒";
            // 每次倒數完，主動禮讓 CPU 1秒鐘，確保網頁能流暢印出這行
            vTaskDelay(1000 / portTICK_PERIOD_MS); 
        }

        // 2. 準備進入辨識
        if (!is_ai_running) {
            is_ai_running = true;
            
            // 瞬間切換狀態，不給 Core 0 任何重數的機會
            global_ai_result = "📸 AI 正在努力辨識中...";
            vTaskDelay(10 / portTICK_PERIOD_MS); // 極短暫讓步，強迫 Core 0 印出這行

            camera_fb_t* fb = esp_camera_fb_get();
            if (fb) {
                unsigned long start_time = millis();
                const char* result = runInference(fb); // 進入 8.7 秒大腦凍結
                unsigned long elapsed_time = millis() - start_time;
                float elapsed_seconds = elapsed_time / 1000.0f;

                // 3. 辨識成功，列印終端機報告
                Serial.println("\n========================================");
                Serial.printf("✅ 辨識成功！成果是 %s (耗時 %.1f 秒)\n", result, elapsed_seconds);
                Serial.printf("🧠 內部 RAM 剩餘: %d Bytes | 💾 PSRAM 剩餘: %d Bytes\n", ESP.getFreeHeap(), ESP.getFreePsram());
                Serial.println("========================================");
                
                // 更新狀態給網頁
                global_ai_result = "✅ 辨識成功！" + String(result);
                
                esp_camera_fb_return(fb);
            } else {
                global_ai_result = "⚠️ 警告: 搶不到相機快照，稍後重試...";
            }
            is_ai_running = false;
        }

        // 辨識成功後，畫面停留 2 秒讓使用者看，隨即進入下一輪
        vTaskDelay(2000 / portTICK_PERIOD_MS); 
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- 進入 OS 雙核心多工模式 ---");

    initCamera();        
    initWiFi();          
    initAI();            
    startCameraServer(); 

    // 🎯【優化點】：把最後倒數第二個參數（優先權）從 1 改成 5！
    // 這樣它的層級就會超越一般任務，時間一到，Core 1 必定會立刻執行它！
    xTaskCreatePinnedToCore(
        aiTask, 
        "AI_WorkerTask", 
        8192, 
        NULL, 
        5,  // 👈 提高優先權 (Priority 5)，解決狂數 54321 的排程問題
        NULL, 
        1
    );
    
    Serial.println("🚀 雙核心架構佈署完畢！");
}

void loop() {
    delay(1000);
}