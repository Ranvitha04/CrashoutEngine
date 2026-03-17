#include "ControllerManager.h"

#include <algorithm>
#include <cmath>
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
        if ((SDL_WasInit(SDL_INIT_EVENTS) & SDL_INIT_EVENTS) == 0U)
        {
            if (SDL_InitSubSystem(SDL_INIT_EVENTS) != 0)
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
            return std::min(static_cast<float>(rawValue) / 32767.0f, 1.0f);
        }

        return std::max(static_cast<float>(rawValue) / 32768.0f, -1.0f);
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
