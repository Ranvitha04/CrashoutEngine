/*!
@file   CEVec2.h
@author Shahrin Idzuan (46%), Renfred Lee (34%), Ranvitha (20%)
@date   05/09/2025

@brief  2D vector type with common operations

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

#include "CETypes.h"

using CE::f32;

/*!
@brief 2D Vector of single precision float

This struct represents a 2D vector and provides common vector operations such as addition, subtraction, scalar multiplication/division and comparison
*/
/** @brief CEVec2 */
struct CEVec2
{
	/// The x component of the vector
	f32 x;

	/// The y component of the vector
	f32 y;

	/*!
	@brief Default constructor initializes vector to (0,0)
	*/
	CEVec2() : x(0.0f), y(0.0f) {}

	/*!
	@brief Construct from x and y values
	@param x The x component
	@param y The y component
	*/
	CEVec2(f32 x, f32 y) : x(x), y(y) {}

	// Copy constructor
	CEVec2(const CEVec2&) = default;

	// Copy assignment
	CEVec2& operator=(const CEVec2&) = default;

	// Move constructor
	CEVec2(CEVec2&&) noexcept = default;

	// Move assignment
	CEVec2& operator=(CEVec2&&) noexcept = default;

	//unary plus and minus
	/*!
	@brief Unary plus operator. Returns the vector as it is
	*/
	CEVec2 operator+() const
	{
		return *this;
	}

	/*!
	@brief Unary minus operator. Returns the negative of the vector
	*/
	CEVec2 operator-() const
	{
		return CEVec2(-x, -y);
	}

	/*!
	@brief Add two vectors
	@param rhs The vector to add
	@return A new vector representing the sum
	*/
	CEVec2 operator+(const CEVec2& rhs) const
	{
		return CEVec2(x + rhs.x, y + rhs.y);
	}

	/*!
	@brief Subtract two vectors
	@param rhs The vector to subtract
	@return A new vector representing the result
	*/
	CEVec2 operator-(const CEVec2& rhs) const
	{
		return CEVec2(x - rhs.x, y - rhs.y); 
	}

	/*!
	@brief Add assign operator
	@param rhs The vector to add
	@return Reference to this vector after addition
	*/
	CEVec2& operator+=(const CEVec2& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	/*!
	@brief Subtract assign operator
	@param rhs The vector to subtract
	@return Reference to this vector after subtraction
	*/
	CEVec2& operator-=(const CEVec2& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}

	//scalar operation
	/*!
	@brief Multiply by a scalar
	@param scalar The scalar multiplier
	@return A new scaled vector
	*/
	CEVec2 operator*(f32 scalar) const
	{
		return CEVec2(x * scalar, y * scalar);
	}

	/*!
	@brief Divide by a scalar
	@param scalar The scalar divisor
	@return A new scaled vector
	*/
	CEVec2 operator/(f32 scalar) const
	{
		return CEVec2(x / scalar, y / scalar);
	}

	/*!
	@brief Multiplication assign by a scalar
	@param scalar The scalar multiplier
	@return Reference to this vector after scaling
	*/
	CEVec2& operator*=(f32 scalar)
	{
		x *= scalar;
		y *= scalar;
		return *this;
	}

	/*!
	@brief Division assign by a scalar
	@param scalar The scalar divisor
	@return Reference to this vector after scaling
	*/
	CEVec2& operator/=(f32 scalar)
	{
		x /= scalar;
		y /= scalar;
		return *this;
	}

	//comparison probably needed down the line
	/*!
	@brief Equality operator
	@param rhs The vector to compare
	@return True if both are equal
	*/
	bool operator==(const CEVec2& rhs) const
	{
		return (x == rhs.x) && (y == rhs.y);
	}

	/*!
    @brief Inequality operator.
    @param rhs The vector to compare
    @return True if any component differs
    */
	bool operator!=(const CEVec2& rhs) const
	{
		return !(*this == rhs);
	}
};

//lets u do scalar * vector if not it will not work
/*!
@brief Multiply a scalar by a vector

Allows scalar * vector as well as vector * scalar

@param scalar The scalar multiplier
@param vector The input vector
@return A new scaled vector
*/
inline CEVec2 operator*(f32 scalar, const CEVec2& vector)
{
	return CEVec2(vector.x * scalar, vector.y * scalar);
}

