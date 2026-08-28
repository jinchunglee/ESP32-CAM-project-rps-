#ifndef AI_INFERENCE_H
#define AI_INFERENCE_H

#include "esp_camera.h"

// Declare these two functions so that the main program can call them.
void initAI(); 
const char* runInference(camera_fb_t* fb);

#endif
