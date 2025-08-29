#pragma once
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

//brief: from rstp url, return videoCapture class
int connectRTSP(std::string url, cv::VideoCapture& cap);


//brief: from vidoeCapture, return an image
cv::Mat getFrame(cv::VideoCapture cap);