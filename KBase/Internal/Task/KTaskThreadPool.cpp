#include "KTaskThreadPool.h"

IKTaskWorkPtr CreateTaskWork(IKTaskWork* work)
{
	return IKTaskWorkPtr(work);
}