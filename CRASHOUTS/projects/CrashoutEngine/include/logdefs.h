/*!
@file       logdef.h
@author     mohamadamirakmal.b@digipen.edu
@date       28/09/2025

@brief      Lightweight logging macro lists for Crashout Engine.
//__________________________________________________________________________*/

#pragma once

// Keep one authoritative list of levels & subsystems.
// Add here -> enums, names, UI filters all update automatically.

/*                                                                   macros
----------------------------------------------------------------------------- */
/*!
 @def LOG_LEVELS(X)
 @brief Defines all supported logging severity levels.
 @details
 Expands the provided macro @p X for each severity level.
 This list is used to generate enum members, string names, filters, etc.

*/

#define LOG_LEVELS(X) X(Info) X(Warning) X(Error) X(Success)

/*!
 @def LOG_SYSTEMS(X)
 @brief Defines all subsystems that produce log messages.
 @details
 Expands the provided macro @p X for each subsystem.
 This list is used to generate subsystem enums, string names, and filters.
*/
#define LOG_SYSTEMS(X) X(Core) X(Physics) X(Collision) X(Input) X(AI) X(ECS) X(Renderer) X(UI)