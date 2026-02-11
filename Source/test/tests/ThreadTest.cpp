
#include "../testTools.h"
#include "../../DFPSR/base/threading.h"
#include "../../DFPSR/api/timeAPI.h"
#include "../../DFPSR/base/StorableCallback.h"

// The dummy tasks are too small to get a benefit from multi-threading. (0.18 ms overhead on 0.04 ms of total work)
START_TEST(Thread)
	{ // Basic version iterating over lambdas in a dynamic list
		const int jobCount = 50;
		int results[jobCount] = {}; // TODO: Make a safer example.
		List<StorableCallback<void()>> jobs;
		for (int i = 0; i < jobCount; i++) {
			int* result = &results[i];
			jobs.push([result, i](){
				// Simulate a heavy workload
				time_sleepSeconds(0.01f);
				*result = i * 26 + 43;
			});
		}
		double totalStartTime = time_getSeconds();
		threadedWorkFromList(jobs);
		printText("Completed all jobs in ", (time_getSeconds() - totalStartTime) * 1000.0, " ms\n");
		for (int i = 0; i < jobCount; i++) {
			ASSERT_EQUAL(results[i], i * 26 + 43);
		}
	}
	{ // Threaded split for automatic division of a big number of jobs
		List<int> items;
		for (int i = 0; i < 100; i++) {
			items.push(0);
		}
		int rowStart = 10; // Inclusive
		int rowEnd = 90; // Exclusive
		double totalStartTime = time_getSeconds();
		int* itemPtr = &items[0];
		threadedSplit(rowStart, rowEnd, [itemPtr](int startIndex, int stopIndex) {
			for (int i = startIndex; i < stopIndex; i++) {
				itemPtr[i] += i * 10;
			}
		});
		printText("Completed all jobs in ", (time_getSeconds() - totalStartTime) * 1000.0, " ms\n");
		for (int i = 0; i < rowStart; i++) {
			ASSERT_EQUAL(items[i], 0);
		}
		for (int i = rowStart; i < rowEnd; i++) {
			ASSERT_EQUAL(items[i], i * 10);
		}
		for (int i = rowEnd; i < items.length(); i++) {
			ASSERT_EQUAL(items[i], 0);
		}
	}
END_TEST

