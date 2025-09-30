/*!
@file       logdef.h
@author     mohamadamirakmal.b@digipen.edu
@date       28/09/2025

@brief      Lightweight logging macro lists for Crashout Engine.
//__________________________________________________________________________*/

#pragma once

// Keep one authoritative list of levels & subsystems.
// Add here -> enums, names, UI filters all update automatically.

#define LOG_LEVELS(X) X(Info) X(Warning) X(Error) X(Success)

#define LOG_SYSTEMS(X) X(Core) X(Physics) X(Collision) X(Input) X(AI) X(ECS) X(Renderer) X(UI)