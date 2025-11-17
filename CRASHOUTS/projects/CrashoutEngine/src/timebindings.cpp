#include "frameratecontroller.h"
#include "mono/metadata/loader.h"
#include "timebindings.h"

//extern FrameRateController gFrameRateController;
auto& gFrameRateController = FrameRateController::GetInstance();


static double TimeGetDT()
{
    return gFrameRateController.DT();
}

static double TimeGetFixedDT()
{
    return gFrameRateController.FixedDT();
}

static double TimeGetFPS()
{
    return gFrameRateController.FPS();
}

static int TimeGetSteps()
{
    return gFrameRateController.StepCount();
}

void CrashoutsEngine::Bindings::RegisterTimeBindings()
{
    mono_add_internal_call("CrashoutsEngine.InternalCalls::TimeGetDT", (const void*)TimeGetDT);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::TimeGetFixedDT", (const void*)TimeGetFixedDT);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::TimeGetFPS", (const void*)TimeGetFPS);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::TimeGetSteps", (const void*)TimeGetSteps);

}