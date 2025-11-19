#include "inference/inference.h"
    
std::vector<BBoxInfo> inference(cv::dnn::Net net, cv::Mat image) {
    // 클래스 이름 정의 (YOLOv8의 80개 COCO 클래스)
    std::vector<std::string> class_names = {
        "standing",
        "lying down on bed",
        "sitting on bed",
        "fallen down",
        "wheel chair",
        "unknown status",
        "sitting on chair",
        "sitting on the floor",
        "food tray",
        "perch on bed", 
        "staff"
    };
    // image padding
    int s = 224;
    cv::Mat input(s,s, CV_8UC3, cv::Scalar(114,114,114));
    cv::Mat resized_image;
    float r_w = s / (float)image.cols;
    float r_h = s / (float)image.rows;
    float r = std::min(r_w, r_h);
    int new_width = (int)(image.cols * r);
    int new_height = (int)(image.rows * r);
    cv::resize(image, resized_image, cv::Size(new_width, new_height), 0, 0, cv::INTER_AREA);
    
    int top_pad = 0;
    int left_pad = (s - new_width) / 2;

    resized_image.copyTo(input(cv::Rect(left_pad, top_pad, new_width, new_height)));

	// 0917 not letterboxing the input image, just resize
	//cv::Mat input;
	//cv::resize(image, input, cv::Size(s, s), 0, 0, cv::INTER_AREA);

    //  이미지 전처리
    cv::Mat blob;
    cv::Size input_size(s, s);
    cv::dnn::blobFromImage(input, blob, 1.0/255.0, input_size, cv::Scalar(), true, false, CV_32F);
    net.setInput(blob);

    //  추론 수행
    std::vector<cv::Mat> outputs;
    std::vector<std::string> output_names = {"output0"};
    net.forward(outputs, output_names);

    //  결과 처리
    float confidence_threshold = 0.4f;
    float nms_threshold = 0.5f;

    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;
    
    // 출력 텐서의 크기 및 구조에 따라 처리
    cv::Mat output_data = outputs[0];
    cv::Mat transposed_output;
    cv::transpose(output_data.reshape(1, output_data.size[1]), transposed_output);

    for (int i = 0; i < transposed_output.rows; i++) {
        cv::Mat row = transposed_output.row(i);
        float* data = (float*)row.data;

        // 클래스 점수는 5번 인덱스부터 시작합니다.
        cv::Mat scores(1, class_names.size(), CV_32FC1, data + 4);

        // 가장 높은 점수를 가진 클래스의 ID와 점수를 찾습니다.
        cv::Point class_id_point;
        double max_class_score;
        cv::minMaxLoc(scores, 0, &max_class_score, 0, &class_id_point);

        // 최종 신뢰도 = 객체 신뢰도 * 클래스 점수
        float confidence = (float)max_class_score;

        // 최종 신뢰도가 임계값을 넘는지 확인합니다.
        if (confidence > confidence_threshold) { // confidence_threshold는 0.5f로 설정되어 있음
            confidences.push_back(confidence);
            class_ids.push_back(class_id_point.x);

            float x = data[0];
            float y = data[1];
            float w = data[2];
            float h = data[3];

            int left = static_cast<int>((x - 0.5 * w - left_pad) / r);
            int top = static_cast<int>((y - 0.5 * h - top_pad) / r);
            // int left = static_cast<int>((x - 0.5 * w));
            // int top = static_cast<int>((y - 0.5 * h));
            int width = static_cast<int>(w / r);
            int height = static_cast<int>(h / r);
            // int width = static_cast<int>(w);
            // int height = static_cast<int>(h);

            boxes.push_back(cv::Rect(left, top, width, height));
        }
    }

    //  NMS 적용
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, confidence_threshold, nms_threshold, indices);

    std::vector<BBoxInfo> detectedObjects;

    for (size_t i = 0; i < indices.size(); ++i) {
        int idx = indices[i];

        BBoxInfo info;
        info.box = boxes[idx];
        info.className = class_names[class_ids[idx]];
        info.classID = class_ids[idx];
        info.confidence = confidences[idx];
        
        detectedObjects.push_back(info);
    }
    return detectedObjects;
}
