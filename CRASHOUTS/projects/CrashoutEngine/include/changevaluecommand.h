/*!
@file   changevaluecommand.h
@author Shahrin Idzuan (37%), m.biniswandi (31%), Ranvitha (32%)
@date   29/11/2025

@brief  command to change a value with undo/redo support

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once
#include "undomanager.h"

namespace CrashoutsEngine
{

    template<typename T>
    /** @brief ChangeValueCommand */
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
    /** @brief ValueCommand */
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

