#pragma once
#include <chrono>
class KTimer
{
protected:
	typedef std::chrono::high_resolution_clock::time_point TimePoint;
	typedef std::chrono::duration<float, std::ratio<1, 1>> SecnodDuration;
	typedef std::chrono::duration<float, std::ratio<1, 1000>> MillisecondDuration;
	typedef std::chrono::high_resolution_clock HighResolutionClock;

	TimePoint m_BeginPoint;
public:
	KTimer()
		:m_BeginPoint(HighResolutionClock::now())
	{
	}

	~KTimer()
	{
	}

	inline void Reset() { m_BeginPoint = HighResolutionClock::now(); }

	inline float GetSeconds()
	{
		TimePoint now = HighResolutionClock::now();
		return std::chrono::duration_cast<SecnodDuration>(now - m_BeginPoint).count();
	}

	inline float GetMilliseconds()
	{
		TimePoint now = HighResolutionClock::now();
		return std::chrono::duration_cast<MillisecondDuration>(now - m_BeginPoint).count();
	}
};