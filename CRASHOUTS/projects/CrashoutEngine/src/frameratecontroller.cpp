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
	Reset();
}

void FrameRateController::SetTargetFPS(int FPSCap)
{
	//if fps cap is above 0, set target fps to that value, otherwise set to 0 which is uncapped
	mTargetFPS = (FPSCap > 0) ? FPSCap : 0;
	//time budget per frame in seconds pretty sure we went thru this in soroor - Rain
	mTargetDT = (mTargetFPS > 0) ? 1.0 / static_cast<double>(mTargetFPS):0.0;
}

void FrameRateController::Reset()
{
	mFrameStart = ClockTime::now();
	//prevent division by zero
	mDT = 1.0 / (mTargetFPS > 0 ? static_cast<double>(mTargetFPS) : defaultFallBack);
	mFPS = defaultFallBack;
}

void FrameRateController::FrameStart()
{
	mFrameStart = ClockTime::now();
}

void FrameRateController::FrameEnd()
{
	using namespace std::chrono;
	double elapsed = duration<double>(ClockTime::now() - mFrameStart).count();
	//only update dt and fps if capped
	if (mTargetFPS > 0) {
		while (elapsed < mTargetDT) {
			elapsed = duration<double>(ClockTime::now() - mFrameStart).count();
		}
	}
	//clamp dt between 0.1ms and 250ms so the game does not blow up - Rain
	mDT = std::clamp(elapsed, 0.0001, 0.25);

	//instant fps this frame
	double inst = 1.0 / mDT;
	mFPS = inst;
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