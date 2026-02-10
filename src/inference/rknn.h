#ifndef INFERENCE_MODULE_H
#define INFERENCE_MODULE_H

#include <opencv2/opencv.hpp>
#include "global.h"

typedef struct
{
    int x;
    int y;
    int w;
    int h;
    float threshold;
} inference_config_t;

// Function to run inference on an image using a given RKNN model.
//
// @param model_path Path to the .rknn model file.
// @param image_path Path to the input image file.
// @return 0 on success, -1 on failure.
int backbone_init(const char* model_path);
// 1105 : classifier 나누기
int backbone_run(const cv::Mat& image, float *features, size_t features_size);
int backbone_release();

#endif // INFERENCE_MODULE_H
