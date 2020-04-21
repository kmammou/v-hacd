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

#ifndef NV_NVFOUNDATION_NVBOUNDS3_H
#define NV_NVFOUNDATION_NVBOUNDS3_H

/** \addtogroup foundation
@{
*/

#include "NvTransform.h"
#include "NvMat33.h"

#if !NV_DOXYGEN
namespace NV_MATH
{
#endif

// maximum extents defined such that floating point exceptions are avoided for standard use cases
#define NV_MAX_BOUNDS_EXTENTS (NV_MAX_REAL * 0.25f)

/**
\brief Class representing 3D range or axis aligned bounding box.

Stored as minimum and maximum extent corners. Alternate representation
would be center and dimensions.
May be empty or nonempty. For nonempty bounds, minimum <= maximum has to hold for all axes.
Empty bounds have to be represented as minimum = NV_MAX_BOUNDS_EXTENTS and maximum = -NV_MAX_BOUNDS_EXTENTS for all
axes.
All other representations are invalid and the behavior is undefined.
*/
class NvBounds3
{
  public:
	/**
	\brief Default constructor, not performing any initialization for performance reason.
	\remark Use empty() function below to construct empty bounds.
	*/
	 inline NvBounds3()
	{
	}

	/**
	\brief Construct from two bounding points
	*/
	 inline NvBounds3(const NvVec3& minimum, const NvVec3& maximum);

	/**
	\brief Return empty bounds.
	*/
	static  inline NvBounds3 empty();

	/**
	\brief returns the AABB containing v0 and v1.
	\param v0 first point included in the AABB.
	\param v1 second point included in the AABB.
	*/
	static  inline NvBounds3 boundsOfPoints(const NvVec3& v0, const NvVec3& v1);

	/**
	\brief returns the AABB from center and extents vectors.
	\param center Center vector
	\param extent Extents vector
	*/
	static  inline NvBounds3 centerExtents(const NvVec3& center, const NvVec3& extent);

	/**
	\brief Construct from center, extent, and (not necessarily orthogonal) basis
	*/
	static  inline NvBounds3
	basisExtent(const NvVec3& center, const NvMat33& basis, const NvVec3& extent);

	/**
	\brief Construct from pose and extent
	*/
	static  inline NvBounds3 poseExtent(const NvTransform& pose, const NvVec3& extent);

	/**
	\brief gets the transformed bounds of the passed AABB (resulting in a bigger AABB).

	This version is safe to call for empty bounds.

	\param[in] matrix Transform to apply, can contain scaling as well
	\param[in] bounds The bounds to transform.
	*/
	static  inline NvBounds3 transformSafe(const NvMat33& matrix, const NvBounds3& bounds);

	/**
	\brief gets the transformed bounds of the passed AABB (resulting in a bigger AABB).

	Calling this method for empty bounds leads to undefined behavior. Use #transformSafe() instead.

	\param[in] matrix Transform to apply, can contain scaling as well
	\param[in] bounds The bounds to transform.
	*/
	static  inline NvBounds3 transformFast(const NvMat33& matrix, const NvBounds3& bounds);

	/**
	\brief gets the transformed bounds of the passed AABB (resulting in a bigger AABB).

	This version is safe to call for empty bounds.

	\param[in] transform Transform to apply, can contain scaling as well
	\param[in] bounds The bounds to transform.
	*/
	static  inline NvBounds3 transformSafe(const NvTransform& transform, const NvBounds3& bounds);

	/**
	\brief gets the transformed bounds of the passed AABB (resulting in a bigger AABB).

	Calling this method for empty bounds leads to undefined behavior. Use #transformSafe() instead.

	\param[in] transform Transform to apply, can contain scaling as well
	\param[in] bounds The bounds to transform.
	*/
	static  inline NvBounds3 transformFast(const NvTransform& transform, const NvBounds3& bounds);

	/**
	\brief Sets empty to true
	*/
	 inline void setEmpty();

	/**
	\brief Sets the bounds to maximum size [-NV_MAX_BOUNDS_EXTENTS, NV_MAX_BOUNDS_EXTENTS].
	*/
	 inline void setMaximal();

	/**
	\brief expands the volume to include v
	\param v Point to expand to.
	*/
	 inline void include(const NvVec3& v);

	/**
	\brief expands the volume to include b.
	\param b Bounds to perform union with.
	*/
	 inline void include(const NvBounds3& b);

	 inline bool isEmpty() const;

	/**
	\brief indicates whether the intersection of this and b is empty or not.
	\param b Bounds to test for intersection.
	*/
	 inline bool intersects(const NvBounds3& b) const;

