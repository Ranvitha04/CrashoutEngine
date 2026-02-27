/*!
@file       key.h
@author     m.biniswandi@digipen.edu
@date       08/09/2025
@brief Definition of the Key enum class

Provides a wrapper around GLFW key codes

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once


#include <GLFW/glfw3.h>
#include <string>

 /**
  * @enum Key
  * @brief Represents keyboard keys mapped to GLFW key codes
  *
  * Includes letters numbers symbols arrow keys and common control keys
  */
enum class Key {
    /// letterssss
    A = GLFW_KEY_A, B = GLFW_KEY_B, C = GLFW_KEY_C, D = GLFW_KEY_D,
    E = GLFW_KEY_E, F = GLFW_KEY_F, G = GLFW_KEY_G, H = GLFW_KEY_H,
    I = GLFW_KEY_I, J = GLFW_KEY_J, K = GLFW_KEY_K, L = GLFW_KEY_L,
    M = GLFW_KEY_M, N = GLFW_KEY_N, O = GLFW_KEY_O, P = GLFW_KEY_P,
    Q = GLFW_KEY_Q, R = GLFW_KEY_R, S = GLFW_KEY_S, T = GLFW_KEY_T,
    U = GLFW_KEY_U, V = GLFW_KEY_V, W = GLFW_KEY_W, X = GLFW_KEY_X,
    Y = GLFW_KEY_Y, Z = GLFW_KEY_Z,

    /// numbers
    Num0 = GLFW_KEY_0, Num1 = GLFW_KEY_1, Num2 = GLFW_KEY_2, Num3 = GLFW_KEY_3,
    Num4 = GLFW_KEY_4, Num5 = GLFW_KEY_5, Num6 = GLFW_KEY_6,
    Num7 = GLFW_KEY_7, Num8 = GLFW_KEY_8, Num9 = GLFW_KEY_9,

    /// symbols and special keys
    Slash = GLFW_KEY_SLASH,
    Backslash = GLFW_KEY_BACKSLASH,
    LeftBracket = GLFW_KEY_LEFT_BRACKET,
    RightBracket = GLFW_KEY_RIGHT_BRACKET,
    Semicolon = GLFW_KEY_SEMICOLON,
    Apostrophe = GLFW_KEY_APOSTROPHE,
    Comma = GLFW_KEY_COMMA,
    Period = GLFW_KEY_PERIOD,
    Minus = GLFW_KEY_MINUS,
    Equal = GLFW_KEY_EQUAL,
	LeftCtrl = GLFW_KEY_LEFT_CONTROL,

    F3 = GLFW_KEY_F3,

    /// arrows
    Up = GLFW_KEY_UP,
    Down = GLFW_KEY_DOWN,
    Left = GLFW_KEY_LEFT,
    Right = GLFW_KEY_RIGHT,

    /// misc stuff
    Space = GLFW_KEY_SPACE,
    Escape = GLFW_KEY_ESCAPE,
    Enter = GLFW_KEY_ENTER,
    Tab = GLFW_KEY_TAB,
    Backspace = GLFW_KEY_BACKSPACE,

    /// mouse buttons
    MouseLeft = GLFW_MOUSE_BUTTON_LEFT,
    MouseRight = GLFW_MOUSE_BUTTON_RIGHT,
    MouseMiddle = GLFW_MOUSE_BUTTON_MIDDLE,
    Mouse4 = GLFW_MOUSE_BUTTON_4,
    Mouse5 = GLFW_MOUSE_BUTTON_5,
    Mouse6 = GLFW_MOUSE_BUTTON_6,
    Mouse7 = GLFW_MOUSE_BUTTON_7,
    Mouse8 = GLFW_MOUSE_BUTTON_8
};
//i died here - Rain
inline std::string KeyToString(Key key) {
    switch (key) {

    case Key::A: return "A"; case Key::B: return "B"; case Key::C: return "C";
    case Key::D: return "D"; case Key::E: return "E"; case Key::F: return "F";
    case Key::G: return "G"; case Key::H: return "H"; case Key::I: return "I";
    case Key::J: return "J"; case Key::K: return "K"; case Key::L: return "L";
    case Key::M: return "M"; case Key::N: return "N"; case Key::O: return "O";
    case Key::P: return "P"; case Key::Q: return "Q"; case Key::R: return "R";
    case Key::S: return "S"; case Key::T: return "T"; case Key::U: return "U";
    case Key::V: return "V"; case Key::W: return "W"; case Key::X: return "X";
    case Key::Y: return "Y"; case Key::Z: return "Z";

    case Key::Num0: return "0"; case Key::Num1: return "1"; case Key::Num2: return "2";
    case Key::Num3: return "3"; case Key::Num4: return "4"; case Key::Num5: return "5";
    case Key::Num6: return "6"; case Key::Num7: return "7"; case Key::Num8: return "8";
    case Key::Num9: return "9";

    case Key::Slash: return "/";
    case Key::Backslash: return "\\";
    case Key::LeftBracket: return "[";
    case Key::RightBracket: return "]";
    case Key::Semicolon: return ";";
    case Key::Apostrophe: return "'";
    case Key::Comma: return ",";
    case Key::Period: return ".";
    case Key::Minus: return "-";
    case Key::Equal: return "=";
	case Key::LeftCtrl: return "Left Control";

    case Key::Up: return "Arrow Up";
    case Key::Down: return "Arrow Down";
    case Key::Left: return "Arrow Left";
    case Key::Right: return "Arrow Right";

    case Key::Space: return "Space";
    case Key::Escape: return "Escape";
    case Key::Enter: return "Enter";
    case Key::Tab: return "Tab";
    case Key::Backspace: return "Backspace";
    
    case Key::F3: return "F3";

    case Key::MouseLeft: return "Mouse Left";
    case Key::MouseRight: return "Mouse Right";
    case Key::MouseMiddle: return "Mouse Middle";
    case Key::Mouse4: return "Mouse 4";
    case Key::Mouse5: return "Mouse 5";
    case Key::Mouse6: return "Mouse 6";
    case Key::Mouse7: return "Mouse 7";
    case Key::Mouse8: return "Mouse 8";

    default: return "Unknown";
    }
}
