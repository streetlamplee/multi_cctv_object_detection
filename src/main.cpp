#include "thread_safe_stack.h"
#include "cctv.h"
#include "inference.h"
#include <thread>
#include <iostream>
#include <vector>
#include <mutex>
#include <chrono>
#include <algorithm> // Required for std::find
#include "read_config.h"
#include "alarm.h"

// A structure to hold all resources for a single camera channel
struct CameraChannel {
    std::string rtsp_url;
    CCTV* cctv_instance = nullptr;
    ThreadSafeStack<cv::Mat> raw_frame_stack{20};
    ThreadSafeStack<cv::Mat> inference_frame_stack{10};
    ThreadSafeStack<std::vector<BBoxInfo>> results_stack;
    cv::Rect display_roi;
    std::vector<int> detected_class;
    int alarm = 0;
    std::thread producer_thread;
    std::thread inference_thread;
    std::thread alarm_thread;

    // Constructor to initialize the ROI
    CameraChannel(cv::Rect roi) : display_roi(roi) {}

    // Cleanup function
    ~CameraChannel() {
        if (cctv_instance) {
            delete cctv_instance;
        }
    }
};

// --- Global Variables ---
cv::Mat g_canvas(900, 1600, CV_8UC3, cv::Scalar(0, 0, 0));
std::mutex g_canvas_mutex;
std::mutex g_alarm_mutex;
bool g_running = true;
std::vector<Alarm> g_alarms;


// --- Configuration ---
std::vector<int> g_allowed_class_ids = {0, 64, 66, 73};

//test Fuction : 지워도 상관 없는 테스트용 코드
void test_allow_class_id() {
    for (int i; i = 0; i++) {
        g_allowed_class_ids.push_back(i);
    }
}


// --- Thread Functions ---

// Producer: Captures frames from a specific camera channel
void producer(CameraChannel* channel) {
    channel->cctv_instance = new CCTV(channel->rtsp_url, &channel->raw_frame_stack);
    channel->cctv_instance->start_image_capture();
}

// Inference Worker: Performs object detection for a specific camera channel
void inference_worker(CameraChannel* channel, const cv::dnn::Net& net) {
    while (g_running) {
        cv::Mat frame = channel->inference_frame_stack.wait_and_pop();
        if (frame.empty() || !g_running) continue;

        cv::Mat frame_rgb;
        cv::cvtColor(frame, frame_rgb, cv::COLOR_BGR2RGB);

        auto results = inference(net, frame_rgb);
        channel->results_stack.push(results);
    }
}

