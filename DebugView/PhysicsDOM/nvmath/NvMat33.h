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

#ifndef NV_NVFOUNDATION_NVMAT33_H
#define NV_NVFOUNDATION_NVMAT33_H
/** \addtogroup foundation
@{
*/

#include "NvVec3.h"
#include "NvQuat.h"

#if !NV_DOXYGEN
namespace NV_MATH
{
#endif
/*!
\brief 3x3 matrix class

Some clarifications, as there have been much confusion about matrix formats etc in the past.

Short:
- Matrix have base vectors in columns (vectors are column matrices, 3x1 matrices).
- Matrix is physically stored in column major format
- Matrices are concaternated from left

Long:
Given three base vectors a, b and c the matrix is stored as

|a.x b.x c.x|
|a.y b.y c.y|
|a.z b.z c.z|

Vectors are treated as columns, so the vector v is

|x|
|y|
|z|

And matrices are applied _before_ the vector (pre-multiplication)
v' = M*v

|x'|   |a.x b.x c.x|   |x|   |a.x*x + b.x*y + c.x*z|
|y'| = |a.y b.y c.y| * |y| = |a.y*x + b.y*y + c.y*z|
|z'|   |a.z b.z c.z|   |z|   |a.z*x + b.z*y + c.z*z|


Physical storage and indexing:
To be compatible with popular 3d rendering APIs (read D3d and OpenGL)
the physical indexing is

|0 3 6|
|1 4 7|
|2 5 8|

index = column*3 + row

which in C++ translates to M[column][row]

The mathematical indexing is M_row,column and this is what is used for _-notation
so _12 is 1st row, second column and operator(row, column)!

*/
class NvMat33
{
  public:
	//! Default constructor
	 inline NvMat33()
	{
	}

	//! identity constructor
	 inline NvMat33(NvIDENTITY r)
	: column0(1.0f, 0.0f, 0.0f), column1(0.0f, 1.0f, 0.0f), column2(0.0f, 0.0f, 1.0f)
	{
		NV_UNUSED(r);
	}

	//! zero constructor
	 inline NvMat33(NvZERO r) : column0(0.0f), column1(0.0f), column2(0.0f)
	{
		NV_UNUSED(r);
	}

	//! Construct from three base vectors
	 NvMat33(const NvVec3& col0, const NvVec3& col1, const NvVec3& col2)
	: column0(col0), column1(col1), column2(col2)
	{
	}

	//! constructor from a scalar, which generates a multiple of the identity matrix
	explicit  inline NvMat33(float r)
	: column0(r, 0.0f, 0.0f), column1(0.0f, r, 0.0f), column2(0.0f, 0.0f, r)
	{
	}

	//! Construct from float[9]
	explicit  inline NvMat33(float values[])
	: column0(values[0], values[1], values[2])
	, column1(values[3], values[4], values[5])
	, column2(values[6], values[7], values[8])
	{
	}

	//! Construct from a quaternion
	explicit  inline NvMat33(const NvQuat& q)
	{
		const float x = q.x;
		const float y = q.y;
		const float z = q.z;
		const float w = q.w;

		const float x2 = x + x;
		const float y2 = y + y;
		const float z2 = z + z;

		const float xx = x2 * x;
		const float yy = y2 * y;
		const float zz = z2 * z;

		const float xy = x2 * y;
		const float xz = x2 * z;
		const float xw = x2 * w;

		const float yz = y2 * z;
		const float yw = y2 * w;
		const float zw = z2 * w;

		column0 = NvVec3(1.0f - yy - zz, xy + zw, xz - yw);
		column1 = NvVec3(xy - zw, 1.0f - xx - zz, yz + xw);
		column2 = NvVec3(xz + yw, yz - xw, 1.0f - xx - yy);
	}

	//! Copy constructor
	 inline NvMat33(const NvMat33& other)
	: column0(other.column0), column1(other.column1), column2(other.column2)
	{
	}

	//! Assignment operator
	 inline NvMat33& operator=(const NvMat33& other)
	{
		column0 = other.column0;
		column1 = other.column1;
		column2 = other.column2;
		return *this;
	}

	//! Construct from diagonal, off-diagonals are zero.
	 inline static NvMat33 createDiagonal(const NvVec3& d)
	{
		return NvMat33(NvVec3(d.x, 0.0f, 0.0f), NvVec3(0.0f, d.y, 0.0f), NvVec3(0.0f, 0.0f, d.z));
	}

	/**
	\brief returns true if the two matrices are exactly equal
	*/
	 inline bool operator==(const NvMat33& m) const
	{
		return column0 == m.column0 && column1 == m.column1 && column2 == m.column2;
	}

	//! Get transposed matrix
	 inline NvMat33 getTranspose() const
	{
		const NvVec3 v0(column0.x, column1.x, column2.x);
		const NvVec3 v1(column0.y, column1.y, column2.y);
		const NvVec3 v2(column0.z, column1.z, column2.z);

		return NvMat33(v0, v1, v2);
	}

