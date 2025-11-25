#include "thread_pool.h"

ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
    start(numThreads);
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::enqueue(std::function<void()> job) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        jobs.push(job);
    }
    condition.notify_one();
}

void ThreadPool::start(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this]() {
            while (true) {
                std::function<void()> job;

                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    condition.wait(lock, [this]() { return stop || !jobs.empty(); });

                    if (stop && jobs.empty())
                        return;

                    job = std::move(jobs.front());
                    jobs.pop();
                }

                job();  // 작업 실행
            }
        });
    }
}

void ThreadPool::shutdown() {
    stop = true;
    condition.notify_all();

    for (std::thread &worker : workers) {
        if (worker.joinable())
            worker.join();
    }
}
