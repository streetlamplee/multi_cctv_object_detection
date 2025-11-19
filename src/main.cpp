#include "thread_safe_stack.h"
#include "thread_safe_queue.h"
#include "cctv.h"
#include "inference.h"
#include <thread>
#include <iostream>
#include <vector>
#include <mutex>
#include <semaphore.h>
#include <chrono>
#include <algorithm> // Required for std::find
#include "configHandler.h"
#include "alarm.h"
#include "iniHandler.h"
#include <sstream>
#include <filesystem>
#include <iomanip>      // std::setw, std::setfill
#include "logHandler.h"
#include "fileServer.h"
#include "timestamp.h"
#include "directory.h"
#include "alarmManager.h" // 1106 hj modbus 적용
#include "_modbus/modbus_handler.h"

// --- Global Variables ---
cv::Mat g_canvas;
// std::mutex g_canvas_mutex;
// std::mutex g_alarm_mutex;
bool g_running = true;
// std::vector<Alarm> g_alarms; // 1106 hj modbus 적용
AlarmManager g_alarm_manager; // 1106 hj modbus 적용
std::unordered_map<std::string, std::string> g_ini;
int g_queueMaxSize = 5;
sem_t *g_sem_image;
sem_t *g_sem_inference;
const char* get_image_sem_name = "/get_image";
const char* infer_sem_name = "/inference";
std::string log_path = "./resource/app.log";
Log log_handler(log_path, 50);
// fileserver fs;       // nginx 사용으로 인해 사용하지 않음

// Developer Option : 데이터 저장 설정을 ini로 빼둠;
const unsigned int total_data_per_channel = 9999;
const unsigned int duration_between_data = 300;

// A structure to hold all resources for a single camera channel
struct CameraChannel {
    int CameraChannelID;
    int channel_number;
    CCTV* cctv_instance = nullptr;
    ThreadSafeQueue<cv::Mat> raw_frame_queue{g_queueMaxSize};               // 0908 : not used
    ThreadSafeQueue<cv::Mat> inference_frame_queue{g_queueMaxSize};         // 0908 : not used
    ThreadSafeQueue<std::vector<BBoxInfo>> results_queue{g_queueMaxSize};   // 0908 : not used
    cv::Rect display_roi;                                                   // 0908 : not used
    std::vector<int> detected_class;
    int alarm = 0;
    std::thread routine_thread;
    // pthread_t routine_thread;
    std::thread producer_thread;            // 0908 : not used
    std::thread inference_alarm_thread;     // 0908 : not used
    std::thread alarm_thread;               // 0908 : not used

