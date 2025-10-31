#include "Includes.h"
#include "Monitor.h"

void Factory::Producer()
{
	for (size_t i = 0; i < 10; i++) {

		std::this_thread::sleep_for(std::chrono::seconds(1));
		{
			std::lock_guard<std::mutex> lock(FactoryMtx);
			auto detail = std::make_unique<Detail>("Bolt");
			detail->isProcessed = true;
			details.push(std::move(detail));
			std::cout << "Деталь обработана" << '\n';
		}
        cv.notify_one();
	}
    {
        std::lock_guard<std::mutex> lock(FactoryMtx);
        isProdFinished = true;
    }
    cv.notify_all();
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

            detail = std::move(details.front());
            details.pop();
        }

        std::cout << "Деталь использована: " << detail->_type << '\n';
    }
}