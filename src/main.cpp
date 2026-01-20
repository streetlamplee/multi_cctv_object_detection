#include "thread_safe/thread_safe_stack.h"
#include "thread_safe/thread_safe_queue.h"
#include "cctv/cctv.h"
#include "inference/inference.h"
#include <thread>
#include <iostream>
#include <vector>
#include <mutex>
#include <semaphore.h>
#include <chrono>
#include <time.h>
#include <algorithm> // Required for std::find
#include "config/configHandler.h"
#include "alarm/alarm.h"
#include <sstream>
#include <filesystem>
#include <iomanip> // std::setw, std::setfill
#include "config/logHandler.h"
#include "time/timestamp.h"
#include "data_save/directory.h"
#include "alarm/alarmManager.h"      // 1106 hj modbus 적용
#include "mqttManager/mqttManager.h" //1119 hj modbus -> mqtt로 변경
#include "config/INIReader.h"
#include <cstdint>
#include "global.h"
#include <deque>

// --- Global Variables ---

cv::Mat g_canvas;

bool g_running = true;

AlarmManager g_alarm_manager; // 1106 hj modbus 적용

int g_queueMaxSize = 5;
sem_t *g_sem_image;
sem_t *g_sem_inference;
const char *get_image_sem_name = "/get_image";
const char *infer_sem_name = "/inference";
std::string log_path = "./resource/app.log";
INIReader *ini_reader;
Log log_handler(log_path, 50);
// fileserver fs;       // nginx 사용으로 인해 사용하지 않음

// Developer Option : 데이터 저장 설정을 ini로 빼둠;
const unsigned int total_data_per_channel = 9999;
const unsigned int duration_between_data = 300;

struct CameraChannel;
void log_worker();
void routine(CameraChannel *channel, std::string net_path);
void signal_handler(int signum);
void loadConfig();

// A structure to hold all resources for a single camera channel
struct CameraChannel
{
    int CameraChannelID;
    int channel_number;
    std::string channel_camera_description;
    std::string robotDestination;
    CCTV *cctv_instance = nullptr;

    std::deque<detected_history_item> detected_class_history;
    std::vector<int> detected_class;
    int alarm = 0;
    std::thread routine_thread;

    // Constructor to initialize the ROI
    CameraChannel() {}
    // CameraChannel(cv::Rect roi) : display_roi(roi) {}            // 0908 : not used
    // Cleanup function
    ~CameraChannel()
    {
        if (cctv_instance)
        {
            delete cctv_instance;
        }
    }
};

int main(int argc, char *argv[])
{
    // --- Initialization ---
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    loadConfig();

    std::string onnx_path = "./resource/yolov8n.quant.onnx";
    std::string config_path = "./resource/alarm.conf";
    std::string ini_path = "./resource/app.ini";

    // read_conf(config_path, g_alarms); // 1106 hj modbus 적용
    // read_ini(ini_path, g_ini);
    g_alarm_manager.load_alarms_from_file(config_path); // 1106 hj modbus 적용
    g_alarm_manager.set_cooltime(std::stoi(ini_reader->Get("Alarm Context", "AlarmCooltime", "180")));

    log_handler.load();

    std::string data_gathering_point = ini_reader->Get("Developer Option", "data_gathering_point", "./data");
    for (int i = 1; i <= std::stoi(ini_reader->Get("Window Configuration", "total_window_count", "12")); i++)
    {
        std::stringstream ss;
        ss << data_gathering_point << "/" << i;
        std::filesystem::create_directories(ss.str());
    }
    for (int i = 1; i <= std::stoi(ini_reader->Get("Alarm Context", "AlarmNum", "6")); i++)
    {
        std::stringstream ss;
        ss << "AlarmID" << i;
        g_alarm_manager.alarm_context.push_back(ini_reader->Get("Alarm Context", ss.str(), "간호사 호출 중"));
    }

    sem_unlink(get_image_sem_name);
    g_sem_image = sem_open(get_image_sem_name, O_CREAT, 0644, 6);
    if (g_sem_image == SEM_FAILED)
    {
        std::cerr << "sem_open failed (get_image)" << std::endl;
        return 1;
    }
    sem_unlink(infer_sem_name);
    g_sem_inference = sem_open(infer_sem_name, O_CREAT, 0644, 6);
    if (g_sem_inference == SEM_FAILED)
    {
        std::cerr << "sem_open failed (inference)" << std::endl;
        return 1;
    }

    // 1119 hj mqtt
    std::string address = ini_reader->Get("MQTT", "BrokerIP", "tcp://192.168.0.35:1883");
    std::string clientID = ini_reader->Get("MQTT", "ClientID", "CCTV_MAIN");
    std::string userID = ini_reader->Get("MQTT", "UserID", "healthmon");
    std::string password = ini_reader->Get("MQTT", "Password", "healthmon");
    MqttManager::getInstance().connect(address, clientID, userID, password);

    // --- Configuration ---
    std::vector<std::unique_ptr<CameraChannel>> channels;

    for (int i = 0; i < std::stoi(ini_reader->Get("Window Configuration", "total_window_count", "12")); i++)
    {
        channels.push_back(std::make_unique<CameraChannel>());
    }

    // IMPORTANT: Set the correct RTSP URL for each camera "rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/202/"
    for (int i = 1; i <= std::stoi(ini_reader->Get("Window Configuration", "total_window_count", "12")); i++)
    {
        std::stringstream ss;
        ss << "window" << i << "_channel";
        std::string key = ss.str();
        channels[i - 1]->channel_number = std::stoi(ini_reader->Get("CCTV Connection", key, "101"));
        channels[i - 1]->CameraChannelID = i;
        channels[i - 1]->channel_camera_description = ini_reader->Get("Camera Description", "Camera" + std::to_string(i), "Unknown");
        channels[i - 1]->robotDestination = ini_reader->Get("Robot Destination", "Camera" + std::to_string(i), "Unknown");
    }

    log_handler.push(Log::Level::INFO, "프로그램 설정 완료. 프로그램 실행", 0);
    //~ channels[0]->routine_thread = std::thread(routine, channels[0].get(), onnx_path);
    for (auto &channel : channels)
    {
        channel->routine_thread = std::thread(routine, channel.get(), onnx_path);
    }

    std::thread log_thread(log_worker);

    for (auto &channel : channels)
    {
        channel->routine_thread.join();
    }
    log_thread.join();

    std::cout << "[Main] Server thread 종료 완료. semaphore unlink 시도..." << std::endl;
    log_handler.push(Log::Level::INFO, "모든 thread 종료 완료. semaphore unlink 시작", 0);
    sem_close(g_sem_image);
    sem_unlink(get_image_sem_name);
    sem_close(g_sem_inference);
    sem_unlink(infer_sem_name);
    std::cout << "[Main] semaphore unlink 완료. 프로그램 종료" << std::endl;
    log_handler.push(Log::Level::INFO, "semaphore unlink 종료. 프로그램 종료.", 0);
    return 0;
}

