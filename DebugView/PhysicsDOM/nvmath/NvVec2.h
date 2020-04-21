// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2014 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#ifndef NV_NVFOUNDATION_NVVEC2_H
#define NV_NVFOUNDATION_NVVEC2_H

/** \addtogroup foundation
@{
*/

#include "Nv.h"

#if !NV_DOXYGEN
namespace NV_MATH
{
#endif

/**
\brief 2 Element vector class.

This is a 2-dimensional vector class with public data members.
*/
class NvVec2
{
  public:
	/**
	\brief default constructor leaves data uninitialized.
	*/
	 inline NvVec2()
	{
	}

	/**
	\brief zero constructor.
	*/
	 inline NvVec2(NvZERO r) : x(0.0f), y(0.0f)
	{
		NV_UNUSED(r);
	}

	/**
	\brief Assigns scalar parameter to all elements.

	Useful to initialize to zero or one.

	\param[in] a Value to assign to elements.
	*/
	explicit  inline NvVec2(float a) : x(a), y(a)
	{
	}

	/**
	\brief Initializes from 2 scalar parameters.

	\param[in] nx Value to initialize X component.
	\param[in] ny Value to initialize Y component.
	*/
	 inline NvVec2(float nx, float ny) : x(nx), y(ny)
	{
	}

	/**
	\brief Copy ctor.
	*/
	 inline NvVec2(const NvVec2& v) : x(v.x), y(v.y)
	{
	}

	// Operators

	/**
	\brief Assignment operator
	*/
	 inline NvVec2& operator=(const NvVec2& p)
	{
		x = p.x;
		y = p.y;
		return *this;
	}

	/**
	\brief returns true if the two vectors are exactly equal.
	*/
	 inline bool operator==(const NvVec2& v) const
	{
		return x == v.x && y == v.y;
	}

	/**
	\brief returns true if the two vectors are not exactly equal.
	*/
	 inline bool operator!=(const NvVec2& v) const
	{
		return x != v.x || y != v.y;
	}

	/**
	\brief tests for exact zero vector
	*/
	 inline bool isZero() const
	{
		return x == 0.0f && y == 0.0f;
	}

	/**
	\brief returns true if all 2 elems of the vector are finite (not NAN or INF, etc.)
	*/
	 inline bool isFinite() const
	{
		return NvIsFinite(x) && NvIsFinite(y);
	}

	/**
	\brief is normalized - used by API parameter validation
	*/
	 inline bool isNormalized() const
	{
		const float unitTolerance = 1e-4f;
		return isFinite() && NvAbs(magnitude() - 1) < unitTolerance;
	}

	/**
	\brief returns the squared magnitude

	Avoids calling NvSqrt()!
	*/
	 inline float magnitudeSquared() const
	{
		return x * x + y * y;
	}

	/**
	\brief returns the magnitude
	*/
	 inline float magnitude() const
	{
		return NvSqrt(magnitudeSquared());
	}

	/**
	\brief negation
	*/
	 inline NvVec2 operator-() const
	{
		return NvVec2(-x, -y);
	}

	/**
	\brief vector addition
	*/
	 inline NvVec2 operator+(const NvVec2& v) const
	{
		return NvVec2(x + v.x, y + v.y);
	}

	/**
	\brief vector difference
	*/
	 inline NvVec2 operator-(const NvVec2& v) const
	{
		return NvVec2(x - v.x, y - v.y);
	}

	/**
	\brief scalar post-multiplication
	*/
	 inline NvVec2 operator*(float f) const
	{
		return NvVec2(x * f, y * f);
	}

	/**
	\brief scalar division
	*/
	 inline NvVec2 operator/(float f) const
	{
		f = 1.0f / f; // PT: inconsistent notation with operator /=
		return NvVec2(x * f, y * f);
	}

	/**
	\brief vector addition
	*/
	 inline NvVec2& operator+=(const NvVec2& v)
	{
		x += v.x;
		y += v.y;
		return *this;
	}

	/**
	\brief vector difference
	*/
	 inline NvVec2& operator-=(const NvVec2& v)
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	/**
	\brief scalar multiplication
	*/
	 inline NvVec2& operator*=(float f)
	{
		x *= f;
		y *= f;
		return *this;
	}
	/**
	\brief scalar division
	*/
	 inline NvVec2& operator/=(float f)
	{
		f = 1.0f / f; // PT: inconsistent notation with operator /
		x *= f;
		y *= f;
		return *this;
	}

	/**
	\brief returns the scalar product of this and other.
	*/
	 inline float dot(const NvVec2& v) const
	{
		return x * v.x + y * v.y;
	}

	/** return a unit vector */

	 inline NvVec2 getNormalized() const
	{
		const float m = magnitudeSquared();
		return m > 0.0f ? *this * NvRecipSqrt(m) : NvVec2(0, 0);
	}

	/**
	\brief normalizes the vector in place
	*/
	 inline float normalize()
	{
		const float m = magnitude();
		if(m > 0.0f)
			*this /= m;
		return m;
	}

	/**
	\brief a[i] * b[i], for all i.
	*/
	 inline NvVec2 multiply(const NvVec2& a) const
	{
		return NvVec2(x * a.x, y * a.y);
	}

	/**
	\brief element-wise minimum
	*/
	 inline NvVec2 minimum(const NvVec2& v) const
	{
		return NvVec2(NvMin(x, v.x), NvMin(y, v.y));
	}

	/**
	\brief returns MIN(x, y);
	*/
	 inline float minElement() const
	{
		return NvMin(x, y);
	}

	/**
	\brief element-wise maximum
	*/
	 inline NvVec2 maximum(const NvVec2& v) const
	{
		return NvVec2(NvMax(x, v.x), NvMax(y, v.y));
	}

	/**
	\brief returns MAX(x, y);
	*/
	 inline float maxElement() const
	{
		return NvMax(x, y);
	}

	float x, y;
};

 static inline NvVec2 operator*(float f, const NvVec2& v)
{
	return NvVec2(f * v.x, f * v.y);
}

#if !NV_DOXYGEN
} // namespace NV_MATH
#endif

/** @} */
#endif // #ifndef NV_NVFOUNDATION_NVVEC2_H
