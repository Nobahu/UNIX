#include "Includes.h"
#include "Monitor.h"

int main() 
{
	setlocale(LC_ALL, "RU");
	Factory factory;
	std::thread t1(&Factory::Producer, &factory);
	std::thread t2(&Factory::Consumer, &factory);

	t1.join();
	t2.join();


	std::cout << "Работа завершена" << '\n';

	return 0;
}