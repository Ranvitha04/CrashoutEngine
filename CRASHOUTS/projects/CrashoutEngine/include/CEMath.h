/*!
@file       CEMath.h
@author     m.biniswandi@digipen.edu
@date       05/09/2025
@brief      Math utilities for 2D vector operations

Provides common vector math operations and a constant for PI
All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.
//__________________________________________________________________________*/

#pragma once

#include "CEVec2.h"
#include <cmath>
#include "CEVec3.h"
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

	/*!
	@brief Returns a vector rotated 90 degrees counter-clockwise
	@param v  The vector2
	@return The normal to v
	*/
	inline CEVec2 GetNormal(const CEVec2& v) {
		return CEVec2{ -v.y, v.x };
	}

	/*!
	@brief Distance between two vec2 points
	@param a  The first vector
	@param b The second vector
	@return The dot product
	*/
	inline f32 Distance(const CEVec2& a, const CEVec2& b) {
		return Length(CEVec2{ b.x - a.x, b.y - a.y });
	}

	/*!
	@brief Calculate the dot product of two 3D vectors
	@param leftVector  The first vector
	@param rightVector The second vector
	@return The dot product
	*/
	inline f32 Dot(const CEVec3& leftVector, const CEVec3& rightVector)
	{
		return (leftVector.x * rightVector.x) +
			   (leftVector.y * rightVector.y) +
			   (leftVector.z * rightVector.z);
	}

	/*!
	@brief Calculate the cross product of two 3D vectors
	@param leftVector  The first vector
	@param rightVector The second vector
	@return A new vector perpendicular to both inputs
	*/
	inline CEVec3 Cross(const CEVec3& leftVector, const CEVec3& rightVector)
	{
		return CEVec3(
			leftVector.y * rightVector.z - leftVector.z * rightVector.y,
			leftVector.z * rightVector.x - leftVector.x * rightVector.z,
			leftVector.x * rightVector.y - leftVector.y * rightVector.x
		);
	}

	/*!
	@brief Calculate the cross product of two 2D vectors
	@param leftVector  The first vector
	@param rightVector The second vector
	@return A scalar value
	*/
	inline f32 Cross(const CEVec2& leftVector, const CEVec2& rightVector)
	{ 
		return leftVector.x * rightVector.y - leftVector.y * rightVector.x;
	}

	/*!
	@brief Calculate the cross product of a	Vector and a scalar value
	@param vec The first vector
	@param f32 scalar value
	@return A new vector
	*/
	inline CEVec3 Cross(const CEVec3& vec, f32 s)
	{
		return CEVec3(
			s * vec.y,
			-s * vec.x,
			0.f
		);
	}

	/*!
	@brief Calculate the cross product of a	Vector and a scalar value
	@param f32 scalar value
	@param vec The second vector
	@return A new vector
	*/
	inline CEVec3 Cross(f32 s, const CEVec3& vec)
	{
		return CEVec3(
			-s * vec.y,
			s * vec.x,
			0.f
		);
	}

	/*!
	@brief Calculate the squared length of a 3D vector
	@param vector The input vector
	@return The squared length of the vector
	*/
	inline f32 LengthSq(const CEVec3& vector)
	{
		return Dot(vector, vector);
	}

	/*!
	@brief Calculate the length of a 3D vector
	@param vector The input vector
	@return The length of the vector
	*/
	inline f32 Length(const CEVec3& vector)
	{
		return std::sqrt(LengthSq(vector));
	}

	/*!
	@brief Normalize a 3D vector to unit length
	@param vector The input vector
	@return A normalized vector or zero vector if the input has zero length
	*/
	inline CEVec3 Normalize(const CEVec3& vector)
	{
		f32 length = Length(vector);
		if (length != 0.0f)
		{
			return CEVec3{ vector.x / length, vector.y / length, vector.z / length };
		}
		return CEVec3{ 0.0f, 0.0f, 0.0f };
	}

}