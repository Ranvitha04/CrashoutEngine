/*!
@file		frameratecontroller.h
@author     m.biniswandi@digipen.edu
@date       24/09/2025
@brief		Declaration for frame rate controller

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#pragma once
#include <chrono>
#include <thread>
#include <cstdint>

class FrameRateController
{
	//refer to https://en.cppreference.com/w/cpp/chrono/steady_clock
	using ClockTime = std::chrono::steady_clock;
public:
	static FrameRateController& GetInstance() {
		static FrameRateController instance;
		return instance;
	}
	//FrameRateController(int FPSCap = 144);

	///copy constructor and assignment operator are deleted to enforce singleton pattern
	FrameRateController(const FrameRateController&) = delete;
	//copy assignment operator is deleted
	FrameRateController& operator=(const FrameRateController&) = delete;

	void SetTargetFPS(int FPSCap);
	void SetFixedDT(double step);
	void Reset();
	void FrameStart();
	void FrameEnd();

	/// seconds
	double DT() const;
	/// smooth fps
	double FPS() const;
	/// current cap
	int CAP() const;

	double FixedDT() const;
	int StepCount() const;

	void SetMaxPhysicsSteps(int max) { mMaxPhysicsSteps = max; }
	int GetMaxPhysicsSteps() const { return mMaxPhysicsSteps; }

	uint32_t GetFixedTickCount() const { return m_fixedTickCount; }
	void IncrementFixedTick() { ++m_fixedTickCount; }

private:
	FrameRateController();
	ClockTime::time_point mFrameStart{};
	int mTargetFPS{};
	double mTargetDT{};
	double mFixedDT{1.0/60.0};
	double mDT{};
	double mAccumulatedTime{};
	int mCurrentSteps{};
	int mMaxPhysicsSteps{ 4 }; // Default to 4 steps max
	//Exponential moving average of FPS
	///EMA calculate smoothed fps to avoid jumping fps values
	double mFPS{};
	double defaultFallBack{ 60.0 };
	uint32_t m_fixedTickCount{ 0 };
};