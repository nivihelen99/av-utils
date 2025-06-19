#ifndef ASYNC_EVENT_QUEUE_H
#define ASYNC_EVENT_QUEUE_H

#include <deque>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <functional>
#include <cstddef> // For size_t

template <typename T>
class AsyncEventQueue {
public:
    explicit AsyncEventQueue(size_t maxsize = 0);

    void put(const T& item);
    void put(T&& item);

    T get();
    std::optional<T> try_get();

    size_t size() const;
    bool empty() const;
    bool full() const;

    void register_callback(std::function<void()> cb);

private:
    std::deque<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_can_put_; // Renamed: Notified when space becomes available in a full queue
    std::condition_variable cv_can_get_; // Renamed: Notified when an item is added to an empty queue
    size_t maxsize_;
    std::function<void()> callback_;
    bool callback_registered_ = false;
};

// Implementation to be modified:

template <typename T>
AsyncEventQueue<T>::AsyncEventQueue(size_t maxsize) : maxsize_(maxsize) {}

template <typename T>
void AsyncEventQueue<T>::put(const T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (maxsize_ > 0) {
        // Wait until there's space in the queue
        cv_can_put_.wait(lock, [this] { return queue_.size() < maxsize_; });
    }

    bool was_empty_on_arrival = queue_.empty(); // Capture state just before push
    queue_.push_back(item);

    // It's generally recommended to unlock before notifying to prevent the notified thread from immediately blocking again on the same mutex.
    lock.unlock();

    cv_can_get_.notify_one(); // Notify a getter

    if (callback_registered_ && was_empty_on_arrival) {
        callback_();
    }
}

template <typename T>
void AsyncEventQueue<T>::put(T&& item) {
    // Similar corrected sequence for move semantics
    std::unique_lock<std::mutex> lock(mutex_);
    if (maxsize_ > 0) {
        cv_can_put_.wait(lock, [this] { return queue_.size() < maxsize_; });
    }

    bool was_empty_on_arrival = queue_.empty();
    queue_.push_back(std::move(item));

    lock.unlock();

    cv_can_get_.notify_one();

    if (callback_registered_ && was_empty_on_arrival) {
        callback_();
    }
}

template <typename T>
T AsyncEventQueue<T>::get() {
    std::unique_lock<std::mutex> lock(mutex_);
    // Wait until the queue is not empty
    cv_can_get_.wait(lock, [this] { return !queue_.empty(); });

    T item = std::move(queue_.front());
    queue_.pop_front();

    // Similar to put, unlock before notifying
    lock.unlock();

    cv_can_put_.notify_one(); // Notify a putter that there's space

    return item;
}

template <typename T>
std::optional<T> AsyncEventQueue<T>::try_get() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return std::nullopt;
    }

    T item = std::move(queue_.front());
    queue_.pop_front();

    lock.unlock();

    cv_can_put_.notify_one(); // Notify a putter that there's space

    return item;
}

template <typename T>
size_t AsyncEventQueue<T>::size() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.size();
}

template <typename T>
bool AsyncEventQueue<T>::empty() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.empty();
}

template <typename T>
bool AsyncEventQueue<T>::full() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return maxsize_ > 0 && queue_.size() >= maxsize_;
}

template <typename T>
void AsyncEventQueue<T>::register_callback(std::function<void()> cb) {
    std::unique_lock<std::mutex> lock(mutex_);
    callback_ = cb;
    callback_registered_ = (cb != nullptr); // Also ensure cb is not null for registration to be true
}

#endif // ASYNC_EVENT_QUEUE_H
