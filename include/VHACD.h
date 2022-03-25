/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#ifndef VHACD_H
#    define VHACD_H

#    define VHACD_VERSION_MAJOR 4
#    define VHACD_VERSION_MINOR 0

// Changes for version 3.0 : April 17, 2020 : John W. Ratcliff mailto:jratcliffscarab@gmail.com
//
// * An optional callback method called 'NotifyVHACDComplete' was added to the IUserCallback interface.
// * This method is called when an asynchronous V-HACD operation is been completed. The application can
// * use this call back as a trigger to wake up the app to start processing the V-HACD results. The
// * original polled method 'IsReady' is still valid and can continue to be used.
//
// * Some code cleanup was done, removed obsolete OpenCL, OpenMP and SIMD code.
// * The purpose of this change was to make the code more vanilla C++ and to make it easier to maintain
// * Removed the obsolete tetrahedral 'mode' as the library currently only supports voxels.
// * Again this was to make the code base smaller and easier to maintain.
// *
// * A new optional virtual interface called 'IUserProfiler' was provided.
// * This allows the user to provide an optional profiling callback interface to assist in
// * diagnosing performance issues. This change was made by Danny Couture at Epic for the UE4 integration.
// * Some profiling macros were also declared in support of this feature.
// *
// * Another new optional virtual interface called 'IUserTaskRunner' was provided.
// * This interface is used to run logical 'tasks' in a background thread. If none is provided
// * then a default implementation using std::thread will be executed.
// * This change was made by Danny Couture at Epic to speed up the voxelization step.
// *
// * Some performance optimizations were made to the code. They include the following:
// *
// * During the voxelization step there was code to rotate every point in the source mesh.
// * However this feature was not actually being used, so it was removed to improve performance
// * during the voxelization stage.
// *
// * Optimizations made to the voxelization step by Danny Couture at Epic include:
// * -Fix a comparison bug in VHACD Volume::Voxelize causing the number of
// *   voxels to explode when x == y and z < x.
// * - Rewrite voxel flood fill algorithm toward cache friendliness and get
// *      rid of extra data structure causing high memory usage on large voxels.
// * - Make the data ordering in vhacdVolume GetVoxel more cache friendly
// *   toward loops iterating over(i, j, k) in that order.
// *
// * During the voxelization phase, the original implementation would attempt to voxelize the input mesh
// * multiple times until it hit some target number of voxels. This step was not needed, as the number of
// * of desired voxels implies a certain voxel resolution so instead it just does the voxelization step
// * in a single pass at the specified detail level.
// *
// * The final performance optimization was to unroll the ComputeACD step so it can be run in parallel.
// * A new helper class was introduced called 'SimpleJobSystem' which can run micro-tasks across multiple threads.
// * When computing the ACD (approximate convex decomposition) the code iterates through all input sub-meshes
// * and computes the concavity, the optimal splitting plane and, optionally, splits the input mesh by a plane.
// * This can happen hundreds of even thousands of times. The original code did this in a for loop using a single
// * thread. The change made was so that this operation could be done in parallel. Since this operation is extremely
// * memory bound, it only provides about a 2x performance improvement, but that is still twice as fast at least.
// * After the voxelization optimizations the ComputeACD step remained slowest part of V-HACD.
// *
// * A new feature was introduced called the 'FillMode'. By default V-HACD performs a flood fill operation after
// * the voxelization step to determine which voxels lie 'inside' the source mesh surface and which are 'outside'.
// * Most of the time this flood fill approach works fine. However, if the source mesh has a hole anywhere in it
// * it can result in no voxels being marked as 'inside the surface'.  When this happens the convex decomposition
// * will treat the mesh as being 'hollow' inside. A good example of this failure case is the Stanford bunny which
// * has a hole on the bottom.
// *
// * A new FillMode was introduced called 'RAYCAST_FILL' which can correct for this. If the FillMode is set to
// * 'RAYCAST_FILL' then rather than doing a flood fill, instead each voxel will be categorized by performing
// * a raycast in the six cardinal directions. If 3 or more of those raycasts hit the 'inside' of the source mesh
// * then that voxel will be marked as being 'inside'. This can correct for the edge case when the source mesh has
// * a voxel sized hole in it.
// *
// * Finally, there are some cases where you might actually want the source mesh to be treated as being a 'hollow'
// * object.  For certain types of simulations this is actually desirable. So, there is a new FillMode called
// * 'SURFACE_ONLY' which will not mark any voxel as being 'inside' which will treat any source mesh as being 'hollow'
// * inside and any deep recursion of the convex decomposition will converge towards the 'skin' of the object.
// * For example, here is a video clip showing a simulation of a torus, without any holes, as a hollow object.
// *     https://www.youtube.com/watch?v=CIOoWN4CDb4
// *
// * Finally an important bug fix is included with this update.
// * When using V-HACD by default, it works by converting the source mesh into a voxel grid and the performing
// * the convex decomposition relative to that voxelization. This has a number of benefits, both performance as
// * well as it greatly simplifies the plane splitting step vs. trying to perform a full CSG operation on an
// * arbitrary triangle mesh.
// *
// * However, one downside of doing this is that the resulting convex hulls suffer a loss of precision. Each
// * point in the final convex hull does not rest on the original source mesh but, instead, is based on the voxel
// * resolution. If you use extremely high voxel resolutions then this loss of precision is less of an issue but
// * using high voxel resolutions can make V-HACD run much more slowly.  If you use a low voxel resolution V-HACD
// * can run much more quickly but the output convex hull points will not be coplanar to the original source mesh
// * and can result in poor simulation behavior.
// *
// * To fix this a new option was introduced called 'm_projectHullVertices'. If th is bool is set to true then
// * as a final step each point on the convex hull output will be 'projected' onto the source mesh. In the previous
// * release of V-HACD this 'projection' was done by performing a raycast operation. However, it was discovered that
// * on some types of meshes this operation could fail producing completely bad convex hulls.
// *
// * This bug was fixed by switching to a 'closest point' algorithm instead of a raycast. A new code snippet was
// * introduced called 'aabbtree', originally written by Miles Macklin, that can compute the closest point on a triangle
// * mesh, within a radius, in a highly efficient manner.


