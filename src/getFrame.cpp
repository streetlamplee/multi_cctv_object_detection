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

int getFrame_api(std::string user, std::string password, std::string ip, int port, int channel, int width, int height, cv::Mat& frame) {
    httplib::Client cli(ip, port);
    cli.set_digest_auth(user, password);

    auto res = cli.Get("/ISAPI/ContentMgmt/StreamingProxy/channels/"+std::to_string(channel)+"/picture?videoResolutionWidth=704&videoResolutionHeight=480");
    if (res && res->status == 200) {
        
        std::vector<char> imageData (res->body.begin(), res->body.end());
        frame = cv::imdecode(imageData, cv::IMREAD_COLOR);
        cv::resize(frame, frame, cv::Size(width, height));
        if (frame.empty()) {
            frame = cv::Mat::zeros(height, width, CV_8SC3);
        }
        cli.stop();
        return 1;
         
    } else {
        auto err = res.error();
        // std::cerr << "Error: " << err << std::endl;
        frame = cv::Mat::zeros(height, width, CV_8SC3);
        cli.stop();
        return 0;
    }

}
