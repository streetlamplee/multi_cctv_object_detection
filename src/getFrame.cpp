#include "getFrame.h"
#include <opencv2/opencv.hpp>

int connectRTSP(std::string url, cv::VideoCapture& cap){
    // 특정 API(VAAPI) 대신 사용 가능한 하드웨어 가속을 자동으로 선택하도록 수정

    if (!cap.open(url, cv::CAP_FFMPEG, {cv::CAP_PROP_HW_ACCELERATION, cv::VIDEO_ACCELERATION_ANY})) {
    // if (!cap.open(url)) {
        std::cerr << "Cannot Open Video with HW Acceleration" << std::endl;
        // 만약 위 코드가 실패하면, 다음 라인의 주석을 풀고 다시 시도해 보세요.
        // if (!cap.open(url, cv::CAP_FFMPEG)) {
        //     std::cerr << "Cannot Open Video" << std::endl;
        //     return -1;
        // }
        return -1;
    }
    return 1;
}

cv::Mat getFrame(cv::VideoCapture& cap) {
        
    cv::Mat frame;
    if(!cap.read(frame)){
        std::cerr<< "Cannot Open frame from Video stream";
        return cv::Mat();
    
    }
    cv::resize(frame, frame, cv::Size(450, 800));

    // cv::imshow("", frame);
    // cv::waitKey(0);
    return frame;
}
