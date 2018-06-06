#include <Windows.h>
#include "Timer.h"

using namespace Spectral;

Timer::Timer()
	: milisecondsPerCycle(0.0), deltaTime(0.0), baseTime(0), stopTime(0),
	prevTime(0), stopped(true)
{
	LARGE_INTEGER cyclesPerSec;
	QueryPerformanceFrequency(&cyclesPerSec);
	milisecondsPerCycle = 1000.0 / static_cast<double>(cyclesPerSec.QuadPart);
}


Timer::~Timer()
{

}

float Timer::TotalTime() const
{
	return static_cast<float>(totalTime);
}

float Timer::ElapsedTime() const
{
	return static_cast<float>(elapsedTime);
}

float Timer::DeltaTime() const
{
	return static_cast<float>(deltaTime);
}

void Timer::Reset()
{
	LARGE_INTEGER currTime;
	QueryPerformanceCounter(&currTime);

	baseTime = static_cast<int64>(currTime.QuadPart);
	prevTime = static_cast<int64>(currTime.QuadPart);
	stopTime = 0;
	pausedTime = 0;

	totalTime = 0;
	elapsedTime = 0;
	deltaTime = 0;
	
	stopped = false;
}

void Timer::Start()
{
	if (!stopped)
		return;

	LARGE_INTEGER currTime;
	QueryPerformanceCounter(&currTime);

	pausedTime += (static_cast<int64>(currTime.QuadPart) - stopTime);
	totalTime = (currTime.QuadPart - baseTime) * milisecondsPerCycle;
	elapsedTime = ((currTime.QuadPart - baseTime) - pausedTime) * milisecondsPerCycle;
	// No delta-time update

	prevTime = static_cast<int64>(currTime.QuadPart);
	stopTime = 0;
	stopped = false;
}

void Timer::Stop()
{
	if (stopped)
		return;

	LARGE_INTEGER currTime;
	QueryPerformanceCounter(&currTime);

	totalTime = (currTime.QuadPart - baseTime) * milisecondsPerCycle;
	elapsedTime = ((currTime.QuadPart - baseTime) - pausedTime) * milisecondsPerCycle;
	deltaTime = (currTime.QuadPart - prevTime) * milisecondsPerCycle;

	stopTime = currTime.QuadPart;
	stopped = true;
}

void Timer::Tick()
{
	if (stopped)
	{
		deltaTime = 0.0;
		return;
	}

	LARGE_INTEGER currTime;
	QueryPerformanceCounter(&currTime);

	totalTime = (currTime.QuadPart - baseTime) * milisecondsPerCycle;
	elapsedTime = ((currTime.QuadPart - baseTime) - pausedTime) * milisecondsPerCycle;
	deltaTime = (currTime.QuadPart - prevTime) * milisecondsPerCycle;

	prevTime = currTime.QuadPart;

	// Watch for CPU resets and being switched
	// do a different processor.
	if (deltaTime < 0.0)
		deltaTime = 0.0;
}
