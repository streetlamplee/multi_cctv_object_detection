#pragma once
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include "cpp-httplib/httplib.h"

//brief: from rstp url, return videoCapture class
int connectRTSP(std::string url, cv::VideoCapture& cap);


//brief: from vidoeCapture, return an image
int getFrame_api(std::string user, std::string password, std::string ip, int port, int channel, int width, int height, cv::Mat& frame);