/*!
@file		frameratecontroller.cpp
@author     Shahrin Idzuan
@author     Renfred Lee
@author     tristan
@date       24/09/2025
@brief		Definition for frame rate controller

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

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
}

void FrameRateController::FrameEnd()
{
	using namespace std::chrono;

	// Frame capping
	if (mTargetFPS > 0 && mTargetDT > 0.0)
	{
		while (true)
		{
			// Check how much time has passed since FrameStart
			double currentElapsed = duration<double>(ClockTime::now() - mFrameStart).count();

			// If we have passed our target duration, break the loop
			if (currentElapsed >= mTargetDT)
				break;

		}
	}

	// Actual elapsed time
	double elapsed = duration<double>(ClockTime::now() - mFrameStart).count();

	//clamp dt between 0.1ms and 250ms so the game does not blow up - Rain
	mDT = std::clamp(elapsed, 0.0001, 0.15);
	
	// Smooth FPS Display
	double currentInstantFPS = 1.0 / mDT;
	if (mFPS == 0.0) mFPS = currentInstantFPS; // Initialize if first frame
	else mFPS = (mFPS * 0.9) + (currentInstantFPS * 0.1);

	mCurrentSteps = 0;
	// accumulate time since last frame
	mAccumulatedTime += mDT;
	// count how many physics steps to run

	while (mAccumulatedTime >= mFixedDT)
	{
		mAccumulatedTime -= mFixedDT;
		mCurrentSteps++;
	}

	if (mCurrentSteps > mMaxPhysicsSteps)
	{
		mCurrentSteps = mMaxPhysicsSteps;
		// If we capped, we should also discard the leftover accumulation 
		// to prevent it from immediately causing more steps next frame.
		mAccumulatedTime = 0.0;
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