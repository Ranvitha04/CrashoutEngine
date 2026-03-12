/*!
@file        audioSystem.cpp
@author      b.mohamadafiq@digipen.edu (70%)
@co-auther   mohamadamirakmal.b@digipen.edu (30%)
@date        28/9/2025
@brief      Minimal FMOD Studio wrapper for the CrashoutsEngine.
             - Creates/initializes FMOD Studio/Core systems
             - Loads/unloads bank and raw files
             - Plays one-shot events (fire-and-forget)
			 - Plays tagged audio files (music/sfx)
			 - Stops audio (all/tagged)
             - Creates long-lived event instances (music/loops)
             - Controls buses (stop/mute/set volume)
             - Updates FMOD every frame
             The class is a singleton accessed via AudioSystem::instance().

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "audioSystem.h"
#include "log.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <cstdio>
#include <algorithm>
#include <filesystem>

namespace CrashoutsEngine::Audio
{
    // Small helpers
    /*!
    @brief Joins two path segments using the correct platform-specific separator.
    */
    static std::string joinPath(const std::string& a, const std::string& b)
    {
#ifdef _WIN32
        const char sep = '\\';
#else
        const char sep = '/';
#endif
        if (a.empty())
            return b;
        if (!a.empty() && (a.back() == '\\' || a.back() == '/'))
            return a + b;
        return a + sep + b;
    }

    static bool IsSupportedAudioFile(const std::string& path)
    {
        static const std::vector<std::string> allowed = {
            ".mp3",
            ".ogg",
            ".wav",
            ".mp4"
        };

        auto dot = path.find_last_of('.');
        if (dot == std::string::npos)
            return false;

        std::string ext = path.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });

        for (const auto& a : allowed)
            if (ext == a)
                return true;

        return false;
    }

    /*
        @brief Resolves an audio file path by checking its existence directly and relative to a root directory.
    */
    std::string AudioSystem::SanitizeKey(const std::string& path)
    {
        std::string clean = path;

        // 1. Force Forward Slashes
        std::replace(clean.begin(), clean.end(), '\\', '/');

        // 2. Force Lowercase (Makes it case-insensitive)
        std::transform(clean.begin(), clean.end(), clean.begin(),
            [](unsigned char c) { return std::tolower(c); });

        // 3. Strip everything before "audio/" if present
        const std::string marker = "audio/";
        size_t pos = clean.rfind(marker);
        if (pos != std::string::npos) {
            clean = clean.substr(pos + marker.size());
        }

        // 4. Safety: If it started with "assets/" but no "audio/" (rare)
        if (clean.rfind("assets/", 0) == 0) {
            clean = clean.substr(7);
        }

        // 5. Trim leading slashes
        while (!clean.empty() && clean.front() == '/') {
            clean.erase(0, 1);
        }

        return clean;
    }

    /*
		@brief Finds a valid audio directory path by checking the suggested path and common alternatives.
    */
    static std::string FindValidAudioPath(const std::string& suggestedPath) {
        namespace fs = std::filesystem;

        // 1. If the suggested path works, trust it immediately.
        if (fs::exists(suggestedPath) && fs::is_directory(suggestedPath)) {
            return suggestedPath;
        }

        std::cout << "[Audio] Warning: Suggested path '" << suggestedPath << "' not found. Searching...\n";

        // 2. List of places to check (prioritized)
        std::vector<std::string> candidates = {
            "../../projects/Descent/assets/audio/",  // Editor Default
            "assets/audio/",                         // Release / Standalone
            "../assets/audio/",                      // One folder up
            "../../../assets/audio/"                 // Deep folder structure
        };

        for (const auto& path : candidates) {
            if (fs::exists(path) && fs::is_directory(path)) {
                std::cout << "[Audio] Found audio folder at: " << fs::absolute(path) << "\n";
                return path;
            }
        }

        // 3. Give up
        std::cout << "[Audio] ERROR: Could not find 'audio' folder anywhere!\n";
        return suggestedPath; // Return original so error logs show what was expected
    }

    // Singleton & error helper
    /*!
    @brief Returns the global AudioSystem singleton instance.
    */
    AudioSystem& AudioSystem::instance()
    {
        static AudioSystem s;
        return s;
    }

    /*!
    @brief Logs an FMOD error message and returns false if the result is not FMOD_OK.
    */
    bool AudioSystem::ok(FMOD_RESULT r, const char* where)
    {
        if (r != FMOD_OK)
        {
            std::fprintf(stderr, "[FMOD] %s -> %d (%s)\n",
                where, r, FMOD_ErrorString(r));
            return false;
        }
        return true;
    }

    // Lifetime
    /*!
    @brief Initializes the FMOD Studio and Core systems and sets up channel groups for music and SFX.
    */
    bool AudioSystem::init(const std::string& banksDir)
    {
        // Auto-Detect the real path
        // This ignores whatever broken path engine.cpp might send if it doesn't exist.
        mBanksDir = FindValidAudioPath(banksDir);

        std::cout << "[Audio] Initializing FMOD...\n";
        std::cout << "[Audio] Using Root Path: " << mBanksDir << "\n";

        // Create Studio system
        FMOD::Studio::System* rawStudio = nullptr;
        if (!ok(FMOD::Studio::System::create(&rawStudio), "System::create")) return false;
        mStudio.reset(rawStudio);

        if (!ok(mStudio->initialize(1024, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr), "System::initialize")) return false;

        FMOD::System* rawCore = nullptr;
        if (!ok(mStudio->getCoreSystem(&rawCore), "getCoreSystem")) return false;
        mCore.reset(rawCore);

        // Channel Groups
        FMOD::ChannelGroup* master = nullptr;
        ok(mCore->getMasterChannelGroup(&master), "getMasterChannelGroup");
        ok(mCore->createChannelGroup("Music", &mMusicGroup), "createChannelGroup Music");
        ok(mCore->createChannelGroup("SFX", &mSFXGroup), "createChannelGroup SFX");

        if (master) {
            master->addGroup(mMusicGroup);
            master->addGroup(mSFXGroup);
        }

        // Configure 3D settings for standard distance attenuation
        // Doppler Scale, Distance Factor (1 unit = 1 meter), Roll-off Scale
        mCore->set3DSettings(1.0f, 1.0f, 1.0f);

        // Scan Directory
        try {
            namespace fs = std::filesystem;
            // Use the verified mBanksDir
            fs::path audioRoot = mBanksDir;

            if (fs::exists(audioRoot)) {
                int count = 0;
                for (auto& e : fs::recursive_directory_iterator(audioRoot)) {
                    if (!e.is_regular_file()) continue;

                    std::string diskPath = e.path().generic_string();
                    if (!IsSupportedAudioFile(diskPath)) continue;

                    std::string key = SanitizeKey(diskPath);

                    if (std::find(mRawAudioFiles.begin(), mRawAudioFiles.end(), key) == mRawAudioFiles.end()) {
                        mRawAudioFiles.push_back(key);
                    }

                    // Lazy Loading: Just store the path for now, don't create the sound yet.
                    mSoundPaths[key] = diskPath;
                    count++;
                }
                std::cout << "[Audio] Scanned " << count << " files for lazy loading.\n";
            }
        }
        catch (...) { std::cout << "[Audio] Scan Error.\n"; }

        return true;
    }

    void AudioSystem::Update(float dt)
    {
        if (mStudio)
            mStudio->update();

        auto fit = mFadingChannels.begin();
        while (fit != mFadingChannels.end())
        {
            bool isPlaying = false;
            fit->channel->isPlaying(&isPlaying);

            if (!isPlaying)
            {
                fit = mFadingChannels.erase(fit);
                continue;
            }

            fit->currentVol -= fit->fadeRate * dt;

            if (fit->currentVol <= 0.0f)
            {
                fit->channel->stop();
                fit = mFadingChannels.erase(fit);
            }
            else
            {
                fit->channel->setVolume(fit->currentVol);
                ++fit;
            }
        }

        // --- Update Channel Volume Fades ---
        auto channelVolFit = mChannelVolumeFades.begin();
        while (channelVolFit != mChannelVolumeFades.end())
        {
            channelVolFit->currentTime += dt;
            float t = channelVolFit->currentTime / channelVolFit->totalTime;
            if (t >= 1.0f) t = 1.0f;

            float currentVol = channelVolFit->startVolume + (channelVolFit->endVolume - channelVolFit->startVolume) * t;
            channelVolFit->channel->setVolume(currentVol);

            if (t >= 1.0f) {
                channelVolFit = mChannelVolumeFades.erase(channelVolFit);
            }
            else {
                ++channelVolFit;
            }
        }

        // --- Update Bus Volume Fades ---
        auto busVolFit = mBusVolumeFades.begin();
        while (busVolFit != mBusVolumeFades.end())
        {
            busVolFit->currentTime += dt;
            float t = busVolFit->currentTime / busVolFit->totalTime;
            if (t >= 1.0f) t = 1.0f;

            float currentVol = busVolFit->startVolume + (busVolFit->endVolume - busVolFit->startVolume) * t;
            busVolFit->bus->setVolume(currentVol);

            if (t >= 1.0f) {
                busVolFit = mBusVolumeFades.erase(busVolFit);
            }
            else {
                ++busVolFit;
            }
        }

        // --- Update DSP Parameter Fades ---
        auto dspFit = mParameterFades.begin();
        while (dspFit != mParameterFades.end())
        {
            dspFit->currentTime += dt;
            float t = dspFit->currentTime / dspFit->totalTime;
            if (t >= 1.0f) t = 1.0f;

            float currentValue = dspFit->startValue + (dspFit->endValue - dspFit->startValue) * t;
            dspFit->dsp->setParameterFloat(dspFit->index, currentValue);

            if (t >= 1.0f) {
                dspFit = mParameterFades.erase(dspFit);
            } else {
                ++dspFit;
            }
        }

        // --- Update Group Volume Fades ---
        auto groupVolFit = mGroupVolumeFades.begin();
        while (groupVolFit != mGroupVolumeFades.end())
        {
            groupVolFit->currentTime += dt;
            float t = groupVolFit->currentTime / groupVolFit->totalTime;
            if (t >= 1.0f) t = 1.0f;

            float currentVol = groupVolFit->startVolume + (groupVolFit->endVolume - groupVolFit->startVolume) * t;
            groupVolFit->group->setVolume(currentVol);

            if (t >= 1.0f) {
                groupVolFit = mGroupVolumeFades.erase(groupVolFit);
            } else {
                ++groupVolFit;
            }
        }

        auto it = mActiveSounds.begin();
        while (it != mActiveSounds.end())
        {
            FMOD::Channel* ch = it->first;
            FMOD::Sound* snd = it->second;

            bool isPlaying = false;
            ch->isPlaying(&isPlaying);

            if (!isPlaying)
            {
                // Remove this channel from mTaggedChannels if it exists
                for (auto mapIt = mTaggedChannels.begin(); mapIt != mTaggedChannels.end(); )
                {
                    if (mapIt->second == ch) {
                        mapIt = mTaggedChannels.erase(mapIt);
                    }
                    else {
                        ++mapIt;
                    }
                }

                // Cleanup DSPs for this channel
                auto dspIt = mChannelDSPs.find(ch);
                if (dspIt != mChannelDSPs.end()) {
                    for (auto& pair : dspIt->second) {
                        pair.second->release();
                    }
                    mChannelDSPs.erase(dspIt);
                }

                // release only the sounds we created temporarily (those tracked in mActiveSounds)
                snd->release();
                it = mActiveSounds.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    /*!
    @brief Shuts down the audio system, unloading banks and releasing FMOD resources.
    */
    void AudioSystem::shutdown()
    {
        // Stop any tagged channels we tracked
        mTaggedChannels.clear();
        std::unordered_map<std::string, FMOD::Channel*>().swap(mTaggedChannels);

        unloadAllRawSounds();
        std::unordered_map<std::string, FMOD::Sound*>().swap(mRawSoundCache);

        unloadAllBanks();
        std::unordered_map<std::string, FMOD::Studio::Bank*>().swap(mBanks);

        mRawAudioFiles.clear();
        std::vector<std::string>().swap(mRawAudioFiles);

        // Cleanup Group DSPs
        for (auto& groupPair : mGroupDSPs) {
            for (auto& dspPair : groupPair.second) {
                if (dspPair.second) dspPair.second->release();
            }
        }
        mGroupDSPs.clear();

        if (mStudio) mStudio->release();
        mStudio.reset();

        if (mCore) mCore->release();
        mCore.reset();

        mMusicGroup = nullptr;
        mSFXGroup = nullptr;
    }

    // Bank management
    /*!
    @brief Loads a FMOD Studio bank file from the configured banks directory.
    */
    bool AudioSystem::loadBank(const std::string& bankFile)
    {
        if (!mStudio)
            return false;

        // Already loaded check
        if (mBanks.count(bankFile))
            return true;

        std::string fullPath;
        std::filesystem::path p(bankFile);

        if (p.is_absolute()) {
            fullPath = bankFile;
        }
        else {
            fullPath = joinPath(mBanksDir, bankFile);
        }

        FMOD::Studio::Bank* b = nullptr;

        // Load using the corrected path
        if (!ok(mStudio->loadBankFile(fullPath.c_str(),
            FMOD_STUDIO_LOAD_BANK_NORMAL,
            &b),
            ("loadBankFile " + bankFile).c_str()))
        {
            return false;
        }

        mBanks.insert(std::make_pair(bankFile, b));

        if (mStudio)
            mStudio->flushCommands();

        return true;
    }

    /*!
    @brief Unloads all currently loaded FMOD Studio banks.
    */
    void AudioSystem::unloadAllBanks()
    {
        for (auto& kv : mBanks)
        {
            FMOD::Studio::Bank* bank = kv.second;
            if (bank)
                bank->unload();
        }
        mBanks.clear();
    }

    // Raw management
    void AudioSystem::registerRawAudio(const std::string& relPath)
    {
        // Normalize incoming path and store as path relative to the audio root, e.g. "Music/Menu.wav"
        std::string p = relPath;
        // normalize separators
        std::replace(p.begin(), p.end(), '\\', '/');

        // Strip "assets/" or "assets/audio/" or leading "audio/" if present
        if (p.rfind("assets/audio/", 0) == 0) {
            p = p.substr(strlen("assets/audio/"));
        }
        else if (p.rfind("assets/", 0) == 0) {
            // if someone registered "assets/..." treat as under audio
            std::string tmp = p.substr(strlen("assets/"));
            if (tmp.rfind("audio/", 0) == 0) tmp = tmp.substr(strlen("audio/"));
            p = tmp;
        }
        else if (p.rfind("audio/", 0) == 0) {
            p = p.substr(strlen("audio/"));
        }

        // Trim any leading slashes
        while (!p.empty() && (p.front() == '/' || p.front() == '\\')) p.erase(p.begin());

        std::string fullPath = joinPath(mBanksDir, p);
        FMOD::Sound* snd = nullptr;
        FMOD_RESULT r = mCore->createSound(
            fullPath.c_str(),
            FMOD_DEFAULT | FMOD_2D,
            nullptr,
            &snd
        );

        if (r == FMOD_OK && snd) {
            std::string cacheKey = SanitizeKey(p);
            mRawSoundCache[cacheKey] = snd;  // Add to cache
            mRawAudioFiles.push_back(cacheKey);
        }
    }

    const std::vector<std::string>& AudioSystem::getRegisteredRawAudio() const
    {
        return mRawAudioFiles;
    }

    void AudioSystem::unregisterRawAudio(const std::string& relPath)
    {
        mRawAudioFiles.erase(
            std::remove(mRawAudioFiles.begin(), mRawAudioFiles.end(), relPath),
            mRawAudioFiles.end()
        );
    }

    void AudioSystem::unloadRawSound(const std::string& path)
    {
        auto it = mRawSoundCache.find(path);
        if (it != mRawSoundCache.end())
        {
            if (it->second)
                it->second->release();

            mRawSoundCache.erase(it);

            std::cout << "[Audio] Unloaded raw sound: " << path << "\n";
        }
    }

    void AudioSystem::unloadAllRawSounds()
    {
        for (auto& kv : mRawSoundCache)
        {
            if (kv.second)
                kv.second->release();
        }

        mRawSoundCache.clear();
        std::cout << "[Audio] Unloaded ALL raw sounds\n";
    }

    // Bus / group controls
    /*!
    @brief Stops all events playing on the specified FMOD Studio bus.
    */
    bool AudioSystem::stopAllOnBus(const std::string& busPath, bool fadeOut)
    {
        if (!mStudio)
            return false;

        FMOD::Studio::Bus* bus = nullptr;
        FMOD_RESULT r = mStudio->getBus(busPath.c_str(), &bus);
        ok(r, "getBus");
        if (r != FMOD_OK || !bus)
            return false;

        r = bus->stopAllEvents(fadeOut
            ? FMOD_STUDIO_STOP_ALLOWFADEOUT
            : FMOD_STUDIO_STOP_IMMEDIATE);
        ok(r, "stopAllEvents");

        if (mStudio)
            mStudio->flushCommands();

        return r == FMOD_OK;
    }

    /*!
    @brief Stops all sounds playing on the raw music and SFX channel groups.
    */
    void AudioSystem::stopAllRaw()
    {
        if (mMusicGroup)
            mMusicGroup->stop();
        if (mSFXGroup)
            mSFXGroup->stop();

        // Tagged channels are no longer valid.
        mTaggedChannels.clear();
        mFadingChannels.clear();
        mChannelVolumeFades.clear();
        mGroupVolumeFades.clear();
        mBusVolumeFades.clear();
    }

    /*!
    @brief Stops all events on the master bus, including music and SFX.
    */
    bool AudioSystem::stopAll(bool fadeOut)
    {
        if (!fadeOut) {
            stopAllRaw();
        }
        // Master bus stops *everything* (music + sfx)
        return stopAllOnBus("bus:/", fadeOut);
    }
    
    /*
    @brief Stopping this group halts all Core audio immediately
    */
    void AudioSystem::stopAllChannels()
    {
        if (!mCore) return;

        FMOD::ChannelGroup* master = nullptr;
        if (mCore->getMasterChannelGroup(&master) == FMOD_OK && master)
        {
            master->stop();
        }

        // Clean up tracked channels so the system knows they are dead
        mTaggedChannels.clear();
        mFadingChannels.clear();
        mGroupVolumeFades.clear();
        mParameterFades.clear();
    }

    /*!
    @brief Stops all sounds currently playing on the music channel group.
    */
    bool AudioSystem::stopMusicGroup()
    {
        if (!mMusicGroup)
            return false;
        return mMusicGroup->stop() == FMOD_OK;
    }

	/*!
    @brief Stops a specific FMOD channel, optionally fading out over a specified duration.
	*/
    void AudioSystem::stopChannel(FMOD::Channel* channel, float fadeSeconds)
    {
        if (!channel) return;

        bool playing = false;
        channel->isPlaying(&playing);
        if (!playing) return;

        if (fadeSeconds <= 0.0f)
        {
            // Immediate stop (Hard cut)
            channel->stop();
        }
        else
        {
            // Reuse the same fading logic you already have in stopTagged!
            float currentVol = 1.0f;
            channel->getVolume(&currentVol);

            FadingChannel fc;
            fc.channel = channel;
            fc.currentVol = currentVol;
            fc.fadeRate = currentVol / fadeSeconds; // Calculate rate to hit 0 in X seconds

            mFadingChannels.push_back(fc);
        }
    }

    /*!
    @brief Stops all sounds currently playing on the SFX channel group.
    */
    bool AudioSystem::stopSFXGroup()
    {
        if (!mSFXGroup)
            return false;
        return mSFXGroup->stop() == FMOD_OK;
    }

    /*!
    @brief Sets the volume of a FMOD Studio bus identified by its path.
    */
    bool AudioSystem::setBusVolume(const std::string& busPath, float vol01)
    {
        if (!mStudio)
            return false;

        FMOD::Studio::Bus* bus = nullptr;
        FMOD_RESULT result = mStudio->getBus(busPath.c_str(), &bus);
        ok(result, "getBus");
        if (result != FMOD_OK || !bus)
            return false;

        result = bus->setVolume(vol01);
        ok(result, "Bus::setVolume");

        if (mStudio)
            mStudio->flushCommands();

        return result == FMOD_OK;
    }

    /*!
    @brief Sets the volume of a VCA identified by its FMOD path.
    */
    bool AudioSystem::setVCAVolume(const std::string& vcaPath, float vol01)
    {
        if (!mStudio)
            return false;

        FMOD::Studio::VCA* vca = nullptr;
        FMOD_RESULT r = mStudio->getVCA(vcaPath.c_str(), &vca);
        ok(r, "getVCA");
        if (r != FMOD_OK || !vca)
            return false;

        r = vca->setVolume(vol01);
        ok(r, "VCA::setVolume");

        if (mStudio)
            mStudio->flushCommands();

        return r == FMOD_OK;
    }

    /*!
    @brief Mutes or unmutes a FMOD Studio bus.
    */
    bool AudioSystem::setBusMute(const std::string& busPath, bool muted)
    {
        if (!mStudio)
            return false;

        FMOD::Studio::Bus* bus = nullptr;
        FMOD_RESULT result = mStudio->getBus(busPath.c_str(), &bus);
        ok(result, "getBus");
        if (result != FMOD_OK || !bus)
            return false;

        result = bus->setMute(muted);
        ok(result, "Bus::setMute");

        if (mStudio)
            mStudio->flushCommands();

        return result == FMOD_OK;
    }

    /*!
    @brief Stops all events on a bus and then mutes it.
    */
    bool AudioSystem::stopAndMuteBus(const std::string& busPath, bool fadeOut)
    {
        // Convenience: stop all events on the bus, then mute it.
        if (!stopAllOnBus(busPath, fadeOut))
            return false;
        return setBusMute(busPath, true);
    }

    // Event-based (Studio) playback
    /*!
    @brief Retrieves the FMOD event description for a given event path.
    */
    FMOD::Studio::EventDescription* AudioSystem::getEventDesc(const std::string& path)
    {
        if (!mStudio)
            return nullptr;

        FMOD::Studio::EventDescription* d = nullptr;
        if (!ok(mStudio->getEvent(path.c_str(), &d), "getEvent"))
            return nullptr;
        return d;
    }

    /*!
    @brief Plays a one-shot FMOD Studio event and immediately releases the instance.
    */
    bool AudioSystem::playOneShot(const std::string& eventPath)
    {
        auto* d = getEventDesc(eventPath);
        if (!d)
            return false;

        FMOD::Studio::EventInstance* inst = nullptr;
        if (!ok(d->createInstance(&inst), "createInstance"))
            return false;

        ok(inst->start(), "EventInstance::start");
        ok(inst->release(), "EventInstance::release");
        return true;
    }

    /*!
    @brief Creates a FMOD Studio event instance for long-lived playback or manual control.
    */
    FMOD::Studio::EventInstance* AudioSystem::createInstance(const std::string& eventPath)
    {
        auto* d = getEventDesc(eventPath);
        if (!d)
            return nullptr;

        FMOD::Studio::EventInstance* inst = nullptr;
        if (!ok(d->createInstance(&inst), "createInstance"))
            return nullptr;
        return inst;
    }

    // Introspection / debug helpers
    /*!
    @brief Collects the paths of all events from all loaded banks.
    */
    bool AudioSystem::listAllEventPaths(std::vector<std::string>& out)
    {
        out.clear();
        if (!mStudio)
            return false;

        mStudio->flushCommands();

        std::vector<std::string> paths;

        for (auto& kv : mBanks)
        {
            FMOD::Studio::Bank* bank = kv.second;
            if (!bank)
                continue;

            int evCount = 0;
            if (!ok(bank->getEventCount(&evCount), "Bank::getEventCount"))
                continue;
            if (evCount <= 0)
                continue;

            std::vector<FMOD::Studio::EventDescription*> evs(evCount, nullptr);
            if (!ok(bank->getEventList(evs.data(), evCount, &evCount),
                "Bank::getEventList"))
                continue;

            for (int i = 0; i < evCount; ++i)
            {
                char path[512] = {};
                int  retrieved = 0;
                if (evs[i] &&
                    evs[i]->getPath(path, sizeof(path), &retrieved) == FMOD_OK &&
                    retrieved > 0)
                {
                    paths.emplace_back(path);
                }
            }
        }

        std::sort(paths.begin(), paths.end());
        paths.erase(std::unique(paths.begin(), paths.end()), paths.end());
        out.swap(paths);
        return true;
    }

    /*!
    @brief Collects the paths of all events from a specific bank file.
    */
    bool AudioSystem::listEventPathsFromBank(const std::string& bankFile,
        std::vector<std::string>& out)
    {
        out.clear();
        if (!mStudio)
            return false;

        // Use already-loaded bank if present; else load temporarily.
        FMOD::Studio::Bank* bank = nullptr;
        bool                tempLoaded = false;

        auto it = mBanks.find(bankFile);
        if (it != mBanks.end())
        {
            bank = it->second;
        }
        else
        {
            std::string full = joinPath(mBanksDir, bankFile);
            if (!ok(mStudio->loadBankFile(full.c_str(),
                FMOD_STUDIO_LOAD_BANK_NORMAL,
                &bank),
                "loadBankFile (inspect)"))
            {
                return false;
            }
            tempLoaded = true;
        }

        int evCount = 0;
        ok(bank->getEventCount(&evCount), "Bank::getEventCount");
        if (evCount > 0)
        {
            std::vector<FMOD::Studio::EventDescription*> evs(evCount, nullptr);
            ok(bank->getEventList(evs.data(), evCount, &evCount),
                "Bank::getEventList");

            for (int i = 0; i < evCount; ++i)
            {
                char path[512] = {};
                int  retrieved = 0;
                if (evs[i] &&
                    evs[i]->getPath(path, sizeof(path), &retrieved) == FMOD_OK &&
                    retrieved > 0)
                {
                    out.emplace_back(path);
                }
            }

            std::sort(out.begin(), out.end());
            out.erase(std::unique(out.begin(), out.end()), out.end());
        }

        if (tempLoaded && bank)
            bank->unload();

        return true;
    }

    /*!
    @brief Collects the paths of all raw audio files.
    */
    void AudioSystem::listRawAudioFiles(std::vector<std::string>& out)
    {
        out = mRawAudioFiles;
    }

    /*!
    @brief Prints information about events, buses, and VCAs in a bank to the console.
    */
    void AudioSystem::inspectBankToConsole(const std::string& bankFile)
    {
        if (!mStudio)
            return;

        FMOD::Studio::Bank* bank = nullptr;
        std::string         full = joinPath(mBanksDir, bankFile);

        if (!ok(mStudio->loadBankFile(full.c_str(),
            FMOD_STUDIO_LOAD_BANK_NORMAL,
            &bank),
            "loadBankFile (inspect console)"))
        {
            return;
        }

        int count = 0;
        std::fprintf(stdout, "\n--- Inspecting bank: %s ---\n", bankFile.c_str());

        // Events
        ok(bank->getEventCount(&count), "getEventCount");
        if (count > 0)
        {
            std::vector<FMOD::Studio::EventDescription*> list(count);
            ok(bank->getEventList(list.data(), count, &count), "getEventList");
            std::fprintf(stdout, "Events (%d):\n", count);
            for (auto* d : list)
            {
                char p[512];
                int  n = 0;
                if (d && d->getPath(p, sizeof(p), &n) == FMOD_OK)
                    std::fprintf(stdout, "  %s\n", p);
            }
        }

        // Buses
        ok(bank->getBusCount(&count), "getBusCount");
        if (count > 0)
        {
            std::vector<FMOD::Studio::Bus*> list(count);
            ok(bank->getBusList(list.data(), count, &count), "getBusList");
            std::fprintf(stdout, "Buses (%d):\n", count);
            for (auto* b : list)
            {
                char p[512];
                int  n = 0;
                if (b && b->getPath(p, sizeof(p), &n) == FMOD_OK)
                    std::fprintf(stdout, "  %s\n", p);
            }
        }

        // VCAs
        ok(bank->getVCACount(&count), "getVCACount");
        if (count > 0)
        {
            std::vector<FMOD::Studio::VCA*> list(count);
            ok(bank->getVCAList(list.data(), count, &count), "getVCAList");
            std::fprintf(stdout, "VCAs (%d):\n", count);
            for (auto* v : list)
            {
                char p[512];
                int  n = 0;
                if (v && v->getPath(p, sizeof(p), &n) == FMOD_OK)
                    std::fprintf(stdout, "  %s\n", p);
            }
        }

        bank->unload();
    }

    /*
    @brief List tagged 
    */
    std::vector<std::string> AudioSystem::listTaggedKeys() const
    {
        std::vector<std::string> out;
        out.reserve(mTaggedChannels.size());
        for (const auto& kv : mTaggedChannels) {
            out.push_back(kv.first);
        }
        std::sort(out.begin(), out.end());
        return out;
    }

    // File-based playback (Core API)
    /*!
    @brief Plays a music file through the music channel group, optionally looping it.
    */
    FMOD::Channel* AudioSystem::playMusicFile(const std::string& path, bool loop)
    {
        if (!mCore || !mMusicGroup) return nullptr;

        std::string key = SanitizeKey(path);
        FMOD::Sound* snd = nullptr;
        bool createdLocal = false;

        auto it = mRawSoundCache.find(key);
        if (it != mRawSoundCache.end()) {
            snd = it->second;
        }
        else {
            auto pathIt = mSoundPaths.find(key);
            std::string fullPath = (pathIt != mSoundPaths.end()) ? pathIt->second : joinPath(mBanksDir, key);
            
            createdLocal = true;
            // To Optimize later: Music should ALWAYS be streamed to save RAM
            FMOD_MODE mode = FMOD_CREATESTREAM | FMOD_2D;
            mode |= (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

            FMOD_RESULT r = mCore->createSound(fullPath.c_str(), mode, nullptr, &snd);

            if (r != FMOD_OK) {
                // Second attempt with the raw path if joinPath failed
                mCore->createSound(path.c_str(), mode, nullptr, &snd);
            }

            // If we successfully loaded it, cache it to avoid redundant I/O next time
            if (snd) {
                mRawSoundCache[key] = snd;
                createdLocal = false; // Now the cache owns it
            }
        }

        if (!snd) return nullptr;

        FMOD::Channel* ch = nullptr;
        // Play the sound
        if (!ok(mCore->playSound(snd, mMusicGroup, false, &ch), "playMusic"))
        {
            return nullptr;
        }

        if (ch) {
            ch->setMode(loop ? (FMOD_LOOP_NORMAL | FMOD_2D) : (FMOD_LOOP_OFF | FMOD_2D));
        }

        return ch;
    }

    /*!
    @brief Plays a one-shot SFX file through the SFX channel group.
    */
    FMOD::Channel* AudioSystem::playSFXFile(const std::string& path, bool spatial3D, float minDistance, float maxDistance)
    {
        if (!mCore || !mSFXGroup) return nullptr;

        std::string key = SanitizeKey(path);
        FMOD::Sound* snd = nullptr;

        // LOOKUP
        auto it = mRawSoundCache.find(key);
        if (it != mRawSoundCache.end()) {
            snd = it->second;
        }
        else {
            auto pathIt = mSoundPaths.find(key);
            std::string fullPath = (pathIt != mSoundPaths.end()) ? pathIt->second : joinPath(mBanksDir, key);

            // Check extensions to be safe
            if (!IsSupportedAudioFile(fullPath)) {
                if (IsSupportedAudioFile(path)) fullPath = path;
                else return nullptr;
            }

            // Load into RAM (FMOD_DEFAULT) for SFX as they are small and latency-sensitive
            FMOD_MODE mode = FMOD_DEFAULT;
            mode |= (spatial3D ? FMOD_3D : FMOD_2D);

            FMOD_RESULT r = mCore->createSound(fullPath.c_str(), mode, nullptr, &snd);

            if (r == FMOD_OK && snd) {
                mRawSoundCache[key] = snd;
            }
            else {
                return nullptr;
            }
        }

        FMOD::Channel* ch = nullptr;
        if (!ok(mCore->playSound(snd, mSFXGroup, false, &ch), "playSound(sfx)"))
        {
            return nullptr;
        }

        if (ch) {
            if (spatial3D) {
                ch->setMode(FMOD_3D);
                ch->set3DMinMaxDistance(minDistance, maxDistance);
            } else {
                ch->setMode(FMOD_2D);
            }
        }

        return ch;
    }

    /*!
    @brief Sets the volume of the music channel group and corresponding Studio bus.
    */
    bool AudioSystem::setMusicGroupVolume(float v01)
    {
        bool okCore = mMusicGroup && (mMusicGroup->setVolume(v01) == FMOD_OK);
        
        // Also set Studio Bus volume
        if (mStudio) {
            FMOD::Studio::Bus* bus = nullptr;
            // Try common paths
            if (mStudio->getBus("bus:/Music", &bus) == FMOD_OK && bus) bus->setVolume(v01);
            else if (mStudio->getBus("bus:/Master/Music", &bus) == FMOD_OK && bus) bus->setVolume(v01);
            else if (mStudio->getBus("bus:/music", &bus) == FMOD_OK && bus) bus->setVolume(v01);
        }

        return okCore;
    }

    /*!
    @brief Sets the volume of the SFX channel group and corresponding Studio bus.
    */
    bool AudioSystem::setSFXGroupVolume(float v01)
    {
        bool okCore = mSFXGroup && (mSFXGroup->setVolume(v01) == FMOD_OK);

        // Also set Studio Bus volume
        if (mStudio) {
            FMOD::Studio::Bus* bus = nullptr;
            if (mStudio->getBus("bus:/SFX", &bus) == FMOD_OK && bus) bus->setVolume(v01);
            else if (mStudio->getBus("bus:/Master/SFX", &bus) == FMOD_OK && bus) bus->setVolume(v01);
            else if (mStudio->getBus("bus:/sfx", &bus) == FMOD_OK && bus) bus->setVolume(v01);
        }

        return okCore;
    }

    /*!
    @brief Sets the master volume on both the FMOD Core master group and Studio master bus.
    */
    bool AudioSystem::setMasterVolume(float v01)
    {
        bool okCore = false;
        if (mCore) {
            FMOD::ChannelGroup* master = nullptr;
            if (mCore->getMasterChannelGroup(&master) == FMOD_OK && master) {
                okCore = (master->setVolume(v01) == FMOD_OK);
            }
        }

        if (mStudio) {
            FMOD::Studio::Bus* masterBus = nullptr;
            if (mStudio->getBus("bus:/", &masterBus) == FMOD_OK && masterBus) {
                masterBus->setVolume(v01);
            }
        }

        return okCore;
    }

    /*!
	@brief Sets the volume of a tagged channel by its string key.
    */
    bool AudioSystem::setTaggedVolume(const std::string& key, float vol)
    {
        auto it = mTaggedChannels.find(key);
        if (it == mTaggedChannels.end() || !it->second) return false;
        FMOD::Channel* ch = it->second;
        return (ch->setVolume(vol) == FMOD_OK);
    }

    void AudioSystem::SetChannelVolume(FMOD::Channel* channel, float volume)
    {
        if (channel)
        {
            channel->setVolume(volume);
        }
    }

    void AudioSystem::SetChannel3DAttributes(FMOD::Channel* channel, const CEVec3& position, float minDistance, float maxDistance)
    {
        if (channel)
        {
            FMOD_MODE mode;
            channel->getMode(&mode);
            // Force 3D and LINEARROLLOFF for predictable "fade to zero" behavior
            if (!(mode & FMOD_3D) || !(mode & FMOD_3D_LINEARROLLOFF)) {
                channel->setMode(FMOD_3D | FMOD_3D_LINEARROLLOFF);
            }

            FMOD_VECTOR pos = { position.x, position.y, position.z };
            FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
            channel->set3DAttributes(&pos, &vel);
            channel->set3DMinMaxDistance(minDistance, maxDistance);
        }
    }

    void AudioSystem::SetListenerAttributes(const CEVec3& position, const CEVec3& forward, const CEVec3& up, const CEVec3& velocity)
    {
        if (!mCore) return;

        FMOD_VECTOR pos = { position.x, position.y, position.z };
        FMOD_VECTOR vel = { velocity.x, velocity.y, velocity.z };
        FMOD_VECTOR fwd = { forward.x, forward.y, forward.z };
        FMOD_VECTOR u = { up.x, up.y, up.z };

        mCore->set3DListenerAttributes(0, &pos, &vel, &fwd, &u);
    }

    void AudioSystem::AddChannelDSP(FMOD::Channel* channel, FMOD_DSP_TYPE dspType)
    {
        if (!mCore || !channel) return;

        // Check if this DSP type is already on the channel
        if (mChannelDSPs.count(channel) && mChannelDSPs[channel].count(dspType)) {
            std::stringstream ss_warn;
            ss_warn << "DSP of type " << dspType << " already added to channel.";
            LOGW(Audio, ss_warn.str());
            return;
        }

        FMOD::DSP* dsp = nullptr;
        if (!ok(mCore->createDSPByType(dspType, &dsp), "createDSPByType")) return;

        if (!ok(channel->addDSP(0, dsp), "Channel::addDSP")) {
            dsp->release();
            return;
        }

        mChannelDSPs[channel][dspType] = dsp;
        std::stringstream ss_add;
        ss_add << "Added DSP of type " << dspType << " to channel.";
        LOGI(Audio, ss_add.str());
    }

    void AudioSystem::RemoveChannelDSP(FMOD::Channel* channel, FMOD_DSP_TYPE dspType)
    {
        if (!channel) return;

        auto channelIt = mChannelDSPs.find(channel);
        if (channelIt == mChannelDSPs.end()) return; // No DSPs for this channel

        auto dspIt = channelIt->second.find(dspType);
        if (dspIt == channelIt->second.end()) return; // DSP type not found for this channel

        FMOD::DSP* dspToRemove = dspIt->second;
        if (dspToRemove) {
            channel->removeDSP(dspToRemove);
            dspToRemove->release();
        }

        channelIt->second.erase(dspIt);
        if (channelIt->second.empty()) {
            mChannelDSPs.erase(channelIt);
        }
        std::stringstream ss_remove;
        ss_remove << "Removed DSP of type " << dspType << " from channel.";
        LOGI(Audio, ss_remove.str());
    }

    /*!
    @brief Returns the current volume of the music channel group (0.0 to 1.0).
    */
    float AudioSystem::GetMusicVolume() const
    {
        float vol = 0.0f;
        if (mMusicGroup)
        {
            mMusicGroup->getVolume(&vol);
        }
        return vol;
    }

    /*!
    @brief Returns the current volume of the SFX channel group (0.0 to 1.0).
    */
    float AudioSystem::GetSFXVolume() const
    {
        float vol = 0.0f;
        if (mSFXGroup)
        {
            mSFXGroup->getVolume(&vol);
        }
        return vol;
    }

    /*!
    @brief Returns the current master volume from the FMOD Core system.
    */
    float AudioSystem::GetMasterVolume() const
    {
        float vol = 0.0f;
        if (!mCore) return vol;

        FMOD::ChannelGroup* master = nullptr;
        if (mCore->getMasterChannelGroup(&master) == FMOD_OK && master)
        {
            master->getVolume(&vol);
        }
        return vol;
    }

    /*!
	@brief Gets the volume of a tagged channel by its string key.
    */
    bool AudioSystem::getTaggedVolume(const std::string& key, float& outVol) const
    {
        auto it = mTaggedChannels.find(key);
        if (it == mTaggedChannels.end() || !it->second) return false;
        FMOD::Channel* ch = it->second;
        FMOD_RESULT r = ch->getVolume(&outVol);
        return (r == FMOD_OK);
    }

    /*!
	@brief Fades out and stops a tagged channel by its string key over the specified duration.
    */
    bool AudioSystem::fadeTagged(const std::string& key, float seconds)
    {
        // reuse existing stopTagged which handles fade queue
        return stopTagged(key, seconds);
    }


    // Tagged playback for different groups (SFX/Music)
    /*!
    @brief Plays a music file and tags its FMOD channel so it can be stopped by key.
    */
    bool AudioSystem::playMusicFileTagged(const std::string& key,
        const std::string& path,
        bool loop)
    {
        if (!mCore || !mMusicGroup) return false;

		// Safety First: Check if already playing
        auto existing = mTaggedChannels.find(key);
        if (existing != mTaggedChannels.end()) {
            bool isPlaying = false;
            existing->second->isPlaying(&isPlaying);
            if (isPlaying) {
                return true;
            }
        }

        std::string cacheKey = SanitizeKey(path);
        FMOD::Sound* snd = nullptr;

        auto it = mRawSoundCache.find(cacheKey);
        if (it != mRawSoundCache.end()) {
            snd = it->second;
        }
        else {
            auto pathIt = mSoundPaths.find(cacheKey);
            std::string fullPath = (pathIt != mSoundPaths.end()) ? pathIt->second : joinPath(mBanksDir, cacheKey);

            FMOD_MODE mode = FMOD_CREATESTREAM | FMOD_2D;
            mode |= (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

            FMOD_RESULT r = mCore->createSound(fullPath.c_str(), mode, nullptr, &snd);
            if (r != FMOD_OK) {
                mCore->createSound(path.c_str(), mode, nullptr, &snd);
            }

            if (snd) {
                mRawSoundCache[cacheKey] = snd;
            }
        }

        if (!snd) return false;

        FMOD::Channel* ch = nullptr;
        if (!ok(mCore->playSound(snd, mMusicGroup, false, &ch), "playMusicTagged"))
        {
            return false;
        }

        if (ch)
        {
            ch->setMode(loop ? (FMOD_LOOP_NORMAL | FMOD_2D) : (FMOD_LOOP_OFF | FMOD_2D));
            mTaggedChannels[key] = ch;
        }

        return true;
    }

    /*!
    @brief Plays a SFX file and tags its FMOD channel so it can be stopped by key.
    */
    bool AudioSystem::playSFXFileTagged(const std::string& key,
        const std::string& path, bool loop)
    {
        if (!mCore || !mSFXGroup) return false;

        // SANITIZE
        std::string cacheKey = SanitizeKey(path);

        FMOD::Sound* snd = nullptr;

        // CACHE LOOKUP
        auto it = mRawSoundCache.find(cacheKey);
        if (it != mRawSoundCache.end()) {
            snd = it->second;
        }
        else {
            auto pathIt = mSoundPaths.find(cacheKey);
            std::string fullPath = (pathIt != mSoundPaths.end()) ? pathIt->second : joinPath(mBanksDir, cacheKey);

            FMOD_MODE mode = FMOD_DEFAULT | FMOD_2D;
            mode |= (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

            FMOD_RESULT r = mCore->createSound(fullPath.c_str(), mode, nullptr, &snd);

            if (r == FMOD_OK && snd) {
                mRawSoundCache[cacheKey] = snd;
            }
            else {
                return false;
            }
        }

        FMOD::Channel* ch = nullptr;
        if (!ok(mCore->playSound(snd, mSFXGroup, false, &ch), "playSound(sfxTagged)"))
        {
            return false;
        }

        if (ch)
        {
            ch->setMode(loop ? (FMOD_LOOP_NORMAL | FMOD_2D) : (FMOD_LOOP_OFF | FMOD_2D));
            mTaggedChannels[key] = ch;
        }

        return true;
    }

    /*!
	@brief Checks if a tagged channel is currently playing.
    */
    bool AudioSystem::isTaggedPlaying(const std::string& key)
    {
        if (!mCore) return false;

        auto it = mTaggedChannels.find(key);
        if (it != mTaggedChannels.end())
        {
            bool playing = false;
            FMOD_RESULT res = it->second->isPlaying(&playing);
            return (res == FMOD_OK) && playing;
        }
        return false;
    }

    /*!
    @brief Stops a previously tagged channel by its string key.
    */
    bool AudioSystem::stopTagged(const std::string& key, float fadeOutSeconds)
    {
        auto it = mTaggedChannels.find(key);
        if (it == mTaggedChannels.end())
            return false;

        FMOD::Channel* ch = it->second;

        // Remove from map immediately so the key is free to be used again
        mTaggedChannels.erase(it);

        if (ch)
        {
            bool playing = false;
            if (ch->isPlaying(&playing) == FMOD_OK && playing)
            {
                if (fadeOutSeconds <= 0.0f)
                {
                    // Immediate stop (Hard cut)
                    ch->stop();
                }
                else
                {
                    // Hand off to the fade system
                    float currentVol = 1.0f;
                    ch->getVolume(&currentVol);

                    FadingChannel fc;
                    fc.channel = ch;
                    fc.currentVol = currentVol;
                    fc.fadeRate = currentVol / fadeOutSeconds; // Calculate rate to hit 0 in X seconds

                    mFadingChannels.push_back(fc);
                }
            }
        }

        return true;
    }

    void AudioSystem::setPaused(bool paused)
    {
        if (!mCore)
            return;

        FMOD::ChannelGroup* master = nullptr;
        if (mCore->getMasterChannelGroup(&master) != FMOD_OK || !master)
            return;

        // Pause or resume all audio
        master->setPaused(paused);
    }

    void AudioSystem::SetChannelDSPParameter(FMOD::Channel* channel, FMOD_DSP_TYPE dspType, int index, float value)
    {
        if (!channel) return;

        auto channelIt = mChannelDSPs.find(channel);
        if (channelIt == mChannelDSPs.end()) return;

        auto dspIt = channelIt->second.find(dspType);
        if (dspIt == channelIt->second.end()) return;

        FMOD::DSP* dsp = dspIt->second;
        if (dsp) {
            dsp->setParameterFloat(index, value);
        }
    }

    void AudioSystem::AddGroupDSP(bool isMusic, FMOD_DSP_TYPE dspType)
    {
        if (!mCore) return;
        FMOD::ChannelGroup* group = isMusic ? mMusicGroup : mSFXGroup;
        if (!group) return;

        if (mGroupDSPs.count(group) && mGroupDSPs[group].count(dspType)) return;

        FMOD::DSP* dsp = nullptr;
        if (!ok(mCore->createDSPByType(dspType, &dsp), "createDSPByType (Group)")) return;

        if (!ok(group->addDSP(0, dsp), "ChannelGroup::addDSP")) {
            dsp->release();
            return;
        }

        mGroupDSPs[group][dspType] = dsp;
        LOGI(Audio, "Added DSP to " + std::string(isMusic ? "Music" : "SFX") + " group.");
    }

    void AudioSystem::RemoveGroupDSP(bool isMusic, FMOD_DSP_TYPE dspType)
    {
        FMOD::ChannelGroup* group = isMusic ? mMusicGroup : mSFXGroup;
        if (!group) return;

        auto groupIt = mGroupDSPs.find(group);
        if (groupIt == mGroupDSPs.end()) return;

        auto dspIt = groupIt->second.find(dspType);
        if (dspIt == groupIt->second.end()) return;

        FMOD::DSP* dspToRemove = dspIt->second;
        if (dspToRemove) {
            group->removeDSP(dspToRemove);
            dspToRemove->release();
        }

        groupIt->second.erase(dspIt);
        LOGI(Audio, "Removed DSP from " + std::string(isMusic ? "Music" : "SFX") + " group.");
    }

    void AudioSystem::SetGroupDSPParameter(bool isMusic, FMOD_DSP_TYPE dspType, int index, float value)
    {
        FMOD::ChannelGroup* group = isMusic ? mMusicGroup : mSFXGroup;
        if (!group) return;

        auto groupIt = mGroupDSPs.find(group);
        if (groupIt == mGroupDSPs.end()) return;

        auto dspIt = groupIt->second.find(dspType);
        if (dspIt == groupIt->second.end()) return;

        FMOD::DSP* dsp = dspIt->second;
        if (dsp) {
            dsp->setParameterFloat(index, value);
        }
    }

    void AudioSystem::FadeGroupDSPParameter(bool isMusic, FMOD_DSP_TYPE dspType, int index, float targetValue, float duration)
    {
        FMOD::ChannelGroup* group = isMusic ? mMusicGroup : mSFXGroup;
        if (!group) return;

        auto groupIt = mGroupDSPs.find(group);
        if (groupIt == mGroupDSPs.end()) return;

        auto dspIt = groupIt->second.find(dspType);
        if (dspIt == groupIt->second.end()) return;

        FMOD::DSP* dsp = dspIt->second;
        if (!dsp) return;

        float currentValue = 0.0f;
        dsp->getParameterFloat(index, &currentValue, nullptr, 0);

        // Remove any existing fade for this specific DSP and index
        mParameterFades.remove_if([dsp, index](const DSPParameterFade& f) {
            return f.dsp == dsp && f.index == index;
        });

        if (duration <= 0.0f) {
            dsp->setParameterFloat(index, targetValue);
            return;
        }

        DSPParameterFade fade;
        fade.dsp = dsp;
        fade.index = index;
        fade.startValue = currentValue;
        fade.endValue = targetValue;
        fade.currentTime = 0.0f;
        fade.totalTime = duration;

        mParameterFades.push_back(fade);
    }

    FMOD::Studio::Bus* AudioSystem::GetStudioBus(bool isMusic)
    {
        if (!mStudio) return nullptr;
        FMOD::Studio::Bus* bus = nullptr;
        std::vector<std::string> paths = isMusic ? 
            std::vector<std::string>{"bus:/Music", "bus:/Master/Music", "bus:/music"} :
            std::vector<std::string>{"bus:/SFX", "bus:/Master/SFX", "bus:/sfx"};

        for (const auto& path : paths) {
            if (mStudio->getBus(path.c_str(), &bus) == FMOD_OK && bus) return bus;
        }
        return nullptr;
    }

    void AudioSystem::FadeGroupVolume(bool isMusic, float targetVolume, float duration)
    {
        // 1. Fade the Core Channel Group (for tagged files)
        FMOD::ChannelGroup* group = isMusic ? mMusicGroup : mSFXGroup;
        if (group) {
            float currentVolume = 0.0f;
            group->getVolume(&currentVolume);

            mGroupVolumeFades.remove_if([group](const GroupVolumeFade& f) {
                return f.group == group;
            });

            if (duration <= 0.0f) {
                group->setVolume(targetVolume);
            }
            else {
                GroupVolumeFade fade;
                fade.group = group;
                fade.startVolume = currentVolume;
                fade.endVolume = targetVolume;
                fade.currentTime = 0.0f;
                fade.totalTime = duration;
                mGroupVolumeFades.push_back(fade);
            }
        }

        // 2. Fade the Studio Bus (for event instances)
        FMOD::Studio::Bus* bus = GetStudioBus(isMusic);
        if (bus) {
            float currentVolume = 0.0f;
            bus->getVolume(&currentVolume);

            char path[256];
            int len;
            bus->getPath(path, 256, &len);
            std::cout << "[Audio] Fading Bus: " << path << " to " << targetVolume << " over " << duration << "s" << std::endl;

            mBusVolumeFades.remove_if([bus](const BusVolumeFade& f) {
                return f.bus == bus;
            });

            if (duration <= 0.0f) {
                bus->setVolume(targetVolume);
            }
            else {
                BusVolumeFade fade;
                fade.bus = bus;
                fade.startVolume = currentVolume;
                fade.endVolume = targetVolume;
                fade.currentTime = 0.0f;
                fade.totalTime = duration;
                mBusVolumeFades.push_back(fade);
            }
        }
        else {
            std::cout << "[Audio] Warning: No Studio Bus found for " << (isMusic ? "Music" : "SFX") << ". FMOD Studio Events will not fade!" << std::endl;
        }
    }

    void AudioSystem::FadeMasterVolume(float targetVolume, float duration)
    {
        // 1. Fade Core Master Group
        FMOD::ChannelGroup* masterGroup = nullptr;
        if (mCore && mCore->getMasterChannelGroup(&masterGroup) == FMOD_OK && masterGroup)
        {
            float currentVolume = 0.0f;
            masterGroup->getVolume(&currentVolume);

            mGroupVolumeFades.remove_if([masterGroup](const GroupVolumeFade& f) {
                return f.group == masterGroup;
            });

            if (duration <= 0.0f) {
                masterGroup->setVolume(targetVolume);
            }
            else {
                GroupVolumeFade fade;
                fade.group = masterGroup;
                fade.startVolume = currentVolume;
                fade.endVolume = targetVolume;
                fade.currentTime = 0.0f;
                fade.totalTime = duration;
                mGroupVolumeFades.push_back(fade);
                std::cout << "[Audio] Master Core Fade: " << currentVolume << " -> " << targetVolume << " over " << duration << "s" << std::endl;
            }
        }

        // 2. Fade Studio Master Bus
        if (mStudio)
        {
            FMOD::Studio::Bus* masterBus = nullptr;
            if (mStudio->getBus("bus:/", &masterBus) == FMOD_OK && masterBus)
            {
                float currentVolume = 0.0f;
                masterBus->getVolume(&currentVolume);

                mBusVolumeFades.remove_if([masterBus](const BusVolumeFade& f) {
                    return f.bus == masterBus;
                });

                if (duration <= 0.0f) {
                    masterBus->setVolume(targetVolume);
                }
                else {
                    BusVolumeFade fade;
                    fade.bus = masterBus;
                    fade.startVolume = currentVolume;
                    fade.endVolume = targetVolume;
                    fade.currentTime = 0.0f;
                    fade.totalTime = duration;
                    mBusVolumeFades.push_back(fade);
                    std::cout << "[Audio] Master Studio Fade: " << currentVolume << " -> " << targetVolume << " over " << duration << "s" << std::endl;
                }
            }
        }
    }

    void AudioSystem::FadeTaggedVolume(const std::string& key, float targetVolume, float duration)
    {
        auto it = mTaggedChannels.find(key);
        if (it == mTaggedChannels.end() || !it->second) return;

        FMOD::Channel* channel = it->second;

        float currentVolume = 0.0f;
        channel->getVolume(&currentVolume);

        // Remove any existing volume fade for this channel
        mChannelVolumeFades.remove_if([channel](const ChannelVolumeFade& f) {
            return f.channel == channel;
        });

        if (duration <= 0.0f) {
            channel->setVolume(targetVolume);
            return;
        }

        ChannelVolumeFade fade;
        fade.channel = channel;
        fade.startVolume = currentVolume;
        fade.endVolume = targetVolume;
        fade.currentTime = 0.0f;
        fade.totalTime = duration;

        mChannelVolumeFades.push_back(fade);
    }


} // namespace CrashoutsEngine::Audio