void loadConfig()
{
    ini_reader = new INIReader("resource/app.ini");

    if (ini_reader->ParseError() < 0)
    {
        std::cerr << "[Config] 설정 파일 'app.ini' 파싱 실패" << std::endl;
        return;
    }
    return;
}

void log_worker()
{
    bool heartbit = true;
    while (g_running)
    {
        log_handler.save();
        if (heartbit)
        {
            std::cout << ">>>" << std::endl;
        }
        else
        {
            std::cout << ">>" << std::endl;
        }
        heartbit = !heartbit;
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
}

void routine(CameraChannel *channel, std::string net_path)
{
    std::string data_gathering_point = ini_reader->Get("Developer Option", "data_gathering_point", "./data");
    std::stringstream l;
    cv::Mat frame;
    cv::Mat sub_frame;
    std::string id = ini_reader->Get("CCTV Connection", "id", "admin");
    std::string password = ini_reader->Get("CCTV Connection", "password", "q1w2e3r4");
    std::string ip = ini_reader->Get("CCTV Connection", "ip", "192.168.1.100");
    int port = std::stoi(ini_reader->Get("CCTV Connection", "port", "80"));
    int width = std::stoi(ini_reader->Get("Window Configuration", "window_width", "480"));
    int height = std::stoi(ini_reader->Get("Window Configuration", "window_height", "270"));
    std::stringstream ss;
    ss << "Camera" << channel->CameraChannelID;
    int cam_description = std::stoi(ini_reader->Get("Camera Description", ss.str(), "Unknown"));
    // std::vector<Alarm> local_alarms = g_alarms; // 1106 hj modbus 적용
    // std::string now_alarm_condition = ""; // 1106 hj modbus 적용
    // int alarm_timeout = 0; // 1106 hj modbus 적용
    std::stringstream net_result;
    cv::dnn::Net net = cv::dnn::readNet(net_path);
    if (net.empty())
    {
        std::cerr << "Error: Cannot load ONNX model" << std::endl;
        log_handler.push(Log::Level::ERROR, "Cannot load ONNX model", channel->CameraChannelID);
        return;
    }
    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

    // 0911 bottleneck test
    bool test = false;
    if (channel->CameraChannelID == 1)
    {
        test = false;
    }
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff;
    while (g_running)
    {
        // 0919 thread elapsed time 체크
        auto thread_start = std::chrono::high_resolution_clock::now();

        // NVR 접속 후, channel에 맞게 데이터 가져오기
        sem_wait(g_sem_image);

        getFrame_api(id, password, ip, port, channel->channel_number, width, height, frame, sub_frame);

        sem_post(g_sem_image);

        // 추론 process
        // int risk_level = 0; // 1106 hj modbus 적용

        channel->detected_class.clear();
        if (frame.empty() || !g_running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        cv::Mat frame_rgb;
        if (cv::sum(frame) == cv::Scalar(0))
        {
            continue;
        }

        sem_wait(g_sem_inference);
        cv::cvtColor(sub_frame, frame_rgb, cv::COLOR_BGR2RGB);
        auto results = inference(net, frame_rgb);
        sem_post(g_sem_inference);

        // channel->results_queue.push(results);
        for (auto &det : results)
        {
            channel->detected_class.push_back(det.classID);
        }
        channel->detected_class_history.push_back({std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),
                                                   channel->detected_class});

        if (channel->detected_class_history.size() >= 20)
        {
            channel->detected_class_history.pop_front();
        }
        // 1119 hj mqtt 적용
        channel->alarm = g_alarm_manager.process_channel_alarms(channel->CameraChannelID,
                                                                channel->detected_class,
                                                                channel->detected_class_history,
                                                                channel->channel_camera_description,
                                                                channel->robotDestination);

        // 추론 후, 그림 그리기
        cv::Scalar color_anchor;
        // cv::Scalar color_boundary;

        uint8_t isAlarm;
        if (channel->alarm == 0)
        {
            color_anchor = cv::Scalar(0, 255, 0);
            isAlarm = 0;
        }
        else
        {
            color_anchor = cv::Scalar(0, 0, 255);
            isAlarm = 1;
        }

        // alarm 테두리 빨간 색 처리 코드
        cv::Mat frame_save = frame.clone();
        // cv::rectangle(sub_frame, cv::Rect(0,0,width, height), color_boundary, 3);

        for (const auto &det : results)
        {
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

        std::string output_path = "./resource/output";
        std::filesystem::path output_path_fs = "./resource/output";
        std::filesystem::create_directories(output_path_fs);

        std::stringstream ss_save_path;
        ss_save_path << output_path << "/" << std::setfill('0') << std::setw(2) << channel->CameraChannelID << ".txt";
        std::ofstream net_res(ss_save_path.str()); // html 에서 읽어가는 txt 파일

        if (net_res.is_open())
        {
            // 소수점 6자리까지 고정하여 정밀도 확보 (0.123456 형태)
            net_res << std::fixed << std::setprecision(6);

            for (const auto &det : results)
            {
                cv::Rect box = det.box;

                // 1. sub_frame과 frame 간의 스케일 비율 계산
                // (원본 크기 / 추론 크기)
                double scale_x = (double)frame.cols / sub_frame.cols;
                double scale_y = (double)frame.rows / sub_frame.rows;

                // 2. 바운딩 박스 좌표를 원본 frame 스케일로 변환
                double scaled_box_x = box.x * scale_x;
                double scaled_box_y = box.y * scale_y;
                double scaled_box_width = box.width * scale_x;
                double scaled_box_height = box.height * scale_y;

                // 3. YOLO 포맷 계산 (Center X, Center Y, Width, Height) - 0~1 정규화
                double center_x = (scaled_box_x + scaled_box_width / 2.0) / frame.cols;
                double center_y = (scaled_box_y + scaled_box_height / 2.0) / frame.rows;
                double normalized_width = scaled_box_width / frame.cols;
                double normalized_height = scaled_box_height / frame.rows;

                net_res << static_cast<int>(isAlarm) << " "
                        << det.classID << " "
                        << center_x << " "
                        << center_y << " "
                        << normalized_width << " "
                        << normalized_height
                        << std::endl;
            }

            net_res.close();
            // std::cout << "Saved detection results to " << ss_save_path.str() << std::endl;
        }
        else
        {
            std::cerr << "Error: Could not open file for writing." << std::endl;
        }

        std::stringstream ss_output_path;

        ss_output_path << output_path << "/" << std::setfill('0') << std::setw(2) << channel->CameraChannelID << ".jpg";
        cv::imwrite(ss_output_path.str(), sub_frame);

        // 0918 데이터 수집 관련 코드 작성
        end = std::chrono::high_resolution_clock::now();
        diff = end - start;
        if (diff.count() > (duration_between_data))
        {
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
            if (label_ofstream.is_open())
            {
                label_ofstream.write(net_result.str().c_str(), net_result.str().size());
            }

            delete_oldest_file_threshold(std::filesystem::path(folderpath.str()), total_data_per_channel * 2);

            start = std::chrono::high_resolution_clock::now();
        }
        net_result.str("");
        net_result.clear();

        // 0919 thread elapsed time 체크
        auto thread_end = std::chrono::high_resolution_clock::now();

        if (test)
        {
            auto thread_diff = thread_end - thread_start;
            std::cout << "thread elapsed time : " << thread_diff.count() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void signal_handler(int signum)
{
    std::cout << "[Main] 종료 신호 수신... " << std::endl;
    std::cout << "[Main] 이미 실행된 thread의 종료까지 대기..." << std::endl;
    log_handler.push(Log::Level::INFO, "종료 신호 수신...", 0);
    log_handler.push(Log::Level::INFO, "이미 실행된 thread의 종료까지 대기...", 0);
    // fs.server_stop(log_handler);
    g_running = false;
    // std::cerr<< "신호 " << signum << " 수신. semaphore 제거중 ..." << std::endl;
    // sem_unlink(get_image_sem_name);
    // exit(signum);
}