	/**
	 \brief computes the 1D-intersection between two AABBs, on a given axis.
	 \param	a		the other AABB
	 \param	axis	the axis (0, 1, 2)
	 */
	 inline bool intersects1D(const NvBounds3& a, uint32_t axis) const;

	/**
	\brief indicates if these bounds contain v.
	\param v Point to test against bounds.
	*/
	 inline bool contains(const NvVec3& v) const;

	/**
	 \brief	checks a box is inside another box.
	 \param	box		the other AABB
	 */
	 inline bool isInside(const NvBounds3& box) const;

	/**
	\brief returns the center of this axis aligned box.
	*/
	 inline NvVec3 getCenter() const;

	/**
	\brief get component of the box's center along a given axis
	*/
	 inline float getCenter(uint32_t axis) const;

	/**
	\brief get component of the box's extents along a given axis
	*/
	 inline float getExtents(uint32_t axis) const;

	/**
	\brief returns the dimensions (width/height/depth) of this axis aligned box.
	*/
	 inline NvVec3 getDimensions() const;

	/**
	\brief returns the extents, which are half of the width/height/depth.
	*/
	 inline NvVec3 getExtents() const;

	/**
	\brief scales the AABB.

	This version is safe to call for empty bounds.

	\param scale Factor to scale AABB by.
	*/
	 inline void scaleSafe(float scale);

	/**
	\brief scales the AABB.

	Calling this method for empty bounds leads to undefined behavior. Use #scaleSafe() instead.

	\param scale Factor to scale AABB by.
	*/
	 inline void scaleFast(float scale);

	/**
	fattens the AABB in all 3 dimensions by the given distance.

	This version is safe to call for empty bounds.
	*/
	 inline void fattenSafe(float distance);

	/**
	fattens the AABB in all 3 dimensions by the given distance.

	Calling this method for empty bounds leads to undefined behavior. Use #fattenSafe() instead.
	*/
	 inline void fattenFast(float distance);

	/**
	checks that the AABB values are not NaN
	*/
	 inline bool isFinite() const;

	/**
	checks that the AABB values describe a valid configuration.
	*/
	 inline bool isValid() const;

	NvVec3 minimum, maximum;
};

 inline NvBounds3::NvBounds3(const NvVec3& minimum_, const NvVec3& maximum_)
: minimum(minimum_), maximum(maximum_)
{
}

 inline NvBounds3 NvBounds3::empty()
{
	return NvBounds3(NvVec3(NV_MAX_BOUNDS_EXTENTS), NvVec3(-NV_MAX_BOUNDS_EXTENTS));
}

 inline bool NvBounds3::isFinite() const
{
	return minimum.isFinite() && maximum.isFinite();
}

 inline NvBounds3 NvBounds3::boundsOfPoints(const NvVec3& v0, const NvVec3& v1)
{
	return NvBounds3(v0.minimum(v1), v0.maximum(v1));
}

 inline NvBounds3 NvBounds3::centerExtents(const NvVec3& center, const NvVec3& extent)
{
	return NvBounds3(center - extent, center + extent);
}

 inline NvBounds3
NvBounds3::basisExtent(const NvVec3& center, const NvMat33& basis, const NvVec3& extent)
{
	// extended basis vectors
	NvVec3 c0 = basis.column0 * extent.x;
	NvVec3 c1 = basis.column1 * extent.y;
	NvVec3 c2 = basis.column2 * extent.z;

	NvVec3 w;
	// find combination of base vectors that produces max. distance for each component = sum of abs()
	w.x = NvAbs(c0.x) + NvAbs(c1.x) + NvAbs(c2.x);
	w.y = NvAbs(c0.y) + NvAbs(c1.y) + NvAbs(c2.y);
	w.z = NvAbs(c0.z) + NvAbs(c1.z) + NvAbs(c2.z);

	return NvBounds3(center - w, center + w);
}

 inline NvBounds3 NvBounds3::poseExtent(const NvTransform& pose, const NvVec3& extent)
{
	return basisExtent(pose.p, NvMat33(pose.q), extent);
}

 inline void NvBounds3::setEmpty()
{
	minimum = NvVec3(NV_MAX_BOUNDS_EXTENTS);
	maximum = NvVec3(-NV_MAX_BOUNDS_EXTENTS);
}

 inline void NvBounds3::setMaximal()
{
	minimum = NvVec3(-NV_MAX_BOUNDS_EXTENTS);
	maximum = NvVec3(NV_MAX_BOUNDS_EXTENTS);
}

