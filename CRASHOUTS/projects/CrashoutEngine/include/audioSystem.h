/*!
@file        audioSystem.h
@author      b.mohamadafiq@digipen.edu
@date        28/9/2025
@brief       Header file that defines the AudioSystem class and a simple wrapper around FMOD

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
            bool stopAllOnBus(const std::string& busPath, bool fadeOut = true);
            bool stopAll(bool fadeOut = true); 
            FMOD::Studio::System* studio() { return mStudio.get(); }
            FMOD::System* core() { return mCore.get(); }
            bool setBusMute(const std::string& busPath, bool muted);
            bool setBusVolume(const std::string& busPath, float vol01);
            bool stopAndMuteBus(const std::string& busPath, bool fadeOut = true);
            bool setVCAVolume(const std::string& vcaPath, float vol01);
            bool cutVCA(const std::string& vcaPath) { return setVCAVolume(vcaPath, 0.0f); }

            bool listAllEventPaths(std::vector<std::string>& out);
            bool listEventPathsFromBank(const std::string& bankFile, std::vector<std::string>& out);
            void inspectBankToConsole(const std::string& bankFile);
            bool isInitialized() const { return mStudio != nullptr; }


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
