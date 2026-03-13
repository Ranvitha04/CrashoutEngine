/*!
@file       CEVec3.h
@author     Shahrin Idzuan
@author     Renfred Lee
@author     Ranvitha
@date       05/09/2025
@brief      3D vector type with common operations

The file defines a 3D vector structure with basic arithmetic and comparison operations.
All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.s
//__________________________________________________________________________*/

#pragma once

#include "CETypes.h"

using CE::f32;

/*!
@brief 3D Vector of single precision float

This struct represents a 3D vector and provides common vector operations such as addition, subtraction, scalar multiplication/division and comparison
*/
struct CEVec3
{
    /// The x component of the vector
    f32 x;
    /// The y component of the vector
    f32 y;
    /// The z component of the vector
    f32 z;

    /*!
    @brief Default constructor initializes vector to (0,0,0)
    */
    CEVec3() : x(0.0f), y(0.0f), z(0.0f) {}

    // Copy constructor
    CEVec3(const CEVec3&) = default;

    // Copy assignment
    CEVec3& operator=(const CEVec3&) = default;

    // Move constructor
    CEVec3(CEVec3&&) noexcept = default;

    // Move assignment
    CEVec3& operator=(CEVec3&&) noexcept = default;

    /*!
    @brief Construct from x, y and z values
    @param x The x component
    @param y The y component
    @param z The z component
    */
    CEVec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}

    /*!
    @brief Unary plus operator. Returns the vector as it is
    */
    CEVec3 operator+() const { return *this; }

    /*!
    @brief Unary minus operator. Returns the negative of the vector
    */
    CEVec3 operator-() const { return CEVec3(-x, -y, -z); }

    // ---------------------------------------------------------------------
    // Vector + Vector
    // ---------------------------------------------------------------------

    /*!
    @brief Add two vectors
    @param rhs The vector to add
    @return A new vector representing the sum
    */
    CEVec3 operator+(const CEVec3& rhs) const
    {
        return CEVec3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    /*!
    @brief Subtract two vectors
    @param rhs The vector to subtract
    @return A new vector representing the result
    */
    CEVec3 operator-(const CEVec3& rhs) const
    {
        return CEVec3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    /*!
    @brief Add assign operator
    @param rhs The vector to add
    @return Reference to this vector after addition
    */
    CEVec3& operator+=(const CEVec3& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    /*!
    @brief Subtract assign operator
    @param rhs The vector to subtract
    @return Reference to this vector after subtraction
    */
    CEVec3& operator-=(const CEVec3& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    /*!
    @brief Multiply by a scalar
    @param scalar The scalar multiplier
    @return A new scaled vector
    */
    CEVec3 operator*(f32 scalar) const
    {
        return CEVec3(x * scalar, y * scalar, z * scalar);
    }

    /*!
    @brief Divide by a scalar
    @param scalar The scalar divisor
    @return A new scaled vector
    */
    CEVec3 operator/(f32 scalar) const
    {
        return CEVec3(x / scalar, y / scalar, z / scalar);
    }

    /*!
    @brief Multiplication assign by a scalar
    @param scalar The scalar multiplier
    @return Reference to this vector after scaling
    */
    CEVec3& operator*=(f32 scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    /*!
    @brief Division assign by a scalar
    @param scalar The scalar divisor
    @return Reference to this vector after scaling
    */
    CEVec3& operator/=(f32 scalar)
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    /*!
    @brief Equality operator
    @param rhs The vector to compare
    @return True if all components are equal
    */
    bool operator==(const CEVec3& rhs) const
    {
        return (x == rhs.x) && (y == rhs.y) && (z == rhs.z);
    }

    /*!
    @brief Inequality operator
    @param rhs The vector to compare
    @return True if any component differs
    */
    bool operator!=(const CEVec3& rhs) const
    {
        return !(*this == rhs);
    }
};

/*!
@brief Multiply a scalar by a vector (scalar * vector)
@param scalar The scalar multiplier
@param vector The input vector
@return A new scaled vector
*/
inline CEVec3 operator*(f32 scalar, const CEVec3& vector)
{
    return CEVec3(vector.x * scalar, vector.y * scalar, vector.z * scalar);
}