#include <stdint.h>
#include <functional>

namespace VHACD
{

/**
* This enumeration determines how the voxels as filled to create a solid
* object. The default should be 'FLOOD_FILL' which generally works fine 
* for closed meshes. However, if the mesh is not watertight, then using
* RAYCAST_FILL may be preferable as it will determine if a voxel is part 
* of the interior of the source mesh by raycasting around it.
* 
* Finally, there are some cases where you might actually want a convex 
* decomposition to treat the source mesh as being hollow. If that is the
* case you can pass in 'SURFACE_ONLY' and then the convex decomposition 
* will converge only onto the 'skin' of the surface mesh.
*/
enum class FillMode
{
    FLOOD_FILL, // This is the default behavior, after the voxelization step it uses a flood fill to determine 'inside'
                // from 'outside'. Howerver, meshes with holes can fail and create hollow results.
    SURFACE_ONLY, // Only consider the 'surface', will create 'skins' with hollow centers.
    RAYCAST_FILL, // Uses raycasting to determine inside from outside.
};

class IVHACD
{
public:
    class IUserCallback
    {
    public:
        virtual ~IUserCallback(){};

        // Be aware that if you are running V-HACD asynchronously (in a background thread) this callback will come from
        // a different thread. So if your print/logging code isn't thread safe, take that into account.
        virtual void Update(const double overallProgress,
                            const double stageProgress,
                            const char* const stage,
                            const char *operation) = 0;

        // This is an optional user callback which is only called when running V-HACD asynchronously.
        // This is a callback performed to notify the user that the
        // convex decomposition background process is completed. This call back will occur from
        // a different thread so the user should take that into account.
        virtual void NotifyVHACDComplete(void)
        {
        }
    };

