/*!
@file       jobsystem.cpp
@author     Shahrin Idzuan
@brief      Implementation of the JobSystem.
*/
#include "jobsystem.h"
#include <algorithm>

namespace CrashoutsEngine {
	namespace {
		unsigned int GetDefaultWorkerCount() {
			unsigned int concurrency = std::thread::hardware_concurrency();
			if (concurrency == 0) return 2;
			return concurrency > 1 ? (concurrency - 1) : 1;
		}
	}

	void JobSystem::Initialize(unsigned int threadCount) {
		if (!workers.empty()) return;

		if (threadCount == 0) {
			threadCount = GetDefaultWorkerCount();
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
		ParallelFor(count, 1u, std::move(func));
	}

	void JobSystem::ParallelFor(unsigned int count, unsigned int minChunkSize,
		std::function<void(unsigned int start, unsigned int end)> func) {
		if (count == 0) return;
		if (minChunkSize == 0) minChunkSize = 1;

		unsigned int numWorkers = GetThreadCount();
		if (numWorkers == 0 || count <= minChunkSize) {
			func(0, count);
			return;
		}

		unsigned int idealChunks = (count + minChunkSize - 1) / minChunkSize;
		unsigned int numChunks = std::min(numWorkers + 1, idealChunks);
		if (numChunks <= 1) {
			func(0, count);
			return;
		}

		unsigned int chunkSize = (count + numChunks - 1) / numChunks;
		std::vector<std::future<void>> futures;
		futures.reserve(numChunks - 1);

		for (unsigned int i = 1; i < numChunks; ++i) {
			unsigned int start = i * chunkSize;
			unsigned int end = std::min(count, start + chunkSize);
			if (start >= end) continue;

			futures.push_back(Enqueue([start, end, &func]() {
				func(start, end);
			}));
		}

		unsigned int mainStart = 0;
		unsigned int mainEnd = std::min(count, chunkSize);
		if (mainStart < mainEnd) {
			func(mainStart, mainEnd);
		}

		for (auto& f : futures) {
			f.get();
		}
	}

}
