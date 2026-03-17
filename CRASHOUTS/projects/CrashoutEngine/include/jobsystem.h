/*!
@file       jobsystem.h
@author     Shahrin Idzuan
@brief      A simple thread-pool based job system for parallelizing engine tasks.
*/
#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <future>
#include <type_traits>

namespace CrashoutsEngine {

    class JobSystem {
    public:
        static JobSystem& Get() {
            static JobSystem instance;
            return instance;
        }

        // Initialize the thread pool
        void Initialize(unsigned int threadCount = 0);
        
        // Shutdown the thread pool
        void Shutdown();

        // Enqueue a job
        template<class F, class... Args>
        auto Enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
            using return_type = std::invoke_result_t<F, Args...>;

            auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );
            
            std::future<return_type> res = task->get_future();
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                if (stop) throw std::runtime_error("Enqueue on stopped JobSystem");
                tasks.emplace([task]() { (*task)(); });
            }
            condition.notify_one();
            return res;
        }

        // Parallel for: splits a range [0, count) into chunks and executes in parallel
        void ParallelFor(unsigned int count, std::function<void(unsigned int start, unsigned int end)> func);
        // Parallel for with minimum chunk size to reduce overhead on small workloads
        void ParallelFor(unsigned int count, unsigned int minChunkSize,
            std::function<void(unsigned int start, unsigned int end)> func);

        unsigned int GetThreadCount() const { return static_cast<unsigned int>(workers.size()); }

    private:
        JobSystem() : stop(false) {}
        ~JobSystem() { Shutdown(); }

        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        
        std::mutex queueMutex;
        std::condition_variable condition;
        std::atomic<bool> stop;
    };

}
