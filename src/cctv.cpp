#include "cctv.h"

CCTV::CCTV(std::string rtspURL, ThreadSafeStack<cv::Mat>* stack) : rtspURL(rtspURL), image_stack(stack) {

}

int CCTV::start_image_capture() {

    connectRTSP(this->rtspURL, this->vicap);
    
    // if (!this->vicap.set(cv::CAP_PROP_FRAME_WIDTH, 800)) {
    //     std::cerr << "Cannot Set Width" << std::endl;
    // }  // 원하는 너비로 설정
    // if (!this->vicap.set(cv::CAP_PROP_FRAME_HEIGHT, 450)) {
    //     std::cerr << "Cannot Set Height" << std::endl;
    // }


    if (!vicap.isOpened()){
        std::cerr << "Failed to open video stream" << std::endl;
        return -1;
    }

    while(true){
        cv::Mat frame;
        cv::Mat frame_resized;
        if (this->vicap.read(frame)){
            // cv::resize(frame, frame_resized, cv::Size(800, 450));
            this->image_stack->push(frame);
            // std::cout << "image push done" << std::endl;    
        } else {
            std::cerr<< "Cannot read frame, try to reconnect..." << std::endl;
            this->vicap.release();
            connectRTSP(this->rtspURL, this->vicap);
            if (!this->vicap.isOpened()){
                std::cerr << "Reconnection Failed" << std::endl;
                break;
            }
        }
    }
    return 1;
}