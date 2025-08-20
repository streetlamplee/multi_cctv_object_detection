#include "cctv.h"

CCTV::CCTV(std::string rtspURL, ThreadSafeStack<cv::Mat>* stack) : rtspURL(rtspURL), image_stack(stack) {}

int CCTV::start_image_capture() {
    cv::VideoCapture vicap = connectRTSP(this->rtspURL);

    if (!vicap.isOpened()){
        std::cerr << "Failed to open video stream" << std::endl;
        return -1;
    }

    while(true){
        cv::Mat frame;
        if (vicap.read(frame)){
            this->image_stack->push(frame);
            // std::cout << "image push done" << std::endl;    
        } else {
            std::cerr<< "Cannot read frame, try to reconnect..." << std::endl;
            vicap.release();
            vicap = connectRTSP(this->rtspURL);
            if (!vicap.isOpened()){
                std::cerr << "Reconnection Failed" << std::endl;
                break;
            }
        }
    }
    return 1;
}