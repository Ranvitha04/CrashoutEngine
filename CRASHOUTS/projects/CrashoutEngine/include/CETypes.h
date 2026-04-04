/*!
@file   CETypes.h
@author Shahrin Idzuan
@date   05/09/2025

@brief  Type aliases for standard integer and floating point types

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

#include <cstdint>

/*!
@namespace CE
@brief Core engine namespace containing type aliases for standard integer and floating point types.
*/
namespace CE {
	//! 8-bit signed integer
	using s8 = int8_t;

	//! 8-bit unsigned integer
	using u8 = uint8_t;

	//! 16-bit signed integer
	using s16 = int16_t;

	//! 16-bit unsigned integer
	using u16 = uint16_t;

	//! 32-bit signed integer
	using s32 = int32_t;

	//! 32-bit unsigned integer
	using u32 = uint32_t;

	//! 64-bit signed integer
	using s64 = int64_t;

	//! 64-bit unsigned integer
	using u64 = uint64_t;

	//! 32-bit floating point single precision
	using f32 = float;

	//! 64-bit floating point double precision
	using f64 = double;
}