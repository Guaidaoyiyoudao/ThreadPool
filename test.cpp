#include "ThreadPool.hpp"
#include<iostream>

int main()
{

	ThreadPool pool(10);

	for (int i = 0; i < 10; i++)
	{
		pool.enque([i](){
				
			std::cout << "This is " << i << " thread running" << std::endl;
			
			});
	}

	return 0;
}