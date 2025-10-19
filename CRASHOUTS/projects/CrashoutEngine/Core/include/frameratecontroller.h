/*!
@file		frameratecontroller.h
@author     m.biniswandi@digipen.edu
@date       24/09/2025
@brief		Declaration for frame rate controller

*/
#pragma once
#pragma warning(disable: 4251)
#include <chrono>
#include <thread>

#include "ImportExport.h"

DLL_API class FrameRateController
{
	//refer to https://en.cppreference.com/w/cpp/chrono/steady_clock
	using ClockTime = std::chrono::steady_clock;
	public:
		DLL_API static FrameRateController& GetInstance() {
		static FrameRateController instance;
		return instance;
	}
	//FrameRateController(int FPSCap = 144);

	///copy constructor and assignment operator are deleted to enforce singleton pattern
	FrameRateController(const FrameRateController&) = delete;
	//copy assignment operator is deleted
	FrameRateController& operator=(const FrameRateController&) = delete;


	DLL_API void SetTargetFPS(int FPSCap);
	DLL_API void Reset();
	DLL_API void FrameStart();
	DLL_API void FrameEnd();

	/// seconds
	DLL_API double DT() const;
	/// smooth fps
	double FPS() const;
	/// current cap
	int CAP() const;

private:
	FrameRateController();
	ClockTime::time_point mFrameStart{};
	int mTargetFPS{};
	double mTargetDT{};
	double mDT{};
	//Exponential moving average of FPS
	///EMA calculate smoothed fps to avoid jumping fps values
	double mFPS{};
	double defaultFallBack{ 60.0 };
};
