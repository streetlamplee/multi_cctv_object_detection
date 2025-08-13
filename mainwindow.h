#include <QMainWindow>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <opencv2/dnn.hpp>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_getImagebtn_clicked();
    void on_Inferencebtn_clicked();
    void updateFrame();

private:
    bool isInference;
    cv::dnn::Net yoloNet;
    bool yoloLoaded = false;

private:
    Ui::MainWindow *ui;

    QTimer *timer;
    std::deque<cv::Mat> frames;
    std::mutex frameLock;
    std::thread streamThread;
    std::atomic<bool> isStreaming;
};
