#ifndef MUTILES_HPP
#define MUTILES_HPP

#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <iostream>

#include <condition_variable>
#include <optional>

namespace mUtiles
{
    void sleep_ms(int ms);

    static int set_thread_priority(std::thread &thread, int policy, int priority);

    template <typename QueueType>
    class mQueue
    {
    public:
        mQueue() = default;
        ~mQueue() = default;
        void push(QueueType &&data)
        {
            {

                std::lock_guard<std::mutex> lock(m_mutex);
                m_queue.push(std::move(data)); // 使用移动语义，减少拷贝
            }
            m_cond.notify_one(); // 提前释放锁，并通知等待线程
        }

        QueueType pop()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond.wait(lock, [this]
                        { return !m_queue.empty(); }); // 等待直到队列非空

            QueueType val = std::move(m_queue.front());
            m_queue.pop();
            return val;
        }

        /**
         * @brief 非阻塞式弹出队列前端元素
         * @param out 弹出的元素将被存储在该参数中
         * @return true 如果成功弹出元素
         * @return false 如果队列为空
         */
        bool try_pop(QueueType &out)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty())
                return false;
            out = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }

        std::optional<QueueType> front() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty())
                return std::nullopt;
            return m_queue.front();
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.empty();
        }

        size_t size() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.size();
        }

    private:
        std::queue<QueueType> m_queue;
        std::mutex m_mutex;
        std::condition_variable m_cond;
    };

} // namespace mUtiles
#endif /* MUTILES_HPP */