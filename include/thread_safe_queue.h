#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <utility> // std::move를 위해 필요

/**
 * @brief 여러 스레드에서 안전하게 사용할 수 있는 템플릿 기반 큐 클래스입니다.
 * 
 * @tparam T 큐에 저장할 데이터의 타입
 */
template<typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    // 복사 및 이동 생성자/대입 연산자 삭제
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    ThreadSafeQueue(ThreadSafeQueue&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex_);
        queue_ = std::move(other.queue_);
    }

    /**
     * @brief 큐의 맨 뒤에 아이템을 추가합니다.
     * 
     * @param item 큐에 추가할 아이템
     */
    void push(T item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(item));
        cond_.notify_one();
    }

    /**
     * @brief 큐의 맨 앞에서 아이템을 꺼냅니다. 큐가 비어있으면 아이템이 추가될 때까지 대기합니다.
     * 
     * @return T 큐에서 꺼낸 아이템
     */
    T wait_and_pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]{ return !queue_.empty(); });
        
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    /**
     * @brief 큐가 비어있는지 확인합니다.
     * 
     * @return true 큐가 비어있으면
     * @return false 큐가 비어있지 않으면
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief 큐에 있는 아이템의 개수를 반환합니다.
     * 
     * @return size_t 큐의 크기
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    mutable std::mutex mutex_; 
    std::queue<T> queue_;
    std::condition_variable cond_;
};

#endif // THREAD_SAFE_QUEUE_H
