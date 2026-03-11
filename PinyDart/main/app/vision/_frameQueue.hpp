#pragma once

#include <condition_variable>
#include <maix_image.hpp>
#include <memory>
#include <mutex>
#include <queue>

class FrameQueue
{
public:
    FrameQueue(size_t max_size = 5)
        : max_size(max_size)
    {
    }

    void push(std::shared_ptr<maix::image::Image> img)
    {
        std::lock_guard<std::mutex> lock(mtx);

        if(queue.size() >= max_size)
            queue.pop();

        queue.push(img);

        cv.notify_one();
    }

    std::shared_ptr<maix::image::Image> pop()
    {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock,[this]{
            return !queue.empty();
        });

        auto img = queue.front();
        queue.pop();

        return img;
    }

    std::shared_ptr<maix::image::Image> pop_latest()
    {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock,[this]{
            return !queue.empty();
        });

        while(queue.size() > 1)
            queue.pop();

        auto img = queue.front();
        queue.pop();

        return img;
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }

private:

    std::queue<std::shared_ptr<maix::image::Image>> queue;

    std::mutex mtx;
    std::condition_variable cv;

    size_t max_size;
};