/*!
@file       uuidcomponent.h
@author     Renfred Lee
@date       19/02/2026
@brief      Unique ID component.
@details    Component for UUID for identification in all entities.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#ifndef UUID_COMPONENT_H
#define UUID_COMPONENT_H

#pragma once
#include <cstdint>
#include <random>

namespace Component {
    /** @brief UUIDComponent */
    struct UUIDComponent {
        uint64_t id;

        // Auto-generate on creation
        UUIDComponent() {
            static std::random_device rd;
            static std::mt19937_64 eng(rd());
            static std::uniform_int_distribution<uint64_t> dist;
            id = dist(eng);
        }

        // Allow manual assignment during deserialization
        UUIDComponent(uint64_t assigned_id) : id(assigned_id) {}
    };
}

#endif