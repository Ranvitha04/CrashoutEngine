#pragma once

#include <SDL.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace CrashoutEngine
{
    // Owns SDL GameController handles and snapshots previous controller state so
    // action queries can expose pressed/released transitions.
    class ControllerManager
    {
    public:
        ControllerManager() = default;
        ~ControllerManager();

        bool Initialize();
        void Shutdown();

        void BeginFrame();
        void SyncState();
        void HandleEvent(const SDL_Event& event);

        bool GetButton(SDL_GameControllerButton button) const;
        bool GetButtonDown(SDL_GameControllerButton button) const;
        bool GetButtonUp(SDL_GameControllerButton button) const;

        float GetAxis(SDL_GameControllerAxis axis, float deadzone) const;
        float GetPreviousAxis(SDL_GameControllerAxis axis, float deadzone) const;
        void SetVibration(float lowFrequency, float highFrequency, std::uint32_t durationMs) const;

        std::size_t GetConnectedControllerCount() const;

    private:
        struct ControllerState
        {
            SDL_GameController* handle = nullptr;
            SDL_Haptic* haptic = nullptr;
            SDL_JoystickID instanceId = -1;
            std::vector<std::uint8_t> previousButtons;
            std::vector<float> previousAxes;
        };

        void OpenController(int deviceIndex);
        void CloseController(SDL_JoystickID instanceId);
        void SnapshotControllerState(ControllerState& controller);
        static float NormalizeAxisValue(Sint16 rawValue);
        static float ApplyDeadzone(float value, float deadzone);

        std::unordered_map<SDL_JoystickID, ControllerState> m_controllers;
        bool m_initialized = false;
    };
}
