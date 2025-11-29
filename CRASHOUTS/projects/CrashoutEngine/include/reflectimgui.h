/*!
@file       reflectimgui.h
@author     mohamadamirakmal.b@digipen.edu
@date       28/09/2025
@brief      Generic ImGui drawers powered by getml/reflect-cpp (rfl::to_view).
@details    Provides a small set of `draw_field(...)` overloads and one public
            function `draw_reflected(T&)` that iterates the reflected fields
            of any type `T` and renders appropriate ImGui editors.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#pragma once
#include <type_traits>
#include <string>
#include <cstring>
#include <utility>
#include <imgui.h>
#include <rfl/to_view.hpp>
#include <rfl/tuple.hpp>   // for rfl::apply and rfl tuples

// -- scalar & string widgets ------------------------------------------------------------

/*!
 @brief Draws a boolean field as a checkbox.
 @param label Text label for the field.
 @param v Reference to the boolean value.
 @return True if the value was changed.
*/
inline bool draw_field(const char* label, bool& v) { return ImGui::Checkbox(label, &v); }

/*!
 @brief Draws an integer field with drag interaction.
 @param label Text label for the field.
 @param v Reference to the integer value.
 @return True if the value was changed.
*/
inline bool draw_field(const char* label, int& v) { return ImGui::DragInt(label, &v, 1.0f); }

/*!
 @brief Draws a float field with drag interaction.
 @param label Text label for the field.
 @param v Reference to the float value.
 @return True if the value was changed.
*/
inline bool draw_field(const char* label, float& v) { return ImGui::DragFloat(label, &v, 0.1f); }

/*!
 @brief Draws a double field with drag interaction (via float proxy).
 @param label Text label for the field.
 @param v Reference to the double value.
 @return True if the value was changed.
*/
inline bool draw_field(const char* label, double& v) {
    float f = static_cast<float>(v);
    bool changed = ImGui::DragFloat(label, &f, 0.1f);
    if (changed) v = static_cast<double>(f);
    return changed;
}

/*!
 @brief Draws a string field using an editable text box.
 @param label Text label for the field.
 @param s Reference to the string value.
 @return True if the value was changed.
*/
inline bool draw_field(const char* label, std::string& s) {
    char buffer[256]{};
    const size_t copyLen = (s.size() < sizeof(buffer) - 1) ? s.size() : sizeof(buffer) - 1;
    std::memcpy(buffer, s.data(), copyLen);

    bool changed = ImGui::InputText(label, buffer, sizeof(buffer));
    if (changed || ImGui::IsItemDeactivatedAfterEdit())
        s = buffer;
    return changed;
}

// -- traits only for vec-likes ------------------------------------------------------------

/*!
 @brief Trait to detect types with `.x` and `.y` members.
*/
template <class T, class = void>
struct has_xy : std::false_type {};

template <class T>
struct has_xy<T, std::void_t<decltype(std::declval<T&>().x), decltype(std::declval<T&>().y)>> : std::true_type {};

/*!
 @brief Trait to detect types with `.x`, `.y`, and `.z` members.
*/
template <class T, class = void>
struct has_xyz : std::false_type {};

template <class T>
struct has_xyz<T, std::void_t<decltype(std::declval<T&>().x), decltype(std::declval<T&>().y), decltype(std::declval<T&>().z)>> : std::true_type {};

// -- enums: skip  ------------------------------------------------------------

/*!
 @brief Skips drawing for enum types by default.
 @tparam E Enum type.
 @param label Field label (unused).
 @param v Enum reference (unused).
 @return Always returns false.
*/
template <class E>
inline std::enable_if_t<std::is_enum<E>::value, bool>
draw_field(const char* /*label*/, E& /*v*/) { return false; }

// -- vec-like editors ------------------------------------------------------------

/*!
 @brief Draws a vec3-like field (types with `.x`, `.y`, `.z`) as a DragFloat3 widget.
 @tparam T Vec3-like type.
 @param label Text label for the field.
 @param v Reference to the vec3-like value.
 @return True if the value was changed.
*/
template <class T>
inline std::enable_if_t<has_xyz<T>::value, bool>
draw_field(const char* label, T& v) {
    float arr[3]{ (float)v.x, (float)v.y, (float)v.z };
    bool changed = ImGui::DragFloat3(label, arr, 0.1f);
    if (changed || ImGui::IsItemDeactivatedAfterEdit()) {
        v.x = arr[0];
        v.y = arr[1];
        v.z = arr[2];
    }
    return changed;
}

/*!
 @brief Draws a vec2-like field (types with `.x`, `.y`) as a DragFloat2 widget.
 @tparam T Vec2-like type.
 @param label Text label for the field.
 @param v Reference to the vec2-like value.
 @return True if the value was changed.
*/
template <class T>
inline std::enable_if_t<!has_xyz<T>::value&& has_xy<T>::value, bool>
draw_field(const char* label, T& v) {
    float arr[2]{ (float)v.x, (float)v.y };
    bool changed = ImGui::DragFloat2(label, arr, 0.1f);
    if (changed || ImGui::IsItemDeactivatedAfterEdit()) {
        v.x = arr[0];
        v.y = arr[1];
    }
    return changed;
}

// -- fallback for unsupported types ------------------------------------------------------------

/*!
 @brief Fallback overload for unsupported field types.
 @tparam T Field type.
 @param label Field label (unused).
 @param v Field value (unused).
 @return Always returns false.
*/
template <class T>
inline std::enable_if_t<!std::is_enum<T>::value &&
    !std::is_arithmetic<T>::value &&
    !has_xy<T>::value && !has_xyz<T>::value, bool>
    draw_field(const char* /*label*/, T& /*v*/) {
    // Unsupported field type for auto-drawer; handled manually by adapters.
    return false;
}

// -- main API ------------------------------------------------------------

/*!
 @brief Draws all reflected fields of a type as ImGui widgets.
 @tparam T Reflected type.
 @param obj Reference to the object instance to draw.
 @param group_label Optional group label displayed as a separator.
 @return True if any field value was changed.
 @note Requires the type `T` to be compatible with `rfl::to_view`.
*/
template <class T>
bool draw_reflected(T& obj, const char* group_label = nullptr) {
    bool changed = false;
    if (group_label) ImGui::SeparatorText(group_label);

    auto view = rfl::to_view(obj);

    // Iterate rfl::NamedTuple fields: each 'f' has f.name() and f.value()
    view.apply([&](auto f) {
        auto name_lit = f.name(); // compile-time literal with string-view conversion
        std::string_view sv = name_lit; // convert to runtime view
        auto* ptr = f.value(); // pointer to the actual field

        // Visible label is the field name; ID after '##' keeps it unique per field
        char label[128];

        _snprintf_s(label, sizeof(label), _TRUNCATE, "%.*s##%p", (int)sv.size(), sv.data(), (void*)ptr);

        if (draw_field(label, *ptr)) changed = true;
        });

    return changed;
}
