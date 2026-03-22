#pragma once

#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template <typename T>
class SharedQueue
{
public:
    explicit SharedQueue(size_t max_size = 1) : _max_size(max_size)
    {
    }

    void push(const T &item)
    {
        std::unique_lock<std::mutex> lock(_mtx);

        if (_max_size > 0) {
            _cv_not_full.wait(lock, [this]() {
                return _queue.size() < _max_size;
            });
        }

        _queue.push(item);
        _cv_not_empty.notify_one();
    }

    void push(T &&item)
    {
        std::unique_lock<std::mutex> lock(_mtx);

        if (_max_size > 0) {
            _cv_not_full.wait(lock, [this]() {
                return _queue.size() < _max_size;
            });
        }

        _queue.push(std::move(item));
        _cv_not_empty.notify_one();
    }

    void push_latest(const T &item)
    {
        std::lock_guard<std::mutex> lock(_mtx);

        if (!_queue.empty()) {
            _queue.pop();
        }

        _queue.push(item);
        _cv_not_empty.notify_one();
    }

    bool try_push(const T &item)
    {
        std::lock_guard<std::mutex> lock(_mtx);

        if (_max_size > 0 && _queue.size() >= _max_size)
            return false;

        _queue.push(item);
        _cv_not_empty.notify_one();
        return true;
    }

    T pop()
    {
        std::unique_lock<std::mutex> lock(_mtx);

        _cv_not_empty.wait(lock, [this]() {
            return !_queue.empty();
        });

        T item = std::move(_queue.front());
        _queue.pop();

        if (_max_size > 0) {
            _cv_not_full.notify_one();
        }

        return item;
    }

    T pop_non_blocking()
    {
        std::lock_guard<std::mutex> lock(_mtx);

        // 队列为空，直接返回失败
        if (_queue.empty()) {
            // 返回一个默认构造的T（避免未定义行为）
            return T{};
        }

        // 队列非空，弹出元素
        T item = std::move(_queue.front());
        _queue.pop();

        // 通知队列非满（和其他pop方法保持一致）
        if (_max_size > 0) {
            _cv_not_full.notify_one();
        }

        return item;
    }

    std::optional<T> try_pop()
    {
        std::lock_guard<std::mutex> lock(_mtx);

        if (_queue.empty())
            return std::nullopt;

        T item = std::move(_queue.front());
        _queue.pop();

        if (_max_size > 0) {
            _cv_not_full.notify_one();
        }

        return item;
    }

    std::optional<T> pop_for(std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(_mtx);

        if (!_cv_not_empty.wait_for(lock, timeout, [this]() {
                return !_queue.empty();
            })) {
            return std::nullopt;
        }

        T item = std::move(_queue.front());
        _queue.pop();

        if (_max_size > 0) {
            _cv_not_full.notify_one();
        }

        return item;
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(_mtx);
        return _queue.size();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(_mtx);
        return _queue.empty();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(_mtx);
        std::queue<T> empty;
        std::swap(_queue, empty);
    }

private:
    std::queue<T> _queue;
    mutable std::mutex _mtx;

    std::condition_variable _cv_not_empty;
    std::condition_variable _cv_not_full;

    size_t _max_size;
};