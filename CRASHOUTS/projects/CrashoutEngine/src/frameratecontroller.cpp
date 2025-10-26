/*!
@file		frameratecontroller.cpp
@author     m.biniswandi@digipen.edu
@date       24/09/2025
@brief		Definition for frame rate controller

*/
#include "frameratecontroller.h"
#include <algorithm>

FrameRateController::FrameRateController()
{
	SetTargetFPS(144);
	SetFixedDT(1.0/60.0);
	Reset();
}

void FrameRateController::SetTargetFPS(int FPSCap)
{
	//if fps cap is above 0, set target fps to that value, otherwise set to 0 which is uncapped
	mTargetFPS = (FPSCap > 0) ? FPSCap : 0;
	//time budget per frame in seconds pretty sure we went thru this in soroor - Rain
	mTargetDT = (mTargetFPS > 0) ? 1.0 / static_cast<double>(mTargetFPS):0.0;
}

void FrameRateController::SetFixedDT(double step)
{
	// just in case someone wanna test with zero lol, i know it will happen somewhere - Rain
	mFixedDT = (step > 0.0) ? step : (1.0 / 60.0);
}

void FrameRateController::Reset()
{
	mFrameStart = ClockTime::now();
	mDT = 0.0;
	mFPS = 0.0;
	mAccumulatedTime = 0.0;
	mCurrentSteps = 0;
}

void FrameRateController::FrameStart()
{
	mFrameStart = ClockTime::now();
	mCurrentSteps = 0;
}

void FrameRateController::FrameEnd()
{
	using namespace std::chrono;
	double elapsed = duration<double>(ClockTime::now() - mFrameStart).count();

	//clamp dt between 0.1ms and 250ms so the game does not blow up - Rain
	mDT = std::clamp(elapsed, 0.0001, 0.25);
	mFPS = 1.0 / mDT;

	// accumulate time since last frame
	mAccumulatedTime += mDT;
	// count how many physics steps to run

	while (mAccumulatedTime >= mFixedDT)
	{
		mAccumulatedTime -= mFixedDT;
		mCurrentSteps++;
	}
}

double FrameRateController::DT() const
{
	return mDT;
}

double FrameRateController::FPS() const
{
	return mFPS;
}

int FrameRateController::CAP() const
{
	return mTargetFPS;
}

double FrameRateController::FixedDT() const
{
	return mFixedDT;
}

int FrameRateController::StepCount() const
{
	return mCurrentSteps;
}