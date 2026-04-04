/*!
@file        audioSystem.h
@author      Mohamad Amir Akmal
@author      mohamadamirakmal.b
@author      Renfred Lee
@date        28/9/2025
@brief       Header file that defines the AudioSystem class and a simple wrapper around FMOD

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <list>
#include <utility>

#include <fmod_studio.hpp>
#include <fmod.hpp>
#include <fmod_errors.h>

#include "CEVec3.h"

namespace CrashoutsEngine
{
    namespace Audio
    {
        /** @brief AudioSystem */
        class AudioSystem
        {
        public:
            // Singleton access
            static AudioSystem& instance();

            // Lifetime
            bool init(const std::string& banksDir);
            void Update(float dt);
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
            void stopAllChannels();

            bool stopMusicGroup();
            bool stopSFXGroup();
            void stopChannel(FMOD::Channel* channel, float fadeSeconds);

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
            std::vector<std::string> listTaggedKeys() const;

            bool isInitialized() const { return mStudio != nullptr; }

            // File-based playback (Core API)
            FMOD::Channel* playMusicFile(const std::string& path, bool loop = true);
            FMOD::Channel* playSFXFile(const std::string& path, bool spatial3D, float minDistance, float maxDistance);

            // Setters
            bool setMusicGroupVolume(float v01);
            bool setSFXGroupVolume(float v01);
            bool setMasterVolume(float v01);
            bool setTaggedVolume(const std::string& key, float vol);
            void SetChannelVolume(FMOD::Channel* channel, float volume) {
                if (channel) channel->setVolume(volume);
            }
            void SetChannel3DAttributes(FMOD::Channel* channel, const CEVec3& position, float minDistance, float maxDistance);
            void SetListenerAttributes(const CEVec3& position, const CEVec3& forward, const CEVec3& up, const CEVec3& velocity = { 0.0f, 0.0f, 0.0f });
            void AddChannelDSP(FMOD::Channel* channel, FMOD_DSP_TYPE dspType);
            void RemoveChannelDSP(FMOD::Channel* channel, FMOD_DSP_TYPE dspType);
            void SetChannelDSPParameter(FMOD::Channel* channel, FMOD_DSP_TYPE dspType, int index, float value);

            void AddGroupDSP(bool isMusic, FMOD_DSP_TYPE dspType);
            void RemoveGroupDSP(bool isMusic, FMOD_DSP_TYPE dspType);
            void SetGroupDSPParameter(bool isMusic, FMOD_DSP_TYPE dspType, int index, float value);
            void FadeGroupDSPParameter(bool isMusic, FMOD_DSP_TYPE dspType, int index, float targetValue, float duration);
            void FadeGroupVolume(bool isMusic, float targetVolume, float duration);
            void FadeMasterVolume(float targetVolume, float duration);

            // Getters
            float GetMusicVolume() const;
            float GetSFXVolume() const;
            float GetMasterVolume() const;
            bool getTaggedVolume(const std::string& key, float& outVol) const;

            bool fadeTagged(const std::string& key, float seconds);
            void FadeTaggedVolume(const std::string& key, float targetVolume, float duration);

            // Tagged playback
            bool playMusicFileTagged(const std::string& key, const std::string& path, bool loop = true);

            bool playSFXFileTagged(const std::string& key, const std::string& path, bool loop);

            bool isTaggedPlaying(const std::string& key);

            bool stopTagged(const std::string& key, float fadeOutSeconds = 0.0f);
            void setPausedTagged(const std::string& key, bool paused);
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

            /** @brief CoreNoop */
            struct CoreNoop
            {
                void operator()(FMOD::System* /*p*/) const {}
            };

            /** @brief FadingChannel */
            struct FadingChannel
            {
                FMOD::Channel* channel;
                float currentVol;
                float fadeRate;
            };

            /** @brief DSPParameterFade */
            struct DSPParameterFade
            {
                FMOD::DSP* dsp;
                int index;
                float startValue;
                float endValue;
                float currentTime;
                float totalTime;
            };

            /** @brief GroupVolumeFade */
            struct GroupVolumeFade
            {
                FMOD::ChannelGroup* group;
                float startVolume;
                float endVolume;
                float currentTime;
                float totalTime;
            };

            /** @brief ChannelVolumeFade */
            struct ChannelVolumeFade
            {
                FMOD::Channel* channel;
                float startVolume;
                float endVolume;
                float currentTime;
                float totalTime;
            };

            /** @brief BusVolumeFade */
            struct BusVolumeFade
            {
                FMOD::Studio::Bus* bus;
                float startVolume;
                float endVolume;
                float currentTime;
                float totalTime;
            };

            std::vector<FadingChannel> mFadingChannels;
            std::list<DSPParameterFade> mParameterFades;
            std::list<GroupVolumeFade> mGroupVolumeFades;
            std::list<ChannelVolumeFade> mChannelVolumeFades;
            std::list<BusVolumeFade> mBusVolumeFades;

            std::unique_ptr<FMOD::Studio::System, StudioDeleter> mStudio;
            std::unique_ptr<FMOD::System, CoreNoop>              mCore;

            std::string mBanksDir;
            std::unordered_map<std::string, FMOD::Studio::Bank*> mBanks;

            std::vector<std::string> mRawAudioFiles;
            std::unordered_map<std::string, std::string> mSoundPaths;
            std::unordered_map<std::string, FMOD::Sound*> mRawSoundCache;

            //For tagged playback
            std::unordered_map<std::string, FMOD::Channel*> mTaggedChannels;

            std::list<std::pair<FMOD::Channel*, FMOD::Sound*>> mActiveSounds;
            std::unordered_map<FMOD::Channel*, std::unordered_map<FMOD_DSP_TYPE, FMOD::DSP*>> mChannelDSPs;
            std::unordered_map<FMOD::ChannelGroup*, std::unordered_map<FMOD_DSP_TYPE, FMOD::DSP*>> mGroupDSPs;

            // Helpers
            static std::string SanitizeKey(const std::string& path);
            FMOD::Studio::EventDescription* getEventDesc(const std::string& eventPath);
            FMOD::Studio::Bus* GetStudioBus(bool isMusic);
            static bool ok(FMOD_RESULT r, const char* where);
        };

    } // namespace Audio
} // namespace CrashoutsEngine