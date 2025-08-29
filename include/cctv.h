#include <string>
#include <iostream>
#include <opencv2/opencv.hpp>
#include "thread_safe_stack.h"
#include "getFrame.h"

class CCTV {
    private:
        std::string rtspURL;
        ThreadSafeStack<cv::Mat>* image_stack;
        cv::VideoCapture vicap;

    public:
        CCTV(std::string rtspURL, ThreadSafeStack<cv::Mat>* stack);
        
        int start_image_capture();


};