/*!
@file       CEMath.h
@author     m.biniswandi@digipen.edu
@date       05/09/2025
@brief      Math utilities for 2D vector operations

Provides common vector math operations and a constant for PI
//__________________________________________________________________________*/

#pragma once

#include "CEVec2.h"
#include <cmath>
using CE::f32;

namespace CEMath
{
	/*!
	@brief Constant value of pi
	*/
	constexpr f32 PI = 3.1415927f;

	/*!
	@brief Calculate the dot product of two 2D vectors
	@param leftVector  The first vector
	@param rightVector The second vector
	@return The dot product
	*/
	inline f32 Dot(const CEVec2& leftVector, const CEVec2& rightVector)
	{
		return (leftVector.x * rightVector.x) + (leftVector.y * rightVector.y);
	}

	/*!
	@brief Calculate the squared length of a vector
	@param vector The input vector
	@return The squared length of the vector
	*/
	inline f32 LengthSq(const CEVec2& vector)
	{
		return Dot(vector, vector);
	}

	/*!
	@brief Calculate the length of a vector
	@param vector The input vector
	@return The length of the vector
	*/
	inline f32 Length(const CEVec2& vector)
	{
		return std::sqrt(LengthSq(vector));
	}

	/*!
    @brief Normalize a vector to unit length
    @param vector The input vector
    @return A normalized vector or zero vector if the input has zero length
    */
	inline CEVec2 Normalize(const CEVec2& vector)
	{
		f32 length = Length(vector);
		if (length != 0.0f)
		{
			return CEVec2{ vector.x / length, vector.y / length };
		}
		return CEVec2{ 0.0f, 0.0f }; //return zero vector if length is zero to avoid division by zero
	}


}