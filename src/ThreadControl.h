#pragma once
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

struct ThreadControl
{
	std::atomic<bool> running{ false };
	std::thread thread;
	std::mutex mutex;
	std::condition_variable_any cv;
};