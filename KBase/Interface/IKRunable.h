#pragma once
#include <memory>

struct IKRunable
{
	virtual ~IKRunable() {}
	virtual void StartUp() = 0;
	virtual void Run() = 0;
	virtual void ShutDown() = 0;
};

typedef std::shared_ptr<IKRunable> IKRunablePtr;