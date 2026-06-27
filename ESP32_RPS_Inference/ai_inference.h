#ifndef AI_INFERENCE_H
#define AI_INFERENCE_H

#include "esp_camera.h"

// 宣告這兩個功能，讓主程式呼叫
void initAI(); 
const char* runInference(camera_fb_t* fb);

#endif