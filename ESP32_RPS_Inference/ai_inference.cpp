#include "ai_inference.h"
#include <Arduino.h>
// 引入 TensorFlow Lite 核心
#include <TensorFlowLite_ESP32.h>
#include "model_data.h" 
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

// 這些變數原本在主程式，現在全部收進這個檔案裡當全域變數
static const tflite::Model* model = nullptr;
static tflite::MicroInterpreter* interpreter = nullptr; 
static TfLiteTensor* input = nullptr;
static TfLiteTensor* output = nullptr;

static uint8_t* tensor_arena = nullptr;
const int tensor_arena_size = 200 * 1024; // 200KB

static const char* LABELS[] = {"paper", "rock", "scissors"};
static char ai_result_str[50] = "Waiting...";

// 【極限優化點 1】開機就固定向 PSRAM 分配好 RGB 快取空間，絕對不再 malloc/free 浪費 CPU 時間
// FRAMESIZE_QQVGA 是 160x120，3個通道最大需要 160*120*3 = 57600 Bytes
static uint8_t* global_rgb_buf = nullptr; 

void initAI() {
    // 申請原本模型的 arena 空間
    uint8_t* raw_tensor_arena = (uint8_t*) ps_malloc(tensor_arena_size + 16); 
    if (raw_tensor_arena == NULL) {
        while(1);
    }
    tensor_arena = (uint8_t*)(((uintptr_t)raw_tensor_arena + 15) & ~15);

    // 【分配優化】同時在開機時就把影像快取緩衝區固定好
    global_rgb_buf = (uint8_t*) ps_malloc(160 * 120 * 3);

    // 載入 TFLite 解釋器
    static tflite::MicroErrorReporter micro_error_reporter;
    static tflite::MicroMutableOpResolver<8> resolver;
    resolver.AddConv2D();
    resolver.AddMaxPool2D();
    resolver.AddFullyConnected();
    resolver.AddSoftmax();
    resolver.AddReshape();
    resolver.AddQuantize();
    resolver.AddDequantize(); 

    model = tflite::GetModel(g_model);
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, tensor_arena_size, &micro_error_reporter);
    interpreter = &static_interpreter;
    interpreter->AllocateTensors();

    input = interpreter->input(0);
    output = interpreter->output(0);
    Serial.println("✅ TFLite 核心初始化與快取配置成功！");
}

const char* runInference(camera_fb_t* fb) {
    if (!input || !interpreter || !global_rgb_buf) return "AI Not Ready"; 

    // 將相機當前幀轉碼到我們開機配置好的全域快取中，大幅提升效能
    if (!fmt2rgb888(fb->buf, fb->len, fb->format, global_rgb_buf)) {
        return "Convert Failed";
    }

    // 【極限優化點 2】高速快取指針與查表降維算法，移除迴圈內所有多餘的乘法運算
    float scale_x = (float)fb->width / 96.0f;
    float scale_y = (float)fb->height / 96.0f;
    int fb_width = fb->width;

    for (int y = 0; y < 96; y++) {
        int src_y = (int)(y * scale_y);
        int row_offset = src_y * fb_width; // 提出到內層迴圈外面，少算 96 次乘法
        
        for (int x = 0; x < 96; x++) {
            int src_x = (int)(x * scale_x);
            int src_idx = (row_offset + src_x) * 3;

            // 用定點整數取代浮點數乘法 (0.299 * 256 大約是 77)
            // 這在微控制器上能產生將近 5-10 倍的運算加速！
            uint32_t r = global_rgb_buf[src_idx];
            uint32_t g = global_rgb_buf[src_idx + 1];
            uint32_t b = global_rgb_buf[src_idx + 2];
            
            uint32_t gray = (77 * r + 150 * g + 29 * b) >> 8; 
            
            input->data.int8[y * 96 + x] = (int8_t)(gray - 128);
        }
    }

    // 執行神經網路推論
    if (interpreter->Invoke() != kTfLiteOk) {
        return "Inference Failed!";
    }

    // 找出最大分數的類別
    int best_class = 0;
    int max_score = -129;
    for (int i = 0; i < 3; i++) {
        int8_t score = output->data.int8[i];
        if (score > max_score) {
            max_score = score;
            best_class = i;
        }
    }
    sprintf(ai_result_str, "Result: %s (%d)", LABELS[best_class], max_score);
    return ai_result_str;
}