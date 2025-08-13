#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <string>
#include <format>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), isStreaming(false), isInference(false) {

    ui->setupUi(this);

    try {
        yoloNet = cv::dnn::readNetFromONNX("yolov8s.onnx");
        yoloNet.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        yoloNet.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        yoloLoaded = true;
        qDebug() << yoloLoaded;
    } catch (const cv::Exception &e) {
        qWarning() << "YOLO ONNX Load Fail" << e.what();
    }

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateFrame);
}

MainWindow::~MainWindow() {
    isStreaming = false;
    isInference = false;
    if (streamThread.joinable())
        streamThread.join();
    delete ui;
}

void MainWindow::on_getImagebtn_clicked() {
    if (isStreaming) return;
    isStreaming = true;

    streamThread = std::thread([this]() {
        std::string url = "rtsp://admin:q1w2e3r4@192.168.1.100:554/Streaming/Channels/201/";
        cv::VideoCapture cap(url);

        if (!cap.isOpened()) {
            qWarning("RTSP 열기 실패");
            return;
        }

        while (isStreaming) {
            cv::Mat frame;
            if (!cap.read(frame)) {
                qWarning("프레임 수신 실패");
                break;
            }

            cv::resize(frame, frame, cv::Size(1280, 720));

            std::lock_guard<std::mutex> guard(frameLock);
            if (frames.size() >= 10)
                frames.pop_front();
            frames.push_back(frame);
        }

        cap.release();
    });

    timer->start(33);
}

void MainWindow::updateFrame()
{
    cv::Mat frame;

    {
        std::lock_guard<std::mutex> guard(frameLock);
        if (!frames.empty())
            frame = frames.back().clone();
    }

    if (!frame.empty()) {
        if (isInference && yoloLoaded) {
            // 1. 전처리
            cv::Mat blob = cv::dnn::blobFromImage(frame, 1/255.0, cv::Size(640, 640), cv::Scalar(), true, false);
            yoloNet.setInput(blob);

            // 2. 추론
            std::vector<cv::Mat> outputs;
            yoloNet.forward(outputs, yoloNet.getUnconnectedOutLayersNames());

            // 3. 후처리
            float confThreshold = 0.5;
            int classID_person = 0; // COCO class index 0 == person

            for (const auto& output : outputs) {
                const float* data = reinterpret_cast<const float*>(output.data);
                for (int i = 0; i < output.rows; ++i) {
                    float obj_conf = output.at<float>(i, 4);
                    if (obj_conf < confThreshold) continue;

                    // class confidence 계산: obj_conf * class_score
                    cv::Mat scores = output.row(i).colRange(5, output.cols);
                    cv::Point classIdPoint;
                    double max_class_score;
                    minMaxLoc(scores, 0, &max_class_score, 0, &classIdPoint);

                    float confidence = obj_conf * max_class_score;

                    if (classIdPoint.x == 0 && confidence > confThreshold) {
                        // 사람(class_id 0) + threshold 넘으면 그려주기
                        float cx = output.at<float>(i, 0) * frame.cols;
                        float cy = output.at<float>(i, 1) * frame.rows;
                        float w  = output.at<float>(i, 2) * frame.cols;
                        float h  = output.at<float>(i, 3) * frame.rows;

                        int x = static_cast<int>(cx - w / 2);
                        int y = static_cast<int>(cy - h / 2);

                        cv::rectangle(frame, cv::Rect(x, y, w, h), cv::Scalar(0, 255, 0), 2);
                        cv::putText(frame, "Person", cv::Point(x, y - 10),
                                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);
                    }
                }

            }
        }
        cv::imshow("debug", frame);
        // Qt로 이미지 전송
        QImage img(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_BGR888);
        ui->FrameLabel->setPixmap(QPixmap::fromImage(img));
    }
}

void MainWindow::on_Inferencebtn_clicked() {
    isInference = !isInference;
    qDebug() << isInference;
}