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
			//����threads�������߳�
			_pool.emplace_back(
				[this]()
				{
					//�߳�һֱ����
					while (true)
					{
						std::function<void()> task;
						//���������Զ��ͷ�
						{
							std::unique_lock<std::mutex> lk(_con_mutex);
							_cond_var.wait(lk, [this]
								{
									return this->stoped || !this->tasks.empty();
								});
							//�߳̽���
							if (this->stoped && tasks.empty())
								return;

							//���������ȡ��һ������
							//��������Ҳ�Ž������������ֶ���Ϊ��
							task = std::move(tasks.front());
							tasks.pop();

						}
						
						//ִ�и�����
						task();


					}





				}


			);
		}

	}


	template<class F,class ...Args>
	auto enque(F&& f,Args&& ... args)
		//����ֵ����
		-> std::future<typename std::result_of<F(Args...)>::type>
	{
		using result_type = typename std::result_of<F(Args...)>::type;


		auto task = std::make_shared<std::packaged_task<result_type(Args...)>>(
			// https://zh.cppreference.com/w/cpp/utility/functional/bind
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);

		//����ֵ
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
