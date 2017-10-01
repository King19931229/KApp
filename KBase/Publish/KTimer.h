#pragma once
#include <chrono>
class KTimer
{
protected:
	typedef std::chrono::steady_clock::time_point TimePoint;
	typedef std::chrono::duration<float, std::ratio<1, 1>> SecnodDuration;
	typedef std::chrono::duration<float, std::ratio<1, 1000>> MillisecondDuration;
	typedef std::chrono::steady_clock SteadyClock;
	
	TimePoint m_BeginPoint;
public:
	KTimer()
		:m_BeginPoint(SteadyClock::now())
	{		
	}

	~KTimer()
	{
	}

	inline void Reset() { m_BeginPoint = SteadyClock::now(); }

	inline float GetSeconds()
	{
		TimePoint now = SteadyClock::now();
		return std::chrono::duration_cast<SecnodDuration>(now - m_BeginPoint).count();
	}

	inline float GetMilliseconds()
	{
		TimePoint now = SteadyClock::now();
		return std::chrono::duration_cast<MillisecondDuration>(now - m_BeginPoint).count();
	}
};