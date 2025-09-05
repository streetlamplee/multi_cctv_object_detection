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
    ThreadSafeStack(int maxSize) : max_size(maxSize) {}
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
     * 만약 스택이 max_size에 도달했다면, 가장 오래된 아이템을 제거하고 새 아이템을 추가합니다.
     * * @param item 스택에 추가할 아이템
     */
    void push(T item) {
        std::lock_guard<std::mutex> lock(mutex_);

        // max_size가 설정(0보다 큼)되어 있고, 스택이 가득 찼는지 확인합니다.
        if (max_size > 0 && data_container_.size() >= max_size) {
            // 가장 오래된 아이템(리스트의 맨 뒤)을 제거합니다.
            data_container_.pop_back();
        }

        // 새로운 아이템을 스택의 맨 위(리스트의 맨 앞)에 추가합니다.
        data_container_.push_front(std::move(item));
        
        // 아이템 추가를 기다리는 스레드(wait_and_pop)가 있다면 깨웁니다.
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
     * @brief 스택의 맨 위에서 아이템을 꺼내려고 시도합니다. (비동기)
     * 
     * @param[out] item 스택에서 꺼낸 아이템이 저장될 변수
     * @return true 아이템을 성공적으로 꺼냈으면
     * @return false 스택이 비어있으면
     */
    bool try_pop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (data_container_.empty()) {
            return false;
        }
        item = std::move(data_container_.front());
        data_container_.pop_front();
        return true;
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
    int max_size = -1;
};

#endif // THREAD_SAFE_STACK_H