/*!
@file        audioSystem.h
@author      afiq.edu
@date        28/9/2025
@brief       audio

*/
#pragma once
#include <string>
#include <unordered_map>
#include <memory>

#include <fmod_studio.hpp>
#include <fmod.hpp>
#include <fmod_errors.h>

namespace CrashoutsEngine {
    namespace Audio {

        class AudioSystem {
        public:
            static AudioSystem& instance();

            bool init(const std::string& banksDir = "assets/audio/banks");
            void update();
            void shutdown();

            bool loadBank(const std::string& bankFile);
            void unloadAllBanks();

            bool playOneShot(const std::string& eventPath);
            FMOD::Studio::EventInstance* createInstance(const std::string& eventPath);

            FMOD::Studio::System* studio() { return mStudio.get(); }
            FMOD::System* core() { return mCore.get(); }

        private:
            AudioSystem() = default;
            ~AudioSystem() = default;
            AudioSystem(const AudioSystem&) = delete;
            AudioSystem& operator=(const AudioSystem&) = delete;

            struct StudioDeleter {
                void operator()(FMOD::Studio::System* p) const {
                    if (p) { p->unloadAll(); p->release(); }
                }
            };
            struct CoreNoop { void operator()(FMOD::System*) const {} };

            std::unique_ptr<FMOD::Studio::System, StudioDeleter> mStudio;
            std::unique_ptr<FMOD::System, CoreNoop>              mCore;

            std::string mBanksDir;
            std::unordered_map<std::string, FMOD::Studio::Bank*> mBanks;

            FMOD::Studio::EventDescription* getEventDesc(const std::string& eventPath);
            static bool ok(FMOD_RESULT r, const char* where);
        };

    }
} 
