#pragma once
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

}
