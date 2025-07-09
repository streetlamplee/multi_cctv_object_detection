#define CVUI_IMPLEMENTATION
#include "cvui.h"
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <thread>
#include <mutex>
#include <deque>
#include <atomic>
#include <iostream>

#define WINDOW_NAME "CVUI Window"

std::deque<cv::Mat> image_deque;
std::mutex queue_mutex;
std::atomic<bool> running(true);
std::atomic<bool> inference_enabled(false);
std::atomic<bool> isImageGet(false);
std::vector<std::string> class_names = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard",
    "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase",
    "scissors", "teddy bear", "hair drier", "toothbrush"
};


void stream_reader(const std::string& rtsp_url) {
    cv::VideoCapture cap(rtsp_url);
    if (!cap.isOpened()) {
        std::cerr << "RTSP 스트림 열기에 실패" << std::endl;
        return;
    }

    while (running) {
        cv::Mat frame;
        if (cap.read(frame)) {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (image_deque.size() > 30) {
                image_deque.pop_front();
            }
            image_deque.push_back(frame);
        }
        else {
            std::cerr << "RTSP 프레임 읽기에 실패" << std::endl;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    cap.release();
}

cv::Mat letterbox(const cv::Mat& src, int new_width, int new_height, cv::Mat& out, int stride = 32) {
    int original_width = src.cols;
    int original_height = src.rows;

    float r = std::min((float)new_width / original_width, (float)new_height / original_height);
    int resized_width = (int)(r * original_width);
    int resized_height = (int)(r * original_height);

    cv::Mat resized;
    cv::resize(src, resized, cv::Size(resized_width, resized_height));

    int top = (new_height - resized_height) / 2;
    int bottom = new_height - resized_height - top;
    int left = (new_width - resized_width) / 2;
    int right = new_width - resized_width - left;

    cv::copyMakeBorder(resized, out, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    return cv::Mat(cv::Size(resized_width, resized_height), CV_8UC3);  // Optional: return for bounding box scaling
}

int main() {
    cv::Mat frame = cv::Mat(980, 1540, CV_8UC3);
    cvui::init(WINDOW_NAME);

    cv::dnn::Net yoloNet;
    try {
        yoloNet = cv::dnn::readNet("yolov5s-simplified.onnx");
    } catch (const cv::Exception e) {
        std::cerr << "YOLO 모델 로딩 실패" << e.what() << std::endl;
    }

    std::thread rtsp_thread(stream_reader, "rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/201/");

    while (true) {
        frame = cv::Scalar(200, 200, 200);

        // UI Elements
        if (cvui::button(frame, 50, 100, 120, 30, "getImagebtn")) {
            isImageGet = !isImageGet;
        }

        if (cvui::button(frame, 50, 180, 120, 30, "Inferencebtn")) {
            inference_enabled = !inference_enabled;
        }

        cvui::text(frame, 80, 80, "FrameLabel");

        if (isImageGet) {
            cv::Mat image;
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                if (!image_deque.empty()) {
                    image = image_deque.back().clone();
                }
            }

            if (!image.empty()) {
                cv::Mat image_input;
                cv::resize(image, image, cv::Size(1280, 720));
                cv::resize(image, image_input, cv::Size(640, 360));
                // if (inference_enabled){
                //     printf(yoloNet.empty() ? "true" : "false");
                //     printf(image.empty() ? "true" : "false");
                // }

                // cv::imshow("debug", letterboxed);
                if (inference_enabled  && !yoloNet.empty() && !image.empty()) {
                    cv::Mat letterboxed;
                    cv::Mat letterboxed_original_size;
                    letterbox(image_input, 640, 640, letterboxed);
                    letterbox(image, 1280, 1280, letterboxed_original_size);
                    cv::Mat blob = cv::dnn::blobFromImage(letterboxed, 1.0/255.0, cv::Size(640, 640), cv::Scalar(), true, false);
                    std::vector<cv::Mat> outputs;
                    
                    if (!blob.empty()) {
                        yoloNet.setInput(blob);
                        try {
                        yoloNet.forward(outputs, yoloNet.getUnconnectedOutLayersNames());
                        } catch (const cv::Exception e) {
                            std::cerr << "YOLO Forward() 실패 : " << e.what() << std::endl;
                        }
                    }
                    // std::cout << "Output Shape: " << outputs[0].size << std::endl;

                    cv::Mat out = outputs[0];
                    if (out.dims == 3 && out.size[0] == 1) {
                        out = out.reshape(1, out.size[1]);
                    }

                    float conf_threshold = 0.5;
                    std::set<int> target_classes = {0};

                    std::vector<cv::Rect> boxes;
                    std::vector<float> confidences;
                    std::vector<int> class_ids;

                    for (int i = 0; i < out.rows; ++i) {
                        float* data = out.ptr<float>(i);
                        float obj_conf = data[4];
                        double class_conf;
                        if (obj_conf > conf_threshold) {
                            float* class_scores = data + 5;
                            cv::Mat scores(1, 80, CV_32FC1, class_scores);
                            cv::Point classIdPoint;
                            
                            minMaxLoc(scores, 0, &class_conf, 0, &classIdPoint);

                            int class_id = classIdPoint.x;

                            if (class_conf > conf_threshold && target_classes.count(class_id)) {
                                int centerX = static_cast<int>(data[0]);
                                int centerY = static_cast<int>(data[1]);
                                int width   = static_cast<int>(data[2]);
                                int height  = static_cast<int>(data[3]);

                                centerX *= 2;
                                centerY *= 2;
                                width *= 2;
                                height *= 2;

                                int left = centerX - width / 2;
                                int top  = centerY - height / 2;

                                boxes.emplace_back(left, top, width, height);
                                confidences.push_back(static_cast<float>(obj_conf * class_conf));
                                class_ids.push_back(class_id);
                            }
                        }
                    }
                    std::vector<int> indices;
                    float iou_threshold = 0.5;
                    cv::dnn::NMSBoxes(boxes, confidences, conf_threshold, iou_threshold, indices);

                    // 🎯 시각화
                    for (int idx : indices) {
                        const cv::Rect& box = boxes[idx];
                        int class_id = class_ids[idx];
                        float conf = confidences[idx];

                        std::string label = class_id < class_names.size()
                                            ? class_names[class_id] + " : " + cv::format("%.2f", conf)
                                            : "class_" + std::to_string(class_id) + " : " + cv::format("%.2f", conf);

                        cv::rectangle(letterboxed_original_size, box, cv::Scalar(0, 0, 255), 2);
                        cv::putText(letterboxed_original_size, label, cv::Point(box.x, box.y - 5),
                                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
                    }

                    int padding = (1280 - 720) / 2;
                    image = letterboxed_original_size(cv::Rect(0, padding, 1280, 720));
                }
                image.copyTo(frame(cv::Rect(240, 180, image.cols, image.rows)));
            } else {
                cvui::text(frame, 80, 80, "RTSP 수신 이미지 없음");
            }
        }

        cvui::update();
        cv::imshow(WINDOW_NAME, frame);

        if (cv::waitKey(20) == 27) { // ESC to quit
            break;
        }
    }

    rtsp_thread.join();
    cv::destroyAllWindows();
    return 0;
}
