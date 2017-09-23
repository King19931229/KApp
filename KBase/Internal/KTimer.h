#pragma once
#include <chrono>
class KTimer
{
protected:
	typedef std::chrono::steady_clock::time_point TimePoint;
	typedef std::chrono::duration<double, std::ratio<1, 1>> Duration;
	typedef std::chrono::steady_clock SteadyClock;
	
	TimePoint m_BeginPoint;
	Duration m_Duration;

public:
	KTimer()
		: m_Duration(Duration(0))
	{
	}

	~KTimer()
	{
	}

	void Start()
	{
		m_BeginPoint = SteadyClock::now();
		m_Duration = Duration(0);
	}

	void Stop()
	{
		TimePoint end = SteadyClock::now();
		m_Duration += std::chrono::duration_cast<Duration>(end - m_BeginPoint);
	}

	void Reset()
	{
		m_Duration = Duration(0);
	}

	double GetDuration()
	{
		return m_Duration.count();
	}
};