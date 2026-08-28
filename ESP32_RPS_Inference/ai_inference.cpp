#include "ai_inference.h"
#include <Arduino.h>
// Include TensorFlow Lite Core
#include <TensorFlowLite_ESP32.h>
#include "model_data.h" 
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

// Static global variables for TFLite inference core
static const tflite::Model* model = nullptr;
static tflite::MicroInterpreter* interpreter = nullptr; 
static TfLiteTensor* input = nullptr;
static TfLiteTensor* output = nullptr;

static uint8_t* tensor_arena = nullptr;
const int tensor_arena_size = 200 * 1024; // 200KB

static const char* LABELS[] = {"paper", "rock", "scissors"};
static char ai_result_str[50] = "Waiting...";

// [Optimization 1]: Allocate fixed RGB cache in PSRAM during boot to eliminate repeated malloc/free overhead
// FRAMESIZE_QQVGA is 160x120; 3 channels require up to 160*120*3 = 57,600 Bytes
static uint8_t* global_rgb_buf = nullptr; 

void initAI() {
    // Allocate original tensor arena memory
    uint8_t* raw_tensor_arena = (uint8_t*) ps_malloc(tensor_arena_size + 16); 
    if (raw_tensor_arena == NULL) {
        while(1); // Infinite loop on allocation failure
    }
    tensor_arena = (uint8_t*)(((uintptr_t)raw_tensor_arena + 15) & ~15);

    // [Allocation Optimization]: Reserve image buffer in PSRAM at boot time
    global_rgb_buf = (uint8_t*) ps_malloc(160 * 120 * 3);

    // Initialize TFLite interpreter and operators
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
    Serial.println("✅ TFLite core initialization and buffer allocation successful!");
}

const char* runInference(camera_fb_t* fb) {
    if (!input || !interpreter || !global_rgb_buf) return "AI Not Ready"; 

    // Convert frame buffer to allocated global RGB cache for higher performance
    if (!fmt2rgb888(fb->buf, fb->len, fb->format, global_rgb_buf)) {
        return "Convert Failed";
    }

    // [Optimization 2]: Pointer caching and fast downsampling without redundant multiplication inside loops
    float scale_x = (float)fb->width / 96.0f;
    float scale_y = (float)fb->height / 96.0f;
    int fb_width = fb->width;

    for (int y = 0; y < 96; y++) {
        int src_y = (int)(y * scale_y);
        int row_offset = src_y * fb_width; // Lifted outside inner loop to eliminate 96 multiplications
        
        for (int x = 0; x < 96; x++) {
            int src_x = (int)(x * scale_x);
            int src_idx = (row_offset + src_x) * 3;

            // Fixed-point integer arithmetic instead of floating point (0.299 * 256 ≈ 77)
            // Yields 5-10x execution speedup on microcontrollers
            uint32_t r = global_rgb_buf[src_idx];
            uint32_t g = global_rgb_buf[src_idx + 1];
            uint32_t b = global_rgb_buf[src_idx + 2];
            
            uint32_t gray = (77 * r + 150 * g + 29 * b) >> 8; 
            
            input->data.int8[y * 96 + x] = (int8_t)(gray - 128);
        }
    }

    // Execute neural network inference
    if (interpreter->Invoke() != kTfLiteOk) {
        return "Inference Failed!";
    }

    // Find class index with highest score
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
