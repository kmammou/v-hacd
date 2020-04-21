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

#ifndef NV_NVFOUNDATION_NVVEC3_H
#define NV_NVFOUNDATION_NVVEC3_H

/** \addtogroup foundation
@{
*/

#include "Nv.h"

#if !NV_DOXYGEN
namespace NV_MATH
{
#endif

/**
\brief 3 Element vector class.

This is a 3-dimensional vector class with public data members.
*/
class NvVec3
{
  public:
	/**
	\brief default constructor leaves data uninitialized.
	*/
	 inline NvVec3()
	{
	}

	/**
	\brief zero constructor.
	*/
	 inline NvVec3(NvZERO r) : x(0.0f), y(0.0f), z(0.0f)
	{
		NV_UNUSED(r);
	}

	/**
	\brief Assigns scalar parameter to all elements.

	Useful to initialize to zero or one.

	\param[in] a Value to assign to elements.
	*/
	explicit  inline NvVec3(float a) : x(a), y(a), z(a)
	{
	}

	/**
	\brief Initializes from 3 scalar parameters.

	\param[in] nx Value to initialize X component.
	\param[in] ny Value to initialize Y component.
	\param[in] nz Value to initialize Z component.
	*/
	 inline NvVec3(float nx, float ny, float nz) : x(nx), y(ny), z(nz)
	{
	}

	/**
	\brief Copy ctor.
	*/
	 inline NvVec3(const NvVec3& v) : x(v.x), y(v.y), z(v.z)
	{
	}

	// Operators

	/**
	\brief Assignment operator
	*/
	 inline NvVec3& operator=(const NvVec3& p)
	{
		x = p.x;
		y = p.y;
		z = p.z;
		return *this;
	}

	/**
	\brief returns true if the two vectors are exactly equal.
	*/
	 inline bool operator==(const NvVec3& v) const
	{
		return x == v.x && y == v.y && z == v.z;
	}

	/**
	\brief returns true if the two vectors are not exactly equal.
	*/
	 inline bool operator!=(const NvVec3& v) const
	{
		return x != v.x || y != v.y || z != v.z;
	}

	/**
	\brief tests for exact zero vector
	*/
	 inline bool isZero() const
	{
		return x == 0.0f && y == 0.0f && z == 0.0f;
	}

	/**
	\brief returns true if all 3 elems of the vector are finite (not NAN or INF, etc.)
	*/
	 inline bool isFinite() const
	{
		return NvIsFinite(x) && NvIsFinite(y) && NvIsFinite(z);
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
		return x * x + y * y + z * z;
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
	 inline NvVec3 operator-() const
	{
		return NvVec3(-x, -y, -z);
	}

	/**
	\brief vector addition
	*/
	 inline NvVec3 operator+(const NvVec3& v) const
	{
		return NvVec3(x + v.x, y + v.y, z + v.z);
	}

	/**
	\brief vector difference
	*/
	 inline NvVec3 operator-(const NvVec3& v) const
	{
		return NvVec3(x - v.x, y - v.y, z - v.z);
	}

	/**
	\brief scalar post-multiplication
	*/
	 inline NvVec3 operator*(float f) const
	{
		return NvVec3(x * f, y * f, z * f);
	}

	/**
	\brief scalar division
	*/
	 inline NvVec3 operator/(float f) const
	{
		f = 1.0f / f;
		return NvVec3(x * f, y * f, z * f);
	}

	/**
	\brief vector addition
	*/
	 inline NvVec3& operator+=(const NvVec3& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	/**
	\brief vector difference
	*/
	 inline NvVec3& operator-=(const NvVec3& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	/**
	\brief scalar multiplication
	*/
	 inline NvVec3& operator*=(float f)
	{
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}
	/**
	\brief scalar division
	*/
	 inline NvVec3& operator/=(float f)
	{
		f = 1.0f / f;
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	/**
	\brief returns the scalar product of this and other.
	*/
	 inline float dot(const NvVec3& v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}

	/**
	\brief cross product
	*/
	 inline NvVec3 cross(const NvVec3& v) const
	{
		return NvVec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}

	/** return a unit vector */

	 inline NvVec3 getNormalized() const
	{
		const float m = magnitudeSquared();
		return m > 0.0f ? *this * NvRecipSqrt(m) : NvVec3(0, 0, 0);
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
	\brief normalizes the vector in place. Does nothing if vector magnitude is under NV_NORMALIZATION_EPSILON.
	Returns vector magnitude if >= NV_NORMALIZATION_EPSILON and 0.0f otherwise.
	*/
	 inline float normalizeSafe()
	{
		const float mag = magnitude();
		if(mag < NV_NORMALIZATION_EPSILON)
			return 0.0f;
		*this *= 1.0f / mag;
		return mag;
	}

	/**
	\brief normalizes the vector in place. Asserts if vector magnitude is under NV_NORMALIZATION_EPSILON.
	returns vector magnitude.
	*/
	 inline float normalizeFast()
	{
		const float mag = magnitude();
		*this *= 1.0f / mag;
		return mag;
	}

	/**
	\brief a[i] * b[i], for all i.
	*/
	 inline NvVec3 multiply(const NvVec3& a) const
	{
		return NvVec3(x * a.x, y * a.y, z * a.z);
	}

	/**
	\brief element-wise minimum
	*/
	 inline NvVec3 minimum(const NvVec3& v) const
	{
		return NvVec3(NvMin(x, v.x), NvMin(y, v.y), NvMin(z, v.z));
	}

	/**
	\brief returns MIN(x, y, z);
	*/
	 inline float minElement() const
	{
		return NvMin(x, NvMin(y, z));
	}

	/**
	\brief element-wise maximum
	*/
	 inline NvVec3 maximum(const NvVec3& v) const
	{
		return NvVec3(NvMax(x, v.x), NvMax(y, v.y), NvMax(z, v.z));
	}

	/**
	\brief returns MAX(x, y, z);
	*/
	 inline float maxElement() const
	{
		return NvMax(x, NvMax(y, z));
	}

	/**
	\brief returns absolute values of components;
	*/
	 inline NvVec3 abs() const
	{
		return NvVec3(NvAbs(x), NvAbs(y), NvAbs(z));
	}

	 /**
	 \brief element access
	 */
	 inline float& operator[](unsigned int index)
	 {
		 return reinterpret_cast<float*>(this)[index];
	 }

	 /**
	 \brief element access
	 */
	 inline const float& operator[](unsigned int index) const
	 {
		 return reinterpret_cast<const float*>(this)[index];
	 }

	float x, y, z;
};

 static inline NvVec3 operator*(float f, const NvVec3& v)
{
	return NvVec3(f * v.x, f * v.y, f * v.z);
}

#if !NV_DOXYGEN
} // namespace NV_MATH
#endif

/** @} */
#endif // #ifndef NV_NVFOUNDATION_NVVEC3_H
