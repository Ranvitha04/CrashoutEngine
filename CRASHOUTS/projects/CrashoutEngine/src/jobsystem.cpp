/*!
@file       jobsystem.cpp
@brief      Implementation of the JobSystem.
*/
#include "jobsystem.h"
#include <algorithm>

namespace CrashoutsEngine {

    void JobSystem::Initialize(unsigned int threadCount) {
        if (!workers.empty()) return;

        if (threadCount == 0) {
            threadCount = std::thread::hardware_concurrency();
            if (threadCount == 0) threadCount = 2; // Fallback
        }

        stop = false;
        for (unsigned int i = 0; i < threadCount; ++i) {
            workers.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queueMutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty()) return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    void JobSystem::Shutdown() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop) return;
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            if (worker.joinable()) worker.join();
        }
        workers.clear();
    }

    void JobSystem::ParallelFor(unsigned int count, std::function<void(unsigned int start, unsigned int end)> func) {
        if (count == 0) return;

        unsigned int numThreads = GetThreadCount();
        if (numThreads <= 1 || count < numThreads) {
            func(0, count);
            return;
        }

        unsigned int chunkSize = count / numThreads;
        std::vector<std::future<void>> futures;

        for (unsigned int i = 0; i < numThreads; ++i) {
            unsigned int start = i * chunkSize;
            unsigned int end = (i == numThreads - 1) ? count : start + chunkSize;

            futures.push_back(Enqueue([start, end, &func]() {
                func(start, end);
            }));
        }

        for (auto& f : futures) {
            f.get();
        }
    }

}
