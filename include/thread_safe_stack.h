#ifndef THREAD_SAFE_STACK_H
#define THREAD_SAFE_STACK_H

#include <list>
#include <mutex>
#include <condition_variable>
#include <utility>

/**
 * @brief 여러 스레드에서 안전하게 사용할 수 있는 템플릿 기반 스택 (LIFO) 클래스입니다.
 * 
 * @tparam T 스택에 저장할 데이터의 타입
 */
template<typename T>
class ThreadSafeStack {
public:
    ThreadSafeStack() = default;
    ~ThreadSafeStack() = default;

    // 복사 및 이동 생성자/대입 연산자 삭제
    ThreadSafeStack(const ThreadSafeStack&) = delete;
    ThreadSafeStack& operator=(const ThreadSafeStack&) = delete;

    ThreadSafeStack(ThreadSafeStack&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex_);
        data_container_ = std::move(other.data_container_);
    }

    /**
     * @brief 스택의 맨 위에 아이템을 추가합니다. (LIFO)
     * 
     * @param item 스택에 추가할 아이템
     */
    void push(T item) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_container_.push_front(std::move(item));
        cond_.notify_one();
    }

    /**
     * @brief 스택의 맨 위에서 아이템을 꺼냅니다. 스택이 비어있으면 아이템이 추가될 때까지 대기합니다.
     * 
     * @return T 스택에서 꺼낸 아이템
     */
    T wait_and_pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]{ return !data_container_.empty(); });
        
        T item = std::move(data_container_.front());
        data_container_.pop_front();
        return item;
    }

    /**
     * @brief 스택이 비어있는지 확인합니다.
     * 
     * @return true 스택이 비어있으면
     * @return false 스택이 비어있지 않으면
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_container_.empty();
    }

    /**
     * @brief 스택에 있는 아이템의 개수를 반환합니다.
     * 
     * @return size_t 스택의 크기
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_container_.size();
    }

private:
    mutable std::mutex mutex_; 
    std::list<T> data_container_;
    std::condition_variable cond_;
};

#endif // THREAD_SAFE_STACK_H
