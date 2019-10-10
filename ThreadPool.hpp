#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include<type_traits>
#include<condition_variable>
#include<mutex>
#include<vector>
#include<queue>
#include<thread>
#include<functional>
#include<cstddef>
#include<future>
class ThreadPool
{

public:

	ThreadPool(size_t threads) :
		stoped(false)
	{

		for (int i = 0; i < threads; i++)
		{
			//放入threads数量的线程
			_pool.emplace_back(
				[this]()
				{
					//线程一直工作
					while (true)
					{
						std::function<void()> task;
						//用于锁的自动释放
						{
							std::unique_lock<std::mutex> lk(_con_mutex);
							_cond_var.wait(lk, [this]
								{
									return this->stoped || !this->tasks.empty();
								});
							//线程结束
							if (this->stoped && tasks.empty())
								return;

							//从任务队列取出一个任务
							//必须把这个也放进来，否则会出现队列为空
							task = std::move(tasks.front());
							tasks.pop();

						}
						
						//执行该任务
						task();


					}





				}


			);
		}

	}


	template<class F,class ...Args>
	auto enque(F&& f,Args&& ... args)
		//返回值类型
		-> std::future<typename std::result_of<F(Args...)>::type>
	{
		using result_type = typename std::result_of<F(Args...)>::type;


		auto task = std::make_shared<std::packaged_task<result_type(Args...)>>(
			// https://zh.cppreference.com/w/cpp/utility/functional/bind
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);

		//返回值
		std::future<result_type> result = task->get_future();

		{
			std::unique_lock<std::mutex> lk(_con_mutex);
			
			if (stoped)
				throw std::runtime_error("enque on stopeed ThreadPool");

			tasks.emplace([task] {(*task)(); });

		}

		_cond_var.notify_one();
		return result;

	}

	~ThreadPool()
	{
		{
			std::unique_lock<std::mutex> lk(_con_mutex);

			stoped = true;
		}
		_cond_var.notify_all();
		for (auto& thread : _pool)
			thread.join();
	}



	

private:
	//thread pool
	std::vector<std::thread> _pool;

	//task
	std::queue<std::function<void()>> tasks;

	std::mutex _con_mutex;
	std::condition_variable _cond_var;
	bool stoped;

};







#endif // !THREAD_POOL_H
