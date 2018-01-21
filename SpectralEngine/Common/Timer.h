#pragma once

#include "Types.h"


// High resolution timer
class Timer
{
public:
	Timer();
	~Timer();

	// Miliseconds since reset
	float TotalTime() const;
	// Miliseconds since reset, excluding stopped time
	float ElapsedTime() const;
	// Miliseconds since last tick
	float DeltaTime() const;

	void Reset();
	void Start();
	void Stop();
	void Tick();

private:
	double totalTime;
	double elapsedTime;
	double deltaTime;
	double milisecondsPerCycle;

	Spectral::int64 baseTime;
	Spectral::int64 pausedTime;
	Spectral::int64 stopTime;
	Spectral::int64 prevTime;

	bool stopped;
};

