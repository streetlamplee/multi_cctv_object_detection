#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <vector>
#include <string>


struct BBoxInfo {
    cv::Rect box;
    std::string className;
    int classID;
    float confidence;
};

std::vector<BBoxInfo> inference(cv::dnn::Net net, cv::Mat image);
