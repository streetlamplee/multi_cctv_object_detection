#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>

int main() {
    std::cout << "DISPLAY=" << (std::getenv("DISPLAY") ? std::getenv("DISPLAY") : "(null)") << "\n";
    cv::Mat img(300, 500, CV_8UC3, cv::Scalar(30, 30, 30));

    cv::namedWindow("sanity", cv::WINDOW_AUTOSIZE);  // 창 이름은 공백 말고 명시!
    cv::imshow("sanity", img);
    int k = cv::waitKey(3000);                       // 3초 보여주고 종료
    std::cout << "waitKey=" << k << "\n";
}