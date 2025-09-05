#include "thread_safe_stack.h"
#include "thread_safe_queue.h"
#include "cctv.h"
#include <thread>
#include <iostream>
#include "inference.h"
#include <chrono>

// global variable define
CCTV* g_cctv1 = nullptr;
// CCTV* g_cctv2;
// CCTV* g_cctv3;
// CCTV* g_cctv4;

ThreadSafeStack<cv::Mat> g_cctv1ImageStack;
// ThreadSafeStack<cv::Mat>* g_cctv2ImageStack = g_cctv2->get_image_stack();
// ThreadSafeStack<cv::Mat>* g_cctv3ImageStack = g_cctv3->get_image_stack();
// ThreadSafeStack<cv::Mat>* g_cctv4ImageStack = g_cctv4->get_image_stack();

cv::Mat cctv1image;

std::vector<BBoxInfo> g_detect1;

cv::Mat g_canvas(900, 1600, CV_8UC3, cv::Scalar(0,0,0));


int producer(){
    std::thread cctv1thread(&CCTV::start_image_capture, g_cctv1);
    // std::thread cctv2thread(g_cctv2->get_image_stack());
    // std::thread cctv3thread(g_cctv3->get_image_stack());
    // std::thread cctv4thread(g_cctv4->get_image_stack());

    cctv1thread.detach();
    // cctv2thread.detach();
    // cctv3thread.detach();
    // cctv4thread.detach();

    return 1;
}

int consumer(){

    cv::Rect roi1(0,    0,      800,   450);
    cv::Rect roi2(800,  0,      800,   450);
    cv::Rect roi3(0,    450,    800,   450);
    cv::Rect roi4(800,  450,    800,   450);    

    while(true){
        
        cv::Mat image1 = g_cctv1ImageStack.wait_and_pop();
        cctv1image = image1.clone();
        // cv::Mat img2 = cctv2imageStack_ptr->wait_and_pop();
        // cv::Mat img3 = cctv3imageStack_ptr->wait_and_pop();
        // cv::Mat img4 = cctv4imageStack_ptr->wait_and_pop();
        
        if (!image1.empty()){
            cv::resize(image1, image1, cv::Size(800, 450));
            image1.copyTo(g_canvas(roi1));
        }
        // if (!image2->empty()){
        //     cv::resize(*image2, *image2, cv::Size(800, 450));
        //     image2->copyTo(g_canvas(roi2));
        // }
        // if (!image3->empty()){
        //     cv::resize(*image3, *image3, cv::Size(800, 450));
        //     image3->copyTo(g_canvas(roi1));
        // }
        // if (!image4->empty()){
        //     cv::resize(*image4, *image4, cv::Size(800, 450));
        //     image4->copyTo(g_canvas(roi1));
        // }
        cv::imshow("NVR Stream", g_canvas);
        if (cv::waitKey(1) & 0xFF == 'q'){
            break;
        }        
    }
    return 1;
}

int inference_worker() {
    std::string onnx_path = "../resource/yolov8n.onnx";
    cv::dnn::Net net = cv::dnn::readNet(onnx_path);
    int font = cv::FONT_HERSHEY_SIMPLEX;
    double font_scale = 0.5;
    int thickness = 5;
    cv::Scalar color(0,0,255);

    if (net.empty()) {
        std::cerr << "Error: Cannot load ONNX model" << std::endl;
        return -1;
    }

    while (true) {
        g_canvas = cv::Mat(900, 1600, CV_8UC3, cv::Scalar(0,0,0));
        cv::Mat frame = cctv1image;
        if (frame.empty()) {
            continue;
        }
        g_detect1 = inference(net, frame);
        std::cout << "detected object : " << g_detect1.size() << std::endl;

        // visualization
        for (const auto& det1 : g_detect1) {
            cv::Rect box = det1.box;
            box.x += 0;
            box.y += 0;

            cv::rectangle(g_canvas, box, color, thickness);

            std::string label = det1.className + ": " + cv::format("%.2f", det1.confidence);

            cv::Size text_size = cv::getTextSize(label, font, font_scale, thickness, 0);
            int text_x = box.x;
            int text_y = box.y;

            if (text_y < 0) {
                text_y = box.y + text_size.height + 5;
            }

            cv::putText(g_canvas, label, cv::Point(text_x, text_y), font, font_scale, cv::Scalar(0,0,255), thickness);
        }

    }
}

int main(int argc, char *argv[]){
    g_cctv1 = new CCTV("rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/201/", &g_cctv1ImageStack);
    // g_cctv2 = new CCTV("rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/201/");
    // g_cctv3 = new CCTV("rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/201/");
    // g_cctv4 = new CCTV("rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/201/");
    
    std::thread producer_thread(producer);
    std::thread customer_thread(consumer);
    std::thread inference_thread(inference_worker);

    producer_thread.detach();
    customer_thread.detach();
    inference_thread.detach();

    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 1;
}