	//! Get the real inverse
	 inline NvMat33 getInverse() const
	{
		const float det = getDeterminant();
		NvMat33 inverse;

		if(det != 0)
		{
			const float invDet = 1.0f / det;

			inverse.column0.x = invDet * (column1.y * column2.z - column2.y * column1.z);
			inverse.column0.y = invDet * -(column0.y * column2.z - column2.y * column0.z);
			inverse.column0.z = invDet * (column0.y * column1.z - column0.z * column1.y);

			inverse.column1.x = invDet * -(column1.x * column2.z - column1.z * column2.x);
			inverse.column1.y = invDet * (column0.x * column2.z - column0.z * column2.x);
			inverse.column1.z = invDet * -(column0.x * column1.z - column0.z * column1.x);

			inverse.column2.x = invDet * (column1.x * column2.y - column1.y * column2.x);
			inverse.column2.y = invDet * -(column0.x * column2.y - column0.y * column2.x);
			inverse.column2.z = invDet * (column0.x * column1.y - column1.x * column0.y);

			return inverse;
		}
		else
		{
			return NvMat33(NvIdentity);
		}
	}

	//! Get determinant
	 inline float getDeterminant() const
	{
		return column0.dot(column1.cross(column2));
	}

	//! Unary minus
	 inline NvMat33 operator-() const
	{
		return NvMat33(-column0, -column1, -column2);
	}

	//! Add
	 inline NvMat33 operator+(const NvMat33& other) const
	{
		return NvMat33(column0 + other.column0, column1 + other.column1, column2 + other.column2);
	}

	//! Subtract
	 inline NvMat33 operator-(const NvMat33& other) const
	{
		return NvMat33(column0 - other.column0, column1 - other.column1, column2 - other.column2);
	}

	//! Scalar multiplication
	 inline NvMat33 operator*(float scalar) const
	{
		return NvMat33(column0 * scalar, column1 * scalar, column2 * scalar);
	}

	friend NvMat33 operator*(float, const NvMat33&);

	//! Matrix vector multiplication (returns 'this->transform(vec)')
	 inline NvVec3 operator*(const NvVec3& vec) const
	{
		return transform(vec);
	}

	// a <op>= b operators

	//! Matrix multiplication
	 inline NvMat33 operator*(const NvMat33& other) const
	{
		// Rows from this <dot> columns from other
		// column0 = transform(other.column0) etc
		return NvMat33(transform(other.column0), transform(other.column1), transform(other.column2));
	}

	//! Equals-add
	 inline NvMat33& operator+=(const NvMat33& other)
	{
		column0 += other.column0;
		column1 += other.column1;
		column2 += other.column2;
		return *this;
	}

	//! Equals-sub
	 inline NvMat33& operator-=(const NvMat33& other)
	{
		column0 -= other.column0;
		column1 -= other.column1;
		column2 -= other.column2;
		return *this;
	}

	//! Equals scalar multiplication
	 inline NvMat33& operator*=(float scalar)
	{
		column0 *= scalar;
		column1 *= scalar;
		column2 *= scalar;
		return *this;
	}

	//! Equals matrix multiplication
	 inline NvMat33& operator*=(const NvMat33& other)
	{
		*this = *this * other;
		return *this;
	}

	// Transform etc

	//! Transform vector by matrix, equal to v' = M*v
	 inline NvVec3 transform(const NvVec3& other) const
	{
		return column0 * other.x + column1 * other.y + column2 * other.z;
	}

	//! Transform vector by matrix transpose, v' = M^t*v
	 inline NvVec3 transformTranspose(const NvVec3& other) const
	{
		return NvVec3(column0.dot(other), column1.dot(other), column2.dot(other));
	}

	 inline const float* front() const
	{
		return &column0.x;
	}

	// Data, see above for format!

	NvVec3 column0, column1, column2; // the three base vectors
};

// implementation from NvQuat.h
 inline NvQuat::NvQuat(const NvMat33& m)
{
	if (m.column2.z < 0)
	{
		if (m.column0.x > m.column1.y)
		{
			float t = 1 + m.column0.x - m.column1.y - m.column2.z;
			*this = NvQuat(t, m.column0.y + m.column1.x, m.column2.x + m.column0.z, m.column1.z - m.column2.y) * (0.5f / NvSqrt(t));
		}
		else
		{
			float t = 1 - m.column0.x + m.column1.y - m.column2.z;
			*this = NvQuat(m.column0.y + m.column1.x, t, m.column1.z + m.column2.y, m.column2.x - m.column0.z) * (0.5f / NvSqrt(t));
		}
	}
	else
	{
		if (m.column0.x < -m.column1.y)
		{
			float t = 1 - m.column0.x - m.column1.y + m.column2.z;
			*this = NvQuat(m.column2.x + m.column0.z, m.column1.z + m.column2.y, t, m.column0.y - m.column1.x) * (0.5f / NvSqrt(t));
		}
		else
		{
			float t = 1 + m.column0.x + m.column1.y + m.column2.z;
			*this = NvQuat(m.column1.z - m.column2.y, m.column2.x - m.column0.z, m.column0.y - m.column1.x, t) * (0.5f / NvSqrt(t));
		}
	}
}

#if !NV_DOXYGEN
} // namespace NV_MATH
#endif

/** @} */
#endif // #ifndef NV_NVFOUNDATION_NVMAT33_H
