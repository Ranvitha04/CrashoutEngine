#include "ControllerManager.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

namespace
{
    template <typename T>
    bool IsPressed(const std::vector<T>& values, std::size_t index)
    {
        return index < values.size() && values[index] != 0;
    }
}

namespace CrashoutEngine
{
    ControllerManager::~ControllerManager()
    {
        Shutdown();
    }

    bool ControllerManager::Initialize()
    {
        SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "1");
        SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
        SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
        SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");

        if ((SDL_WasInit(SDL_INIT_EVENTS) & SDL_INIT_EVENTS) == 0U)
        {
            if (SDL_InitSubSystem(SDL_INIT_EVENTS) != 0)
            {
                return false;
            }
        }

        if ((SDL_WasInit(SDL_INIT_JOYSTICK) & SDL_INIT_JOYSTICK) == 0U)
        {
            if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != 0)
            {
                return false;
            }
        }

        // SDL's GameController layer gives us a normalized mapping for Xbox,
        // DualSense, and other supported pads instead of device-specific code.
        if ((SDL_WasInit(SDL_INIT_GAMECONTROLLER) & SDL_INIT_GAMECONTROLLER) == 0U)
        {
            if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
            {
                return false;
            }
        }

        if ((SDL_WasInit(SDL_INIT_HAPTIC) & SDL_INIT_HAPTIC) == 0U)
        {
            SDL_InitSubSystem(SDL_INIT_HAPTIC);
        }

        m_initialized = true;

        const int joystickCount = SDL_NumJoysticks();
        for (int deviceIndex = 0; deviceIndex < joystickCount; ++deviceIndex)
        {
            if (SDL_IsGameController(deviceIndex) == SDL_TRUE)
            {
                OpenController(deviceIndex);
            }
        }

        return true;
    }

    void ControllerManager::Shutdown()
    {
        for (auto& [instanceId, controller] : m_controllers)
        {
            (void)instanceId;
            if (controller.haptic != nullptr)
            {
                SDL_HapticClose(controller.haptic);
                controller.haptic = nullptr;
            }

            if (controller.handle != nullptr)
            {
                SDL_GameControllerClose(controller.handle);
                controller.handle = nullptr;
            }
        }

        m_controllers.clear();
        m_initialized = false;
    }

    void ControllerManager::BeginFrame()
    {
        // Snapshot before polling the new frame so GetButtonDown/GetButtonUp can
        // compare against the previous controller state.
        for (auto& [instanceId, controller] : m_controllers)
        {
            (void)instanceId;
            SnapshotControllerState(controller);
        }
    }

    void ControllerManager::SyncState()
    {
        // Force "previous" state to match the controller's live state so held
        // buttons and sticks do not retrigger across scene loads or UI mode
        // transitions.
        for (auto& [instanceId, controller] : m_controllers)
        {
            (void)instanceId;
            SnapshotControllerState(controller);
        }
    }

    void ControllerManager::HandleEvent(const SDL_Event& event)
    {
        switch (event.type)
        {
        case SDL_CONTROLLERDEVICEADDED:
            OpenController(event.cdevice.which);
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            CloseController(event.cdevice.which);
            break;

        default:
            break;
        }
    }

    bool ControllerManager::GetButton(SDL_GameControllerButton button) const
    {
        for (const auto& [instanceId, controller] : m_controllers)
        {
            (void)instanceId;
            if (controller.handle != nullptr &&
                SDL_GameControllerGetButton(controller.handle, button) == 1)
            {
                return true;
            }
        }

        return false;
    }

    bool ControllerManager::GetButtonDown(SDL_GameControllerButton button) const
    {
        for (const auto& [instanceId, controller] : m_controllers)
        {
            (void)instanceId;
            if (controller.handle == nullptr)
            {
                continue;
            }

            const bool isPressedNow = SDL_GameControllerGetButton(controller.handle, button) == 1;
            const bool wasPressed = IsPressed(controller.previousButtons, static_cast<std::size_t>(button));
            if (isPressedNow && !wasPressed)
            {
                return true;
            }
        }

        return false;
    }

    bool ControllerManager::GetButtonUp(SDL_GameControllerButton button) const
    {
        for (const auto& [instanceId, controller] : m_controllers)
        {
            (void)instanceId;
            if (controller.handle == nullptr)
            {
                continue;
            }

            const bool isPressedNow = SDL_GameControllerGetButton(controller.handle, button) == 1;
            const bool wasPressed = IsPressed(controller.previousButtons, static_cast<std::size_t>(button));
            if (!isPressedNow && wasPressed)
            {
                return true;
            }
        }

        return false;
    }

    float ControllerManager::GetAxis(SDL_GameControllerAxis axis, float deadzone) const
    {
        float value = 0.0f;

        for (const auto& [instanceId, controller] : m_controllers)
        {
            (void)instanceId;
            if (controller.handle == nullptr)
            {
                continue;
            }

            const float axisValue = ApplyDeadzone(
                NormalizeAxisValue(SDL_GameControllerGetAxis(controller.handle, axis)),
                deadzone);

            if (std::fabs(axisValue) > std::fabs(value))
            {
                value = axisValue;
            }
        }

        return value;
    }

    float ControllerManager::GetPreviousAxis(SDL_GameControllerAxis axis, float deadzone) const
    {
        float value = 0.0f;

        for (const auto& [instanceId, controller] : m_controllers)
        {
            (void)instanceId;
            const std::size_t axisIndex = static_cast<std::size_t>(axis);
            if (axisIndex >= controller.previousAxes.size())
            {
                continue;
            }

            const float axisValue = ApplyDeadzone(controller.previousAxes[axisIndex], deadzone);
            if (std::fabs(axisValue) > std::fabs(value))
            {
                value = axisValue;
            }
        }

        return value;
    }

    void ControllerManager::SetVibration(float lowFrequency, float highFrequency, std::uint32_t durationMs) const
    {
        const float clampedLow = std::clamp(lowFrequency, 0.0f, 1.0f);
        const float clampedHigh = std::clamp(highFrequency, 0.0f, 1.0f);
        const Uint16 low = static_cast<Uint16>(clampedLow * 65535.0f);
        const Uint16 high = static_cast<Uint16>(clampedHigh * 65535.0f);
        bool rumbleSent = false;

        for (const auto& [instanceId, controller] : m_controllers)
        {
            (void)instanceId;
            if (controller.handle == nullptr)
            {
                continue;
            }

            if (SDL_GameControllerRumble(controller.handle, low, high, durationMs) == 0)
            {
                rumbleSent = true;
                continue;
            }

            SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller.handle);
            if (joystick != nullptr && SDL_JoystickRumble(joystick, low, high, durationMs) == 0)
            {
                rumbleSent = true;
                continue;
            }

            if (controller.haptic != nullptr)
            {
                if ((SDL_HapticQuery(controller.haptic) & SDL_HAPTIC_LEFTRIGHT) != 0)
                {
                    SDL_HapticEffect effectUnion{};
                    effectUnion.leftright.type = SDL_HAPTIC_LEFTRIGHT;
                    effectUnion.leftright.length = durationMs;
                    effectUnion.leftright.large_magnitude = low;
                    effectUnion.leftright.small_magnitude = high;

                    if (SDL_HapticEffectSupported(controller.haptic, &effectUnion) == SDL_TRUE)
                    {
                        const int effectId = SDL_HapticNewEffect(controller.haptic, &effectUnion);
                        if (effectId >= 0)
                        {
                            if (SDL_HapticRunEffect(controller.haptic, effectId, 1) == 0)
                            {
                                rumbleSent = true;
                            }

                            SDL_HapticDestroyEffect(controller.haptic, effectId);
                        }
                    }
                }
                
                if (!rumbleSent && SDL_HapticRumbleSupported(controller.haptic) == SDL_TRUE)
                {
                    const float strength = clampedLow > clampedHigh ? clampedLow : clampedHigh;
                    if (SDL_HapticRumblePlay(controller.haptic, strength, durationMs) == 0)
                    {
                        rumbleSent = true;
                    }
                }
            }
        }
    }

    std::size_t ControllerManager::GetConnectedControllerCount() const
    {
        return m_controllers.size();
    }

    void ControllerManager::OpenController(int deviceIndex)
    {
        if (!m_initialized || SDL_IsGameController(deviceIndex) != SDL_TRUE)
        {
            return;
        }

        SDL_GameController* handle = SDL_GameControllerOpen(deviceIndex);
        if (handle == nullptr)
        {
            return;
        }

        SDL_Joystick* joystick = SDL_GameControllerGetJoystick(handle);
        if (joystick == nullptr)
        {
            SDL_GameControllerClose(handle);
            return;
        }

        const SDL_JoystickID instanceId = SDL_JoystickInstanceID(joystick);
        if (m_controllers.find(instanceId) != m_controllers.end())
        {
            SDL_GameControllerClose(handle);
            return;
        }

        ControllerState state;
        state.handle = handle;
        state.instanceId = instanceId;
        state.previousButtons.resize(static_cast<std::size_t>(SDL_CONTROLLER_BUTTON_MAX), 0U);
        state.previousAxes.resize(static_cast<std::size_t>(SDL_CONTROLLER_AXIS_MAX), 0.0f);

        if (SDL_JoystickIsHaptic(joystick) == SDL_TRUE)
        {
            state.haptic = SDL_HapticOpenFromJoystick(joystick);
            if (state.haptic != nullptr && SDL_HapticRumbleSupported(state.haptic) == SDL_TRUE)
            {
                if (SDL_HapticRumbleInit(state.haptic) != 0)
                {
                    SDL_HapticClose(state.haptic);
                    state.haptic = nullptr;
                }
            }
        }

        SnapshotControllerState(state);

        m_controllers.emplace(instanceId, std::move(state));
    }

    void ControllerManager::CloseController(SDL_JoystickID instanceId)
    {
        auto controllerIt = m_controllers.find(instanceId);
        if (controllerIt == m_controllers.end())
        {
            return;
        }

        if (controllerIt->second.handle != nullptr)
        {
            SDL_GameControllerClose(controllerIt->second.handle);
        }

        if (controllerIt->second.haptic != nullptr)
        {
            SDL_HapticClose(controllerIt->second.haptic);
        }

        m_controllers.erase(controllerIt);
    }

    void ControllerManager::SnapshotControllerState(ControllerState& controller)
    {
        if (controller.handle == nullptr)
        {
            return;
        }

        for (std::size_t buttonIndex = 0; buttonIndex < controller.previousButtons.size(); ++buttonIndex)
        {
            controller.previousButtons[buttonIndex] = static_cast<std::uint8_t>(
                SDL_GameControllerGetButton(controller.handle, static_cast<SDL_GameControllerButton>(buttonIndex)));
        }

        for (std::size_t axisIndex = 0; axisIndex < controller.previousAxes.size(); ++axisIndex)
        {
            controller.previousAxes[axisIndex] = NormalizeAxisValue(
                SDL_GameControllerGetAxis(controller.handle, static_cast<SDL_GameControllerAxis>(axisIndex)));
        }
    }

    float ControllerManager::NormalizeAxisValue(Sint16 rawValue)
    {
        if (rawValue >= 0)
        {
            const float normalized = static_cast<float>(rawValue) / 32767.0f;
            return normalized < 1.0f ? normalized : 1.0f;
        }

        const float normalized = static_cast<float>(rawValue) / 32768.0f;
        return normalized > -1.0f ? normalized : -1.0f;
    }

    float ControllerManager::ApplyDeadzone(float value, float deadzone)
    {
        // Remap the usable range back to [0, 1] after removing the deadzone so
        // sticks still reach full magnitude.
        const float clampedDeadzone = std::clamp(deadzone, 0.0f, 0.99f);
        const float magnitude = std::fabs(value);

        if (magnitude <= clampedDeadzone)
        {
            return 0.0f;
        }

        const float scaled = (magnitude - clampedDeadzone) / (1.0f - clampedDeadzone);
        return std::copysign(std::clamp(scaled, 0.0f, 1.0f), value);
    }
}
