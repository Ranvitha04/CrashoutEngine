#pragma once

/*!
All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/

#include "undomanager.h"

namespace CrashoutsEngine
{

    template<typename T>
    class ChangeValueCommand : public EditorCommand {
        T mOldValue, mNewValue;
        std::function<void(const T&)> mSetter;

    public:
        ChangeValueCommand(const T& oldVal, const T& newVal, std::function<void(const T&)> setter)
            : mOldValue(oldVal), mNewValue(newVal), mSetter(std::move(setter)) {
        }

        void Execute() override { mSetter(mNewValue); }
        void Undo() override { mSetter(mOldValue); }
    };

    template <typename T>
    class ValueCommand : public EditorCommand
    {
	    T& mRef, mOldValue, mNewValue;
    public:
        ValueCommand(T& ref, const T& oldVal, const T& newVal)
            : mRef(ref), mOldValue(oldVal), mNewValue(newVal) {
        }

        void Execute() override { mRef = mNewValue; }
        void Undo() override { mRef = mOldValue; }
    };

}
