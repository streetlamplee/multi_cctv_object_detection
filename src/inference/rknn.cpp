#include <fstream>
#include <iostream>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <sys/time.h>
#include <vector>
#include <string>
#include "global.h"
#include "rknn.h"
#include "rknn_api.h"



// Helper function to read a file into a buffer
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

int backbone_init(const char *model_path)
{
    int ret;

    // 1. Read RKNN model
    auto model_data = read_file(model_path);
    if (model_data.empty())
    {
        return -1;
    }

    // 2. Initialize RKNN context
    ret = rknn_init(&ctx, model_data.data(), model_data.size(), 0, NULL);
    if (ret < 0)
    {
        std::cerr << "ERROR: rknn_init failed, ret=" << ret << std::endl;
        return ret;
    }

    // Get Model Input Output Info
    {
        int ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return ret;
        }
        printf("Input Count: %d, Output Count: %d\n", io_num.n_input, io_num.n_output);
    }

    // Get Input Attributes
    {
        printf("Input Tensor:\n");
        input_attrs = new rknn_tensor_attr[io_num.n_input];
        memset(input_attrs, 0, io_num.n_input * sizeof(rknn_tensor_attr));
        for (uint32_t i = 0; i < io_num.n_input; i++)
        {
            input_attrs[i].index = i;
            // query info
            int ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
            if (ret < 0)
            {
                return ret;
            }
            dump_tensor_attr(&input_attrs[i]);
        }
    }

    // Get Output Attributes
    {
        printf("Output Tensor:\n");
        output_attrs = new rknn_tensor_attr[io_num.n_output];
        memset(output_attrs, 0, io_num.n_output * sizeof(rknn_tensor_attr));
        for (uint32_t i = 0; i < io_num.n_output; i++)
        {
            output_attrs[i].index = i;
            // query info
            int ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
            if (ret != RKNN_SUCC)
            {
                return ret;
            }
            dump_tensor_attr(&output_attrs[i]);
        }
    }

    rknn_tensor_type input_type = RKNN_TENSOR_UINT8;
    rknn_tensor_format input_layout = RKNN_TENSOR_NHWC;

    // Create input tensor memory
    // default input type is int8 (normalize and quantize need compute in outside)
    // if set uint8, will fuse normalize and quantize to npu
    input_attrs[0].type = input_type;
    // default fmt is NHWC, npu only support NHWC in zero copy mode
    input_attrs[0].fmt = input_layout;

    input_mems = rknn_create_mem(ctx, input_attrs[0].size_with_stride);

    // Set input tensor memory
    ret = rknn_set_io_mem(ctx, input_mems, &input_attrs[0]);
    if (ret < 0)
    {
        printf("rknn_set_io_mem fail! ret=%d\n", ret);
        return ret;
    }

    // Create output tensor memory
    int output_size = output_attrs[0].n_elems * sizeof(float);
    output_mems = rknn_create_mem(ctx, output_size);

    // Set output tensor memory
    output_attrs[0].type = RKNN_TENSOR_FLOAT32;

    // set output memory and attribute
    ret = rknn_set_io_mem(ctx, output_mems, &output_attrs[0]);
    if (ret < 0)
    {
        printf("rknn_set_io_mem fail! ret=%d\n", ret);
        return ret;
    }

    return 0;
}

int backbone_release()
{
    // Release Attributes
    delete[] input_attrs;
    delete[] output_attrs;
    input_attrs = NULL;
    output_mems = NULL;

    // Destroy rknn memory
    rknn_destroy_mem(ctx, input_mems);
    rknn_destroy_mem(ctx, output_mems);
    input_mems = NULL;
    output_mems = NULL;

    // destroy
    rknn_destroy(ctx);
    return 0;
}

int backbone_run(const cv::Mat &image, float *features, size_t features_size)
{
    // 1105 : classifier 나누기
    int ret;
    // 4. Read and preprocess image with OpenCV
    if (image.empty())
    {
        std::cerr << "ERROR: not valid image: " << std::endl;
        return -1;
    }

    // Load image
    int req_height = 0;
    int req_width = 0;
    int req_channel = 0;

    switch (input_attrs[0].fmt)
    {
    case RKNN_TENSOR_NHWC:
        req_height = input_attrs[0].dims[1];
        req_width = input_attrs[0].dims[2];
        req_channel = input_attrs[0].dims[3];
        break;
    case RKNN_TENSOR_NCHW:
        req_height = input_attrs[0].dims[2];
        req_width = input_attrs[0].dims[3];
        req_channel = input_attrs[0].dims[1];
        break;
    default:
        printf("meet unsupported layout\n");
        return -1;
    }

    cv::Mat img;
    img = image;

    // Copy input data to input tensor memory
    int width = input_attrs[0].dims[2];
    int stride = input_attrs[0].w_stride;

    if (width == stride)
    {
        memcpy(input_mems[0].virt_addr, img.data, width * input_attrs[0].dims[1] * input_attrs[0].dims[3]);
    }
    else
    {
        int height = input_attrs[0].dims[1];
        int channel = input_attrs[0].dims[3];
        // copy from src to dst with stride
        uint8_t *src_ptr = img.data;
        uint8_t *dst_ptr = (uint8_t *)input_mems[0].virt_addr;

        // 1105 hj: 에러 수정
        int src_wc_elems = width * channel;
        int dst_wc_elems = stride;

        for (int h = 0; h < height; ++h)
        {
            memcpy(dst_ptr, src_ptr, src_wc_elems);
            src_ptr += src_wc_elems;
            dst_ptr += dst_wc_elems;
        }
    }

    // Run
    ret = rknn_run(ctx, NULL);
    if (ret < 0)
    {
        printf("rknn run error %d\n", ret);
        return -1;
    }

    float *buffer = (float *)output_mems->virt_addr;
    // softmax(buffer, scores, 2); // Commented out: no longer performing classification here

    // Copy features to the provided buffer
    // Assuming the RKNN model outputs a 1D feature vector of size 576
    size_t expected_feature_size = output_attrs[0].n_elems;
    if (features_size < expected_feature_size)
    {
        std::cerr << "[ERROR] Feature buffer is too small. Expected " << expected_feature_size << ", got " << features_size << std::endl;
        return -1;
    }
    memcpy(features, buffer, expected_feature_size * sizeof(float));

    return 0;
}
