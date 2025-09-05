#include "thread_safe_stack.h"
#include "thread_safe_queue.h"
#include "cctv.h"
#include "inference.h"
#include <thread>
#include <iostream>
#include <vector>
#include <mutex>
#include <chrono>
#include <algorithm> // Required for std::find
#include "configHandler.h"
#include "alarm.h"
#include "iniHandler.h"
#include <sstream>



// --- Global Variables ---
cv::Mat g_canvas;
std::mutex g_canvas_mutex;
std::mutex g_alarm_mutex;
bool g_running = true;
std::vector<Alarm> g_alarms;
std::unordered_map<std::string, std::string> g_ini;
int g_queueMaxSize = 5;

// A structure to hold all resources for a single camera channel
struct CameraChannel {
    int channel_number;
    CCTV* cctv_instance = nullptr;
    ThreadSafeQueue<cv::Mat> raw_frame_queue{g_queueMaxSize};
    ThreadSafeQueue<cv::Mat> inference_frame_queue{g_queueMaxSize};
    ThreadSafeQueue<std::vector<BBoxInfo>> results_queue{g_queueMaxSize};
    cv::Rect display_roi;
    std::vector<int> detected_class;
    int alarm = 0;
    std::thread producer_thread;
    std::thread inference_alarm_thread;
    std::thread alarm_thread;

    // Constructor to initialize the ROI
    CameraChannel() {}
    CameraChannel(cv::Rect roi) : display_roi(roi) {}
    // Cleanup function
    ~CameraChannel() {
        if (cctv_instance) {
            delete cctv_instance;
        }
    }
};

// --- Configuration ---
std::vector<int> g_allowed_class_ids = {0, 64, 66, 73};


// --- Thread Functions ---

// Producer: Captures frames from a specific camera channel
void producer(CameraChannel* channel) {
    // channel->cctv_instance = new CCTV(channel->rtsp_url, &channel->raw_frame_stack);
    // channel->cctv_instance->start_image_capture();
    // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    cv::Mat frame;
    std::string id = g_ini["id"];
    std::string password = g_ini["password"];
    std::string ip = g_ini["ip"];
    int port = std::stoi(g_ini["port"]);
    int width = std::stoi(g_ini.at("window_width")) / std::stoi(g_ini.at("window_col"));
    int height = std::stoi(g_ini.at("window_height")) / std::stoi(g_ini.at("window_row"));
    while(g_running){
        if (channel->raw_frame_queue.size() >= g_queueMaxSize) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));    
            continue;
        }
        getFrame_api(id, password, ip, port, channel->channel_number, width, height, frame);
        channel->raw_frame_queue.push(frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
}