 inline void NvBounds3::include(const NvVec3& v)
{
	minimum = minimum.minimum(v);
	maximum = maximum.maximum(v);
}

 inline void NvBounds3::include(const NvBounds3& b)
{
	minimum = minimum.minimum(b.minimum);
	maximum = maximum.maximum(b.maximum);
}

 inline bool NvBounds3::isEmpty() const
{
	return minimum.x > maximum.x;
}

 inline bool NvBounds3::intersects(const NvBounds3& b) const
{
	return !(b.minimum.x > maximum.x || minimum.x > b.maximum.x || b.minimum.y > maximum.y || minimum.y > b.maximum.y ||
	         b.minimum.z > maximum.z || minimum.z > b.maximum.z);
}

 inline bool NvBounds3::intersects1D(const NvBounds3& a, uint32_t axis) const
{
	return maximum[axis] >= a.minimum[axis] && a.maximum[axis] >= minimum[axis];
}

 inline bool NvBounds3::contains(const NvVec3& v) const
{
	return !(v.x < minimum.x || v.x > maximum.x || v.y < minimum.y || v.y > maximum.y || v.z < minimum.z ||
	         v.z > maximum.z);
}

 inline bool NvBounds3::isInside(const NvBounds3& box) const
{
	if(box.minimum.x > minimum.x)
		return false;
	if(box.minimum.y > minimum.y)
		return false;
	if(box.minimum.z > minimum.z)
		return false;
	if(box.maximum.x < maximum.x)
		return false;
	if(box.maximum.y < maximum.y)
		return false;
	if(box.maximum.z < maximum.z)
		return false;
	return true;
}

 inline NvVec3 NvBounds3::getCenter() const
{
	return (minimum + maximum) * 0.5f;
}

 inline float NvBounds3::getCenter(uint32_t axis) const
{
	return (minimum[axis] + maximum[axis]) * 0.5f;
}

 inline float NvBounds3::getExtents(uint32_t axis) const
{
	return (maximum[axis] - minimum[axis]) * 0.5f;
}

 inline NvVec3 NvBounds3::getDimensions() const
{
	return maximum - minimum;
}

 inline NvVec3 NvBounds3::getExtents() const
{
	return getDimensions() * 0.5f;
}

 inline void NvBounds3::scaleSafe(float scale)
{
	if(!isEmpty())
		scaleFast(scale);
}

 inline void NvBounds3::scaleFast(float scale)
{
	*this = centerExtents(getCenter(), getExtents() * scale);
}

 inline void NvBounds3::fattenSafe(float distance)
{
	if(!isEmpty())
		fattenFast(distance);
}

 inline void NvBounds3::fattenFast(float distance)
{
	minimum.x -= distance;
	minimum.y -= distance;
	minimum.z -= distance;

	maximum.x += distance;
	maximum.y += distance;
	maximum.z += distance;
}

 inline NvBounds3 NvBounds3::transformSafe(const NvMat33& matrix, const NvBounds3& bounds)
{
	return !bounds.isEmpty() ? transformFast(matrix, bounds) : bounds;
}

 inline NvBounds3 NvBounds3::transformFast(const NvMat33& matrix, const NvBounds3& bounds)
{
	return NvBounds3::basisExtent(matrix * bounds.getCenter(), matrix, bounds.getExtents());
}

 inline NvBounds3 NvBounds3::transformSafe(const NvTransform& transform, const NvBounds3& bounds)
{
	return !bounds.isEmpty() ? transformFast(transform, bounds) : bounds;
}

 inline NvBounds3 NvBounds3::transformFast(const NvTransform& transform, const NvBounds3& bounds)
{
	return NvBounds3::basisExtent(transform.transform(bounds.getCenter()), NvMat33(transform.q), bounds.getExtents());
}

 inline bool NvBounds3::isValid() const
{
	return (isFinite() && (((minimum.x <= maximum.x) && (minimum.y <= maximum.y) && (minimum.z <= maximum.z)) ||
	                       ((minimum.x == NV_MAX_BOUNDS_EXTENTS) && (minimum.y == NV_MAX_BOUNDS_EXTENTS) &&
	                        (minimum.z == NV_MAX_BOUNDS_EXTENTS) && (maximum.x == -NV_MAX_BOUNDS_EXTENTS) &&
	                        (maximum.y == -NV_MAX_BOUNDS_EXTENTS) && (maximum.z == -NV_MAX_BOUNDS_EXTENTS))));
}

#if !NV_DOXYGEN
} // namespace NV_MATH
#endif

/** @} */
#endif // #ifndef NV_NVFOUNDATION_NVBOUNDS3_H
