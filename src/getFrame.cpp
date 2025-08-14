#include "getFrame.h"
#include <opencv2/opencv.hpp>

cv::VideoCapture connectRTSP(std::string url){

    cv::VideoCapture cap(url);

    if(!cap.isOpened()) {
        std::cerr<< "Cannot Open Video";
        return cv::VideoCapture();
    }

    return cap;
}

cv::Mat getFrame(cv::VideoCapture& cap) {
        
    cv::Mat frame;
    if(!cap.read(frame)){
        std::cerr<< "Cannot Open frame from Video stream";
        return cv::Mat();
    
    }
    cv:resize(frame, frame, cv::Size(800, 450));

    // cv::imshow("", frame);
    // cv::waitKey(0);
    return frame;
}
