#include "inference.h"

std::vector<BBoxInfo> inference(cv::dnn::Net net, cv::Mat image) {
    // 클래스 이름 정의 (YOLOv8의 80개 COCO 클래스)
    std::vector<std::string> class_names = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
        "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
        "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
        "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
        "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
        "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
        "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
        "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
        "hair drier", "toothbrush"
    };
    // image padding
    cv::Mat input(640,640, CV_8UC3, cv::Scalar(0,0,0));
    cv::Mat resized_image;
    float r_w = 640 / (float)image.cols;
    float r_h = 640 / (float)image.rows;
    float r = std::min(r_w, r_h);
    int new_width = (int)(image.cols * r);
    int new_height = (int)(image.rows * r);
    cv::resize(image, resized_image, cv::Size(new_width, new_height), 0, 0, cv::INTER_AREA);
    
    int top_pad = (640 - new_height) / 2;
    int left_pad = (640 - new_width) / 2;

    resized_image.copyTo(input(cv::Rect(left_pad, top_pad, new_width, new_height)));

    //  이미지 전처리
    cv::Mat blob;
    cv::Size input_size(640, 640);
    cv::dnn::blobFromImage(input, blob, 1.0/255.0, input_size, cv::Scalar(), true, false, CV_32F);
    net.setInput(blob);

    //  추론 수행
    std::vector<cv::Mat> outputs;
    std::vector<std::string> output_names = {"output0"};
    net.forward(outputs, output_names);

    //  결과 처리
    float confidence_threshold = 0.5f;
    float nms_threshold = 0.45f;

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
        float confidence = data[4];

        if (confidence >= confidence_threshold) {
            float* classes_scores = data + 4;
            cv::Mat scores(1, class_names.size(), CV_32FC1, classes_scores);
            cv::Point class_id;
            double max_class_score;
            cv::minMaxLoc(scores, 0, &max_class_score, 0, &class_id);

            if (max_class_score > 0.25) {
                confidences.push_back(confidence);
                class_ids.push_back(class_id.x);

                float x = data[0];
                float y = data[1];
                float w = data[2];
                float h = data[3];

                int left = static_cast<int>((x - 0.5 * w - left_pad) / r);
                int top = static_cast<int>((y - 0.5 * h - top_pad) / r);
                int width = static_cast<int>(w / r);
                int height = static_cast<int>(h / r);

                boxes.push_back(cv::Rect(left, top, width, height));
            }
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