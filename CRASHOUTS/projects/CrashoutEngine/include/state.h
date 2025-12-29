/*!
@file       state.h
@author     miratristan.d@digipen.edu
@date       31/10/2025
@brief      Declaration for the template class of the FiniteStateMachine. This is used to provide a way to streamline creating a state machine for entities

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

#include <map>
#include <string>
#include <optional>
#include <functional>
#include "CETypes.h"

namespace State {

    using StateType = CE::s32; /*Used for ease(replacement for enums)*/

    template <typename T>
    class FiniteStateMachine;

    template <typename T>
    class State;

    template<typename T>
    class StateWrapper {
        protected:
            StateWrapper(T mID) : mID{ mID } {}

            std::function<void()> enter;
            std::function<void()> Update;
            std::function<void()> exit;

        public:
            virtual operator State<T>() {
                State<T> converted{mID};

                converted.enter = std::move(enter);
                converted.Update = std::move(Update);
                converted.exit = std::move(exit);

                return converted;
            }

            T mID;
    };

    template <typename T>
    class State {
    public:
        inline T GetID() { return mID; }
        inline T GetID() const { return mID; }

        inline const std::string& getName() const { return mName; }

        State() = default;

        explicit State(T id, std::string name = "default") : mName(name), mID(id) {}

        ~State() = default;

        bool operator==(const State<T>& source2) const {
            return mID == source2.mID;
        }

        std::function<void()> enter;
        std::function<void()> Update;
        std::function<void()> exit;

    private:
        std::string mName;
        T mID;
    };

    template <typename T>
    class FiniteStateMachine {
    private:
        std::map<T, State<T>> mStates;

        std::optional<State<T>> mCurrentState;

        void SetCurrentState(State<T>& state) {
            if (mCurrentState == state) {
                return;
            }

            if (mCurrentState) {
                mCurrentState->exit();
            }

            mCurrentState = state;

            if (mCurrentState) {
                mCurrentState->enter();
            }
        }

    public:
        FiniteStateMachine() : mCurrentState{ std::nullopt }, mStates{} {}

        State<T>& Add(State<T> const& state) {
            mStates[state.GetID()] = state;
            return mStates[state.GetID()];
        }

        State<T>& GetState(T stateID) {
            return mStates[stateID];
        }

        State<T>& GetCurrentState() {
            return mCurrentState.value();
        }

        const State<T>& GetCurrentState() const {
            return mCurrentState.value();
        }

        void SetCurrentState(T stateID) {
            State<T>& state = GetState(stateID);
            SetCurrentState(state);
        }

        void Update() {
            if (mCurrentState) {
                mCurrentState->Update();
            }
        }
    };
}