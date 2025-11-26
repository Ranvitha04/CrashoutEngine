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
#include <vector>

#include <fmod_studio.hpp>
#include <fmod.hpp>
#include <fmod_errors.h>

namespace CrashoutsEngine
{
    namespace Audio
    {
        class AudioSystem
        {
        public:
            // Singleton access
            static AudioSystem& instance();

            // Lifetime
            bool init(const std::string& banksDir = "assets/audio/banks");
            void update();
            void shutdown();

            // Bank management
            bool loadBank(const std::string& bankFile);
            void unloadAllBanks();

			// Raw Management
            void registerRawAudio(const std::string& relPath);
            const std::vector<std::string>& getRegisteredRawAudio() const;
			void unregisterRawAudio(const std::string& relPath);
            void unloadRawSound(const std::string& path);
            void unloadAllRawSounds();

            // Event-based (Studio) playback
            bool playOneShot(const std::string& eventPath);
            FMOD::Studio::EventInstance* createInstance(const std::string& eventPath);

            // Bus / group controls
            bool stopAllOnBus(const std::string& busPath, bool fadeOut = true);
            void stopAllRaw(); // Stop only raw (music/sfx) channel groups
            bool stopAll(bool fadeOut = true); // Stop everything on master bus

            bool stopMusicGroup();
            bool stopSFXGroup();

            FMOD::Studio::System* studio() { return mStudio.get(); }
            FMOD::System* core() { return mCore.get(); }

            bool setBusMute(const std::string& busPath, bool muted);
            bool setBusVolume(const std::string& busPath, float vol01);
            bool stopAndMuteBus(const std::string& busPath, bool fadeOut = true);

            // VCAs
            bool setVCAVolume(const std::string& vcaPath, float vol01);
            bool cutVCA(const std::string& vcaPath) { return setVCAVolume(vcaPath, 0.0f); }

            // Introspection / debug helpers
            bool listAllEventPaths(std::vector<std::string>& out);
            bool listEventPathsFromBank(const std::string& bankFile, std::vector<std::string>& out);
            void inspectBankToConsole(const std::string& bankFile);
			void listRawAudioFiles(std::vector<std::string>& out);

            bool isInitialized() const { return mStudio != nullptr; }

            // File-based playback (Core API)
            FMOD::Channel* playMusicFile(const std::string& path, bool loop = true);
            FMOD::Channel* playSFXFile(const std::string& path);
            FMOD::Channel* playSFXFileEx(const std::string& path);

            bool setMusicGroupVolume(float v01);
            bool setSFXGroupVolume(float v01);
            bool setMasterVolume(float v01);

            // Tagged playback for Unity-like Play/Stop("Name")
            // Thin helpers so C# AudioManager can track and stop by name.
            bool playMusicFileTagged(const std::string& key, const std::string& path, bool loop = true);

            bool playSFXFileTagged(const std::string& key,
                const std::string& path, bool loop);

            bool stopTagged(const std::string& key);

            void setPaused(bool paused);

        private:
            AudioSystem() = default;
            ~AudioSystem() = default;
            AudioSystem(const AudioSystem&) = delete;
            AudioSystem& operator=(const AudioSystem&) = delete;

            // Channel groups for raw music/SFX files
            FMOD::ChannelGroup* mMusicGroup = nullptr;
            FMOD::ChannelGroup* mSFXGroup = nullptr;

            // Lifetime managers for Studio/Core systems
            struct StudioDeleter
            {
                void operator()(FMOD::Studio::System* p) const
                {
                    if (p)
                    {
                        p->unloadAll();
                        p->release();
                    }
                }
            };
            struct CoreNoop
            {
                void operator()(FMOD::System* /*p*/) const {}
            };

            std::unique_ptr<FMOD::Studio::System, StudioDeleter> mStudio;
            std::unique_ptr<FMOD::System, CoreNoop>              mCore;

            std::string mBanksDir;
            std::unordered_map<std::string, FMOD::Studio::Bank*> mBanks;

            std::vector<std::string> mRawAudioFiles;
            std::unordered_map<std::string, FMOD::Sound*> mRawSoundCache;

            //For tagged playback (Unity-style Stop("Name"))
            std::unordered_map<std::string, FMOD::Channel*> mTaggedChannels;

            // Helpers
            FMOD::Studio::EventDescription* getEventDesc(const std::string& eventPath);
            static bool ok(FMOD_RESULT r, const char* where);
        };

    } // namespace Audio
} // namespace CrashoutsEngine