    // Constructor to initialize the ROI
    CameraChannel() {}
    CameraChannel(cv::Rect roi) : display_roi(roi) {}            // 0908 : not used
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

// nginx 사용으로 변경
// void server_worker() {
//     fs.server_init(log_handler);
//     fs.server_start(log_handler);
// }

// log_worker : n 초마다 log class의 정보를 텍스트 파일로 저장
void log_worker() {
    bool heartbit = true;
    while(g_running) {
        log_handler.save();
        if (heartbit) {
            std::cout << ">>>" << std::endl;
        } else {
            std::cout << ">>" << std::endl;
        }
        heartbit = !heartbit;
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
}

void modbus_alarm_reset_worker() {
    while (g_running) {
        int total_channels = std::stoi(g_ini.at("total_window_count"));
        for (int channel_id = 1; channel_id <= total_channels; ++channel_id) {
            int complete_reg = g_alarm_manager.get_modbus_alarm_complete_reg(channel_id);
            if (complete_reg != -1 && modbus_handler_get_hreg(complete_reg) == 1) {
                int status_reg = g_alarm_manager.get_modbus_alarm_status_reg(channel_id);
                int id_reg = g_alarm_manager.get_modbus_alarm_id_reg(channel_id);

                // 알람 상태 초기화
                modbus_handler_set_ireg(status_reg, 0); // status = false
                modbus_handler_set_ireg(id_reg, 0);     // id = 0
                modbus_handler_set_hreg(complete_reg, 0); // 완료 신호 초기화

                // 쿨다운 시작
                g_alarm_manager.start_cooldown(channel_id);

                log_handler.push(Log::Level::INFO, "Alarm reset for channel " + std::to_string(channel_id));
                std::cout << "INFO: Alarm reset for channel " << channel_id << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 1 마다 확인
    }
}

// routine : 하나의 Camera Channel에 할당되는 routine 구현
void routine(CameraChannel* channel, std::string net_path){
    std::string data_gathering_point = g_ini.at("data_gathering_point");
    std::stringstream l;
    cv::Mat frame;
    cv::Mat sub_frame;
    std::string id = g_ini["id"];
    std::string password = g_ini["password"];
    std::string ip = g_ini["ip"];
    int port = std::stoi(g_ini["port"]);
    int width = std::stoi(g_ini.at("window_width"));
    int height = std::stoi(g_ini.at("window_height"));
    // std::vector<Alarm> local_alarms = g_alarms; // 1106 hj modbus 적용
    // std::string now_alarm_condition = ""; // 1106 hj modbus 적용
    // int alarm_timeout = 0; // 1106 hj modbus 적용
    std::stringstream net_result;
    cv::dnn::Net net = cv::dnn::readNet(net_path);
    if (net.empty()) {
        std::cerr << "Error: Cannot load ONNX model" << std::endl;
        log_handler.push(Log::Level::ERROR, "Cannot load ONNX model", channel->CameraChannelID);
        return;
    }
    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
	
	// 0911 bottleneck test
	bool test = false;
	if (channel->CameraChannelID == 1) {
		test = false;
	}
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff;
    while (g_running) {
        // 0919 thread elapsed time 체크
        auto thread_start = std::chrono::high_resolution_clock::now();
        
        
        // NVR 접속 후, channel에 맞게 데이터 가져오기
        // std::cout << "[Thread " << channel->CameraChannelID << "] semaphore 할당 준비" << std::endl;
        sem_wait(g_sem_image);
        

        getFrame_api(id, password, ip, port, channel->channel_number, width, height, frame, sub_frame);
		//~ if (test) {
			//~ std::chrono::duration<double> diff = end-start;
			//~ std::cout << "[Thread " << channel->CameraChannelID << "] image grab time: " << diff.count() << " s" << std::endl;
		//~ }
        // std::cout << "[Thread " << channel->CameraChannelID << "] semaphore 할당 해제" << std::endl;
        sem_post(g_sem_image);
        
        
        // 추론 process
        // int risk_level = 0; // 1106 hj modbus 적용

        channel->detected_class.clear();
        if (frame.empty() || !g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        cv::Mat frame_rgb;
        if (cv::sum(frame) == cv::Scalar(0)) { continue; }

        sem_wait(g_sem_inference);
        cv::cvtColor(sub_frame, frame_rgb, cv::COLOR_BGR2RGB);
        auto results = inference(net, frame_rgb);
        sem_post(g_sem_inference);
        
		// if (test) {
		// 	std::chrono::duration<double> diff = end-start;
		// 	std::cout << "[Thread " << channel->CameraChannelID << "] inference time: " << diff.count() << " s" << std::endl;
		// }
		//~ start = std::chrono::high_resolution_clock::now();
        
        // channel->results_queue.push(results);
        for (auto& det: results) {
            channel->detected_class.push_back(det.classID);
        }

        // 1106 hj modbus 적용
        g_alarm_manager.process_channel_alarms(channel->CameraChannelID, channel->detected_class);
        int status_reg = g_alarm_manager.get_modbus_alarm_status_reg(channel->CameraChannelID);
        if (status_reg != -1) {
            channel->alarm = modbus_handler_get_ireg(status_reg);
        }

        // 1106 hj modbus 적용 - 기존 알람 로직 주석 처리
        /*
        std::vector<int> detectedClass = channel->detected_class;

        // for (auto& det: result) {
        //     det.classID 로 추론된 classID 생성 가능
        // }

        for (Alarm alarm : local_alarms) {
            std::string condition = alarm.get_condition();
            int target_channel = alarm.get_target_channel();
            std::string alarm_sentence = alarm.get_alarm_sentence();
            
            if (target_channel != channel->CameraChannelID) {
                continue;
            }
            if (alarm.get_risk_level() <= risk_level) { 
                continue;
            }
            if (define_alarm(condition, detectedClass)) {  // 알람 condition이 충족되면
                risk_level = alarm.get_risk_level();
                now_alarm_condition = alarm_sentence;
            }
        }
        // std::cout << "risk level : " << risk_level << std::endl;
        if (risk_level > channel->alarm) {
            channel->alarm = risk_level;
            l <<  now_alarm_condition << std::endl;
            log_handler.push(Log::Level::ALARM, l.str(), channel->CameraChannelID);
            l.str("");
            l.clear();
            std::cout << "[ " << std::setw(2) << channel->CameraChannelID << "번 채널 ALARM ]"  << now_alarm_condition << std::endl;

            // l << "Warning condition approved,";
            // log_handler.push(Log::Level::ALARM, l.str(), channel->CameraChannelID);
            // l.str("");
            // l.clear();
            std::cout << "[Thread " << std::setw(2) << channel->CameraChannelID << "]" << "[Alarm] " << "Warning condition approved," << std::endl;
        } else if (risk_level == 0 and channel->alarm != 0) {
            alarm_timeout++;
            // std::cout << "alarm_timeout : " << alarm_timeout << std::endl;
            if (alarm_timeout > 50) {
                channel->alarm = 0;
            }
        } else { }
        */
        
        //~ end = std::chrono::high_resolution_clock::now();
		//~ if (test) {
			//~ std::chrono::duration<double> diff = end-start;
			//~ std::cout << "[Thread " << channel->CameraChannelID << "] alarm detect time: " << diff.count() << " s" << std::endl;
		//~ }
		//~ start = std::chrono::high_resolution_clock::now();
        
        // 추론 후, 그림 그리기
        cv::Scalar color_anchor;
        cv::Scalar color_boundary;
        
        // 2. Lock the canvas and draw everything
        

            // Draw the latest frame to its ROI


            // alarm 발생 시, 빨간 색, 아닐 시 초록 색

        if (channel->alarm == 0){
            color_anchor = cv::Scalar(0,255,0);
            color_boundary = cv::Scalar(255,255,255);

        } else if (channel->alarm == 1) {
            color_anchor = cv::Scalar(0,0,255);
            color_boundary = cv::Scalar(255,255,255);

        } else if (channel->alarm == 2){
            color_anchor = cv::Scalar(0,0,255);
            color_boundary = cv::Scalar(0,0,255);
        }

        // 0829 이현진 점유율 테스트
        // if (!latest_frames[i].empty()){
        //     cv::imshow("image", latest_frames[i]);
        //     // std::cout << latest_frames[i].size << std::endl;
        // }
        
        
        // Draw the latest bounding boxes to its ROI, applying the class filter
        // alarm 테두리 빨간 색 처리 코드  
        cv::Mat frame_save = frame.clone(); 
        cv::rectangle(sub_frame, cv::Rect(0,0,width, height), color_boundary, 3);
        
        for (const auto& det : results) {
            cv::Rect box = det.box;
            // 1. sub_frame과 frame 간의 스케일 비율 계산
            double scale_x = (double)frame.cols / sub_frame.cols;
            double scale_y = (double)frame.rows / sub_frame.rows;

            // 2. 바운딩 박스 좌표를 원본 frame 스케일로 변환
            double scaled_box_x = box.x * scale_x;
            double scaled_box_y = box.y * scale_y;
            double scaled_box_width = box.width * scale_x;
            double scaled_box_height = box.height * scale_y;

            // 3. 변환된 좌표를 사용해 중심점과 너비/높이를 계산하고, frame 기준으로 정규화
            double center_x = (scaled_box_x + scaled_box_width / 2.0) / frame.cols;
            double center_y = (scaled_box_y + scaled_box_height / 2.0) / frame.rows;
            double normalized_width = scaled_box_width / frame.cols;
            double normalized_height = scaled_box_height / frame.rows;

            // YOLO 포맷에 맞게 저장
            net_result << det.classID << " " 
                    << center_x << " " 
                    << center_y << " " 
                    << normalized_width << " " 
                    << normalized_height
                    << std::endl;
            
            // 화면에 그리는 부분은 기존처럼 sub_frame에 그리면 됩니다.
            cv::rectangle(sub_frame, box, color_anchor, 2);
            std::string label = det.className + ": " + cv::format("%.2f", det.confidence);
            cv::putText(sub_frame, label, cv::Point(box.x, box.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, color_anchor, 2);
        }
        
		//~ end = std::chrono::high_resolution_clock::now();
		//~ if (test) {
			//~ std::chrono::duration<double> diff = end-start;
			//~ std::cout << "[Thread " << channel->CameraChannelID << "] image drawing time: " << diff.count() << " s" << std::endl;
		//~ }
		//~ start = std::chrono::high_resolution_clock::now();
        
        std::stringstream ss_output_path;
        std::string output_path = "./resource/output";
        std::filesystem::path output_path_fs = "./resource/output";
        std::filesystem::create_directories(output_path_fs);
        ss_output_path << output_path << "/" << std::setfill('0') << std::setw(2) << channel->CameraChannelID << ".jpg";
        cv::imwrite(ss_output_path.str(), sub_frame);
        
        //~ end = std::chrono::high_resolution_clock::now();
		//~ if (test) {
			//~ std::chrono::duration<double> diff = end-start;
			//~ std::cout << "[Thread " << channel->CameraChannelID << "] image saving time: " << diff.count() << " s" << std::endl;
		//~ }
		//~ start = std::chrono::high_resolution_clock::now();

        // 0910 디버그용.
        // log_handler.push(Log::Level::ALARM, "Debug...", channel->CameraChannelID);

        // 0918 데이터 수집 관련 코드 작성
        end = std::chrono::high_resolution_clock::now();
        diff = end - start;
        if (diff.count() > (duration_between_data)) {
            std::stringstream imgfilepath;
            std::stringstream labelfilepath;
            std::stringstream folderpath;
            auto filename = time_stamp().str();
            std::ofstream label_ofstream;
            folderpath << data_gathering_point << "/" << channel->CameraChannelID;
            imgfilepath << folderpath.str() << "/" << filename << ".jpg";
            labelfilepath << folderpath.str() << "/" << filename << ".txt";
            cv::imwrite(imgfilepath.str(), frame_save);
            label_ofstream.open(labelfilepath.str());
            if (label_ofstream.is_open()) {
                label_ofstream.write(net_result.str().c_str(), net_result.str().size());
            }


            delete_oldest_file_threshold(std::filesystem::path(folderpath.str()), total_data_per_channel * 2);

            start = std::chrono::high_resolution_clock::now();

        }
        net_result.str("");
        net_result.clear();

        //0919 thread elapsed time 체크
        auto thread_end = std::chrono::high_resolution_clock::now();

        if (test) {
            auto thread_diff = thread_end - thread_start;
            std::cout << "thread elapsed time : " << thread_diff.count() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// // Producer: Captures frames from a specific camera channel
// void producer(CameraChannel* channel) {
//     // channel->cctv_instance = new CCTV(channel->rtsp_url, &channel->raw_frame_stack);
//     // channel->cctv_instance->start_image_capture();
//     // std::this_thread::sleep_for(std::chrono::milliseconds(1));
//     cv::Mat frame;
//     std::string id = g_ini["id"];
//     std::string password = g_ini["password"];
//     std::string ip = g_ini["ip"];
//     int port = std::stoi(g_ini["port"]);
//     int width = std::stoi(g_ini.at("window_width")) / std::stoi(g_ini.at("window_col"));
//     int height = std::stoi(g_ini.at("window_height")) / std::stoi(g_ini.at("window_row"));
//     while(g_running){
//         if (channel->raw_frame_queue.size() >= g_queueMaxSize) {
//             std::this_thread::sleep_for(std::chrono::milliseconds(10));    
//             continue;
//         }
//         getFrame_api(id, password, ip, port, channel->channel_number, width, height, frame);
//         channel->raw_frame_queue.push(frame);
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//     }
// }
//
// // Inference Worker: Performs object detection for a specific camera channel
// void inference_alarm_worker(CameraChannel* channel, const std::string net_path) {
//     std::vector<Alarm> local_alarms = g_alarms;
//     std::string alarm_condition = "";
//     int counter = 0;
//     cv::dnn::Net net = cv::dnn::readNet(net_path);
//     if (net.empty()) {
//         std::cerr << "Error: Cannot load ONNX model" << std::endl;
//         return;
//     }
//     while (g_running) {
//         cv::Mat frame;
//         int risk_level = 0;
//         bool isAlarm = false;
//         channel->detected_class.clear();
//         channel->inference_frame_queue.try_pop(frame);
//         if (frame.empty() || !g_running) {
//             std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//             continue;
//         }
//         cv::Mat frame_rgb;
//         if (cv::sum(frame) == cv::Scalar(0)) { continue; }
//         cv::cvtColor(frame, frame_rgb, cv::COLOR_BGR2RGB);
//         auto results = inference(net, frame_rgb);
//         channel->results_queue.push(results);
//         for (auto& det: results) {
//             channel->detected_class.push_back(det.classID);
//         }    
//         for (Alarm alarm : local_alarms) {
//             std::string condition = alarm.get_condition();
//             {
//                 // std::lock_guard<std::mutex> lock(g_alarm_mutex);
//                 std::vector<int> detectedClass = channel->detected_class;
//                 if (alarm.get_risk_level() < risk_level) { 
//                     continue;
//                 }
//                 if (define_alarm(condition, detectedClass)) {  // 알람 condition이 충족되면
//                     isAlarm = true;
//                     risk_level = alarm.get_risk_level();
//                     alarm_condition = condition;
//                 }
//                 channel->alarm = risk_level;
//             }
//         }
//         if (isAlarm) {
//             ++counter;     
//         }
//         std::cout << "[Alarm Thread] " << "Condition : " << alarm_condition << ", risk level : " << risk_level << std::endl;
//         std::cout << "[Alarm Thread] " << "Warning condition approved, " << counter << "times" << std::endl;
//         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//     }
// }
//
// // Display: Manages the main canvas, drawing frames and results from all channels
// void canvas_painter(std::vector<std::unique_ptr<CameraChannel>>& channels) {
//     std::vector<cv::Mat> latest_frames(channels.size());
//     std::vector<std::vector<BBoxInfo>> latest_results(channels.size());
//     // 0903 fps 테스트용
//     // bool isUpdated = false;
//     // int frame_count = 0;
//     // auto start = std::chrono::high_resolution_clock::now();
//     if (g_ini.at("window_width") == "0" || g_ini.at("window_height") == "0") {
//         g_canvas = cv::Mat(960, 1408, CV_8UC3, cv::Scalar(0, 0, 0));
//     } else {
//         g_canvas = cv::Mat(std::stoi(g_ini.at("window_height")), std::stoi(g_ini.at("window_width")), CV_8UC3, cv::Scalar(0,0,0));
//     }
//     while (g_running) {
//         // isUpdated = false;
//         // 1. Gather latest frames and results from all channels (non-blocking)
//         cv::Scalar color_anchor;
//         cv::Scalar color_boundary;
//         for (size_t i = 0; i < channels.size(); ++i) {
//             if (channels[i]->raw_frame_queue.try_pop(latest_frames[i])) {
//                 if (!latest_frames[i].empty() && cv::sum(latest_frames[i]) != cv::Scalar(0)) {
//                     channels[i]->inference_frame_queue.push(latest_frames[i].clone());
//                     // isUpdated = true;
//                 }
//                 else {
//                     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//                 }
//             }
//             else {
//                 std::this_thread::sleep_for(std::chrono::milliseconds(100));
//             }
//             channels[i]->results_queue.try_pop(latest_results[i]);
//         }
//         // 2. Lock the canvas and draw everything
//         {
//             // std::lock_guard<std::mutex> lock(g_canvas_mutex);
//             // std::lock_guard<std::mutex> lock2(g_alarm_mutex);
//             for (size_t i = 0; i < channels.size(); ++i) {
//                 // Draw the latest frame to its ROI
//                 // alarm 발생 시, 빨간 색, 아닐 시 초록 색
//                 if (channels[i]->alarm == 0){
//                     color_anchor = cv::Scalar(0,255,0);
//                     color_boundary = cv::Scalar(255,255,255);
//                 } else if (channels[i]->alarm == 1) {
//                     color_anchor = cv::Scalar(0,0,255);
//                     color_boundary = cv::Scalar(255,255,255);
//                 } else if (channels[i]->alarm == 2){
//                     color_anchor = cv::Scalar(0,0,255);
//                     color_boundary = cv::Scalar(0,0,255);
//                 }
//                 if (!latest_frames[i].empty()) {
//                     latest_frames[i].copyTo(g_canvas(channels[i]->display_roi));
//                 }
//                 // 0829 이현진 점유율 테스트
//                 // if (!latest_frames[i].empty()){
//                 //     cv::imshow("image", latest_frames[i]);
//                 //     // std::cout << latest_frames[i].size << std::endl;
//                 //        
//                 // Draw the latest bounding boxes to its ROI, applying the class filter
//                 // alarm 테두리 빨간 색 처리 코드   
//                 cv::rectangle(g_canvas, channels[i]->display_roi, color_boundary, 3);
//                 if (!latest_results[i].empty()) {
//                     for (const auto& det : latest_results[i]) {
//                         // FILTERING LOGIC: Check if the classID is in the allowed list
//                         if (std::find(g_allowed_class_ids.begin(), g_allowed_class_ids.end(), det.classID) != g_allowed_class_ids.end()) {
//                             cv::Rect box = det.box;
//                             // IMPORTANT: Adjust resolution (e.g., 1920, 1080) for each camera if they differ
//                             float scale_x = (float)channels[i]->display_roi.width / (std::stof(g_ini.at("window_width")) / std::stof(g_ini.at("window_col")));
//                             float scale_y = (float)channels[i]->display_roi.height / (std::stof(g_ini.at("window_height")) / std::stof(g_ini.at("window_row")));
//                             cv::Rect scaled_box;
//                             scaled_box.x = channels[i]->display_roi.x + static_cast<int>(box.x * scale_x);
//                             scaled_box.y = channels[i]->display_roi.y + static_cast<int>(box.y * scale_y);
//                             scaled_box.width = static_cast<int>(box.width * scale_x);
//                             scaled_box.height = static_cast<int>(box.height * scale_y);
//                             cv::rectangle(g_canvas, scaled_box, color_anchor, 2);
//                             std::string label = det.className + ": " + cv::format("%.2f", det.confidence);
//                             cv::putText(g_canvas, label, cv::Point(scaled_box.x, scaled_box.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, color_anchor, 2);
//                         }
//                     }
//                 }
//             }
//         }
//
//         //0903 fps 테스트용
//         // if (isUpdated){
//         // auto end = std::chrono::high_resolution_clock::now();
//         // std::chrono::duration<double> diff = end - start;
//         // std::cout << "frame count: " << ++frame_count << std::endl;
//         // std::cout << "Elapsed Time: " << diff.count() << "seconds" << std::endl;
//         // std::cout << "fps: " << static_cast<double>(frame_count / diff.count()) << std::endl;
//         // }
//         // 0829 이현진  CPU 점유율 test
//         // std::cout << "col : " << g_canvas.cols << ", row : " << g_canvas.rows << std::endl;
//         // std::cout << "some value: " << g_canvas.dims << std::endl;
//         std::cout << std::endl;
//     }
// }
//
// // void alarm_worker(CameraChannel* cc) {
// //     std::vector<Alarm> local_alarms = g_alarms;
// //     std::string alarm_condition = "";
// //     int counter = 0;
// //    
// //     while(g_running) {
// //         int risk_level = 0;
// //         bool isAlarm = false;
// //         for (Alarm alarm : local_alarms) {
// //             std::string condition = alarm.get_condition();
// //             {
// //                 std::lock_guard<std::mutex> lock(g_alarm_mutex);
// //                 std::vector<int> detectedClass = cc->detected_class;
// //                 if (alarm.get_risk_level() < risk_level) { 
// //                     continue;
// //                 }
// //                 if (define_alarm(condition, detectedClass)) {  // 알람 condition이 충족되면
// //                     isAlarm = true;
// //                     risk_level = alarm.get_risk_level();
// //                     alarm_condition = condition;
// //
// //                 }
// //                 cc->alarm = risk_level;
// //             }
// //         }
// //         if (isAlarm) {
// //             ++counter;
// //            
// //         }
// //         std::cout << "[Alarm Thread] " << "Condition : " << alarm_condition << ", risk level : " << risk_level << std::endl;
// //         std::cout << "[Alarm Thread] " << "Warning condition approved, " << counter << "times" << std::endl;
// //
// //         cc->detected_class.clear();
// //
// //         std::this_thread::sleep_for(std::chrono::milliseconds(3000));
// //
// //     }
// // }
//
// void image_show_worker() {
//     std::this_thread::sleep_for(std::chrono::milliseconds(2000));
//     std::stringstream ss_title;
//     ss_title <<"NVR Stream - " << std::stoi(g_ini.at("window_row")) * std::stoi(g_ini.at("window_col")) << " Channels";
//     while (g_running) {
//         {
//             // std::lock_guard<std::mutex> lock(g_canvas_mutex);
//             cv::imshow(ss_title.str(), g_canvas);
//         }
//         if (cv::waitKey(1000) == 'q') {
//             g_running = false;
//             cv::destroyAllWindows();
//         }
//     }
// }
//
// int main_for_test(int argc, char* argv[]) {
//     std::string ini_path = "../app.ini";
//     read_ini(ini_path, g_ini);
//     cv::Mat frame;
//     std::string id = g_ini["id"];
//     std::string password = g_ini["password"];
//     std::string ip = g_ini["ip"];
//     int port = std::stoi(g_ini["port"]);
//     int width = std::stoi(g_ini.at("window_width")) / std::stoi(g_ini.at("window_col"));
//     int height = std::stoi(g_ini.at("window_height")) / std::stoi(g_ini.at("window_row"));
//     getFrame_api(id, password, ip, port, 202, width, height, frame);
//     cv::imshow("NVR test", frame);
//     cv::waitKey(0);
//     cv::destroyAllWindows();
//     return 0;
// }
//
// int main_before_0908(int argc, char* argv[]) { // channel과 기능 별로 모든 thread 를 분리
    // --- Initialization ---
//     std::string onnx_path = "./resource/yolov8n.onnx";
//     std::string config_path = "./resource/alarm.conf";
//     std::string ini_path = "./resource/app.ini";
//     read_conf(config_path, g_alarms);
//     read_ini(ini_path, g_ini);
//     // --- Configuration ---
//     std::vector<std::unique_ptr<CameraChannel>> channels;
//     // Define ROIs for a 2x2 grid
//     std::vector<cv::Rect> roi_vector;
//     configurate_roi_with_ini(g_ini, roi_vector);
//     for (int i = 0; i < std::stoi(g_ini.at("window_row")) * std::stoi(g_ini.at("window_col")); i ++) {
//         channels.push_back(std::make_unique<CameraChannel>(roi_vector[i]));
//     }
//     // channels.push_back(std::make_unique<CameraChannel>(cv::Rect(0,  0,  704, 480), 0));
//     // channels.push_back(std::make_unique<CameraChannel>(cv::Rect(704,0,  704, 480), 202));
//     // channels.push_back(std::make_unique<CameraChannel>(cv::Rect(0,  480,704, 480), 0));
//     // channels.push_back(std::make_unique<CameraChannel>(cv::Rect(704,480,704, 480), 0));
//     // IMPORTANT: Set the correct RTSP URL for each camera "rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/202/"
//     for (int i = 1; i <= std::stoi(g_ini.at("window_row")) * std::stoi(g_ini.at("window_col")); i ++) {
//         std::stringstream ss;
//         ss << "window" << i << "_channel";
//         std::string key = ss.str();
//         channels[i-1]->channel_number = std::stoi(g_ini[key]);
//     }
//     // channels[0]->connection_url = "/ISAPI/ContentMgmt/StreamingProxy/channels/"+std::to_string(channel)+"/picture?videoResolutionWidth=704&videoResolutionHeight=480";
//     // channels[1]->connection_url = ""; 
//     // channels[2]->connection_url = ""; 
//     // channels[3]->connection_url = "";
//     // --- Start Threads ---
//     for (auto& channel : channels) {
//         channel->producer_thread = std::thread(producer, channel.get());
//         channel->inference_alarm_thread = std::thread(inference_alarm_worker, channel.get(), onnx_path);
//         // channel->alarm_thread = std::thread(alarm_worker, channel.get());
//     }
//     std::thread painter_thread(canvas_painter, std::ref(channels));
//     std::thread imageshow_thread(image_show_worker);
//     // --- Wait for Threads to Finish ---
//     for (auto& channel : channels) {
//         channel->producer_thread.join();
//         channel->inference_alarm_thread.join();
//         // channel->alarm_thread.join();
//     }
//     painter_thread.join();
//     imageshow_thread.join();
//     return 0;
// }

void signal_handler(int signum) {
    std::cout << "[Main] 종료 신호 수신... " << std::endl;
    std::cout << "[Main] 이미 실행된 thread의 종료까지 대기..." << std::endl;
    log_handler.push(Log::Level::INFO, "종료 신호 수신...");
    log_handler.push(Log::Level::INFO, "이미 실행된 thread의 종료까지 대기...");
    // fs.server_stop(log_handler);
    g_running = false;
    // std::cerr<< "신호 " << signum << " 수신. semaphore 제거중 ..." << std::endl;
    // sem_unlink(get_image_sem_name);
    // exit(signum);
}

int main(int argc, char* argv[]) {
     // --- Initialization ---
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::string onnx_path = "./resource/yolov8n.quant.onnx";
    std::string config_path = "./resource/alarm.conf";
    std::string ini_path = "./resource/app.ini";

    // read_conf(config_path, g_alarms); // 1106 hj modbus 적용
    read_ini(ini_path, g_ini);
    g_alarm_manager.load_alarms_from_file(config_path); // 1106 hj modbus 적용

    log_handler.load();

    std::string data_gathering_point = g_ini.at("data_gathering_point");
    for (int i = 1; i <= std::stoi(g_ini.at("total_window_count")); i++) {
        std::stringstream ss;
        ss << data_gathering_point << "/" << i;
        std::filesystem::create_directories(ss.str());
    }

    sem_unlink(get_image_sem_name);
    g_sem_image = sem_open(get_image_sem_name, O_CREAT, 0644, 6);
    if (g_sem_image == SEM_FAILED) {
        std::cerr<<"sem_open failed (get_image)" << std::endl;
        return 1;
    }
    sem_unlink(infer_sem_name);
    g_sem_inference = sem_open(infer_sem_name, O_CREAT, 0644, 6);
    if (g_sem_inference == SEM_FAILED) {
        std::cerr << "sem_open failed (inference)" << std::endl;
        return 1;
    }

    

    // --- Configuration ---
    std::vector<std::unique_ptr<CameraChannel>> channels;
    
    for (int i = 0; i < std::stoi(g_ini.at("total_window_count")); i ++) {
        channels.push_back(std::make_unique<CameraChannel>());
    }
    
    // IMPORTANT: Set the correct RTSP URL for each camera "rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/202/"
    for (int i = 1; i <= std::stoi(g_ini.at("total_window_count")); i ++) {
        std::stringstream ss;
        ss << "window" << i << "_channel";
        std::string key = ss.str();
        channels[i-1]->channel_number = std::stoi(g_ini[key]);
        channels[i-1]->CameraChannelID = i;
    }
    // channels[0]->connection_url = "/ISAPI/ContentMgmt/StreamingProxy/channels/"+std::to_string(channel)+"/picture?videoResolutionWidth=704&videoResolutionHeight=480";
    // channels[1]->connection_url = ""; 
    // channels[2]->connection_url = ""; 
    // channels[3]->connection_url = "";

    // --- Start Threads ---
    modbus_handler_init();
    modbus_handler_start();
    printf("Modbus Handler Started!\n");

    log_handler.push(Log::Level::INFO, "프로그램 설정 완료. 프로그램 실행");
    //~ channels[0]->routine_thread = std::thread(routine, channels[0].get(), onnx_path);
    for (auto& channel : channels) {
		channel->routine_thread = std::thread(routine, channel.get(), onnx_path);
        // pthread_create(&channel->routine_thread, NULL, routine, channel.get(), onnx_path); 
        // channel->producer_thread = std::thread(producer, channel.get());
        // channel->inference_alarm_thread = std::thread(inference_alarm_worker, channel.get(), onnx_path);
        // channel->alarm_thread = std::thread(alarm_worker, channel.get());
    }
    // std::thread painter_thread(canvas_painter, std::ref(channels));
    // std::thread imageshow_thread(image_show_worker);
    std::thread log_thread(log_worker);
    std::thread modbus_reset_thread(modbus_alarm_reset_worker);

    // 0910 httplib 대신 nginx 사용으로 변경
    // std::thread server_thread(server_worker);

    // --- Wait for Threads to Finish ---
    //~ channels[0]->routine_thread.join();
    for (auto& channel : channels) {
        channel->routine_thread.join();
    //     // channel->producer_thread.join();
    //     // channel->inference_alarm_thread.join();
    //     // channel->alarm_thread.join();

    }
    // painter_thread.join();
    // imageshow_thread.join();
    log_thread.join();
    modbus_reset_thread.join();

    // 0910 httplib 대신 nginx 사용으로 변경
    // server_thread.join();


    
    

    
    std::cout << "[Main] Server thread 종료 완료. semaphore unlink 시도..." << std::endl;
    log_handler.push(Log::Level::INFO, "모든 thread 종료 완료. semaphore unlink 시작");
    sem_close(g_sem_image);
    sem_unlink(get_image_sem_name);
    sem_close(g_sem_inference);
    sem_unlink(infer_sem_name);
    std::cout << "[Main] semaphore unlink 완료. 프로그램 종료" << std::endl;
    log_handler.push(Log::Level::INFO, "semaphore unlink 종료. 프로그램 종료.");
    return 0;
}
