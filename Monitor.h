#pragma once
#include "Includes.h"

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