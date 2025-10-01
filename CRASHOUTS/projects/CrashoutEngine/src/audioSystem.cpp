/*!
@file        audioSystem.cpp
@author      afiq.edu
@date        28/9/2025
@brief       audio

*/


#include "audioSystem.h"
#include <cstdio>

using namespace CrashoutsEngine::Audio;

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
    if (!a.empty() && (a.back() == '\\' || a.back() == '/'))
        return a + b;
    return a + sep + b;
}

bool AudioSystem::init(const std::string& banksDir) {
    mBanksDir = banksDir;

    FMOD::Studio::System* rawStudio = nullptr;
    if (!ok(FMOD::Studio::System::create(&rawStudio), "System::create")) return false;
    mStudio.reset(rawStudio);

    if (!ok(mStudio->initialize(1024, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr),
        "System::initialize")) return false;

    FMOD::System* rawCore = nullptr;
    if (!ok(mStudio->getCoreSystem(&rawCore), "getCoreSystem")) return false;
    mCore.reset(rawCore); 

    loadBank("Master.strings.bank");
    loadBank("Master.bank");
    return true;
}

void AudioSystem::update() {
    if (mStudio) mStudio->update();
}

void AudioSystem::shutdown() {
    unloadAllBanks();
    mStudio.reset();
    mCore.reset();
}
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
    return true;
}


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
