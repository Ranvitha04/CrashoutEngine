#pragma once

#include <SDL.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace CrashoutEngine
{
    /*!
    @brief Owns SDL controller state, axis polling, and controller rumble backends.

    The manager wraps SDL controller handles and haptic handles so the engine can
    expose action/axis queries and vibration through a single abstraction. It
    snapshots previous-frame button/axis state to support triggered and released
    style input queries used by the gameplay scripts.
    */
    class ControllerManager
    {
    public:
        ControllerManager() = default;
        ~ControllerManager();

        /*!
        @brief Initialize SDL controller and haptic subsystems and open connected pads.
        @return True if controller support initialized successfully.
        */
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
        /*!
        @brief Trigger vibration on connected controllers using SDL rumble/haptics.
        @param lowFrequency Low-frequency motor strength in the range [0, 1].
        @param highFrequency High-frequency motor strength in the range [0, 1].
        @param durationMs Duration of the vibration pulse in milliseconds.
        */
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
