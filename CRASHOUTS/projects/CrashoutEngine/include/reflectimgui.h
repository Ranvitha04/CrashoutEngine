/*!
@file       reflectimgui.h
@author     mohamadamirakmal.b@digipen.edu
@date       28/09/2025
@brief      Generic ImGui drawers powered by getml/reflect-cpp (rfl::to_view).
@details    Provides a small set of `draw_field(...)` overloads and one public
            function `draw_reflected(T&)` that iterates the reflected fields
            of any type `T` and renders appropriate ImGui editors.
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#pragma once
#include <type_traits>
#include <tuple>
#include <string>
#include <cstring>
#include <utility>
#include <initializer_list>
#include <imgui.h>
#include <rfl/to_view.hpp>
#include <rfl/tuple.hpp>   // for rfl::apply and rfl tuples

// -- scalar & string widgets ------------------------------------------------------------

inline bool draw_field(const char* label, bool& v) { return ImGui::Checkbox(label, &v); }
inline bool draw_field(const char* label, int& v) { return ImGui::DragInt(label, &v, 1.0f); }
inline bool draw_field(const char* label, float& v) { return ImGui::DragFloat(label, &v, 0.1f); }
inline bool draw_field(const char* label, double& v) { float f = (float)v; bool ch = ImGui::DragFloat(label, &f, 0.1f); if (ch) v = (double)f; return ch; }

inline bool draw_field(const char* label, std::string& s) {
    char buf[256]{};
    const size_t n = (s.size() < sizeof(buf) - 1) ? s.size() : sizeof(buf) - 1;
    std::memcpy(buf, s.data(), n);
    bool ch = ImGui::InputText(label, buf, sizeof(buf));
    if (ch || ImGui::IsItemDeactivatedAfterEdit()) s = buf;
    return ch;
}

// -- traits only for vec-likes ------------------------------------------------------------

template <class T, class = void> struct has_xy : std::false_type {};
template <class T> struct has_xy<T, std::void_t<decltype(std::declval<T&>().x), decltype(std::declval<T&>().y)>> : std::true_type {};
template <class T, class = void> struct has_xyz : std::false_type {};
template <class T> struct has_xyz<T, std::void_t<decltype(std::declval<T&>().x), decltype(std::declval<T&>().y), decltype(std::declval<T&>().z)>> : std::true_type {};

// -- enums: skip  ------------------------------------------------------------

template <class E>
inline std::enable_if_t<std::is_enum<E>::value, bool>
draw_field(const char*, E&) { return false; }

// -- vec-like editors ------------------------------------------------------------

template <class T>
inline std::enable_if_t<has_xyz<T>::value, bool>
draw_field(const char* label, T& v) {
    float a[3]{ (float)v.x, (float)v.y, (float)v.z };
    bool ch = ImGui::DragFloat3(label, a, 0.1f);
    if (ch || ImGui::IsItemDeactivatedAfterEdit()) { v.x = a[0]; v.y = a[1]; v.z = a[2]; }
    return ch;
}

template <class T>
inline std::enable_if_t<!has_xyz<T>::value&& has_xy<T>::value, bool>
draw_field(const char* label, T& v) {
    float a[2]{ (float)v.x, (float)v.y };
    bool ch = ImGui::DragFloat2(label, a, 0.1f);
    if (ch || ImGui::IsItemDeactivatedAfterEdit()) { v.x = a[0]; v.y = a[1]; }
    return ch;
}

/* =====================  fallback for unsupported types  ===================== */
// -- fallback for unsupported types ------------------------------------------------------------


template <class T>
inline std::enable_if_t<!std::is_enum<T>::value
    && !std::is_arithmetic<T>::value
    && !has_xy<T>::value && !has_xyz<T>::value, bool>
    draw_field(const char* /*label*/, T& /*v*/) {
    // Unsupported field type for auto-drawer; handled manually by adapters.
    return false;
}

// -- main API ------------------------------------------------------------

template <class T>
bool draw_reflected(T& obj, const char* group_label = nullptr) {
    bool changed = false;
    if (group_label) ImGui::SeparatorText(group_label);

    auto view = rfl::to_view(obj);
    auto values = view.values();   // tuple (rfl or std) of pointers

    // Iterate field pointers using rfl::apply to avoid std::tuple_size specializations
    rfl::apply([&](auto*... ptrs) {
        size_t i = 0;
        ((ptrs ? ([&] {
            char id[32];
#if defined(_MSC_VER)
            _snprintf_s(id, sizeof(id), _TRUNCATE, "Field##%zu", i++);
#else
            std::snprintf(id, sizeof(id), "Field##%zu", i++);
#endif
            if (draw_field(id, *ptrs)) changed = true;
            }(), 0) : 0), ...);
        }, values);

    return changed;
}