// Display: Manages the main canvas, drawing frames and results from all channels
void display_manager(std::vector<std::unique_ptr<CameraChannel>>& channels) {
    std::vector<cv::Mat> latest_frames(channels.size());
    std::vector<std::vector<BBoxInfo>> latest_results(channels.size());

    while (g_running) {
        // 1. Gather latest frames and results from all channels (non-blocking)
        for (size_t i = 0; i < channels.size(); ++i) {
            if (channels[i]->raw_frame_stack.try_pop(latest_frames[i])) {
                if (!latest_frames[i].empty()) {
                    channels[i]->inference_frame_stack.push(latest_frames[i].clone());
                }
            }
            channels[i]->results_stack.try_pop(latest_results[i]);
        }

        // 2. Lock the canvas and draw everything
        {
            std::lock_guard<std::mutex> lock(g_canvas_mutex);
            std::lock_guard<std::mutex> lock2(g_alarm_mutex);

            for (size_t i = 0; i < channels.size(); ++i) {
                // Draw the latest frame to its ROI
                if (!latest_frames[i].empty()) {
                    cv::Mat resized_frame;
                    cv::resize(latest_frames[i], resized_frame, channels[i]->display_roi.size());
                    resized_frame.copyTo(g_canvas(channels[i]->display_roi));
                }
                
                channels[i]->detected_class.clear();
                // Draw the latest bounding boxes to its ROI, applying the class filter
                if (!latest_results[i].empty()) {
                    for (const auto& det : latest_results[i]) {
                        cv::Scalar color;

                        channels[i]->detected_class.push_back(det.classID);
                        if (channels[i]->alarm != 0) {
                            color = cv::Scalar(0,0,255);
                        } else {
                            color = cv::Scalar(0,255,0);
                        }
                    
                        // FILTERING LOGIC: Check if the classID is in the allowed list
                        if (std::find(g_allowed_class_ids.begin(), g_allowed_class_ids.end(), det.classID) != g_allowed_class_ids.end()) {
                            cv::Rect box = det.box;
                            // IMPORTANT: Adjust resolution (e.g., 1920, 1080) for each camera if they differ
                            float scale_x = (float)channels[i]->display_roi.width / 1920.0f;
                            float scale_y = (float)channels[i]->display_roi.height / 1080.0f;

                            cv::Rect scaled_box;
                            scaled_box.x = channels[i]->display_roi.x + static_cast<int>(box.x * scale_x);
                            scaled_box.y = channels[i]->display_roi.y + static_cast<int>(box.y * scale_y);
                            scaled_box.width = static_cast<int>(box.width * scale_x);
                            scaled_box.height = static_cast<int>(box.height * scale_y);

                            cv::rectangle(g_canvas, scaled_box, color, 2);
                            std::string label = det.className + ": " + cv::format("%.2f", det.confidence);
                            cv::putText(g_canvas, label, cv::Point(scaled_box.x, scaled_box.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);
                        }
                    }
                }
                
            }
        }

        // 3. Show the final canvas
        cv::imshow("NVR Stream - 4 Channels", g_canvas);
        if (cv::waitKey(1) == 'q') {
            g_running = false;
        }
    }
}

void set_alarm() {
    std::string config_path = "cctv.config";
    std::unordered_map<std::string, std::vector<int>> config;
    read_config(config_path, config);

    for (auto& pair : config) {
        Alarm a = Alarm();
        a.set_description(pair.first);
        a.set_condition(pair.second);
        a.set_risk_level(1);
        g_alarms.push_back(a);
    }
}

void alarm_worker(CameraChannel* cc) {
    std::vector<Alarm> local_alarms = g_alarms;
    int counter = 0;
    while(g_running) {
        
        for (Alarm alarm : local_alarms) {
            std::vector<int> condition = alarm.get_condition();

            {
                std::lock_guard<std::mutex> lock(g_alarm_mutex);
                std::vector<int> detectedClass = cc->detected_class;
                if (define_alarm(condition, detectedClass) && cc->alarm <= alarm.get_risk_level()) {
                    cc->alarm = alarm.get_risk_level();
                    ++counter;
                    std::cout << "Warning condition approved, " << counter << "times" << std::endl;

                } else {
                    cc->alarm = 0;
                }

                cc->detected_class.clear();
            }
            
            
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    }
}

int main(int argc, char* argv[]) {
    // --- Configuration ---
    std::vector<std::unique_ptr<CameraChannel>> channels;
    // Define ROIs for a 2x2 grid
    channels.push_back(std::make_unique<CameraChannel>(cv::Rect(0, 0, 800, 450)));
    channels.push_back(std::make_unique<CameraChannel>(cv::Rect(800, 0, 800, 450)));
    channels.push_back(std::make_unique<CameraChannel>(cv::Rect(0, 450, 800, 450)));
    channels.push_back(std::make_unique<CameraChannel>(cv::Rect(800, 450, 800, 450)));

    // IMPORTANT: Set the correct RTSP URL for each camera
    channels[0]->rtsp_url = "rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/201/";
    channels[1]->rtsp_url = ""; // Change this URL
    channels[2]->rtsp_url = ""; // Change this URL
    channels[3]->rtsp_url = "";// Change this URL

    // --- Initialization ---
    std::string onnx_path = "../resource/yolov8n.onnx";
    cv::dnn::Net net = cv::dnn::readNet(onnx_path);
    if (net.empty()) {
        std::cerr << "Error: Cannot load ONNX model" << std::endl;
        return -1;
    }
    set_alarm();
    test_allow_class_id();

    // --- Start Threads ---
    std::thread display_thread(display_manager, std::ref(channels));

    for (auto& channel : channels) {
        channel->producer_thread = std::thread(producer, channel.get());
        channel->inference_thread = std::thread(inference_worker, channel.get(), std::ref(net));
        channel->alarm_thread = std::thread(alarm_worker, channel.get());
    }

    // --- Wait for Threads to Finish ---
    display_thread.join();
    for (auto& channel : channels) {
        channel->producer_thread.join();
        channel->inference_thread.join();
        channel->alarm_thread.join();

    }

    return 0;
}