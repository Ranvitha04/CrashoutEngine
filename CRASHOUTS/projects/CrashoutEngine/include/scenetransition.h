#pragma once
#include <string>

namespace CrashoutsEngine {

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
	};

}
