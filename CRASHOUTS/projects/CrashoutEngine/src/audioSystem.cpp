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
#include <cstdio>
#include <algorithm>

using namespace CrashoutsEngine::Audio;
//One audio system
AudioSystem& AudioSystem::instance() {
    static AudioSystem s;
    return s;
}

bool AudioSystem::ok(FMOD_RESULT r, const char* where) {
    if (r != FMOD_OK) {
        std::fprintf(stderr, "[FMOD] %s -> %d (%s)\n", where, r, FMOD_ErrorString(r));
        return false;
    }
    return true;
}
static std::string joinPath(const std::string& a, const std::string& b) {
#ifdef _WIN32
    const char sep = '\\';
#else
    const char sep = '/';
#endif
    if (a.empty()) return b;
    if (!a.empty() && (a.back() == '\\' || a.back() == '/')) return a + b;
    return a + sep + b;
}
//Start fmod to play sounds
bool AudioSystem::init(const std::string& banksDir) {
    mBanksDir = banksDir;
    // Create the main FMOD Studio system 
    FMOD::Studio::System* rawStudio = nullptr;
    if (!ok(FMOD::Studio::System::create(&rawStudio), "System::create")) return false;
    mStudio.reset(rawStudio);
    //Turn on the system with some settings
    if (!ok(mStudio->initialize(1024, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr),
        "System::initialize")) return false;

    FMOD::System* rawCore = nullptr;
    if (!ok(mStudio->getCoreSystem(&rawCore), "getCoreSystem")) return false;
    mCore.reset(rawCore); 
    //Load main bank
    //loadBank("banks/Master.strings.bank");
    //loadBank("banks/Master.bank");
    return true;
}
//Update fmod
void AudioSystem::update() {
    if (mStudio) mStudio->update();
}
//Close everything
void AudioSystem::shutdown() {
    unloadAllBanks();
    mStudio.reset();
    mCore.reset();
}
//load bank from file
bool AudioSystem::loadBank(const std::string& bankFile) {
    if (!mStudio) return false;
    if (mBanks.count(bankFile)) return true;

    std::string full = joinPath(mBanksDir, bankFile);  
    FMOD::Studio::Bank* b = 0;
    if (!ok(mStudio->loadBankFile(full.c_str(),
        FMOD_STUDIO_LOAD_BANK_NORMAL, &b),
        ("loadBankFile " + bankFile).c_str())) {
        return false;
    }

    mBanks.insert(std::make_pair(bankFile, b));
    if (mStudio) mStudio->flushCommands();
    return true;
}
//Stop all sound
bool AudioSystem::stopAllOnBus(const std::string& busPath, bool fadeOut) {
    if (!mStudio) return false;

    FMOD::Studio::Bus* bus = nullptr;
    FMOD_RESULT r = mStudio->getBus(busPath.c_str(), &bus);
    ok(r, "getBus");                 
    if (r != FMOD_OK || !bus) return false;

    r = bus->stopAllEvents(fadeOut ? FMOD_STUDIO_STOP_ALLOWFADEOUT
        : FMOD_STUDIO_STOP_IMMEDIATE);
    ok(r, "stopAllEvents");            
    mStudio->flushCommands();          
    return r == FMOD_OK;
}


bool AudioSystem::stopAll(bool fadeOut) {
    // Master bus stops *everything* (music + sfx)
    return stopAllOnBus("bus:/", fadeOut);
}
//Testing volume
bool AudioSystem::setVCAVolume(const std::string& vcaPath, float vol01) {
    if (!mStudio) return false;
    FMOD::Studio::VCA* vca = nullptr;
    FMOD_RESULT r = mStudio->getVCA(vcaPath.c_str(), &vca);
    ok(r, "getVCA");
    if (r != FMOD_OK || !vca) return false;

    r = vca->setVolume(vol01);
    if (mStudio) mStudio->flushCommands();
    return r == FMOD_OK;
}

bool AudioSystem::setBusVolume(const std::string& busPath, float vol01)
{
    FMOD::Studio::Bus* bus = nullptr;
    FMOD_RESULT result = mStudio->getBus(busPath.c_str(), &bus);
    if (result != FMOD_OK || !bus) return false;
    return bus->setVolume(vol01) == FMOD_OK;
}
//Unload
void AudioSystem::unloadAllBanks() {
    for (std::unordered_map<std::string, FMOD::Studio::Bank*>::iterator it = mBanks.begin();
        it != mBanks.end(); ++it) {
        FMOD::Studio::Bank* bank = it->second;
        if (bank) bank->unload();
    }
    mBanks.clear();
}


FMOD::Studio::EventDescription* AudioSystem::getEventDesc(const std::string& path) {
    if (!mStudio) return nullptr;
    FMOD::Studio::EventDescription* d = nullptr;
    if (!ok(mStudio->getEvent(path.c_str(), &d), "getEvent")) return nullptr;
    return d;
}
//Play one song
bool AudioSystem::playOneShot(const std::string& eventPath) {
    auto* d = getEventDesc(eventPath);
    if (!d) return false;
    FMOD::Studio::EventInstance* inst = nullptr;
    if (!ok(d->createInstance(&inst), "createInstance")) return false;
    ok(inst->start(), "start");
    ok(inst->release(), "release");
    return true;
}

