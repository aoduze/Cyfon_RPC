#pragma once 

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>
#include <atomic>
#include <type_traits>
#include <utility>
#include <memory>
#include <tuple>

class ThreadPool {
public:
	explicit ThreadPool(size_t threads_count = std::thread::hardware_concurrency());
	~ThreadPool();

	// ��ֹ��������
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

    template<class F, class... Args>
	auto enqueue(F&& f, Args&&... args)
		-> std::future<std::invoke_result_t<F, Args...>>;

private:
	void worker_thread_loop();

	// �߳��������������
	std::vector<std::thread> workers_;
	std::queue<std::function<void()>> tasks_;

	// ͬ��ԭ��
	std::mutex queue_mutex_;
	std::condition_variable condition_;

	// ���Ʊ�ʶ
	std::atomic<bool> stop_; 
};

inline ThreadPool::ThreadPool(size_t threads_count) : stop_(false) {
	if (threads_count == 0) {
		throw std::invalid_argument("ThreadPool constructor requires a thread count greater than 0.");
	}

	workers_.reserve(threads_count);

	for (size_t i = 0; i < threads_count; ++i) {
		workers_.emplace_back([this] {
			this -> worker_thread_loop();
			});
	}
}

inline ThreadPool::~ThreadPool() {
	stop_.store(true);
	condition_.notify_all();

	for (std::thread& worker : workers_) {
		if (worker.joinable()) {
			worker.join();
		}
	}
}

inline void ThreadPool::worker_thread_loop() {
	while (true) {
		std::function<void()> task;
	
		{
			std::unique_lock<std::mutex> lock(queue_mutex_);
			condition_.wait(lock, [this] {
				return this->stop_.load() || !this->tasks_.empty();
			});

			if (stop_.load() && tasks_.empty()) {
				return;
			}

			// �Ӷ�����ȡ��һ������
			task = std::move(this -> tasks_.front());
			this -> tasks_.pop();
		}

		// ִ������

		if (task) {
			task();
		}
	}	
}

template <class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
  -> std::future<std::invoke_result_t<F, Args...>> {
	
	// �ƶ�����ķ�������
	using return_type = std::invoke_result_t<F, Args...>;

	if (stop_.load()) {
		throw std::runtime_error("enqueue on stopped ThreadPool");
	}

	// ����һ���������
	auto task = std::make_shared<std::packaged_task<return_type()>>(
		[func = std::forward<F>(f), t = std::make_tuple(std::forward<Args>(args)...)]() mutable -> return_type {
			return std::apply(std::move(func), std::move(t));
		}
	);

	std::future<return_type> res = task -> get_future();

	{
		std::unique_lock<std::mutex> lock(queue_mutex_);
		
		if (stop_.load()) {
			throw std::runtime_error("enqueue on stopped ThreadPool");
		}

		tasks_.emplace([task]() { (*task)(); });
	}

	// ֪ͨһ���ȴ����߳�
	condition_.notify_one();
	return res;
}

