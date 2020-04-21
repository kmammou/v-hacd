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

#pragma once

#include <stdint.h>

#define SKELETON_RIG_VERSION 100 // 1.11

namespace skeletonrig
{

class Constraint
{
public:
    uint32_t	mHullA;					// Convex Hull A index
    uint32_t	mHullB;					// Convex Hull B index
    double		mConstraintPoint[3];	// The point of intersection between the two convex hulls
    double		mConstraintOrientation[4]; // the orientation of the constraint as a quaternion
    double		mPlaneEquation[4];		// The plane equation of the splitting plane for the constraint
};

class SimpleMesh
{
public:
    uint32_t        mVertexCount{0}; // number of vertices in the triangle mesh
    uint32_t        mTriangeCount{0}; // number of triangles in the mesh
    const double    *mVertices{nullptr}; // the vertices of the triangle mesh as a double precision floats (x1,y1,z1,x2,y2,z2...)
    const uint32_t  *mIndices{nullptr}; // indices for the triangle mesh
};

class SkeletonRig
{
public:

    // Will compute the center of mass of the convex hulls and return it
    // in 'centerOfMass'.  Returns false if the center of mass could not be computed.
    virtual bool computeCenterOfMass(double centerOfMass[3]) const = 0;

    // Will analyze the HACD results and compute the constraints solutions.
    // It will analyze the point at which any two convex hulls touch each other and 
    // return the total number of constraint pairs found
    virtual uint32_t computeConstraints(uint32_t forwardAxis = 1) = 0;

    // Set the default forward axis for constraints.  For hinge joints using the y-axis
    // as the forward axis generally produces the best results. 
    virtual void	setConstraintForwardAxis(uint32_t forwardAxis = 1) = 0;

    // Returns a pointer to the list of constraints generated and the 'constraintCount'
    // Returns a null pointer if 'ComputeConstraints' has not yet been called or
    // no constraints were found.
    virtual const Constraint *getConstraints(uint32_t &constraintCount) const = 0;

    // For skeletal mesh deformation, given this input position we find the four
    // nearest constraints and assign a 'weighting' value to each based on the
    // exponential distance.  Default exponent is 1, which is a linear falloff.
    // 2 is quadratic, etc.
    virtual bool getBoneWeightings(const double *position,
        double boneWeightFalloff,	// Exponential falloff distance factor 1=linear 2=quadratic, etc.
        double boneWeightPercentage, // Percentage distance to include neighboring hulls 0-100
        uint32_t boneIndices[4], // 4 nearest constraints.
        double	 boneWeighting[4]) = 0;	// 4 bone weights, sum is always equal to 1

    // Returns the number of collision pairs which need to be filtered.
    // These are convex hulls that overlap in their rest pose but are not constrained
    // to each other.  These will generate potentially bad collision contacts which 
    // prevent the objects from coming to rest.
    // Use these pairs to exclude those collisions
    // If it returns a null pointer, then no collision pair filters are required (or constraints haven't been generated yet)
    // collisionFilterPairCount will be assigned the number of pairs to filter.
    // The return value will be pairs of integer; example 3,4 meaning body 3 and body 4 should not collide
    virtual const uint32_t *getCollisionFilterPairs(uint32_t &collisionPairFilterCount) const = 0;

    virtual void release(void) = 0;
protected:
    virtual ~SkeletonRig(void)
    {
    }
};

class SkeletonRigFactory
{
public:
    virtual SkeletonRig *create(uint32_t convexHullCount,const SimpleMesh *convexHulls) = 0;

    virtual void release(void) = 0;
protected:
    virtual ~SkeletonRigFactory(void)
    {
    }
};

#ifdef _MSC_VER
__declspec(dllexport) SkeletonRigFactory* __cdecl createSkeletonRigPlugin(uint32_t versionNumber,const char *dllName);
#else
extern "C" MemTracker* createSkeletonRigPlugin(uint32_t versionNumber,const char *dllName);
#endif

extern skeletonrig::SkeletonRigFactory *gSkeletonRigFactory;


} // end of namespace




