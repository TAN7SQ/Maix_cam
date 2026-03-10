#pragma once

#include <condition_variable>
#include <maix_image.hpp>
#include <memory>
#include <mutex>
#include <queue>

class FrameQueue
{
public:
    FrameQueue(size_t max_size = 5) : max_size(max_size)
    {
    }

    void push(std::shared_ptr<maix::image::Image> img)
    {
        std::unique_lock<std::mutex> lock(mtx);

        if (queue.size() >= max_size) {
            queue.pop(); // 丢掉最旧帧
        }

        queue.push(img);
        cv.notify_one();
    }

    std::shared_ptr<maix::image::Image> pop()
    {
        std::unique_lock<std::mutex> lock(mtx);

        while (queue.empty()) {
            cv.wait(lock);
        }

        auto img = queue.front();
        queue.pop();

        return img;
    }

private:
    std::queue<std::shared_ptr<maix::image::Image>> queue;
    std::mutex mtx;
    std::condition_variable cv;
    size_t max_size;
};