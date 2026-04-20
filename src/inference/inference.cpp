#include "inference.h"
#include <numeric>
#include <iostream>

inference_module::inference_module()
{
    this->class_names = {
        "standing", "lying down on bed", "sitting on bed", "fallen down",
        "wheel chair", "unknown status", "sitting on chair",
        "sitting on the floor", "food tray", "perch on bed", "staff", "Standing on bed", "jump over"};

    // 출력 버퍼 크기 할당 (15 * 1764)
    this->output_features.resize(num_rows * num_grids);
}

inference_module::~inference_module()
{
    if (this->ctx > 0)
    {
        if (this->input_attrs)
            delete[] this->input_attrs;
        if (this->output_attrs)
            delete[] this->output_attrs;
        if (this->input_mems)
            rknn_destroy_mem(this->ctx, this->input_mems);
        if (this->output_mems)
            rknn_destroy_mem(this->ctx, this->output_mems);
        rknn_destroy(this->ctx);
    }
}



void inference_module::inference_init(const std::string &model_path)
{
    int ret;
    auto model_data = this->read_file(model_path);
    if (model_data.empty()) return;

    // 1. RKNN Context 초기화
    ret = rknn_init(&this->ctx, model_data.data(), model_data.size(), 0, NULL);
    if (ret < 0) {
        std::cerr << "rknn_init fail! ret=" << ret << std::endl;
        return;
    }

    // 2. I/O 개수 쿼리 및 유효성 확인
    ret = rknn_query(this->ctx, RKNN_QUERY_IN_OUT_NUM, &this->io_num, sizeof(this->io_num));
    if (ret != RKNN_SUCC || this->io_num.n_input == 0) {
        std::cerr << "rknn_query(IO_NUM) fail or no inputs!" << std::endl;
        return;
    }

    // 3. 입력 속성 설정 및 메모리 할당
    this->input_attrs = new rknn_tensor_attr[this->io_num.n_input];
    // [현진님 지적 사항] 반드시 0으로 초기화하여 쓰레기 값을 제거해야 합니다.
    memset(this->input_attrs, 0, this->io_num.n_input * sizeof(rknn_tensor_attr));
    
    for (uint32_t i = 0; i < this->io_num.n_input; i++) {
        this->input_attrs[i].index = i;
        ret = rknn_query(this->ctx, RKNN_QUERY_INPUT_ATTR, &(this->input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret < 0) { std::cerr << "input attr query fail!"; return; }
    }

    this->input_width = this->input_attrs[0].dims[2];
    this->input_height = this->input_attrs[0].dims[1];

    // [핵심] 입력 메모리 할당 및 NULL 체크
    this->input_attrs[0].type = RKNN_TENSOR_UINT8;
    this->input_attrs[0].fmt = RKNN_TENSOR_NHWC;
    this->input_mems = rknn_create_mem(this->ctx, this->input_attrs[0].size_with_stride);
    
    // 만약 여기서 NULL이 리턴되면 이후 inference()에서 memcpy 시 Segfault가 발생합니다.
    if (this->input_mems == nullptr) {
        std::cerr << "ERROR: Failed to create input memory!" << std::endl;
        return;
    }
    rknn_set_io_mem(this->ctx, this->input_mems, &this->input_attrs[0]);

    // 4. 출력 속성 설정 및 메모리 할당
    this->output_attrs = new rknn_tensor_attr[this->io_num.n_output];
    memset(this->output_attrs, 0, this->io_num.n_output * sizeof(rknn_tensor_attr));
    
    for (uint32_t i = 0; i < this->io_num.n_output; i++) {
        this->output_attrs[i].index = i;
        ret = rknn_query(this->ctx, RKNN_QUERY_OUTPUT_ATTR, &(this->output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret < 0) { std::cerr << "output attr query fail!"; return; }
    }

    // [핵심 수정] 모델의 실제 출력 차원을 가져옵니다.
    // YOLOv8 RKNN 출력은 보통 [1, 15, 1764] 형태입니다 (NCHW 기준)
    this->num_rows = this->output_attrs[0].dims[1];  // 15 (4 + classes)
    this->num_grids = this->output_attrs[0].dims[2]; // 1764 (Grid 개수)

    // 클래스 개수와 모델 출력이 맞는지 검증
    if (this->num_rows != (this->num_classes + 4)) {
        std::cerr << "Warning: Model output rows (" << num_rows 
                  << ") doesn't match classes + 4!" << std::endl;
    }

    // 출력 버퍼를 실제 모델 크기에 맞게 리사이즈
    this->output_features.resize(this->output_attrs[0].n_elems);

    this->output_attrs[0].type = RKNN_TENSOR_FLOAT32;
    int output_size = this->output_attrs[0].n_elems * sizeof(float);
    this->output_mems = rknn_create_mem(this->ctx, output_size);
    
    // 출력 메모리 NULL 체크
    if (this->output_mems == nullptr) {
        std::cerr << "ERROR: Failed to create output memory!" << std::endl;
        return;
    }
    rknn_set_io_mem(this->ctx, this->output_mems, &this->output_attrs[0]);

    std::cout << "RKNN Channel Init ACTUAL Success: " << model_path << std::endl;
    printf("Output Dim: %d, %d, %d\n", output_attrs[0].dims[0], output_attrs[0].dims[1], output_attrs[0].dims[2]);
}

std::vector<BBoxInfo> inference_module::inference(cv::Mat image)
{
    if (image.empty() || this->ctx == 0 || this->input_mems == nullptr || this->output_mems == nullptr)
    {
        return {};
    }

    std::lock_guard<std::mutex> lock(this->mtx);

    // 1. [버그 수정] 전처리: 입력 이미지를 모델 크기(224x384)로 리사이즈
    cv::Mat resized_img;
    cv::resize(image, resized_img, cv::Size(this->input_width, this->input_height));

    // Copy input data to input tensor memory
    int width = input_attrs[0].dims[2];
    int stride = input_attrs[0].w_stride;

    if (width == stride)
    {
        memcpy(input_mems[0].virt_addr, resized_img.data, width * input_attrs[0].dims[1] * input_attrs[0].dims[3]);
    }
    else
    {
        int height = input_attrs[0].dims[1];
        int channel = input_attrs[0].dims[3];
        // copy from src to dst with stride
        uint8_t *src_ptr = resized_img.data;
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

    // 2. NPU 추론 실행
    if (rknn_run(this->ctx, NULL) < 0)
        return {};

    // 3. 결과 데이터 복사 (FLOAT32)
    float *buffer = (float *)this->output_mems[0].virt_addr;
    size_t expected_elems = output_attrs[0].n_elems;
    if (this->output_features.size() < expected_elems)
    {
        std::cerr << "[ERROR] Feature buffer is too small. Expected " << expected_elems << ", got " << this->output_features.size() << std::endl;
        return {};
    }
    memcpy(this->output_features.data(), buffer, expected_elems * sizeof(float));

    // 4. 후처리 (Post-processing)
    cv::Mat output_data(this->num_rows, this->num_grids, CV_32F, this->output_features.data());
    cv::Mat transposed_output;
    cv::transpose(output_data, transposed_output); // [1764, 15]

    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;
    float confidence_threshold = 0.5f;

    for (int i = 0; i < transposed_output.rows; i++)
    {
        float *data = transposed_output.ptr<float>(i);
        cv::Mat scores(1, class_names.size(), CV_32FC1, data + 4);

        cv::Point class_id_point;
        double max_class_score;
        cv::minMaxLoc(scores, 0, &max_class_score, 0, &class_id_point);

        if (max_class_score > confidence_threshold)
        {
            float x = data[0], y = data[1], w = data[2], h = data[3];
            float x_scale = image.cols / (float)this->input_width;
            float y_scale = image.rows / (float)this->input_height;

            int left = static_cast<int>((x - 0.5f * w) * x_scale);
            int top = static_cast<int>((y - 0.5f * h) * y_scale);
            int width_box = static_cast<int>(w * x_scale);
            int height_box = static_cast<int>(h * y_scale);

            boxes.push_back(cv::Rect(left, top, static_cast<int>(w * x_scale), static_cast<int>(h * y_scale)));
            confidences.push_back((float)max_class_score);
            class_ids.push_back(class_id_point.x);
        }
    }

    // 5. NMS 적용
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, confidence_threshold, 0.5f, indices);

    std::vector<BBoxInfo> detectedObjects;
    for (int idx : indices)
    {
        detectedObjects.push_back({boxes[idx], class_names[class_ids[idx]], class_ids[idx], confidences[idx]});
    }
    return detectedObjects;
}