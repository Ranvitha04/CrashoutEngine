/*!
@file   CEColor.h
@author Ranvitha (68%), Renfred Lee (32%)
@date   23/11/2025

@brief  Vector 4 for colors (Struct based, Contiguous memory)

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

#include "CETypes.h"
#include <cassert> // Required for assert

using CE::f32;

/*!
@brief Vector 4 of single precision float representing a Color.
       Stored as r, g, b, a but accessible via array index or x, y, z, w aliases.
*/
/** @brief CEColor */
struct CEColor
{
    // ---------------------------------------------------------------------
    // 1. Actual Data Storage
    // ---------------------------------------------------------------------
    f32 r, g, b, a;

    // ---------------------------------------------------------------------
    // 2. Constructors
    // ---------------------------------------------------------------------

    /*!
    @brief Default constructor initializes vector to White (1,1,1,1)
    */
    CEColor() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}

    /*!
    @brief Construct from r, g, b and a values
    @param _r The red component
    @param _g The green component
    @param _b The blue component
    @param _a The alpha component
    */
    CEColor(f32 _r, f32 _g, f32 _b, f32 _a = 1.f) : r(_r), g(_g), b(_b), a(_a) {}

    // Copy constructor
    CEColor(const CEColor&) = default;

    // Copy assignment
    CEColor& operator=(const CEColor&) = default;

    // Move constructor
    CEColor(CEColor&&) noexcept = default;

    // Move assignment
    CEColor& operator=(CEColor&&) noexcept = default;

    // ---------------------------------------------------------------------
    // 3. Accessors (Aliases for x, y, z, w)
    // ---------------------------------------------------------------------

    // Returning a reference (&) allows assignment: color.x() = 5.0f;
    f32& x() { return r; }
    f32& y() { return g; }
    f32& z() { return b; }
    f32& w() { return a; }

    // Const Accessors (for const CEColor objects)
    f32 x() const { return r; }
    f32 y() const { return g; }
    f32 z() const { return b; }
    f32 w() const { return a; }

    // ---------------------------------------------------------------------
    // 4. Array Access
    // ---------------------------------------------------------------------

    /*!
    @brief Array subscript operator for read/write access.
    @note Relies on standard struct memory packing (r, g, b, a are contiguous).
    @param index Index of the component (0 to 3).
    @return Reference to the component.
    */
    f32& operator[](int index) {
        assert(index >= 0 && index < 4 && "CEColor index out of bounds");
        // Taking the address of 'r' and treating it as the start of an array
        return (&r)[index];
    }

    /*!
    @brief Array subscript operator for read-only access.
    @param index Index of the component (0 to 3).
    @return Const reference to the component.
    */
    const f32& operator[](int index) const {
        assert(index >= 0 && index < 4 && "CEColor index out of bounds");
        return (&r)[index];
    }

    // ---------------------------------------------------------------------
    // 5. Unary Operators
    // ---------------------------------------------------------------------

    /*!
    @brief Unary plus operator. Returns the vector as it is
    */
    CEColor operator+() const { return *this; }

    /*!
    @brief Unary minus operator. Returns the negative of the vector
    */
    CEColor operator-() const { return CEColor(-r, -g, -b, -a); }

    // ---------------------------------------------------------------------
    // 6. Binary Arithmetic Operators
    // ---------------------------------------------------------------------

    /*!
    @brief Add two vectors
    */
    CEColor operator+(const CEColor& rhs) const {
        return CEColor(r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a);
    }

    /*!
    @brief Subtract two vectors
    */
    CEColor operator-(const CEColor& rhs) const {
        return CEColor(r - rhs.r, g - rhs.g, b - rhs.b, a - rhs.a);
    }

    /*!
    @brief Multiply by a scalar
    */
    CEColor operator*(f32 scalar) const {
        return CEColor(r * scalar, g * scalar, b * scalar, a * scalar);
    }

    /*!
    @brief Divide by a scalar
    */
    CEColor operator/(f32 scalar) const {
        // Optional: Add assert(scalar != 0.0f) here if desired
        f32 inv = 1.0f / scalar;
        return CEColor(r * inv, g * inv, b * inv, a * inv);
    }

    // ---------------------------------------------------------------------
    // 7. Assignment Operators
    // ---------------------------------------------------------------------

    CEColor& operator+=(const CEColor& rhs) {
        r += rhs.r; g += rhs.g; b += rhs.b; a += rhs.a;
        return *this;
    }

    CEColor& operator-=(const CEColor& rhs) {
        r -= rhs.r; g -= rhs.g; b -= rhs.b; a -= rhs.a;
        return *this;
    }

    CEColor& operator*=(f32 scalar) {
        r *= scalar; g *= scalar; b *= scalar; a *= scalar;
        return *this;
    }

    CEColor& operator/=(f32 scalar) {
        f32 inv = 1.0f / scalar;
        r *= inv; g *= inv; b *= inv; a *= inv;
        return *this;
    }

    // ---------------------------------------------------------------------
    // 8. Comparison Operators
    // ---------------------------------------------------------------------

    bool operator==(const CEColor& rhs) const {
        return (r == rhs.r) && (g == rhs.g) && (b == rhs.b) && (a == rhs.a);
    }

    bool operator!=(const CEColor& rhs) const {
        return !(*this == rhs);
    }
};

// ---------------------------------------------------------------------
// Non-Member Operators
// ---------------------------------------------------------------------

/*!
@brief Multiply a scalar by a vector (scalar * vector)
Allows for commutative multiplication: 5.0f * color
*/
inline CEColor operator*(f32 scalar, const CEColor& vector)
{
    return CEColor(vector.r * scalar, vector.g * scalar, vector.b * scalar, vector.a * scalar);
}