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
	mTargetDT = (mTargetFPS > 0) ? 1.0 / static_cast<double>(FPSCap):0.0;
}

void FrameRateController::Reset()
{
	mFrameStart = ClockTime::now();
	mDT = 1.0 / (mTargetFPS > 0 ? static_cast<double>(mTargetFPS) : 60.0);
	mFPSEma = 60.0;
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
		double remain = mTargetDT - elapsed;
		//
		if (remain > 0.001) {
			std::this_thread::sleep_for(duration<double>(remain));
			elapsed = duration<double>(ClockTime::now() - mFrameStart).count();
		}
	}
	//clamp dt between 0.1ms and 250ms so the game does not blow up - Rain
	mDT = std::clamp(elapsed, 0.0001, 0.25);

	//smoothing factor
	const double alpha = 0.1;
	//instant fps this frame
	double inst = 1.0 / mDT;
	mFPSEma = mFPSEma > 0 ? (1 - alpha) * mFPSEma + alpha * inst : inst;
}

double FrameRateController::DT() const
{
	return mDT;
}

double FrameRateController::FPS() const
{
	return mFPSEma;
}

int FrameRateController::CAP() const
{
	return mTargetFPS;
}