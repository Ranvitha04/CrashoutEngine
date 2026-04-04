/*!
@file       scenetransition.h
@author     Shahrin Idzuan
@author     Renfred (RTIS)
@author     Renfred Lee
@date       31/01/2026
@brief      Header file for scene transition functionalities.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
#pragma once
#include <string>

namespace CrashoutsEngine {

	/** @brief SceneTransition */
	class SceneTransition {
	public:
		static void StartFade(float fadeOutTime = 1.0f, float fadeInTime = 1.0f);
		static void Start(const std::string& nextSceneName, float fadeOutTime = 1.0f, float fadeInTime = 1.0f);
		static void Update(float dt);
		static void Render();
		static bool IsTransitioning();

	private:
		static std::string s_nextScene;
		static float s_timer;
		static float s_duration;
		static bool s_active;
		static bool s_fadingOut;
		static float s_alpha;
		static float s_durationOut;
		static float s_durationIn;
		static float s_holdDuration;
		static bool s_isHolding;
		static float s_baseMasterVolume;
	};

}