FMOD::Studio::EventInstance* AudioSystem::createInstance(const std::string& eventPath) {
    auto* d = getEventDesc(eventPath);
    if (!d) return nullptr;
    FMOD::Studio::EventInstance* inst = nullptr;
    if (!ok(d->createInstance(&inst), "createInstance")) return nullptr;
    return inst; 
}

bool AudioSystem::listAllEventPaths(std::vector<std::string>& out) {
    out.clear();
    if (!mStudio) return false;
    mStudio->flushCommands();

    std::vector<std::string> paths;
    for (auto& kv : mBanks) {
        FMOD::Studio::Bank* bank = kv.second;
        if (!bank) continue;

        int evCount = 0;
        if (!ok(bank->getEventCount(&evCount), "Bank::getEventCount")) continue;
        if (evCount <= 0) continue;

        std::vector<FMOD::Studio::EventDescription*> evs(evCount, nullptr);
        if (!ok(bank->getEventList(evs.data(), evCount, &evCount), "Bank::getEventList")) continue;

        for (int i = 0; i < evCount; ++i) {
            char path[512] = {};
            int retrieved = 0;
            if (evs[i] && evs[i]->getPath(path, sizeof(path), &retrieved) == FMOD_OK && retrieved > 0) {
                paths.emplace_back(path);
            }
        }
    }

    std::sort(paths.begin(), paths.end());
    paths.erase(std::unique(paths.begin(), paths.end()), paths.end());
    out.swap(paths);
    return true;
}

bool AudioSystem::listEventPathsFromBank(const std::string& bankFile, std::vector<std::string>& out) {
    out.clear();
    if (!mStudio) return false;

    // Use already-loaded bank if present; else load temporarily.
    FMOD::Studio::Bank* bank = nullptr;
    auto it = mBanks.find(bankFile);
    bool tempLoaded = false;

    if (it != mBanks.end()) {
        bank = it->second;
    }
    else {
        std::string full = joinPath(mBanksDir, bankFile);
        if (!ok(mStudio->loadBankFile(full.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank), "loadBankFile (inspect)")) {
            return false;
        }
        tempLoaded = true;
    }

    int evCount = 0;
    ok(bank->getEventCount(&evCount), "Bank::getEventCount");
    if (evCount > 0) {
        std::vector<FMOD::Studio::EventDescription*> evs(evCount, nullptr);
        ok(bank->getEventList(evs.data(), evCount, &evCount), "Bank::getEventList");
        for (int i = 0; i < evCount; ++i) {
            char path[512] = {};
            int retrieved = 0;
            if (evs[i] && evs[i]->getPath(path, sizeof(path), &retrieved) == FMOD_OK && retrieved > 0) {
                out.emplace_back(path);
            }
        }
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
    }

    if (tempLoaded && bank) bank->unload();
    return true;
}

void AudioSystem::inspectBankToConsole(const std::string& bankFile) {
    FMOD::Studio::Bank* bank = nullptr;
    std::string full = joinPath(mBanksDir, bankFile);
    if (!ok(mStudio->loadBankFile(full.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank), "loadBankFile (inspect console)"))
        return;

    int count = 0;
    std::fprintf(stdout, "\n--- Inspecting bank: %s ---\n", bankFile.c_str());

    // Events
    ok(bank->getEventCount(&count), "getEventCount");
    if (count > 0) {
        std::vector<FMOD::Studio::EventDescription*> list(count);
        ok(bank->getEventList(list.data(), count, &count), "getEventList");
        std::fprintf(stdout, "Events (%d):\n", count);
        for (auto* d : list) {
            char p[512]; int n = 0;
            if (d && d->getPath(p, sizeof(p), &n) == FMOD_OK) std::fprintf(stdout, "  %s\n", p);
        }
    }

    // Buses
    ok(bank->getBusCount(&count), "getBusCount");
    if (count > 0) {
        std::vector<FMOD::Studio::Bus*> list(count);
        ok(bank->getBusList(list.data(), count, &count), "getBusList");
        std::fprintf(stdout, "Buses (%d):\n", count);
        for (auto* b : list) {
            char p[512]; int n = 0;
            if (b && b->getPath(p, sizeof(p), &n) == FMOD_OK) std::fprintf(stdout, "  %s\n", p);
        }
    }

    // VCAs
    ok(bank->getVCACount(&count), "getVCACount");
    if (count > 0) {
        std::vector<FMOD::Studio::VCA*> list(count);
        ok(bank->getVCAList(list.data(), count, &count), "getVCAList");
        std::fprintf(stdout, "VCAs (%d):\n", count);
        for (auto* v : list) {
            char p[512]; int n = 0;
            if (v && v->getPath(p, sizeof(p), &n) == FMOD_OK) std::fprintf(stdout, "  %s\n", p);
        }
    }

    bank->unload();
}