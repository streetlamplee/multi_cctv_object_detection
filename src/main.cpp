#include "thread_safe_stack.h"
#include "cctv.h"
#include <thread>
#include <iostream>

// global variable define
CCTV* g_cctv1 = nullptr;
// CCTV* g_cctv2;
// CCTV* g_cctv3;
// CCTV* g_cctv4;

ThreadSafeStack<cv::Mat>* g_cctv1ImageStack = nullptr;
// ThreadSafeStack<cv::Mat>* g_cctv2ImageStack = g_cctv2->get_image_stack();
// ThreadSafeStack<cv::Mat>* g_cctv3ImageStack = g_cctv3->get_image_stack();
// ThreadSafeStack<cv::Mat>* g_cctv4ImageStack = g_cctv4->get_image_stack();

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

int customer(){
    cv::Rect roi1(0,    0,      800,   450);
    cv::Rect roi2(800,  0,      800,   450);
    cv::Rect roi3(0,    450,    800,   450);
    cv::Rect roi4(800,  450,    800,   450);    

    while(true){
        cv::Mat img1 = g_cctv1ImageStack->wait_and_pop();
        // cv::Mat img2 = cctv2imageStack_ptr->wait_and_pop();
        // cv::Mat img3 = cctv3imageStack_ptr->wait_and_pop();
        // cv::Mat img4 = cctv4imageStack_ptr->wait_and_pop();

        if (!img1.empty()){
            cv::resize(img1, img1, cv::Size(800, 450));
            // cv::resize(img2, img2, cv::Size(540, 960));
            // cv::resize(img3, img3, cv::Size(540, 960));
            // cv::resize(img4, img4, cv::Size(540, 960));

            img1.copyTo(g_canvas(roi1));

        }
        cv::imshow("NVR Stream", g_canvas);
        if (cv::waitKey(1) & 0xFF == 'q'){
            break;
        }
    }
    return 1;
}


int main(int argc, char *argv[]){
    g_cctv1 = new CCTV("rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/201/");
    // g_cctv2 = new CCTV("rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/201/");
    // g_cctv3 = new CCTV("rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/201/");
    // g_cctv4 = new CCTV("rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/201/");

    g_cctv1ImageStack = g_cctv1->get_image_stack();
    // g_cctv2ImageStack = g_cctv2->get_image_stack();
    // g_cctv3ImageStack = g_cctv3->get_image_stack();
    // g_cctv4ImageStack = g_cctv4->get_image_stack();
    
    std::thread producer_thread(producer);
    std::thread customer_thread(customer);

    producer_thread.join();
    customer_thread.join();

    return 1;
}