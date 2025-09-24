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
	mFPSEma = defaultFallBack;
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
		//const double target = mTargetDT;
		//time remaining in the frame budget
		/*double remain = mTargetDT - elapsed;
		//sleep if enough time is left
		if (remain > 0.001) {
			std::this_thread::sleep_for(duration<double>(remain));
			//recalculate elapsed time after sleep
			elapsed = duration<double>(ClockTime::now() - mFrameStart).count();
		}*/
		while (elapsed < mTargetDT) {
			elapsed = duration<double>(ClockTime::now() - mFrameStart).count();
		}
		//i cannot get this to work properly and instead of the engine blowing up, i did - Rain
		/*
		const double sleepGuard = 0.001;
		const double sleepMin = 0.002;

		// sleep most of the remainder
		while (true) {
			elapsed = duration<double>(ClockTime::now() - mFrameStart).count();
			double remain = target - elapsed;
			if (remain <= 0.0) break;
			if (remain > sleepMin) {
				std::this_thread::sleep_for(duration<double>(remain - sleepGuard));
			}
			else {
				break;
			}
		}
		
		while (duration<double>(ClockTime::now() - mFrameStart).count() < target) { }

		elapsed = duration<double>(ClockTime::now() - mFrameStart).count();*/

	}
	//clamp dt between 0.1ms and 250ms so the game does not blow up - Rain
	mDT = std::clamp(elapsed, 0.0001, 0.25);

	//smoothing factor
	const double alpha = 0.1;
	//instant fps this frame
	double inst = 1.0 / mDT;
	mFPSEma = mFPSEma > 0.0 ? (1.0 - alpha) * mFPSEma + alpha * inst : inst;
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