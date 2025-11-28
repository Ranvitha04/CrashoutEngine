/*!
@file        audioSystem.cpp
@author      b.mohamadafiq@digipen.edu
@date        28/9/2025
@brief      Minimal FMOD Studio wrapper for the CrashoutsEngine.
             - Creates/initializes FMOD Studio/Core systems
             - Loads/unloads bank files
             - Plays one-shot events (fire-and-forget)
             - Creates long-lived event instances (music/loops)
             - Controls buses (stop/mute/set volume)
             - Updates FMOD every frame
             The class is a singleton accessed via AudioSystem::instance().

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
    // -------------------------------------------------------------------------
    // Small helpers
    // -------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------
    // Singleton & error helper
    // -------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------
    // Lifetime
    // -------------------------------------------------------------------------

    /*!
    @brief Initializes the FMOD Studio and Core systems and sets up channel groups for music and SFX.
    */
    bool AudioSystem::init(const std::string& banksDir)
    {
        mBanksDir = banksDir;

        // Create Studio system
        FMOD::Studio::System* rawStudio = nullptr;
        if (!ok(FMOD::Studio::System::create(&rawStudio), "System::create"))
            return false;
        mStudio.reset(rawStudio);

        // Initialize Studio system
        if (!ok(mStudio->initialize(1024,
            FMOD_STUDIO_INIT_NORMAL,
            FMOD_INIT_NORMAL,
            nullptr),
            "System::initialize"))
            return false;

        // Get Core system
        FMOD::System* rawCore = nullptr;
        if (!ok(mStudio->getCoreSystem(&rawCore), "getCoreSystem"))
            return false;
        mCore.reset(rawCore);

        // Create channel groups for raw-file playback
        FMOD::ChannelGroup* master = nullptr;
        ok(mCore->getMasterChannelGroup(&master), "getMasterChannelGroup");

        ok(mCore->createChannelGroup("Music", &mMusicGroup),
            "createChannelGroup Music");
        ok(mCore->createChannelGroup("SFX", &mSFXGroup),
            "createChannelGroup SFX");

        if (master)
        {
            ok(master->addGroup(mMusicGroup), "master.addGroup(Music)");
            ok(master->addGroup(mSFXGroup), "master.addGroup(SFX)");
        }

        // Banks are loaded on demand via loadBank()
        return true;
    }

    /*!
    @brief Updates the FMOD Studio system; must be called once per frame.
    */
    void AudioSystem::update()
    {
        if (mStudio)
            mStudio->update();
    }

    /*!
    @brief Shuts down the audio system, unloading banks and releasing FMOD resources.
    */
    void AudioSystem::shutdown()
    {
        // Stop any tagged channels we tracked
        mTaggedChannels.clear();

        unloadAllRawSounds();
        unloadAllBanks();
        mStudio.reset();
        mCore.reset();

        mMusicGroup = nullptr;
        mSFXGroup = nullptr;
    }

    // -------------------------------------------------------------------------
    // Bank management
    // -------------------------------------------------------------------------

    /*!
    @brief Loads a FMOD Studio bank file from the configured banks directory.
    */
    bool AudioSystem::loadBank(const std::string& bankFile)
    {
        if (!mStudio)
            return false;

        // Already loaded
        if (mBanks.count(bankFile))
            return true;

        std::string full = joinPath(mBanksDir, bankFile);
        FMOD::Studio::Bank* b = nullptr;

        if (!ok(mStudio->loadBankFile(full.c_str(),
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

    // -------------------------------------------------------------------------
    // Raw management
    // -------------------------------------------------------------------------
    void AudioSystem::registerRawAudio(const std::string& relPath)
    {
        auto it = std::find(mRawAudioFiles.begin(), mRawAudioFiles.end(), relPath);
        if (it == mRawAudioFiles.end())
            mRawAudioFiles.push_back(relPath);
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

    // -------------------------------------------------------------------------
    // Bus / group controls
    // -------------------------------------------------------------------------

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
        bool studioResult = stopAllOnBus("bus:/", fadeOut);

        stopAllRaw();

        return studioResult;
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

    // -------------------------------------------------------------------------
    // Event-based (Studio) playback
    // -------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------
    // Introspection / debug helpers
    // -------------------------------------------------------------------------

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
        out.clear();

        namespace fs = std::filesystem;
        fs::path audioRoot = "assets/audio";

        if (!fs::exists(audioRoot))
            return;

        for (auto& e : fs::directory_iterator(audioRoot))
        {
            if (e.is_regular_file())
            {
                std::string ext = e.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (ext == ".wav" || ext == ".ogg" || ext == ".mp3")
                {
                    out.push_back(e.path().generic_string());
                }
            }
        }
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

    // -------------------------------------------------------------------------
    // File-based playback (Core API)
    // -------------------------------------------------------------------------

    /*!
    @brief Plays a music file through the music channel group, optionally looping it.
    */
    FMOD::Channel* AudioSystem::playMusicFile(const std::string& path, bool loop)
    {
        if (!mCore || !mMusicGroup) return nullptr;

        FMOD::Sound* snd = nullptr;
        if (!ok(mCore->createSound(path.c_str(),
            FMOD_CREATESTREAM | (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF),
            nullptr,
            &snd),
            "playMusic createSound"))
            return nullptr;

        FMOD::Channel* ch = nullptr;
        if (!ok(mCore->playSound(snd, mMusicGroup, false, &ch),
            "playMusic playSound"))
            return nullptr;

        return ch;
    }

    /*!
    @brief Plays a one-shot SFX file through the SFX channel group.
    */
    FMOD::Channel* AudioSystem::playSFXFile(const std::string& path)
    {
        if (!mCore || !mSFXGroup) return nullptr;

        FMOD::Sound* snd = nullptr;
        if (!ok(mCore->createSound(path.c_str(), FMOD_DEFAULT | FMOD_2D, nullptr, &snd),
            "playSFX createSound"))
            return nullptr;

        FMOD::Channel* ch = nullptr;
        if (!ok(mCore->playSound(snd, mSFXGroup, false, &ch),
            "playSFX playSound"))
            return nullptr;

        return ch;
    }

    FMOD::Channel* AudioSystem::playSFXFileEx(const std::string& path)
    {
        if (!mCore || !mSFXGroup) return nullptr;

        if (!IsSupportedAudioFile(path)) {
            std::string msg = "Unsupported audio format: " + path;
            ShowAudioErrorPopup(msg);
            LOGE(Audio, msg);
            return nullptr;
        }

        FMOD_MODE mode = FMOD_DEFAULT | FMOD_2D | FMOD_CREATESAMPLE;

        FMOD::Sound* snd = nullptr;
        FMOD_RESULT r = mCore->createSound(path.c_str(), mode, nullptr, &snd);

        if (r != FMOD_OK) {
            LOGE(Audio, "Preview Failed (CreateSound): " + path);
            return nullptr;
        }

        FMOD::Channel* ch = nullptr;

        r = mCore->playSound(snd, mSFXGroup, true, &ch);

        if (r != FMOD_OK) {
            snd->release();
            return nullptr;
        }

        if (ch) {
            ch->setMode(FMOD_2D);

            ch->setCallback([](FMOD_CHANNELCONTROL* channelcontrol,
                FMOD_CHANNELCONTROL_TYPE,
                FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
                void*, void*) -> FMOD_RESULT {
                    if (callbacktype == FMOD_CHANNELCONTROL_CALLBACK_END) {
                        FMOD::Channel* ch = (FMOD::Channel*)channelcontrol;
                        FMOD::Sound* snd = nullptr;
                        if (ch->getCurrentSound(&snd) == FMOD_OK && snd) {
                            snd->release();
                        }
                    }
                    return FMOD_OK;
                });

            ch->setPaused(false);
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

    // -------------------------------------------------------------------------
    // NEW: Tagged playback for Unity-like Play/Stop("Name")
    // -------------------------------------------------------------------------

    /*!
    @brief Plays a music file and tags its FMOD channel so it can be stopped by key.
    */
    bool AudioSystem::playMusicFileTagged(const std::string& key,
        const std::string& path, bool loop, float volume)
    {
        if (!mCore || !mMusicGroup)
            return false;

        FMOD::Sound* snd = nullptr;
        if (!ok(mCore->createSound(path.c_str(),
            MusicMode(loop),
            nullptr,
            &snd),
            "createSound(musicTagged)"))
        {
            return false;
        }

        FMOD::Channel* ch = nullptr;
        if (!ok(mCore->playSound(snd, mMusicGroup, false, &ch),
            "playSound(musicTagged)"))
        {
            snd->release();
            return false;
        }

        if (ch)
        {
            ch->setMode(FMOD_2D);
            ch->setVolume(volume);
            mTaggedChannels[key] = ch;
        }

        return true;
    }

    /*!
    @brief Plays a SFX file and tags its FMOD channel so it can be stopped by key.
    */
    bool AudioSystem::playSFXFileTagged(const std::string& key,
        const std::string& path, bool loop, float volume)
    {
        if (!mCore || !mSFXGroup) return false;

        FMOD_MODE mode = FMOD_DEFAULT | FMOD_2D | FMOD_CREATESAMPLE;
        mode |= (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

        FMOD::Sound* snd = nullptr;
        FMOD_RESULT r = mCore->createSound(path.c_str(), mode, nullptr, &snd);
        if (r != FMOD_OK) {
            LOGE(Audio, "SFX File Missing: " + path);
            return false;
        }

        FMOD::Channel* ch = nullptr;

        r = mCore->playSound(snd, mSFXGroup, true, &ch);
        if (r != FMOD_OK) {
            snd->release();
            return false;
        }

        if (ch)
        {
            ch->setMode(FMOD_2D);
            ch->setVolume(volume);

            ch->setCallback([](FMOD_CHANNELCONTROL* channelcontrol,
                FMOD_CHANNELCONTROL_TYPE,
                FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
                void*, void*) -> FMOD_RESULT
                {
                    if (callbacktype == FMOD_CHANNELCONTROL_CALLBACK_END) {
                        FMOD::Channel* ch = (FMOD::Channel*)channelcontrol;
                        FMOD::Sound* snd = nullptr;
                        if (ch->getCurrentSound(&snd) == FMOD_OK && snd) {
                            snd->release();
                        }
                    }
                    return FMOD_OK;
                });

            mTaggedChannels[key] = ch;

            ch->setPaused(false);
        }

        return true;
    }

    /*!
    @brief Stops a previously tagged channel by its string key.
    */
    bool AudioSystem::stopTagged(const std::string& key)
    {
        auto it = mTaggedChannels.find(key);
        if (it == mTaggedChannels.end())
            return false;

        FMOD::Channel* ch = it->second;
        if (ch)
        {
            bool playing = false;
            if (ch->isPlaying(&playing) == FMOD_OK && playing)
            {
                ch->stop();
            }
        }

        mTaggedChannels.erase(it);
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