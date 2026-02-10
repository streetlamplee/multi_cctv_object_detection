#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "rknn_api.h"
#include <mutex>

struct BBoxInfo
{
    cv::Rect box;
    std::string className;
    int classID;
    float confidence;
};

// inference.h 수정안
class inference_module
{
private:
    // --- RKNN 관련 멤버 변수 (rknn.cpp에서 이사옴) ---
    rknn_context ctx = 0;
    rknn_input_output_num io_num;
    rknn_tensor_attr *input_attrs = nullptr;
    rknn_tensor_attr *output_attrs = nullptr;
    rknn_tensor_mem *input_mems = nullptr;
    rknn_tensor_mem *output_mems = nullptr;

    int input_width = 384;
    int input_height = 224;

    // --- 기존 멤버 변수 ---
    std::vector<std::string> class_names;
    int num_classes = 11;
    int num_rows = 15;
    int num_grids = 0;
    std::vector<float> output_features;

    std::mutex mtx;

    // 내부 도우미 함수 (rknn.cpp의 로직을 private 메서드로 통합)
static std::vector<char> read_file(const std::string &path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        std::cerr << "ERROR: Failed to open file: " << path << std::endl;
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size))
    {
        std::cerr << "ERROR: Failed to read file: " << path << std::endl;
        return {};
    }
    return buffer;
}

static void dump_tensor_attr(rknn_tensor_attr *attr)
{
    printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
           attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
           get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

static float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }

static void softmax(const float *input, float *output, int length)
{
    float max_val = input[0];
    for (int i = 1; i < length; ++i)
    {
        if (input[i] > max_val)
            max_val = input[i];
    }

    float sum = 0.0f;
    for (int i = 0; i < length; ++i)
    {
        output[i] = std::exp(input[i] - max_val); // numerical stability
        sum += output[i];
    }

    for (int i = 0; i < length; ++i)
    {
        output[i] /= sum;
    }
}

public:
    inference_module();
    ~inference_module();
    void inference_init(const std::string &model_path);
    std::vector<BBoxInfo> inference(cv::Mat image);
};