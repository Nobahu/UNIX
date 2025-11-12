#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <memory>
#include <queue>


class Detail {
public:
	std::string _type;
	bool isProcessed = false;
	Detail(std::string type) : _type(type) {};
};

class Factory {
private:
	std::mutex FactoryMtx;
	std::condition_variable cv;
	std::queue<std::unique_ptr<Detail>> details;
	bool isProdFinished = false;
public:

	void Producer();
	void Consumer();

};

void Factory::Producer()
{
	for (size_t i = 0; i < 10; i++) {

		std::this_thread::sleep_for(std::chrono::seconds(1));
		{
			std::lock_guard<std::mutex> lock(FactoryMtx);
			auto detail = std::make_unique<Detail>("Bolt");
			detail->isProcessed = true;
			details.push(std::move(detail));
			cv.notify_one();
			std::cout << "Detail was produced and processed" << '\n';
		}
	}

	{
		std::lock_guard<std::mutex> lock(FactoryMtx);
		isProdFinished = true;
		cv.notify_all();
	}
}

void Factory::Consumer()
{
	while (true) {
		std::unique_ptr<Detail> detail;
		{
			std::unique_lock<std::mutex> lock(FactoryMtx);

			cv.wait(lock, [this]() {
				return !details.empty() || isProdFinished;
				});

			if (isProdFinished && details.empty()) {
				break;
			}

			if (!details.empty()) {
				detail = std::move(details.front());
				details.pop();
			}
		}

		if (detail) {
			std::cout << "Detail was used: " << detail->_type << '\n';
		}
	}
}


int main()
{
	setlocale(LC_ALL, "RU");
	Factory factory;
	std::thread t1(&Factory::Producer, &factory);
	std::thread t2(&Factory::Consumer, &factory);

	t1.join();
	t2.join();

	std::cout << "Work was ended" << '\n';

	return 0;
}
