/*!
@file       CEMtx33.h
@author     m.biniswandi@digipen.edu
@date       05/09/2025
@brief      3x3 matrix for 2D affine transforms

bruh
//__________________________________________________________________________*/

#pragma once

#include "math/CETypes.h"
#include "CEVec2.h"
#include "CEVec3.h"
#include <cmath>
#include "CEMath.h"

using CE::f32;
/*!
@brief 3x3 matrix type for 2D transforms (translation, rotation, scale)

Provides constructors,Identity, Zero, Translation/Rotation/Scale, TRS and basic matrix operations
*/
struct CEMtx33
{
	f32 m[3][3];


	/*!
	@brief Default constructor initializes identity
	*/
	CEMtx33()
	{
		m[0][0] = 1; m[0][1] = 0; m[0][2] = 0;
		m[1][0] = 0; m[1][1] = 1; m[1][2] = 0;
		m[2][0] = 0; m[2][1] = 0; m[2][2] = 1;
	}

	/*!
	@brief Construct from 9 numbers
	*/
	CEMtx33(f32 m00, f32 m01, f32 m02,
			f32 m10, f32 m11, f32 m12,
			f32 m20, f32 m21, f32 m22)
	{
		m[0][0] = m00; m[0][1] = m01; m[0][2] = m02;
		m[1][0] = m10; m[1][1] = m11; m[1][2] = m12;
		m[2][0] = m20; m[2][1] = m21; m[2][2] = m22;
	}

	/*!
	@brief Access element at row column
	@param row Row index
	@param column Column index
	@return Reference to the element for writing or reading
	*/
	f32& operator()(int row, int column)
	{
		return m[row][column];
	}
	/*!
	@brief Read only access to element at row, column.
	@param row Row index
	@param column Column index
	@return Copy of the element value.
	*/
	f32  operator()(int row, int column) const
	{
		return m[row][column];
	}
	/*!
	@brief Identity matrix
	@return 3x3 identity
	*/
	static CEMtx33 Identity()
	{
		return CEMtx33();
	}
	/*!
	@brief Zero matrix
	@return 3x3 zero matrix
	*/
	static CEMtx33 Zero()
	{
		return CEMtx33(0, 0, 0, 0, 0, 0, 0, 0, 0);
	}

	/*!
	@brief 2D translation matrix
	@param tx Translation in x
	@param ty Translation in y
	@return Matrix representing translation by tx, ty
	*/
	static CEMtx33 Translation(f32 tx, f32 ty)
	{
		return CEMtx33(1, 0, tx,
					   0, 1, ty,
					   0, 0, 1);
	}
	/*!
	@brief 2D translation matrix
	@param vector Translation vector x,y
	@return Matrix representing translation by vector
	*/
	static CEMtx33 Translation(const CEVec2& vector)
	{
		return Translation(vector.x, vector.y);
	}

	/*!
	@brief Rotation about the origin
	@param radians Angle in radians
	@return Rotation matrix
	*/
	static CEMtx33 RotationRadians(f32 radians)
	{
		f32 cos = std::cos(radians);
		f32 sin = std::sin(radians);
		return CEMtx33(cos, -sin, 0,
					   sin, cos, 0,
					   0, 0, 1);
	}

	/*!
	@brief Rotation about the origin
	@param degrees Angle in degrees
	@return Rotation matrix
	*/
	static CEMtx33 RotationDegrees(f32 degrees)
	{
		return RotationRadians(degrees * (CEMath::PI / 180.0f));
	}
	/*!
	@brief Scale matrix
	@param sx Scale along x
	@param sy Scale along y
	@return Scaling matrix
	*/
	static CEMtx33 Scale(f32 sx, f32 sy)
	{
		return CEMtx33(sx, 0, 0,
					   0, sy, 0,
					   0, 0, 1);
	}

	static CEMtx33 Scale(const CEVec2& scale)
	{
		return Scale(scale.x, scale.y);
	}

	/*!
	@brief Compose Translation * Rotation * Scale
	@param translate Translation x,y
	@param radians Rotation in radians
	@param scale Scale
	@return M = Scale(scale) * Rotation(radians) * Translation(translate)
	@note This applies translation first, then rotation, then scale
	*/
	static CEMtx33 TRS(const CEVec2& translate, f32 radians, const CEVec2& scale)
	{
		return Scale(scale) * RotationRadians(radians) * Translation(translate);
	}

	//Matrix operation
	/*!
	@brief Matrix multiplication
	@param rhs Right-hand side matrix
	@return Product result
	*/
	CEMtx33 operator*(const CEMtx33& rhs) const
	{
		CEMtx33 result = Zero();
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				result.m[i][j] = m[i][0] * rhs.m[0][j]
							   + m[i][1] * rhs.m[1][j]
							   + m[i][2] * rhs.m[2][j];
			}
		}
		return result;
	}

	/*!
	@brief matrix equal multiplication
	@param rhs Right hand side matrix
	@return Reference to *this after multiplication
	*/
	CEMtx33& operator*=(const CEMtx33& rhs)
	{
		*this = *this * rhs;
		return *this;
	}

	/*!
	@brief Transpose of the matrix
	@return Transposed copy
	*/
	CEMtx33 Transpose() const
	{
		return CEMtx33(m[0][0], m[1][0], m[2][0],
					   m[0][1], m[1][1], m[2][1],
					   m[0][2], m[1][2], m[2][2]);
	}

	/*!
	@brief Determinant of the 3x3 matrix
	@return Determinant
	*/
	f32 Determinant() const
	{
		return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
			 - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
			 + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
	}

};