// Inference Worker: Performs object detection for a specific camera channel
void inference_alarm_worker(CameraChannel* channel, const std::string net_path) {
    std::vector<Alarm> local_alarms = g_alarms;
    std::string alarm_condition = "";
    int counter = 0;
    cv::dnn::Net net = cv::dnn::readNet(net_path);
    if (net.empty()) {
        std::cerr << "Error: Cannot load ONNX model" << std::endl;
        return;
    }
    while (g_running) {
        cv::Mat frame;
        int risk_level = 0;
        bool isAlarm = false;

        channel->detected_class.clear();
        channel->inference_frame_queue.try_pop(frame);
        if (frame.empty() || !g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        cv::Mat frame_rgb;
        if (cv::sum(frame) == cv::Scalar(0)) { continue; }
        cv::cvtColor(frame, frame_rgb, cv::COLOR_BGR2RGB);

        auto results = inference(net, frame_rgb);
        channel->results_queue.push(results);
        for (auto& det: results) {
            channel->detected_class.push_back(det.classID);
        }
        
        for (Alarm alarm : local_alarms) {
            std::string condition = alarm.get_condition();
            {
                std::lock_guard<std::mutex> lock(g_alarm_mutex);
                std::vector<int> detectedClass = channel->detected_class;
                if (alarm.get_risk_level() < risk_level) { 
                    continue;
                }
                if (define_alarm(condition, detectedClass)) {  // 알람 condition이 충족되면
                    isAlarm = true;
                    risk_level = alarm.get_risk_level();
                    alarm_condition = condition;

                }
                channel->alarm = risk_level;
            }
        }
        if (isAlarm) {
            ++counter;
            
        }
        std::cout << "[Alarm Thread] " << "Condition : " << alarm_condition << ", risk level : " << risk_level << std::endl;
        std::cout << "[Alarm Thread] " << "Warning condition approved, " << counter << "times" << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

// Display: Manages the main canvas, drawing frames and results from all channels
void canvas_painter(std::vector<std::unique_ptr<CameraChannel>>& channels) {
    std::vector<cv::Mat> latest_frames(channels.size());
    std::vector<std::vector<BBoxInfo>> latest_results(channels.size());
    // 0903 fps 테스트용
    // bool isUpdated = false;
    // int frame_count = 0;
    // auto start = std::chrono::high_resolution_clock::now();

    if (g_ini.at("window_width") == "0" || g_ini.at("window_height") == "0") {
        g_canvas = cv::Mat(960, 1408, CV_8UC3, cv::Scalar(0, 0, 0));
    } else {
        g_canvas = cv::Mat(std::stoi(g_ini.at("window_height")), std::stoi(g_ini.at("window_width")), CV_8UC3, cv::Scalar(0,0,0));
    }
    
    while (g_running) {
        // isUpdated = false;
        // 1. Gather latest frames and results from all channels (non-blocking)

        cv::Scalar color_anchor;
        cv::Scalar color_boundary;
        for (size_t i = 0; i < channels.size(); ++i) {
            if (channels[i]->raw_frame_queue.try_pop(latest_frames[i])) {
                if (!latest_frames[i].empty() && cv::sum(latest_frames[i]) != cv::Scalar(0)) {
                    channels[i]->inference_frame_queue.push(latest_frames[i].clone());
                    // isUpdated = true;
                }
                else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            channels[i]->results_queue.try_pop(latest_results[i]);
        }

        // 2. Lock the canvas and draw everything
        {
            std::lock_guard<std::mutex> lock(g_canvas_mutex);
            std::lock_guard<std::mutex> lock2(g_alarm_mutex);

            for (size_t i = 0; i < channels.size(); ++i) {
                // Draw the latest frame to its ROI


                // alarm 발생 시, 빨간 색, 아닐 시 초록 색

                if (channels[i]->alarm == 0){
                    color_anchor = cv::Scalar(0,255,0);
                    color_boundary = cv::Scalar(255,255,255);

                } else if (channels[i]->alarm == 1) {
                    color_anchor = cv::Scalar(0,0,255);
                    color_boundary = cv::Scalar(255,255,255);

                } else if (channels[i]->alarm == 2){
                    color_anchor = cv::Scalar(0,0,255);
                    color_boundary = cv::Scalar(0,0,255);
                }

                if (!latest_frames[i].empty()) {
                    latest_frames[i].copyTo(g_canvas(channels[i]->display_roi));
                }

                // 0829 이현진 점유율 테스트
                // if (!latest_frames[i].empty()){
                //     cv::imshow("image", latest_frames[i]);
                //     // std::cout << latest_frames[i].size << std::endl;
                // }
                
                
                // Draw the latest bounding boxes to its ROI, applying the class filter
                // alarm 테두리 빨간 색 처리 코드   
                cv::rectangle(g_canvas, channels[i]->display_roi, color_boundary, 3);
                if (!latest_results[i].empty()) {
                    for (const auto& det : latest_results[i]) {
                        // FILTERING LOGIC: Check if the classID is in the allowed list
                        if (std::find(g_allowed_class_ids.begin(), g_allowed_class_ids.end(), det.classID) != g_allowed_class_ids.end()) {
                            cv::Rect box = det.box;
                            // IMPORTANT: Adjust resolution (e.g., 1920, 1080) for each camera if they differ
                            float scale_x = (float)channels[i]->display_roi.width / (std::stof(g_ini.at("window_width")) / std::stof(g_ini.at("window_col")));
                            float scale_y = (float)channels[i]->display_roi.height / (std::stof(g_ini.at("window_height")) / std::stof(g_ini.at("window_row")));

                            cv::Rect scaled_box;
                            scaled_box.x = channels[i]->display_roi.x + static_cast<int>(box.x * scale_x);
                            scaled_box.y = channels[i]->display_roi.y + static_cast<int>(box.y * scale_y);
                            scaled_box.width = static_cast<int>(box.width * scale_x);
                            scaled_box.height = static_cast<int>(box.height * scale_y);

                            cv::rectangle(g_canvas, scaled_box, color_anchor, 2);
                            std::string label = det.className + ": " + cv::format("%.2f", det.confidence);
                            cv::putText(g_canvas, label, cv::Point(scaled_box.x, scaled_box.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, color_anchor, 2);
                        }
                    }
                }
            }
        }



        //0903 fps 테스트용

        // if (isUpdated){
        // auto end = std::chrono::high_resolution_clock::now();
        // std::chrono::duration<double> diff = end - start;
        // std::cout << "frame count: " << ++frame_count << std::endl;
        // std::cout << "Elapsed Time: " << diff.count() << "seconds" << std::endl;
        // std::cout << "fps: " << static_cast<double>(frame_count / diff.count()) << std::endl;
        // }
        
        
        // 0829 이현진  CPU 점유율 test
        // std::cout << "col : " << g_canvas.cols << ", row : " << g_canvas.rows << std::endl;
        // std::cout << "some value: " << g_canvas.dims << std::endl;
    }
    
}

// void alarm_worker(CameraChannel* cc) {
//     std::vector<Alarm> local_alarms = g_alarms;
//     std::string alarm_condition = "";
//     int counter = 0;
//    
//     while(g_running) {
//         int risk_level = 0;
//         bool isAlarm = false;
//         for (Alarm alarm : local_alarms) {
//             std::string condition = alarm.get_condition();
//             {
//                 std::lock_guard<std::mutex> lock(g_alarm_mutex);
//                 std::vector<int> detectedClass = cc->detected_class;
//                 if (alarm.get_risk_level() < risk_level) { 
//                     continue;
//                 }
//                 if (define_alarm(condition, detectedClass)) {  // 알람 condition이 충족되면
//                     isAlarm = true;
//                     risk_level = alarm.get_risk_level();
//                     alarm_condition = condition;
//
//                 }
//                 cc->alarm = risk_level;
//             }
//         }
//         if (isAlarm) {
//             ++counter;
//            
//         }
//         std::cout << "[Alarm Thread] " << "Condition : " << alarm_condition << ", risk level : " << risk_level << std::endl;
//         std::cout << "[Alarm Thread] " << "Warning condition approved, " << counter << "times" << std::endl;
//
//         cc->detected_class.clear();
//
//         std::this_thread::sleep_for(std::chrono::milliseconds(3000));
//
//     }
// }

void image_show_worker() {
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    std::stringstream ss_title;
    ss_title <<"NVR Stream - " << std::stoi(g_ini.at("window_row")) * std::stoi(g_ini.at("window_col")) << " Channels";
    while (g_running) {
        {
            // std::lock_guard<std::mutex> lock(g_canvas_mutex);
            cv::imshow(ss_title.str(), g_canvas);
        }
        if (cv::waitKey(1000) == 'q') {
            g_running = false;
            cv::destroyAllWindows();
        }
    }
}

int main1(int argc, char* argv[]) {
    std::string ini_path = "../app.ini";

    read_ini(ini_path, g_ini);

    cv::Mat frame;
    std::string id = g_ini["id"];
    std::string password = g_ini["password"];
    std::string ip = g_ini["ip"];
    int port = std::stoi(g_ini["port"]);
    int width = std::stoi(g_ini.at("window_width")) / std::stoi(g_ini.at("window_col"));
    int height = std::stoi(g_ini.at("window_height")) / std::stoi(g_ini.at("window_row"));


    getFrame_api(id, password, ip, port, 202, width, height, frame);
    cv::imshow("NVR test", frame);
    cv::waitKey(0);
    cv::destroyAllWindows();

    return 0;
}

int main(int argc, char* argv[]) {
    // --- Initialization ---
    std::string onnx_path = "./resource/yolov8n.onnx";
    std::string config_path = "./resource/alarm.conf";
    std::string ini_path = "./resource/app.ini";

    read_conf(config_path, g_alarms);
    read_ini(ini_path, g_ini);

    // --- Configuration ---
    std::vector<std::unique_ptr<CameraChannel>> channels;
    // Define ROIs for a 2x2 grid
    std::vector<cv::Rect> roi_vector;

    configurate_roi_with_ini(g_ini, roi_vector);

    for (int i = 0; i < std::stoi(g_ini.at("window_row")) * std::stoi(g_ini.at("window_col")); i ++) {
        channels.push_back(std::make_unique<CameraChannel>(roi_vector[i]));
    }
    // channels.push_back(std::make_unique<CameraChannel>(cv::Rect(0,  0,  704, 480), 0));
    // channels.push_back(std::make_unique<CameraChannel>(cv::Rect(704,0,  704, 480), 202));
    // channels.push_back(std::make_unique<CameraChannel>(cv::Rect(0,  480,704, 480), 0));
    // channels.push_back(std::make_unique<CameraChannel>(cv::Rect(704,480,704, 480), 0));

    // IMPORTANT: Set the correct RTSP URL for each camera "rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/202/"
    for (int i = 1; i <= std::stoi(g_ini.at("window_row")) * std::stoi(g_ini.at("window_col")); i ++) {
        std::stringstream ss;
        ss << "window" << i << "_channel";
        std::string key = ss.str();
        channels[i-1]->channel_number = std::stoi(g_ini[key]);
    }
    // channels[0]->connection_url = "/ISAPI/ContentMgmt/StreamingProxy/channels/"+std::to_string(channel)+"/picture?videoResolutionWidth=704&videoResolutionHeight=480";
    // channels[1]->connection_url = ""; 
    // channels[2]->connection_url = ""; 
    // channels[3]->connection_url = "";

    // --- Start Threads ---


    for (auto& channel : channels) {
        channel->producer_thread = std::thread(producer, channel.get());

        channel->inference_alarm_thread = std::thread(inference_alarm_worker, channel.get(), onnx_path);

        // channel->alarm_thread = std::thread(alarm_worker, channel.get());
    }
    std::thread painter_thread(canvas_painter, std::ref(channels));
    std::thread imageshow_thread(image_show_worker);

    // --- Wait for Threads to Finish ---
    for (auto& channel : channels) {
        channel->producer_thread.join();
        channel->inference_alarm_thread.join();
        // channel->alarm_thread.join();

    }
    painter_thread.join();
    imageshow_thread.join();


    return 0;
}