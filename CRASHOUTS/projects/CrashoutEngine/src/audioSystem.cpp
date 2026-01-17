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

    /*!
    @brief Returns the FMOD_MODE flags used for music sounds based on loop state.
    */
    static FMOD_MODE MusicMode(bool loop)
    {
        FMOD_MODE m = FMOD_DEFAULT | FMOD_CREATESTREAM | FMOD_2D;
        if (loop)
            m |= FMOD_LOOP_NORMAL;
        else
            m |= FMOD_LOOP_OFF;
        return m;
    }

    static void ShowAudioErrorPopup(const std::string& msg)
    {
#ifdef _WIN32
        MessageBoxA(
            nullptr,
            msg.c_str(),
            "Audio Error",
            MB_OK | MB_ICONERROR | MB_TOPMOST
        );
#endif

        // Also log to console/log window
        LOGE(Audio, msg);
    }

    static bool IsSupportedAudioFile(const std::string& path)
    {
        static const std::vector<std::string> allowed = {
            ".mp3",
            ".ogg",
            ".wav"
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
    static std::string SanitizeKey(const std::string& path)
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

                    FMOD::Sound* s = nullptr;
                    // FMOD_DEFAULT | FMOD_2D -> Load entirely into RAM
                    FMOD_RESULT r = mCore->createSound(diskPath.c_str(), FMOD_DEFAULT | FMOD_2D, nullptr, &s);
                    if (r == FMOD_OK && s) {
                        mRawSoundCache[key] = s;
                        count++;
                    }
                }
                std::cout << "[Audio] Loaded " << count << " files.\n";
            }
        }
        catch (...) { std::cout << "[Audio] Scan Error.\n"; }

        return true;
    }

    /*!
    @brief Updates the FMOD Studio system; must be called once per frame.
    */
    void AudioSystem::Update()
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

            float dt = 1.0f / 60.0f;

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

        auto it = mActiveSounds.begin();
        while (it != mActiveSounds.end())
        {
            FMOD::Channel* ch = it->first;
            FMOD::Sound* snd = it->second;

            bool isPlaying = false;
            ch->isPlaying(&isPlaying);

            if (!isPlaying)
            {
                for (auto mapIt = mTaggedChannels.begin(); mapIt != mTaggedChannels.end(); )
                {
                    if (mapIt->second == ch) {
                        mapIt = mTaggedChannels.erase(mapIt);
                    }
                    else {
                        ++mapIt;
                    }
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
    }

    /*!
    @brief Stops all events on the master bus, including music and SFX.
    */
    bool AudioSystem::stopAll(bool fadeOut)
    {
        // Master bus stops *everything* (music + sfx)
        return stopAllOnBus("bus:/", fadeOut);
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
            std::string fullPath = joinPath(mBanksDir, key);
            createdLocal = true;
            // Failsafe: Stream if not in cache
            FMOD_RESULT r = mCore->createSound(fullPath.c_str(),
                FMOD_CREATESTREAM | (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF),
                nullptr, &snd);

            if (r != FMOD_OK) {
                mCore->createSound(path.c_str(),
                    FMOD_CREATESTREAM | (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF),
                    nullptr, &snd);
            }
        }

        if (!snd) return nullptr;

        FMOD::Channel* ch = nullptr;
        // Play the sound
        if (!ok(mCore->playSound(snd, mMusicGroup, false, &ch), "playMusic"))
        {
            if (createdLocal && snd) snd->release();
            return nullptr;
        }

        if (ch) {
            // Cached sounds default to LOOP_OFF, must override it here.
            ch->setMode(loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

            if (createdLocal) mActiveSounds.emplace_back(ch, snd);
        }

        return ch;
    }

    /*!
    @brief Plays a one-shot SFX file through the SFX channel group.
    */
    FMOD::Channel* AudioSystem::playSFXFile(const std::string& path) {
        return playSFXFileEx(path);
    }

    FMOD::Channel* AudioSystem::playSFXFileEx(const std::string& path)
    {
        if (!mCore || !mSFXGroup) return nullptr;

        // SANITIZE (MATCH THE KEY)
        std::string key = SanitizeKey(path);

        FMOD::Sound* snd = nullptr;
        bool createdLocal = false;

        // LOOKUP
        auto it = mRawSoundCache.find(key);
        if (it != mRawSoundCache.end()) {
            snd = it->second;
        }
        else {
            // FAIL-SAFE
            // This handles files dragged in while the game is running
            std::string fullPath = joinPath(mBanksDir, key);

            // Check extensions to be safe
            if (!IsSupportedAudioFile(fullPath)) {
                // Try the original path if the joinPath failed (e.g. absolute paths)
                if (IsSupportedAudioFile(path)) fullPath = path;
                else return nullptr;
            }

            createdLocal = true;
            // Load into RAM
            FMOD_RESULT r = mCore->createSound(fullPath.c_str(), FMOD_DEFAULT | FMOD_2D, nullptr, &snd);

            if (r == FMOD_OK) {
                mRawSoundCache[key] = snd;
                createdLocal = false;

                if (std::find(mRawAudioFiles.begin(), mRawAudioFiles.end(), key) == mRawAudioFiles.end()) {
                    mRawAudioFiles.push_back(key);
                }
            }
            else {
                return nullptr;
            }
        }

        FMOD::Channel* ch = nullptr;
        if (!ok(mCore->playSound(snd, mSFXGroup, false, &ch), "playSound(sfx)"))
        {
            if (createdLocal && snd) snd->release();
            return nullptr;
        }

        if (ch) {
            ch->setMode(FMOD_2D);
            if (createdLocal) mActiveSounds.emplace_back(ch, snd);
        }

        return ch;
    }

    /*!
    @brief Sets the volume of the music channel group.
    */
    bool AudioSystem::setMusicGroupVolume(float v01)
    {
        return mMusicGroup && (mMusicGroup->setVolume(v01) == FMOD_OK);
    }

    /*!
    @brief Sets the volume of the SFX channel group.
    */
    bool AudioSystem::setSFXGroupVolume(float v01)
    {
        return mSFXGroup && (mSFXGroup->setVolume(v01) == FMOD_OK);
    }

    /*!
    @brief Sets the master volume on the FMOD Core master channel group.
    */
    bool AudioSystem::setMasterVolume(float v01)
    {
        if (!mCore)
            return false;

        FMOD::ChannelGroup* master = nullptr;
        if (!ok(mCore->getMasterChannelGroup(&master), "getMasterChannelGroup"))
            return false;

        if (!master)
            return false;

        return master->setVolume(v01) == FMOD_OK;
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

        std::string cacheKey = SanitizeKey(path);
        FMOD::Sound* snd = nullptr;
        bool createdLocal = false;

        auto it = mRawSoundCache.find(cacheKey);
        if (it != mRawSoundCache.end()) {
            snd = it->second;
        }
        else {
            std::string fullPath = joinPath(mBanksDir, cacheKey);
            FMOD_MODE mode = FMOD_CREATESTREAM | FMOD_2D;
            if (loop) mode |= FMOD_LOOP_NORMAL;
            else      mode |= FMOD_LOOP_OFF;

            FMOD_RESULT r = mCore->createSound(fullPath.c_str(), mode, nullptr, &snd);
            if (r != FMOD_OK) {
                mCore->createSound(path.c_str(), mode, nullptr, &snd);
            }
            if (snd) createdLocal = true;
        }

        if (!snd) return false;

        FMOD::Channel* ch = nullptr;
        if (!ok(mCore->playSound(snd, mMusicGroup, false, &ch), "playMusicTagged"))
        {
            if (createdLocal) snd->release();
            return false;
        }

        if (ch)
        {
            // Loop Mode override
            ch->setMode(loop ? (FMOD_LOOP_NORMAL | FMOD_2D) : (FMOD_LOOP_OFF | FMOD_2D));

            mTaggedChannels[key] = ch;

            if (createdLocal) {
                mActiveSounds.emplace_back(ch, snd);
            }
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
        bool createdLocal = false;

        // CACHE LOOKUP
        auto it = mRawSoundCache.find(cacheKey);
        if (it != mRawSoundCache.end()) {
            snd = it->second;
        }
        else {
            // FAILSAFE
            std::string fullPath = joinPath(mBanksDir, cacheKey);

            FMOD_MODE mode = FMOD_DEFAULT | FMOD_2D;
            if (loop) mode |= FMOD_LOOP_NORMAL;
            else      mode |= FMOD_LOOP_OFF;

            FMOD_RESULT r = mCore->createSound(fullPath.c_str(), mode, nullptr, &snd);

            // If we loaded it successfully, let's cache it for next time!
            if (r == FMOD_OK && snd) {
                mRawSoundCache[cacheKey] = snd;
                if (std::find(mRawAudioFiles.begin(), mRawAudioFiles.end(), cacheKey) == mRawAudioFiles.end()) {
                    mRawAudioFiles.push_back(cacheKey);
                }
                // createdLocal stays FALSE because now the cache owns it
            }
            else {
                return false;
            }
        }

        FMOD::Channel* ch = nullptr;
        if (!ok(mCore->playSound(snd, mSFXGroup, false, &ch), "playSound(sfxTagged)"))
        {
            if (createdLocal) snd->release();
            return false;
        }

        if (ch)
        {
            ch->setMode(FMOD_2D);
            mTaggedChannels[key] = ch;

            if (createdLocal) {
                mActiveSounds.emplace_back(ch, snd);
            }
        }

        return true;
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


} // namespace CrashoutsEngine::Audio