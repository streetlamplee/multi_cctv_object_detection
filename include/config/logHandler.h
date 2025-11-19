#pragma once
#include <string>
#include <fstream>      // for std::ofstream
#include <sstream>      // for std::stringstream
#include <iostream>     // for std::cout, std::cerr
#include <deque>        // for std::deque
#include <mutex>        // for std::mutex
#include <semaphore.h>
#include <iomanip>      // for std::put_time

class Log{
    private:
    std::string filename;
    std::deque<std::string> lastNlines;
    int maxLogSize = 100;
    std::mutex Log_lock;

    
    public:
    enum class Level { INFO, WARNING, ALARM, ERROR, SIZE };
    Log() = delete;
    Log(const std::string& fname, int maxLSize) : filename(fname), maxLogSize(maxLSize)
    {

    }
    ~Log() {
        try {
            if (!this->lastNlines.empty()) {
                this->save();
            }
        } catch (...) {
            // Destructors should not throw. Suppress all exceptions.
        }
    }

    
    void push(Level level, std::string message, int thread_num = -1);
    std::string getFilename();
    void save();
    void load();
};

inline void Log::push(Level level, std::string message, int thread_num) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << " | ";
    if (thread_num != -1) {
        ss << "Thread " << std::setw(2) << thread_num << " | ";
    }

    std::string prefix = ss.str();
    switch (level){
        case Level::INFO:       prefix += " INFO   ]";     break;
        case Level::WARNING:    prefix += " WARNING]";  break;
        case Level::ERROR:      prefix += " ERROR  ]";    break;
        case Level::ALARM:      prefix += " ALARM  ]";    break;
        case Level::SIZE:       prefix += "";           break;
    }
    prefix.resize(50, ' ');
    prefix += " | ";
    std::lock_guard<std::mutex> lock(Log_lock);
    this->lastNlines.push_back(prefix + message);
    if (this->lastNlines.size() > maxLogSize) {
        this->lastNlines.pop_front();
    }
}

inline std::string Log::getFilename() {
    return this->filename;
}

inline void Log::load() {
    if (this->filename.empty()) {
        std::cerr << "ERR: No Filename" << std::endl;
    }

    std::ifstream inputfile(this->filename);
    std::string line;
    while (std::getline(inputfile, line)){
        this->lastNlines.push_back(line);
        if (this->lastNlines.size() > maxLogSize) {
            this->lastNlines.pop_front();
        }
    }
}

inline void Log::save() {
    if (this->filename.empty()) {
        std::cerr << "ERR: No Filename" << std::endl;
    }
    std::ofstream outFile(this->filename);

    if (!outFile.is_open()) {
        std::cerr << "ERR: Cannot open output file stream" << std::endl;
    }

    std::lock_guard<std::mutex> lock(Log_lock);

    for (auto l : this->lastNlines) {
        outFile << l << std::endl;
    }
    outFile.close();
}