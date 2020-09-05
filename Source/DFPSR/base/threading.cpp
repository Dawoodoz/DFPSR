// zlib open source license
//
// Copyright (c) 2017 to 2019 David Forsgren Piuva
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 
//    2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 
//    3. This notice may not be removed or altered from any source
//    distribution.

#include "threading.h"

// Requires -pthread for linking
#include <future>
#include <thread>
#include <mutex>
#include <atomic>

namespace dsr {

// Enable this macro to disable multi-threading
//   If your application still crashes when using a single thread, it's probably not a concurrency problem
//#define DISABLE_MULTI_THREADING

// Prevent doing other multi-threaded work at the same time
//   As a side effect, this makes it safe to use global variables to prevent unsafe use of stack memory
static std::mutex workLock, getTaskLock;
static std::atomic<int> nextJobIndex{0};

// TODO: This method really needs a thread pool for starting jobs faster,
//       but third-party libraries often use low-level platform specific solutions.
// TODO: Let each worker have one future doing scheduling on it's own to prevent stalling on a scheduling main thread.
//       When a worker is done with a task, it will use a mutex protected volatile variable to pick the next task from the queue.
void threadedWorkFromArray(std::function<void()>* jobs, int jobCount) {
	#ifdef DISABLE_MULTI_THREADING
		// Reference implementation
		for (int i = 0; i < jobCount; i++) {
			jobs[i]();
		}
	#else
		if (jobCount <= 0) {
			return;
		} else if (jobCount == 1) {
			jobs[0]();
		} else {
			workLock.lock();
				nextJobIndex = 0;
				// Multi-threaded work loop
				int workerCount = std::min((int)std::thread::hardware_concurrency() - 1, jobCount); // All used threads
				int helperCount = workerCount - 1; // Excluding the main thread
				std::function<void()> workers[workerCount];
				std::future<void> helpers[helperCount];
				for (int w = 0; w < workerCount; w++) {
					workers[w] = [jobs, jobCount]() {
						while (true) {
							getTaskLock.lock();
							int taskIndex = nextJobIndex;
							nextJobIndex++;
							getTaskLock.unlock();
							if (taskIndex < jobCount) {
								jobs[taskIndex]();
							} else {
								break;
							}
						}
					};
				}
				// Start working in the helper threads
				for (int h = 0; h < helperCount; h++) {
					helpers[h] = std::async(std::launch::async, workers[h]);
				}
				// Perform the same work on the main thread
				workers[workerCount - 1]();
				// Wait for all helpers to complete their work once all tasks have been handed out
				for (int h = 0; h < helperCount; h++) {
					if (helpers[h].valid()) {
						helpers[h].wait();
					}
				}
			workLock.unlock();
		}
	#endif
}

void threadedWorkFromList(List<std::function<void()>> jobs) {
	threadedWorkFromArray(&jobs[0], jobs.length());
	jobs.clear();
}

void threadedSplit(int startIndex, int stopIndex, std::function<void(int startIndex, int stopIndex)> task, int minimumJobSize, int jobsPerThread) {
	int totalCount = stopIndex - startIndex;
	int maxJobs = totalCount / minimumJobSize;
	int jobCount = std::thread::hardware_concurrency() * jobsPerThread;
	if (jobCount > maxJobs) { jobCount = maxJobs; }
	if (jobCount < 1) { jobCount = 1; }
	if (jobCount == 1) {
		// Too little work for multi-threading
		task(startIndex, stopIndex);
	} else {
		// Use multiple threads
		std::function<void()> jobs[jobCount];
		int givenRow = startIndex;
		for (int s = 0; s < jobCount; s++) {
			int remainingJobs = jobCount - s;
			int remainingRows = stopIndex - givenRow;
			int y1 = givenRow; // Inclusive
			int taskSize = remainingRows / remainingJobs;
			givenRow = givenRow + taskSize;
			int y2 = givenRow; // Exclusive
			jobs[s] = [task, y1, y2]() {
				task(y1, y2);
			};
		}
		threadedWorkFromArray(jobs, jobCount);
	}
}

void threadedSplit_disabled(int startIndex, int stopIndex, std::function<void(int startIndex, int stopIndex)> task) {
	task(startIndex, stopIndex);
}

void threadedSplit(const IRect& bound, std::function<void(const IRect& bound)> task, int minimumRowsPerJob, int jobsPerThread) {
	int maxJobs = bound.height() / minimumRowsPerJob;
	int jobCount = std::thread::hardware_concurrency() * jobsPerThread;
	if (jobCount > maxJobs) { jobCount = maxJobs; }
	if (jobCount < 1) { jobCount = 1; }
	if (jobCount == 1) {
		// Too little work for multi-threading
		task(bound);
	} else {
		// Use multiple threads
		std::function<void()> jobs[jobCount];
		int givenRow = bound.top();
		for (int s = 0; s < jobCount; s++) {
			int remainingJobs = jobCount - s;
			int remainingRows = bound.bottom() - givenRow;
			int y1 = givenRow;
			int taskSize = remainingRows / remainingJobs;
			givenRow = givenRow + taskSize;
			IRect subBound = IRect(bound.left(), y1, bound.width(), taskSize);
			jobs[s] = [task, subBound]() {
				task(subBound);
			};
		}
		threadedWorkFromArray(jobs, jobCount);
	}
}

void threadedSplit_disabled(const IRect& bound, std::function<void(const IRect& bound)> task) {
	task(bound);
}

}