    class IUserLogger
    {
    public:
        virtual ~IUserLogger(){};
        virtual void Log(const char* const msg) = 0;
    };

    class IUserTaskRunner
    {
    public:
        virtual ~IUserTaskRunner(){};
        virtual void* StartTask(std::function<void()> func) = 0;
        virtual void JoinTask(void* Task) = 0;
    };

    class ConvexHull
    {
    public:
        uint32_t m_nPoints{0};                  // Total number of vertices in the convex hull
        double* m_points{nullptr};              // Points in the convex hull as doubles x1,y1,z1,x2,y2,z2...

        uint32_t m_nTriangles{0};               // Total number of triangles in the convex hull
        uint32_t* m_triangles{nullptr};         // Triangle indices

        double m_volume{0};                     // The volume of the convex hull
        double m_center[3]{0,0,0};              // The centroid of the convex hull
        uint32_t    m_meshId{0};                // A unique id for this convex hull
        double  mBmin[3];
        double  mBmax[3];
    };

    class Parameters
    {
    public:
        IUserCallback*      m_callback{nullptr};
        IUserLogger*        m_logger{nullptr};
        IUserTaskRunner*    m_taskRunner{nullptr};
        uint32_t            m_maxConvexHulls{64};
        uint32_t            m_resolution{100000};
        double              m_minimumVolumePercentErrorAllowed{8}; // if the voxels are within 8% of the volume of the hull, we consider this a close enough approximation
        uint32_t            m_maxRecursionDepth{12};
        bool                m_shrinkWrap{true};
        FillMode            m_fillMode{ FillMode::FLOOD_FILL };
        uint32_t            m_maxNumVerticesPerCH{64};
        bool                m_asyncACD{ true };
        uint32_t            m_minEdgeLength{4};                     // Once a voxel patch has an edge length of less than 4 on all 3 sides, we don't keep recursing
    };

    virtual void Cancel() = 0;

    virtual bool Compute(const float* const points,
                         const uint32_t countPoints,
                         const uint32_t* const triangles,
                         const uint32_t countTriangles,
                         const Parameters& params) = 0;

    virtual bool Compute(const double* const points,
                         const uint32_t countPoints,
                         const uint32_t* const triangles,
                         const uint32_t countTriangles,
                         const Parameters& params) = 0;

    virtual uint32_t GetNConvexHulls() const = 0;

    virtual bool GetConvexHull(const uint32_t index, ConvexHull& ch) const = 0;

    virtual void Clean(void) = 0; // release internally allocated memory

    virtual void Release(void) = 0; // release IVHACD

    // Will compute the center of mass of the convex hull decomposition results and return it
    // in 'centerOfMass'.  Returns false if the center of mass could not be computed.
    virtual bool ComputeCenterOfMass(double centerOfMass[3]) const = 0;

    // In synchronous mode (non-multi-threaded) the state is always 'ready'
    // In asynchronous mode, this returns true if the background thread is not still actively computing
    // a new solution.  In an asynchronous config the 'IsReady' call will report any update or log
    // messages in the caller's current thread.
    virtual bool IsReady(void) const
    {
        return true;
    }

    /**
    * At the request of LegionFu : out_look@foxmail.com
    * This method will return which convex hull is closest to the source position.
    * You can use this method to figure out, for example, which vertices in the original
    * source mesh are best associated with which convex hull.
    * 
    * @param pos : The input 3d position to test against
    * 
    * @return : Returns which convex hull this position is closest to.
    */
    virtual uint32_t findNearestConvexHull(const double pos[3],double &distanceToHull) = 0;

protected:
    virtual ~IVHACD(void)
    {
    }
};

IVHACD* CreateVHACD(void);
IVHACD* CreateVHACD_ASYNC(void);
} // namespace VHACD
#endif // VHACD_H
