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

// Please view this slide deck which describes usage and how the algorithm works.
// https://docs.google.com/presentation/d/1OZ4mtZYrGEC8qffqb8F7Le2xzufiqvaPpRbLHKKgTIM/edit?usp=sharing

// VHACD is now a header only library.
// In just *one* of your CPP files *before* you include 'VHACD.h' you must declare
// #define ENABLE_VHACD_IMPLEMENTATION 1
// This will compile the implementation code into your project. If you don't
// have this define, you will get link errors since the implementation code will
// not be present. If you define it more than once in your code base, you will get
// link errors due to a duplicate implementation. This is the same pattern used by
// ImGui and StbLib and other popular open source libraries.

#    define VHACD_VERSION_MAJOR 4
#    define VHACD_VERSION_MINOR 0

// Changes for version 4.0
//
// * The code has been significantly refactored to be cleaner and easier to maintain
//      * All OpenCL related code removed
//      * All Bullet code removed
//      * All SIMD code removed
//      * Old plane splitting code removed
// 
// * The code is now delivered as a single hearder file 'VHACD.h' which has both the API
// * declaration as well as the implementation.  Simply add '#define ENABLE_VHACD_IMPLEMENTATION 1'
// * to any CPP in your application prior to including 'VHACD.h'. Only do this in one CPP though.
// * If you do not have this define once, you will get link errors since the implementation code
// * will not be compiled in. If you have this define more than once, you are likely to get
// * duplicate symbol link errors.
//
// * Since the library is now delivered as a single header file, we do not provide binaries
// * or build scripts as these are not needed.
//
// * The old DebugView and test code has all been removed and replaced with a much smaller and
// * simpler test console application with some test meshes to work with.
//
// * The convex hull generation code has changed. The previous version came from Bullet. 
// * However, the new version is courtesy of Julio Jerez, the author of the Newton
// * physics engine. His new version is faster and more numerically stable.
//
// * The code can now detect if the input mesh is, itself, already a convex object and
// * can early out.
//
// * Significant performance improvements have been made to the code and it is now much
// * faster, stable, and is easier to tune than previous versions.
//
// * A bug was fixed with the shrink wrapping code (project hull vertices) that could
// * sometime produce artifacts in the results. The new version uses a 'closest point'
// * algorithm that is more reliable.
//
// * You can now select which 'fill mode' to use. For perfectly closed meshes, the default
// * behavior using a flood fill generally works fine. However, some meshes have small 
// * holes in them and therefore the flood fill will fail, treating the mesh as being
// * hollow. In these cases, you can use the 'raycast' fill option to determine which 
// * parts of the voxelized mesh are 'inside' versus being 'outside'. Finally, there
// * are some rare instances where a user might actually want the mesh to be treated as
// * hollow, in which case you can pass in 'surface' only.
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



// The history of V-HACD:
//
// The initial version was written by John W. Ratcliff and was called 'ACD'
// This version did not perform CSG operations on the source mesh, so if you 
// recursed too deeply it would produce hollow results.
//
// The next version was written by Khaled Mamou and was called 'HACD'
// In this version Khaled tried to perform a CSG operation on the source 
// mesh to produce more robust results. Howerver, Khaled learned that the
// CSG library he was using had licensing issues so he started work on the
// next version.
//
// The next version was called 'V-HACD' because Khaled made the observation
// that plane splitting would be far easier to implement working in voxel space.
// 
// V-HACD has been integrated into UE4, Blender, and a number of other projects.
// This new release, version4, is a siginficant refactor of the code to fix 
// some bugs, improve performance, and to make the codebase easier to maintain
// going forward.

#include <stdint.h>
#include <functional>

#include <vector>
#include <array>

namespace VHACD {

struct Vertex
{
    double mX;
    double mY;
    double mZ;

    Vertex() = default;
    Vertex(double x, double y, double z) : mX(x), mY(y), mZ(z) {}

    const double& operator[](size_t idx) const
    {
        switch(idx)
        {
            case 0: return mX;
            case 1: return mY;
            case 2: return mZ;
        };
        return mX;
    }
};

struct Triangle
{
    uint32_t mI0;
    uint32_t mI1;
    uint32_t mI2;

    Triangle() = default;
    Triangle(uint32_t i0, uint32_t i1, uint32_t i2) : mI0(i0), mI1(i1), mI2(i2) {}
};

} // namespace VHACD

namespace nd
{
    namespace VHACD
    {

template <typename T>
class Vect3 {
public:
    /*
     * Getters
     */
    T& operator[](size_t i);
    const T& operator[](size_t i) const;
    T& getX();
    T& getY();
    T& getZ();
    const T& getX() const;
    const T& getY() const;
    const T& getZ() const;

    /*
     * Normalize and norming
     */
    T Normalize();
    Vect3 Normalized();
    T GetNorm() const;
    T GetNormSquared() const;
    int LongestAxis() const;

    /*
     * Vector-vector operations
     */
    Vect3& operator=(const Vect3& rhs);
    Vect3& operator+=(const Vect3& rhs);
    Vect3& operator-=(const Vect3& rhs);

    Vect3 CWiseMul(const Vect3& rhs) const;
    Vect3 Cross(const Vect3& rhs) const;
    T Dot(const Vect3& rhs) const;
    Vect3 operator+(const Vect3& rhs) const;
    Vect3 operator-(const Vect3& rhs) const;

    /*
     * Vector-scalar operations
     */
    Vect3& operator-=(T a);
    Vect3& operator+=(T a);
    Vect3& operator/=(T a);
    Vect3& operator*=(T a);

    Vect3 operator*(T rhs) const;
    Vect3 operator/(T rhs) const;

    /*
     * Unary operations
     */
    Vect3 operator-() const;

    /*
     * Comparison operators
     */
    bool operator<(const Vect3& rhs) const;
    bool operator>(const Vect3& rhs) const;

    Vect3 CWiseMin(const Vect3& rhs) const;
    Vect3 CWiseMax(const Vect3& rhs) const;
    T MinCoeff() const;
    T MaxCoeff() const;

    T MinCoeff(uint32_t& idx) const;
    T MaxCoeff(uint32_t& idx) const;

    /*
     * Constructors
     */
    Vect3() = default;
    Vect3(T a);
    Vect3(T x, T y, T z);
    Vect3(const Vect3& rhs);
    ~Vect3() = default;

    Vect3(const ::VHACD::Vertex&);
    Vect3(const ::VHACD::Triangle&);

    operator ::VHACD::Vertex() const;

private:
    std::array<T, 3> mData;
};

}
}// namespace nd::VHACD

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
    /**
    * This optional pure virtual interface is used to notify the caller of the progress
    * of convex decomposition as well as a signal when it is complete when running in
    * a background thread
    */
    class IUserCallback
    {
    public:
        virtual ~IUserCallback(){};

        /**
        * Notifies the appliication of the current state of the convex decomposition operation
        * 
        * @param overallProgress : Total progress from 0-100%
        * @param stageProgress : Progress of the current stage 0-100%
        * @param stage : A text description of the current stage we are in
        * @param operatoin : A text description of what operation is currently being performed.
        */
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

    /**
    * Optional user provided pure virtual interface to be notified of warning or informational messages
    */
    class IUserLogger
    {
    public:
        virtual ~IUserLogger(){};
        virtual void Log(const char* const msg) = 0;
    };

    /**
    * An optional user provided pure virtual interface to perform a background task.
    * This was added by Danny Couture at Epic as they wanted to use their own
    * threading system instead of the standard library version which is the default.
    */
    class IUserTaskRunner
    {
    public:
        virtual ~IUserTaskRunner(){};
        virtual void* StartTask(std::function<void()> func) = 0;
        virtual void JoinTask(void* Task) = 0;
    };

    /**
    * A simple class that represents a convex hull as a triangle mesh with 
    * double precision vertices. Polygons are not currently provided.
    */
    class ConvexHull
    {
    public:
        std::vector<VHACD::Vertex> m_points;
        std::vector<VHACD::Triangle> m_triangles;

        double m_volume{0};                     // The volume of the convex hull
        nd::VHACD::Vect3<double> m_center{0, 0, 0}; // The centroid of the convex hull
        uint32_t    m_meshId{0};                // A unique id for this convex hull
        nd::VHACD::Vect3<double>  mBmin;        // Bounding box minimum of the AABB
        nd::VHACD::Vect3<double>  mBmax;        // Bounding box maximum of he AABB
    };

    /**
    * This class provides the parameters controlling the convex decomposition operation
    */
    class Parameters
    {
    public:
        IUserCallback*      m_callback{nullptr};            // Optional user provided callback interface for progress
        IUserLogger*        m_logger{nullptr};              // Optional user provided callback interface for log messages
        IUserTaskRunner*    m_taskRunner{nullptr};          // Optional user provided interface for creating tasks
        uint32_t            m_maxConvexHulls{64};           // The maximum number of convex hulls to produce
        uint32_t            m_resolution{400000};           // The voxel resolution to use
        double              m_minimumVolumePercentErrorAllowed{1}; // if the voxels are within 1% of the volume of the hull, we consider this a close enough approximation
        uint32_t            m_maxRecursionDepth{14};        // The maximum recursion depth
        bool                m_shrinkWrap{true};             // Whether or not to shrinkwrap the voxel positions to the source mesh on output
        FillMode            m_fillMode{ FillMode::FLOOD_FILL }; // How to fill the interior of the voxelized mesh
        uint32_t            m_maxNumVerticesPerCH{64};      // The maximum number of vertices allowed in any output convex hull
        bool                m_asyncACD{ true };             // Whether or not to run asynchronously, taking advantage of additonal cores
        uint32_t            m_minEdgeLength{2};             // Once a voxel patch has an edge length of less than 4 on all 3 sides, we don't keep recursing
        bool                m_findBestPlane{false};         // Whether or not to attempt to split planes along the best location. Experimental feature. False by default.
    };

    /**
    * Will cause the convex decomposition operation to be canceled early. No results will be produced but the background operaiton will end as soon as it can.
    */
    virtual void Cancel() = 0;

    /**
    * Compute a convex decomposition of a triangle mesh using float vertices and the provided user parameters.
    * 
    * @param points : The vertices of the source mesh as floats in the form of X1,Y1,Z1,  X2,Y2,Z2,.. etc.
    * @param countPoints : The number of vertices in the source mesh.
    * @param triangles : The indices of triangles in the source mesh in the form of I1,I2,I3, .... 
    * @param countTriangles : The number of triangles in the source mesh
    * @param params : The convex decomposition parameters to apply
    * @return : Returns true if the convex decomposition operation can be started
    */
    virtual bool Compute(const float* const points,
                         const uint32_t countPoints,
                         const uint32_t* const triangles,
                         const uint32_t countTriangles,
                         const Parameters& params) = 0;

    /**
    * Compute a convex decomposition of a triangle mesh using double vertices and the provided user parameters.
    * 
    * @param points : The vertices of the source mesh as floats in the form of X1,Y1,Z1,  X2,Y2,Z2,.. etc.
    * @param countPoints : The number of vertices in the source mesh.
    * @param triangles : The indices of triangles in the source mesh in the form of I1,I2,I3, .... 
    * @param countTriangles : The number of triangles in the source mesh
    * @param params : The convex decomposition parameters to apply
    * @return : Returns true if the convex decomposition operation can be started
    */
    virtual bool Compute(const double* const points,
                         const uint32_t countPoints,
                         const uint32_t* const triangles,
                         const uint32_t countTriangles,
                         const Parameters& params) = 0;

    /**
    * Returns the number of convex hulls that were produced.
    * 
    * @return : Returns the number of convex hulls produced, or zero if it failed or was canceled
    */
    virtual uint32_t GetNConvexHulls() const = 0;

    /**
    * Retrieves one of the convex hulls in the solution set
    * 
    * @param index : Which convex hull to retrieve
    * @param ch : The convex hull descriptor to return
    * @return : Returns true if the convex hull exists and could be retrieved
    */
    virtual bool GetConvexHull(const uint32_t index,
                               ConvexHull& ch) const = 0;

    /**
    * Releases any memory allocated by the V-HACD class
    */
    virtual void Clean(void) = 0; // release internally allocated memory

    /**
    * Releases this instance of the V-HACD class
    */
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
    virtual uint32_t findNearestConvexHull(const double pos[3],
                                           double &distanceToHull) = 0;

protected:
    virtual ~IVHACD(void)
    {
    }
};

IVHACD* CreateVHACD(void);      // Create a synchronous (blocking) implementation of V-HACD
IVHACD* CreateVHACD_ASYNC(void);    // Create an asynchronous (non-blocking) implementation of V-HACD
} // namespace VHACD


#if ENABLE_VHACD_IMPLEMENTATION
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <limits.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100 4189 4456 4701 4702 4127 4996)
#endif // _MSC_VER

#ifdef __GNUC__
#pragma GCC diagnostic push
// #pragma GCC diagnostic warning "-Wold-style-cast"
// #pragma GCC diagnostic warning "-Wreorder"
// #pragma GCC diagnostic warning "-Wunused-variable"
// #pragma GCC diagnostic warning "-Wignored-qualifiers"
// #pragma GCC diagnostic warning "-Wnon-virtual-dtor"
// #pragma GCC diagnostic warning "-Wuninitialized"
#endif // __GNUC__

#define VHACD_SAFE_RELEASE(x) if ( x ) { x->release(); x = nullptr; }

// Scoped Timer
namespace VHACD
{

class Timer
{
public:
    Timer() : mStartTime(std::chrono::high_resolution_clock::now())
    {
    }

    void reset()
    {
        mStartTime = std::chrono::high_resolution_clock::now();
    }

    double getElapsedSeconds()
    {
        auto s = peekElapsedSeconds();
        reset();
        return s;
    }

    double peekElapsedSeconds()
    {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = now - mStartTime;
        return diff.count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> mStartTime;
};

class ScopedTime
{
public:
    ScopedTime(const char *action,VHACD::IVHACD::IUserLogger *logger) : mAction(action), mLogger(logger)
    {
        mTimer.reset();
    }

    ~ScopedTime(void)
    {
        double dtime = mTimer.getElapsedSeconds();
        if( mLogger )
        {
            char scratch[512];
            snprintf(scratch,sizeof(scratch),"%s took %0.5f seconds", mAction, dtime);
            mLogger->Log(scratch);
        }
    }

    const char *mAction{nullptr};
    Timer       mTimer;
    VHACD::IVHACD::IUserLogger *mLogger{nullptr};
};

} // namespace VHACD

//***********************************************************************************************
// ConvexHull generation code by Julio Jerez <jerezjulio0@gmail.com>
//***********************************************************************************************

template <class T>
inline T Max(T a, T b)
{
    return (a > b) ? a : b;
}

template <class T>
inline T Min(T a, T b)
{
    return (a < b) ? a : b;
}

namespace nd
{
namespace VHACD
{
    /*
     * Out of line definitions
     */

    /*
     * Getters
     */
    template <typename T>
    inline T& Vect3<T>::operator[](size_t i)
    {
        return mData[i];
    }

    template <typename T>
    inline const T& Vect3<T>::operator[](size_t i) const
    {
        return mData[i];
    }

    template <typename T>
    inline T& Vect3<T>::getX()
    {
        return mData[0];
    }

    template <typename T>
    inline T& Vect3<T>::getY()
    {
        return mData[1];
    }

    template <typename T>
    inline T& Vect3<T>::getZ()
    {
        return mData[2];
    }

    template <typename T>
    inline const T& Vect3<T>::getX() const
    {
        return mData[0];
    }

    template <typename T>
    inline const T& Vect3<T>::getY() const
    {
        return mData[1];
    }

    template <typename T>
    inline const T& Vect3<T>::getZ() const
    {
        return mData[2];
    }

    /*
     * Normalize and norming
     */
    template <typename T>
    inline T Vect3<T>::Normalize()
    {
        T n = GetNorm();
        if (n != 0.0) (*this) /= n;
        return n;
    }

    template <typename T>
    inline Vect3<T> Vect3<T>::Normalized()
    {
        Vect3<T> ret = *this;
        T n = GetNorm();
        if (n != 0.0) ret /= n;
        return ret;
    }

    template <typename T>
    inline T Vect3<T>::GetNorm() const
    {
        return std::sqrt(GetNormSquared());
    }

    template <typename T>
    inline T Vect3<T>::GetNormSquared() const
    {
        return this->Dot(*this);
    }

    template <typename T>
    inline int Vect3<T>::LongestAxis() const
    {
        if (getX() > getY() && getX() > getZ())
            return 0;
        if (getY() > getZ())
            return 1 ;
        return 2;
    }

    /*
     * Vector-vector operations
     */
    template <typename T>
    inline Vect3<T>& Vect3<T>::operator=(const Vect3<T>& rhs)
    {
        getX() = rhs.getX();
        getY() = rhs.getY();
        getZ() = rhs.getZ();
        return *this;
    }

    template <typename T>
    inline Vect3<T>& Vect3<T>::operator+=(const Vect3<T>& rhs)
    {
        getX() += rhs.getX();
        getY() += rhs.getY();
        getZ() += rhs.getZ();
        return *this;
    }

    template <typename T>
    inline Vect3<T>& Vect3<T>::operator-=(const Vect3<T>& rhs)
    {
        getX() -= rhs.getX();
        getY() -= rhs.getY();
        getZ() -= rhs.getZ();
        return *this;
    }

    template <typename T>
    inline Vect3<T> Vect3<T>::CWiseMul(const Vect3<T>& rhs) const
    {
        return Vect3<T>(getX() * rhs.getX(),
                        getY() * rhs.getY(),
                        getZ() * rhs.getZ());
    }

    template <typename T>
    inline Vect3<T> Vect3<T>::Cross(const Vect3<T>& rhs) const
    {
        return Vect3<T>(getY() * rhs.getZ() - getZ() * rhs.getY(),
                        getZ() * rhs.getX() - getX() * rhs.getZ(),
                        getX() * rhs.getY() - getY() * rhs.getX());
    }

    template <typename T>
    inline T Vect3<T>::Dot(const Vect3<T>& rhs) const
    {
        return getX() * rhs.getX() + getY() * rhs.getY() + getZ() * rhs.getZ();
    }

    template <typename T>
    inline Vect3<T> Vect3<T>::operator+(const Vect3<T>& rhs) const
    {
        return Vect3<T>(getX() + rhs.getX(),
                        getY() + rhs.getY(),
                        getZ() + rhs.getZ());
    }

    template <typename T>
    inline  Vect3<T> Vect3<T>::operator-(const Vect3<T>& rhs) const
    {
        return Vect3<T>(getX() - rhs.getX(),
                        getY() - rhs.getY(),
                        getZ() - rhs.getZ());
    }

    template <typename T>
    inline Vect3<T> operator*(T lhs, const Vect3<T> & rhs)
    {
        return Vect3<T>(lhs * rhs.getX(),
                        lhs * rhs.getY(),
                        lhs * rhs.getZ());
    }

    /*
     * Vector-scalar operations
     */
    template <typename T>
    inline Vect3<T>& Vect3<T>::operator-=(T a)
    {
        getX() -= a;
        getY() -= a;
        getZ() -= a;
        return *this;
    }

    template <typename T>
    inline Vect3<T>& Vect3<T>::operator+=(T a)
    {
        getX() += a;
        getY() += a;
        getZ() += a;
        return *this;
    }

    template <typename T>
    inline Vect3<T>& Vect3<T>::operator/=(T a)
    {
        getX() /= a;
        getY() /= a;
        getZ() /= a;
        return *this;
    }

    template <typename T>
    inline Vect3<T>& Vect3<T>::operator*=(T a)
    {
        getX() *= a;
        getY() *= a;
        getZ() *= a;
        return *this;
    }

    template <typename T>
    inline Vect3<T> Vect3<T>::operator*(T rhs) const
    {
        return Vect3<T>(getX() * rhs,
                        getY() * rhs,
                        getZ() * rhs);
    }

    template <typename T>
    inline Vect3<T> Vect3<T>::operator/(T rhs) const
    {
        return Vect3<T>(getX() / rhs,
                        getY() / rhs,
                        getZ() / rhs);
    }

    /*
     * Unary operations
     */
    template <typename T>
    inline  Vect3<T> Vect3<T>::operator-() const
    {
        return Vect3<T>(-getX(),
                        -getY(),
                        -getZ());
    }

    /*
     * Comparison operators
     */
    template <typename T>
    inline bool Vect3<T>::operator<(const Vect3<T>& rhs) const
    {
        if (getX() == rhs[0])
        {
            if (getY() == rhs[1])
            {
                return (getZ() < rhs[2]);
            }
            return (getY() < rhs[1]);
        }
        return (getX() < rhs[0]);
    }

    template <typename T>
    inline bool Vect3<T>::operator>(const Vect3<T>& rhs) const
    {
        if (getX() == rhs[0])
        {
            if (getY() == rhs[1])
            {
                return (getZ() > rhs[2]);
            }
            return (getY() > rhs[1]);
        }
        return (getX() > rhs[0]);
    }

    template <typename T>
    inline Vect3<T> Vect3<T>::CWiseMin(const Vect3<T>& rhs) const
    {
        return Vect3<T>(Min(getX(), rhs.getX()),
                        Min(getY(), rhs.getY()),
                        Min(getZ(), rhs.getZ()));
    }

    template <typename T>
    inline Vect3<T> Vect3<T>::CWiseMax(const Vect3<T>& rhs) const
    {
        return Vect3<T>(Max(getX(), rhs.getX()),
                        Max(getY(), rhs.getY()),
                        Max(getZ(), rhs.getZ()));
    }

    template <typename T>
    inline T Vect3<T>::MinCoeff() const
    {
        if (getX() < getY() && getX() < getZ())
        {
            return getX();
        }
        if (getY() < getZ())
        {
            return getY();
        }
        return getZ();
    }

    template <typename T>
    inline T Vect3<T>::MaxCoeff() const
    {
        if (getX() > getY() && getX() > getZ())
        {
            return getX();
        }
        if (getY() > getZ())
        {
            return getY();
        }
        return getZ();
    }

    template <typename T>
    inline T Vect3<T>::MinCoeff(uint32_t& idx) const
    {
        if (getX() < getY() && getX() < getZ())
        {
            idx = 0;
            return getX();
        }
        if (getY() < getZ())
        {
            idx = 1;
            return getY();
        }
        idx = 2;
        return getZ();
    }

    template <typename T>
    inline T Vect3<T>::MaxCoeff(uint32_t& idx) const
    {
        if (getX() > getY() && getX() < getZ())
        {
            idx = 0;
            return getX();
        }
        if (getY() > getZ())
        {
            idx = 1;
            return getY();
        }
        idx = 2;
        return getZ();
    }

    /*
     * Constructors
     */
    template <typename T>
    inline Vect3<T>::Vect3(T a)
        : mData{a, a, a}
    {
    }

    template <typename T>
    inline Vect3<T>::Vect3(T x, T y, T z)
        : mData{x, y, z}
    {
    }

    template <typename T>
    inline Vect3<T>::Vect3(const Vect3& rhs)
        : mData{rhs.mData}
    {
    }

    template <typename T>
    inline Vect3<T>::Vect3(const ::VHACD::Vertex& rhs)
        : Vect3<T>(rhs.mX, rhs.mY, rhs.mZ)
    {
        static_assert(std::is_same<T, double>::value, "Vertex to Vect3 constructor only enabled for double");
    }

    template <typename T>
    inline Vect3<T>::Vect3(const ::VHACD::Triangle& rhs)
        : Vect3<T>(rhs.mI0, rhs.mI1, rhs.mI2)
    {
        static_assert(std::is_same<T, uint32_t>::value, "Triangle to Vect3 constructor only enabled for uint32_t");
    }

    template <typename T>
    inline Vect3<T>::operator ::VHACD::Vertex() const
    {
        static_assert(std::is_same<T, double>::value, "Vect3 to Vertex conversion only enable for double");
        return ::VHACD::Vertex(getX(), getY(), getZ());
    }
}
}// namespace nd::VHACD

namespace nd
{
	namespace VHACD 
	{
		#define VHACD_GOOGOL_SIZE		4

		class Googol;
		Googol Determinant2x2(const Googol matrix[2][2]);
		Googol Determinant3x3(const Googol matrix[3][3]);
		double Determinant2x2(const double matrix[2][2], double* const error);
		double Determinant3x3(const double matrix[3][3], double* const error);

		inline int dExp2(int x)
		{
			int exp;
			for (exp = -1; x; x >>= 1)
			{
				exp++;
			}
			return exp;
		}

		inline int dBitReversal(int v, int base)
		{
			int x = 0;
			int power = dExp2(base) - 1;
			do
			{
				x += (v & 1) << power;
				v >>= 1;
				power--;
			} while (v);
			return x;
		}

		template<class T>
		class List
		{
			public:
			class ndNode
			{
				ndNode(ndNode* const prev, ndNode* const next)
					:m_info()
					, m_next(next)
					, m_prev(prev)
				{
					if (m_prev)
					{
						m_prev->m_next = this;
					}
					if (m_next)
					{
						m_next->m_prev = this;
					}
				}

				ndNode(const T &info, ndNode* const prev, ndNode* const next)
					:m_info(info)
					,m_next(next)
					,m_prev(prev)
				{
					if (m_prev)
					{
						m_prev->m_next = this;
					}
					if (m_next)
					{
						m_next->m_prev = this;
					}
				}

				~ndNode()
				{
				}

				void Unlink()
				{
					if (m_prev)
					{
						m_prev->m_next = m_next;
					}
					if (m_next)
					{
						m_next->m_prev = m_prev;
					}
					m_prev = nullptr;
					m_next = nullptr;
				}

				void AddLast(ndNode* const node)
				{
					m_next = node;
					node->m_prev = this;
				}

				void AddFirst(ndNode* const node)
				{
					m_prev = node;
					node->m_next = this;
				}

				public:
				T& GetInfo()
				{
					return m_info;
				}

				ndNode *GetNext() const
				{
					return m_next;
				}

				ndNode *GetPrev() const
				{
					return m_prev;
				}

				private:
				T m_info;
				ndNode *m_next;
				ndNode *m_prev;
				friend class List<T>;
			};

			public:
			List()
				:m_first(nullptr)
				, m_last(nullptr)
				, m_count(0)
			{
			}

			~List()
			{
				RemoveAll();
			}

			void RemoveAll()
			{
				for (ndNode *node = m_first; node; node = m_first)
				{
					m_count--;
					m_first = node->GetNext();
					node->Unlink();
					delete node;
				}
				m_last = nullptr;
				m_first = nullptr;
			}

			ndNode* Append()
			{
				m_count++;
				if (m_first == nullptr)
				{
					m_first = new ndNode(nullptr, nullptr);
					m_last = m_first;
				}
				else
				{
					m_last = new ndNode(m_last, nullptr);
				}
				return m_last;
			}

			ndNode* Append(const T &element)
			{
				m_count++;
				if (m_first == nullptr)
				{
					m_first = new ndNode(element, nullptr, nullptr);
					m_last = m_first;
				}
				else
				{
					m_last = new ndNode(element, m_last, nullptr);
				}
				return m_last;
			}

			ndNode* Addtop(const T &element)
			{
				m_count++;
				if (m_last == nullptr)
				{
					m_last = new ndNode(element, nullptr, nullptr);
					m_first = m_last;
				}
				else
				{
					m_first = new ndNode(element, nullptr, m_first);
				}
				return m_first;
			}

			int GetCount() const
			{
				return m_count;
			}

			//operator int() const;

			ndNode* GetLast() const
			{
				return m_last;
			}

			ndNode* GetFirst() const
			{
				return m_first;
			}

			void Remove(ndNode* const node)
			{
				Unlink(node);
				delete node;
			}

			void Unlink(ndNode* const node)
			{
				m_count--;
				if (node == m_first)
				{
					m_first = m_first->GetNext();
				}
				if (node == m_last)
				{
					m_last = m_last->GetPrev();
				}
				node->Unlink();
			}

			void Remove(const T &element)
			{
				ndNode *const node = Find(element);
				if (node)
				{
					Remove(node);
				}
			}

			ndNode* Find(const T &element) const
			{
				ndNode *node;
				for (node = m_first; node; node = node->GetNext())
				{
					if (element == node->m_info)
					{
						break;
					}
				}
				return node;
			}

			private:
			ndNode* m_first;
			ndNode* m_last;
			int m_count;
			friend class ndNode;
		};

		class hullPlane : public VHACD::Vect3<double>
		{
			public:
			hullPlane(double x, double y, double z, double w)
				: VHACD::Vect3<double>(x, y, z)
				, m_w(w)
			{
			}

            hullPlane(const VHACD::Vect3<double> &P0,
                      const VHACD::Vect3<double> &P1,
                      const VHACD::Vect3<double> &P2)
				: VHACD::Vect3<double>((P1 - P0).Cross(P2 - P0))
			{
				m_w = -Dot(P0);
			}

			hullPlane Scale(double s) const
			{
				return hullPlane(getX() * s, getY() * s, getZ() * s, m_w * s);
			}

			inline hullPlane operator= (const hullPlane &rhs)
			{
				getX() = rhs.getX();
				getY() = rhs.getY();
				getZ() = rhs.getZ();
				m_w = rhs.m_w;
				return *this;
			}

			inline VHACD::Vect3<double> operator*(const VHACD::Vect3<double> & rhs) const
			{
                return VHACD::Vect3<double>(getX() * rhs.getX(),
                                  getY() * rhs.getY(),
                                  getZ() * rhs.getZ());
			}

			double Evalue(const VHACD::Vect3<double> &point) const
			{
				return Dot(point) + m_w;
			}

			double m_w;
		};

		class Googol
		{
			public:
			Googol(void);
			Googol(double value);

			operator double() const;
			Googol operator+ (const Googol &A) const;
			Googol operator- (const Googol &A) const;
			Googol operator* (const Googol &A) const;
			Googol operator/ (const Googol &A) const;

			Googol operator+= (const Googol &A);
			Googol operator-= (const Googol &A);

			bool operator> (const Googol &A) const;
			bool operator>= (const Googol &A) const;
			bool operator< (const Googol &A) const;
			bool operator<= (const Googol &A) const;
			bool operator== (const Googol &A) const;
			bool operator!= (const Googol &A) const;

			Googol Abs() const;
			Googol Sqrt() const;
			Googol InvSqrt() const;
			Googol Floor() const;

			void Trace() const;
			void ToString(char* const string) const;

			private:
			void InitFloatFloat(double value);
			void NegateMantissa(uint64_t* const mantissa) const;
			void CopySignedMantissa(uint64_t* const mantissa) const;
			int NormalizeMantissa(uint64_t* const mantissa) const;
			uint64_t CheckCarrier(uint64_t a, uint64_t b) const;
			void ShiftRightMantissa(uint64_t* const mantissa, int bits) const;

			int LeadingZeros(uint64_t a) const;
			void ExtendeMultiply(uint64_t a, uint64_t b, uint64_t& high, uint64_t& low) const;
			void ScaleMantissa(uint64_t* const out, uint64_t scale) const;

			int m_sign;
			int m_exponent;
			uint64_t m_mantissa[VHACD_GOOGOL_SIZE];

			public:
			static Googol m_zero;
			static Googol m_one;
			static Googol m_two;
			static Googol m_three;
			static Googol m_half;
		};

		template <class T>
		inline void Swap(T& A, T& B)
		{
			T tmp(A);
			A = B;
			B = tmp;
		}

		template <class T, class dCompareKey>
		void Sort(T* const array, int elements)
		{
			const int batchSize = 8;
			int stack[1024][2];

			stack[0][0] = 0;
			stack[0][1] = elements - 1;
			int stackIndex = 1;
			const dCompareKey comparator;
			while (stackIndex)
			{
				stackIndex--;
				int lo = stack[stackIndex][0];
				int hi = stack[stackIndex][1];
				if ((hi - lo) > batchSize)
				{
					int mid = (lo + hi) >> 1;
					if (comparator.Compare(array[lo], array[mid]) > 0)
					{
						Swap(array[lo], array[mid]);
					}
					if (comparator.Compare(array[mid], array[hi]) > 0)
					{
						Swap(array[mid], array[hi]);
					}
					if (comparator.Compare(array[lo], array[mid]) > 0)
					{
						Swap(array[lo], array[mid]);
					}
					int i = lo + 1;
					int j = hi - 1;
					const T pivot(array[mid]);
					do
					{
						while (comparator.Compare(array[i], pivot) < 0)
						{
							i++;
						}
						while (comparator.Compare(array[j], pivot) > 0)
						{
							j--;
						}

						if (i <= j)
						{
							Swap(array[i], array[j]);
							i++;
							j--;
						}
					} while (i <= j);

					if (i < hi)
					{
						stack[stackIndex][0] = i;
						stack[stackIndex][1] = hi;
						stackIndex++;
					}
					if (lo < j)
					{
						stack[stackIndex][0] = lo;
						stack[stackIndex][1] = j;
						stackIndex++;
					}
					assert(stackIndex < int(sizeof(stack) / (2 * sizeof(stack[0][0]))));
				}
			}

			int stride = batchSize + 1;
			if (elements < stride)
			{
				stride = elements;
			}
			for (int i = 1; i < stride; ++i)
			{
				if (comparator.Compare(array[0], array[i]) > 0)
				{
					Swap(array[0], array[i]);
				}
			}

			for (int i = 1; i < elements; ++i)
			{
				int j = i;
				const T tmp(array[i]);
				for (; comparator.Compare(array[j - 1], tmp) > 0; --j)
				{
					assert(j > 0);
					array[j] = array[j - 1];
				}
				array[j] = tmp;
			}
		}
	}
}

namespace nd
{
	namespace VHACD
	{
		#define VHACD_ABS(a)  ((a) >= 0.0 ? (a) : -(a))

		Googol Googol::m_zero(0.0);
		Googol Googol::m_one(1.0);
		Googol Googol::m_two(2.0);
		Googol Googol::m_three(3.0);
		Googol Googol::m_half(0.5);

		Googol::Googol(void)
			:m_sign(0)
			,m_exponent(0)
		{
			memset(m_mantissa, 0, sizeof(m_mantissa));
		}

		Googol::Googol(double value)
			:m_sign(0)
			, m_exponent(0)
		{
			int exp;
			double mantissa = fabs(frexp(value, &exp));

			m_exponent = int(exp);
			m_sign = (value >= 0) ? 0 : 1;

			memset(m_mantissa, 0, sizeof(m_mantissa));
			m_mantissa[0] = uint64_t(double(uint64_t(1) << 62) * mantissa);
		}

		void Googol::CopySignedMantissa(uint64_t* const mantissa) const
		{
			memcpy(mantissa, m_mantissa, sizeof(m_mantissa));
			if (m_sign)
			{
				NegateMantissa(mantissa);
			}
		}

		Googol::operator double() const
		{
			double mantissa = (double(1.0f) / double(uint64_t(1) << 62)) * double(m_mantissa[0]);
			mantissa = ldexp(mantissa, m_exponent) * (m_sign ? double(-1.0f) : double(1.0f));
			return mantissa;
		}

		Googol Googol::operator+ (const Googol &A) const
		{
			Googol tmp;
			if (m_mantissa[0] && A.m_mantissa[0])
			{
				uint64_t mantissa0[VHACD_GOOGOL_SIZE];
				uint64_t mantissa1[VHACD_GOOGOL_SIZE];
				uint64_t mantissa[VHACD_GOOGOL_SIZE];

				CopySignedMantissa(mantissa0);
				A.CopySignedMantissa(mantissa1);

				int exponetDiff = m_exponent - A.m_exponent;
				int exponent = m_exponent;
				if (exponetDiff > 0)
				{
					ShiftRightMantissa(mantissa1, exponetDiff);
				}
				else if (exponetDiff < 0)
				{
					exponent = A.m_exponent;
					ShiftRightMantissa(mantissa0, -exponetDiff);
				}

				uint64_t carrier = 0;
				for (int i = VHACD_GOOGOL_SIZE - 1; i >= 0; i--)
				{
					uint64_t m0 = mantissa0[i];
					uint64_t m1 = mantissa1[i];
					mantissa[i] = m0 + m1 + carrier;
					carrier = CheckCarrier(m0, m1) | CheckCarrier(m0 + m1, carrier);
				}

				int sign = 0;
				if (int64_t(mantissa[0]) < 0)
				{
					sign = 1;
					NegateMantissa(mantissa);
				}

				int bits = NormalizeMantissa(mantissa);
				if (bits <= (-64 * VHACD_GOOGOL_SIZE))
				{
					tmp.m_sign = 0;
					tmp.m_exponent = 0;
				}
				else
				{
					tmp.m_sign = sign;
					tmp.m_exponent = int(exponent + bits);
				}

				memcpy(tmp.m_mantissa, mantissa, sizeof(m_mantissa));
			}
			else if (A.m_mantissa[0])
			{
				tmp = A;
			}
			else
			{
				tmp = *this;
			}

			return tmp;
		}

		Googol Googol::operator- (const Googol &A) const
		{
			Googol tmp(A);
			tmp.m_sign = !tmp.m_sign;
			return *this + tmp;
		}

		void Googol::ScaleMantissa(uint64_t* const dst, uint64_t scale) const
		{
			uint64_t carrier = 0;
			for (int i = VHACD_GOOGOL_SIZE - 1; i >= 0; i--)
			{
				if (m_mantissa[i])
				{
					uint64_t low;
					uint64_t high;
					ExtendeMultiply(scale, m_mantissa[i], high, low);
					uint64_t acc = low + carrier;
					carrier = CheckCarrier(low, carrier);
					carrier += high;
					dst[i + 1] = acc;
				}
				else
				{
					dst[i + 1] = carrier;
					carrier = 0;
				}

			}
			dst[0] = carrier;
		}

		Googol Googol::operator* (const Googol &A) const
		{
			if (m_mantissa[0] && A.m_mantissa[0])
			{
				uint64_t mantissaAcc[VHACD_GOOGOL_SIZE * 2];
				memset(mantissaAcc, 0, sizeof(mantissaAcc));
				for (int i = VHACD_GOOGOL_SIZE - 1; i >= 0; i--)
				{
					uint64_t a = m_mantissa[i];
					if (a)
					{
						uint64_t mantissaScale[2 * VHACD_GOOGOL_SIZE];
						memset(mantissaScale, 0, sizeof(mantissaScale));
						A.ScaleMantissa(&mantissaScale[i], a);

						uint64_t carrier = 0;
						for (int j = 0; j < 2 * VHACD_GOOGOL_SIZE; j++)
						{
							const int k = 2 * VHACD_GOOGOL_SIZE - 1 - j;
							uint64_t m0 = mantissaAcc[k];
							uint64_t m1 = mantissaScale[k];
							mantissaAcc[k] = m0 + m1 + carrier;
							carrier = CheckCarrier(m0, m1) | CheckCarrier(m0 + m1, carrier);
						}
					}
				}

				uint64_t carrier = 0;
				//int bits = uint64_t(LeadingZeros (mantissaAcc[0]) - 2);
				int bits = LeadingZeros(mantissaAcc[0]) - 2;
				for (int i = 0; i < 2 * VHACD_GOOGOL_SIZE; i++)
				{
					const int k = 2 * VHACD_GOOGOL_SIZE - 1 - i;
					uint64_t a = mantissaAcc[k];
					mantissaAcc[k] = (a << uint64_t(bits)) | carrier;
					carrier = a >> uint64_t(64 - bits);
				}

				int exp = m_exponent + A.m_exponent - (bits - 2);

				Googol tmp;
				tmp.m_sign = m_sign ^ A.m_sign;
				tmp.m_exponent = int(exp);
				memcpy(tmp.m_mantissa, mantissaAcc, sizeof(m_mantissa));

				return tmp;
			}
			return Googol(0.0);
		}

		Googol Googol::operator/ (const Googol &A) const
		{
			Googol tmp(1.0 / A);
			tmp = tmp * (m_two - A * tmp);
			tmp = tmp * (m_two - A * tmp);
			int test = 0;
			int passes = 0;
			do
			{
				passes++;
				Googol tmp0(tmp);
				tmp = tmp * (m_two - A * tmp);
				test = memcmp(&tmp0, &tmp, sizeof(Googol));
			} while (test && (passes < (2 * VHACD_GOOGOL_SIZE)));
			return (*this) * tmp;
		}

		Googol Googol::Abs() const
		{
			Googol tmp(*this);
			tmp.m_sign = 0;
			return tmp;
		}

		Googol Googol::Floor() const
		{
			if (m_exponent < 1)
			{
				return Googol(0.0);
			}
			int bits = m_exponent + 2;
			int start = 0;
			while (bits >= 64)
			{
				bits -= 64;
				start++;
			}

			Googol tmp(*this);
			for (int i = VHACD_GOOGOL_SIZE - 1; i > start; i--)
			{
				tmp.m_mantissa[i] = 0;
			}
			// some compilers do no like this and I do not know why is that
			//uint64_t mask = (-1LL) << (64 - bits);
			uint64_t mask(~0ULL);
			mask <<= (64 - bits);
			tmp.m_mantissa[start] &= mask;
			return tmp;
		}

		Googol Googol::InvSqrt() const
		{
			const Googol& me = *this;
			Googol x(1.0f / sqrt(me));

			int test = 0;
			int passes = 0;
			do
			{
				passes++;
				Googol tmp(x);
				x = m_half * x * (m_three - me * x * x);
				test = memcmp(&x, &tmp, sizeof(Googol));
			} while (test && (passes < (2 * VHACD_GOOGOL_SIZE)));
			return x;
		}

		Googol Googol::Sqrt() const
		{
			return *this * InvSqrt();
		}

		void Googol::ToString(char* const string) const
		{
			Googol tmp(*this);
			Googol base(10.0);
			while (double(tmp) > 1.0)
			{
				tmp = tmp / base;
			}

			int index = 0;
			while (tmp.m_mantissa[0])
			{
				tmp = tmp * base;
				Googol digit(tmp.Floor());
				tmp -= digit;
				double val = digit;
				string[index] = char(val) + '0';
				index++;
			}
			string[index] = 0;
		}

		void Googol::NegateMantissa(uint64_t* const mantissa) const
		{
			uint64_t carrier = 1;
			for (int i = VHACD_GOOGOL_SIZE - 1; i >= 0; i--)
			{
				uint64_t a = ~mantissa[i] + carrier;
				if (a)
				{
					carrier = 0;
				}
				mantissa[i] = a;
			}
		}

		void Googol::ShiftRightMantissa(uint64_t* const mantissa, int bits) const
		{
			uint64_t carrier = 0;
			if (int64_t(mantissa[0]) < int64_t(0))
			{
				carrier = uint64_t(-1);
			}

			while (bits >= 64)
			{
				for (int i = VHACD_GOOGOL_SIZE - 2; i >= 0; i--)
				{
					mantissa[i + 1] = mantissa[i];
				}
				mantissa[0] = carrier;
				bits -= 64;
			}

			if (bits > 0)
			{
				carrier <<= (64 - bits);
				for (int i = 0; i < VHACD_GOOGOL_SIZE; i++)
				{
					uint64_t a = mantissa[i];
					mantissa[i] = (a >> bits) | carrier;
					carrier = a << (64 - bits);
				}
			}
		}

		int Googol::LeadingZeros(uint64_t a) const
		{
			#define VHACD_COUNTBIT(mask,add)		\
			{									\
				uint64_t test = a & mask;		\
				n += test ? 0 : add;			\
				a = test ? test : (a & ~mask);	\
			}

			int n = 0;
			VHACD_COUNTBIT(0xffffffff00000000LL, 32);
			VHACD_COUNTBIT(0xffff0000ffff0000LL, 16);
			VHACD_COUNTBIT(0xff00ff00ff00ff00LL, 8);
			VHACD_COUNTBIT(0xf0f0f0f0f0f0f0f0LL, 4);
			VHACD_COUNTBIT(0xccccccccccccccccLL, 2);
			VHACD_COUNTBIT(0xaaaaaaaaaaaaaaaaLL, 1);

			return n;
		}

		int Googol::NormalizeMantissa(uint64_t* const mantissa) const
		{
			int bits = 0;
			if (int64_t(mantissa[0] * 2) < 0)
			{
				bits = 1;
				ShiftRightMantissa(mantissa, 1);
			}
			else
			{
				while (!mantissa[0] && bits > (-64 * VHACD_GOOGOL_SIZE))
				{
					bits -= 64;
					for (int i = 1; i < VHACD_GOOGOL_SIZE; i++) {
						mantissa[i - 1] = mantissa[i];
					}
					mantissa[VHACD_GOOGOL_SIZE - 1] = 0;
				}

				if (bits > (-64 * VHACD_GOOGOL_SIZE))
				{
					int n = LeadingZeros(mantissa[0]) - 2;
					if (n > 0)
					{
						uint64_t carrier = 0;
						for (int i = VHACD_GOOGOL_SIZE - 1; i >= 0; i--)
						{
							uint64_t a = mantissa[i];
							mantissa[i] = (a << n) | carrier;
							carrier = a >> (64 - n);
						}
						bits -= n;
					}
					else if (n < 0)
					{
						// this is very rare but it does happens, whee the leading zeros of the mantissa is an exact multiple of 64
						uint64_t carrier = 0;
						int shift = -n;
						for (int i = 0; i < VHACD_GOOGOL_SIZE; i++)
						{
							uint64_t a = mantissa[i];
							mantissa[i] = (a >> shift) | carrier;
							carrier = a << (64 - shift);
						}
						bits -= n;
					}
				}
			}
			return bits;
		}

		uint64_t Googol::CheckCarrier(uint64_t a, uint64_t b) const
		{
			return ((uint64_t(-1) - b) < a) ? uint64_t(1) : 0;
		}

		void Googol::ExtendeMultiply(uint64_t a, uint64_t b, uint64_t& high, uint64_t& low) const
		{
			uint64_t bLow = b & 0xffffffff;
			uint64_t bHigh = b >> 32;
			uint64_t aLow = a & 0xffffffff;
			uint64_t aHigh = a >> 32;

			uint64_t l = bLow * aLow;

			uint64_t c1 = bHigh * aLow;
			uint64_t c2 = bLow * aHigh;
			uint64_t m = c1 + c2;
			uint64_t carrier = CheckCarrier(c1, c2) << 32;

			uint64_t h = bHigh * aHigh + carrier;

			uint64_t ml = m << 32;
			uint64_t ll = l + ml;
			uint64_t mh = (m >> 32) + CheckCarrier(l, ml);
			uint64_t hh = h + mh;

			low = ll;
			high = hh;
		}

		Googol Googol::operator+= (const Googol &A)
		{
			*this = *this + A;
			return *this;
		}

		Googol Googol::operator-= (const Googol &A)
		{
			*this = *this - A;
			return *this;
		}

		bool Googol::operator> (const Googol &A) const
		{
			Googol tmp(*this - A);
			return double(tmp) > 0.0;
		}

		bool Googol::operator>= (const Googol &A) const
		{
			Googol tmp(*this - A);
			return double(tmp) >= 0.0;
		}

		bool Googol::operator< (const Googol &A) const
		{
			Googol tmp(*this - A);
			return double(tmp) < 0.0;
		}

		bool Googol::operator<= (const Googol &A) const
		{
			Googol tmp(*this - A);
			return double(tmp) <= 0.0;
		}

		bool Googol::operator== (const Googol &A) const
		{
			Googol tmp(*this - A);
			return double(tmp) == 0.0;
		}

		bool Googol::operator!= (const Googol &A) const
		{
			Googol tmp(*this - A);
			return double(tmp) != 0.0;
		}

		void Googol::Trace() const
		{
			//dTrace (("%f ", double (*this)));
		}

		double Determinant2x2(const double matrix[2][2], double* const error)
		{
			double a00xa11 = matrix[0][0] * matrix[1][1];
			double a01xa10 = matrix[0][1] * matrix[1][0];
			*error = VHACD_ABS(a00xa11) + VHACD_ABS(a01xa10);
			return a00xa11 - a01xa10;
		}

		double Determinant3x3(const double matrix[3][3], double* const error)
		{
			double sign = double(-1.0f);
			double det = double(0.0f);
			double accError = double(0.0f);
			for (int i = 0; i < 3; i++)
			{
				double cofactor[2][2];
				for (int j = 0; j < 2; j++)
				{
					int k0 = 0;
					for (int k = 0; k < 3; k++)
					{
						if (k != i)
						{
							cofactor[j][k0] = matrix[j][k];
							k0++;
						}
					}
				}

				double parcialError;
				double minorDet = Determinant2x2(cofactor, &parcialError);
				accError += parcialError * VHACD_ABS(matrix[2][i]);
				det += sign * minorDet * matrix[2][i];
				sign *= double(-1.0f);
			}

			*error = accError;
			return det;
		}

		Googol Determinant2x2(const Googol matrix[2][2])
		{
			Googol a00xa11(matrix[0][0] * matrix[1][1]);
			Googol a01xa10(matrix[0][1] * matrix[1][0]);
			return a00xa11 - a01xa10;
		}

		Googol Determinant3x3(const Googol matrix[3][3])
		{
			Googol negOne(double(-1.0f));
			Googol sign(double(-1.0f));
			Googol det = double(0.0f);
			for (int i = 0; i < 3; i++)
			{
				Googol cofactor[2][2];
				for (int j = 0; j < 2; j++)
				{
					int k0 = 0;
					for (int k = 0; k < 3; k++)
					{
						if (k != i)
						{
							cofactor[j][k0] = matrix[j][k];
							k0++;
						}
					}
				}

				Googol minorDet(Determinant2x2(cofactor));
				det = det + sign * minorDet * matrix[2][i];
				sign = sign * negOne;
			}
			return det;
		}
	}
}


namespace nd
{
namespace VHACD
{
    class ConvexHullVertex;
    class ConvexHullAABBTreeNode;

    class ConvexHullFace
    {
        public:
        ConvexHullFace();
        double Evalue(const VHACD::Vect3<double>* const pointArray,
                        const VHACD::Vect3<double>& point) const;
        hullPlane GetPlaneEquation(const VHACD::Vect3<double>* const pointArray,
                                    bool& isValid) const;

        public:
        int m_index[3];

        private:
        int m_mark;
        List<ConvexHullFace>::ndNode* m_twin[3];

        friend class ConvexHull;
    };

    class ConvexHull : public List<ConvexHullFace>
    {
        class ndNormalMap;

        public:
        ConvexHull(const ConvexHull& source);
        ConvexHull(const std::vector<::VHACD::Vertex>& vertexCloud,
                   double distTol,
                   int maxVertexCount = 0x7fffffff);
        ~ConvexHull() = default;

        const std::vector<VHACD::Vect3<double>>& GetVertexPool() const;

        private:
        void BuildHull(const std::vector<::VHACD::Vertex>& vertexCloud,
                       double distTol,
                       int maxVertexCount);

        void GetUniquePoints(std::vector<ConvexHullVertex>& points);
        int InitVertexArray(std::vector<ConvexHullVertex>& points,
                            void* const memoryPool,
                            int maxMemSize);

        ConvexHullAABBTreeNode* BuildTreeNew(std::vector<ConvexHullVertex>& points,
                                             char** const memoryPool,
                                             int& maxMemSize) const;
        ConvexHullAABBTreeNode* BuildTreeOld(std::vector<ConvexHullVertex>& points,
                                             char** const memoryPool,
                                             int& maxMemSize);
        ConvexHullAABBTreeNode* BuildTreeRecurse(ConvexHullAABBTreeNode* const parent,
                                                 ConvexHullVertex* const points,
                                                 int count,
                                                 int baseIndex,
                                                 char** const memoryPool,
                                                 int& maxMemSize) const;

        ndNode* AddFace(int i0, int i1, int i2);

        void CalculateConvexHull3d(ConvexHullAABBTreeNode* vertexTree,
                                   std::vector<ConvexHullVertex>& points,
                                   int count,
                                   double distTol,
                                   int maxVertexCount);

        int SupportVertex(ConvexHullAABBTreeNode** const tree,
                          const std::vector<ConvexHullVertex>& points,
                          const VHACD::Vect3<double>& dir,
                          const bool removeEntry = true) const;
        double TetrahedrumVolume(const VHACD::Vect3<double>& p0,
                                 const VHACD::Vect3<double>& p1,
                                 const VHACD::Vect3<double>& p2,
                                 const VHACD::Vect3<double>& p3) const;

        VHACD::Vect3<double> m_aabbP0;
        VHACD::Vect3<double> m_aabbP1;
        double m_diag;
        std::vector<VHACD::Vect3<double>> m_points;
    };
}
}

namespace nd
{
	namespace VHACD
	{

		#define VHACD_CONVEXHULL_3D_VERTEX_CLUSTER_SIZE 8

		ConvexHullFace::ConvexHullFace()
		{
			m_mark = 0;
			m_twin[0] = nullptr;
			m_twin[1] = nullptr;
			m_twin[2] = nullptr;
		}

		hullPlane ConvexHullFace::GetPlaneEquation(const VHACD::Vect3<double>* const pointArray,
                                                   bool& isvalid) const
		{
			const VHACD::Vect3<double>& p0 = pointArray[m_index[0]];
			const VHACD::Vect3<double>& p1 = pointArray[m_index[1]];
			const VHACD::Vect3<double>& p2 = pointArray[m_index[2]];
			hullPlane plane(p0, p1, p2);

			isvalid = false;
			double mag2 = plane.Dot(plane);
			if (mag2 > 1.0e-16f)
			{
				isvalid = true;
				plane = plane.Scale(1.0f / sqrt(mag2));
			}
			return plane;
		}

		double ConvexHullFace::Evalue(const VHACD::Vect3<double>* const pointArray,
                                      const VHACD::Vect3<double>& point) const
		{
			const VHACD::Vect3<double>& p0 = pointArray[m_index[0]];
			const VHACD::Vect3<double>& p1 = pointArray[m_index[1]];
			const VHACD::Vect3<double>& p2 = pointArray[m_index[2]];

			double matrix[3][3];
			for (int i = 0; i < 3; ++i) 
			{
				matrix[0][i] = p2[i] - p0[i];
				matrix[1][i] = p1[i] - p0[i];
				matrix[2][i] = point[i] - p0[i];
			}

			double error;
			double det = Determinant3x3(matrix, &error);

			// the code use double, however the threshold for accuracy test is the machine precision of a float.
			// by changing this to a smaller number, the code should run faster since many small test will be considered valid
			// the precision must be a power of two no smaller than the machine precision of a double, (1<<48)
			// float64(1<<30) can be a good value

			// double precision	= double (1.0f) / double (1<<30);
			double precision = double(1.0f) / double(1 << 24);
			double errbound = error * precision;
			if (fabs(det) > errbound) 
			{
				return det;
			}
	
			Googol exactMatrix[3][3];
			for (int i = 0; i < 3; ++i) 
			{
				exactMatrix[0][i] = Googol(p2[i]) - Googol(p0[i]);
				exactMatrix[1][i] = Googol(p1[i]) - Googol(p0[i]);
				exactMatrix[2][i] = Googol(point[i]) - Googol(p0[i]);
			}
			return Determinant3x3(exactMatrix);
		}

		class ConvexHullVertex : public nd::VHACD::Vect3<double>
		{
			public:
			int m_mark;
		};

		class ConvexHullAABBTreeNode
		{
			public:
			//ConvexHullAABBTreeNode(ConvexHullAABBTreeNode* const parent)
			ConvexHullAABBTreeNode()
				:m_left(nullptr)
				,m_right(nullptr)
				,m_parent(nullptr)
			{
			}

			ConvexHullAABBTreeNode(ConvexHullAABBTreeNode* const parent)
				:m_left(nullptr)
				,m_right(nullptr)
				,m_parent(parent)
			{
			}

			nd::VHACD::Vect3<double> m_box[2];
			ConvexHullAABBTreeNode* m_left;
			ConvexHullAABBTreeNode* m_right;
			ConvexHullAABBTreeNode* m_parent;
		};

		class ConvexHull3dPointCluster : public ConvexHullAABBTreeNode
		{
			public:
			ConvexHull3dPointCluster()
				:ConvexHullAABBTreeNode()
			{
			}

			ConvexHull3dPointCluster(ConvexHullAABBTreeNode* const parent)
				:ConvexHullAABBTreeNode(parent)
			{
			}

			int m_count;
			int m_indices[VHACD_CONVEXHULL_3D_VERTEX_CLUSTER_SIZE];
		};

		class ConvexHull::ndNormalMap
		{
			public:
			ndNormalMap()
				: m_count(sizeof(m_normal) / sizeof(m_normal[0]))
			{
				nd::VHACD::Vect3<double> p0( 1.0,  0.0,  0.0);
				nd::VHACD::Vect3<double> p1(-1.0,  0.0,  0.0);
				nd::VHACD::Vect3<double> p2( 0.0,  1.0,  0.0);
				nd::VHACD::Vect3<double> p3( 0.0, -1.0,  0.0);
				nd::VHACD::Vect3<double> p4( 0.0,  0.0,  1.0);
				nd::VHACD::Vect3<double> p5( 0.0,  0.0, -1.0);

				int count = 0;
				int subdivitions = 2;
				TessellateTriangle(subdivitions, p4, p0, p2, count);
				TessellateTriangle(subdivitions, p0, p5, p2, count);
				TessellateTriangle(subdivitions, p5, p1, p2, count);
				TessellateTriangle(subdivitions, p1, p4, p2, count);
				TessellateTriangle(subdivitions, p0, p4, p3, count);
				TessellateTriangle(subdivitions, p5, p0, p3, count);
				TessellateTriangle(subdivitions, p1, p5, p3, count);
				TessellateTriangle(subdivitions, p4, p1, p3, count);
			}

			static const ndNormalMap& GetNormaMap()
			{
				static ndNormalMap normalMap;
				return normalMap;
			}

            void TessellateTriangle(int level,
                                    const nd::VHACD::Vect3<double>& p0,
                                    const nd::VHACD::Vect3<double>& p1,
                                    const nd::VHACD::Vect3<double>& p2,
                                    int& count)
			{
				if (level) 
				{
					assert(fabs(p0.Dot(p0) - double(1.0f)) < double(1.0e-4f));
					assert(fabs(p1.Dot(p1) - double(1.0f)) < double(1.0e-4f));
					assert(fabs(p2.Dot(p2) - double(1.0f)) < double(1.0e-4f));
					nd::VHACD::Vect3<double> p01(p0 + p1);
					nd::VHACD::Vect3<double> p12(p1 + p2);
					nd::VHACD::Vect3<double> p20(p2 + p0);

					p01 = p01 * (1.0 / p01.GetNorm());
					p12 = p12 * (1.0 / p12.GetNorm());
					p20 = p20 * (1.0 / p20.GetNorm());

					assert(fabs(p01.GetNormSquared() - double(1.0f)) < double(1.0e-4f));
					assert(fabs(p12.GetNormSquared() - double(1.0f)) < double(1.0e-4f));
					assert(fabs(p20.GetNormSquared() - double(1.0f)) < double(1.0e-4f));

					TessellateTriangle(level - 1, p0, p01, p20, count);
					TessellateTriangle(level - 1, p1, p12, p01, count);
					TessellateTriangle(level - 1, p2, p20, p12, count);
					TessellateTriangle(level - 1, p01, p12, p20, count);
				}
				else 
				{
					hullPlane n(p0, p1, p2);
					n = n.Scale(double(1.0f) / sqrt(n.Dot(n)));
					n.m_w = double(0.0f);
					int index = dBitReversal(count, sizeof(m_normal) / sizeof(m_normal[0]));
					m_normal[index] = n;
					count++;
					assert(count <= int(sizeof(m_normal) / sizeof(m_normal[0])));
				}
			}

			nd::VHACD::Vect3<double> m_normal[128];
			int m_count;
		};

        ConvexHull::ConvexHull(const std::vector<::VHACD::Vertex>& vertexCloud,
                               double distTol,
                               int maxVertexCount)
			: List<ConvexHullFace>()
			, m_aabbP0(0)
			, m_aabbP1(0)
			, m_diag()
			, m_points()
		{
			m_points.resize(0);
			if (vertexCloud.size() >= 4)
			{
                BuildHull(vertexCloud,
                          distTol,
                          maxVertexCount);
			}
		}

		const std::vector<nd::VHACD::Vect3<double>>& ConvexHull::GetVertexPool() const
		{
			return m_points;
		}

        void ConvexHull::BuildHull(const std::vector<::VHACD::Vertex>& vertexCloud,
                                   double distTol,
                                   int maxVertexCount)
		{
			size_t treeCount = vertexCloud.size() / (VHACD_CONVEXHULL_3D_VERTEX_CLUSTER_SIZE >> 1);
			if (treeCount < 4)
			{
				treeCount = 4;
			}
			treeCount *= 2;

			std::vector<ConvexHullVertex> points(vertexCloud.size());
			std::vector<ConvexHull3dPointCluster> treePool(treeCount + 256);

			for (int i = 0; i < vertexCloud.size(); ++i)
			{
				nd::VHACD::Vect3<double>& vertex = points[i];
                vertex = nd::VHACD::Vect3<double>(vertexCloud[i]);
				points[i].m_mark = 0;
			}
            int count = InitVertexArray(points,
                                        treePool.data(),
                                        sizeof(ConvexHull3dPointCluster) * int(treePool.size()));

			if (m_points.size() >= 4)
			{
                CalculateConvexHull3d(treePool.data(),
                                      points,
                                      count,
                                      distTol,
                                      maxVertexCount);
			}
		}

		void ConvexHull::GetUniquePoints(std::vector<ConvexHullVertex>& points)
		{
			class CompareVertex
			{
				public:
				int Compare(const ConvexHullVertex& elementA, const ConvexHullVertex& elementB) const
				{
					for (int i = 0; i < 3; i++)
					{
						if (elementA[i] < elementB[i])
						{
							return -1;
						}
						else if (elementA[i] > elementB[i])
						{
							return 1;
						}
					}
					return 0;
				}
			};

			int count = int(points.size());
			Sort<ConvexHullVertex, CompareVertex>(&points[0], count);

			int indexCount = 0;
			CompareVertex compareVetex;
			for (int i = 1; i < count; ++i)
			{
				for (; i < count; ++i)
				{
					if (compareVetex.Compare(points[indexCount], points[i]))
					{
						indexCount++;
						points[indexCount] = points[i];
						break;
					}
				}
			}
			points.resize(indexCount + 1);
		}

        ConvexHullAABBTreeNode* ConvexHull::BuildTreeRecurse(ConvexHullAABBTreeNode* const parent,
                                                             ConvexHullVertex* const points,
                                                             int count,
                                                             int baseIndex,
                                                             char** memoryPool,
                                                             int& maxMemSize) const
		{
			ConvexHullAABBTreeNode* tree = nullptr;

			assert(count);
			nd::VHACD::Vect3<double> minP(double(1.0e15f));
			nd::VHACD::Vect3<double> maxP(-double(1.0e15f));
			if (count <= VHACD_CONVEXHULL_3D_VERTEX_CLUSTER_SIZE)
			{
				ConvexHull3dPointCluster* const clump = new (*memoryPool) ConvexHull3dPointCluster();
				*memoryPool += sizeof(ConvexHull3dPointCluster);
				maxMemSize -= sizeof(ConvexHull3dPointCluster);
				assert(maxMemSize >= 0);

				assert(clump);
				clump->m_count = count;
				for (int i = 0; i < count; ++i)
				{
					clump->m_indices[i] = i + baseIndex;

					const nd::VHACD::Vect3<double>& p = points[i];
					minP = minP.CWiseMin(p);
					maxP = maxP.CWiseMax(p);
				}

				clump->m_left = nullptr;
				clump->m_right = nullptr;
				tree = clump;
			}
			else
			{
				nd::VHACD::Vect3<double> median(0);
				nd::VHACD::Vect3<double> varian(0);
				for (int i = 0; i < count; ++i)
				{
					const nd::VHACD::Vect3<double>& p = points[i];
					minP = minP.CWiseMin(p);
					maxP = maxP.CWiseMax(p);
					median += p;
					varian += p.CWiseMul(p);
				}

				varian = varian * double(count) - median.CWiseMul(median);
				int index = 0;
				double maxVarian = double(-1.0e10f);
				for (int i = 0; i < 3; ++i)
				{
					if (varian[i] > maxVarian)
					{
						index = i;
						maxVarian = varian[i];
					}
				}
				nd::VHACD::Vect3<double> center(median * (1.0 / double(count)));

				double test = center[index];

				int i0 = 0;
				int i1 = count - 1;
				do
				{
					for (; i0 <= i1; i0++)
					{
						double val = points[i0][index];
						if (val > test)
						{
							break;
						}
					}

					for (; i1 >= i0; i1--)
					{
						double val = points[i1][index];
						if (val < test)
						{
							break;
						}
					}

					if (i0 < i1)
					{
						Swap(points[i0], points[i1]);
						i0++;
						i1--;
					}
				} while (i0 <= i1);

				if (i0 == 0)
				{
					i0 = count / 2;
				}
				if (i0 >= (count - 1))
				{
					i0 = count / 2;
				}

				tree = new (*memoryPool) ConvexHullAABBTreeNode();
				*memoryPool += sizeof(ConvexHullAABBTreeNode);
				maxMemSize -= sizeof(ConvexHullAABBTreeNode);
				assert(maxMemSize >= 0);

				assert(i0);
				assert(count - i0);

				tree->m_left = BuildTreeRecurse(tree, points, i0, baseIndex, memoryPool, maxMemSize);
				tree->m_right = BuildTreeRecurse(tree, &points[i0], count - i0, i0 + baseIndex, memoryPool, maxMemSize);
			}

			assert(tree);
			tree->m_parent = parent;
			tree->m_box[0] = minP - nd::VHACD::Vect3<double>(double(1.0e-3f));
			tree->m_box[1] = maxP + nd::VHACD::Vect3<double>(double(1.0e-3f));
			return tree;
		}

        ConvexHullAABBTreeNode* ConvexHull::BuildTreeOld(std::vector<ConvexHullVertex>& points,
                                                         char** const memoryPool,
                                                         int& maxMemSize)
		{
			GetUniquePoints(points);
			int count = int(points.size());
			if (count < 4)
			{
				return nullptr;
			}
			return BuildTreeRecurse(nullptr, &points[0], count, 0, memoryPool, maxMemSize);
		}

        ConvexHullAABBTreeNode* ConvexHull::BuildTreeNew(std::vector<ConvexHullVertex>& points,
                                                         char** const memoryPool,
                                                         int& maxMemSize) const
		{
			class dCluster
			{
				public:
				nd::VHACD::Vect3<double> m_sum;
				nd::VHACD::Vect3<double> m_sum2;
				int m_start;
				int m_count;
			};

			dCluster firstCluster;
			firstCluster.m_start = 0;
			firstCluster.m_count = int (points.size());
			firstCluster.m_sum = nd::VHACD::Vect3<double>(0);
			firstCluster.m_sum2 = nd::VHACD::Vect3<double>(0);

			for (int i = 0; i < firstCluster.m_count; ++i)
			{
				const nd::VHACD::Vect3<double>& p = points[i];
				firstCluster.m_sum += p;
				firstCluster.m_sum2 += p.CWiseMul(p);
			}

			int baseCount = 0;
			const int clusterSize = 16;

			if (firstCluster.m_count > clusterSize)
			{
				dCluster spliteStack[128];
				spliteStack[0] = firstCluster;
				int stack = 1;

				while (stack)
				{
					stack--;
					dCluster cluster (spliteStack[stack]);

					const nd::VHACD::Vect3<double> origin(cluster.m_sum * (1.0 / cluster.m_count));
					const nd::VHACD::Vect3<double> variance2(cluster.m_sum2 * (1.0 / cluster.m_count) - origin.CWiseMul(origin));
					double maxVariance2 = Max(Max(variance2.getX(), variance2.getY()), variance2.getZ());

					if (   (cluster.m_count <= clusterSize)
                        || (stack > (sizeof(spliteStack) / sizeof(spliteStack[0]) - 4))
                        || (maxVariance2 < 1.e-4f))
					{
						// no sure if this is beneficial, 
						// the array is so small that seem too much overhead
						//int maxIndex = 0;
						//double min_x = 1.0e20f;
						//for (int i = 0; i < cluster.m_count; ++i)
						//{
						//	if (points[cluster.m_start + i].getX() < min_x)
						//	{
						//		maxIndex = i;
						//		min_x = points[cluster.m_start + i].getX();
						//	}
						//}
						//Swap(points[cluster.m_start], points[cluster.m_start + maxIndex]);
						//
						//for (int i = 2; i < cluster.m_count; ++i)
						//{
						//	int j = i;
						//	ConvexHullVertex tmp(points[cluster.m_start + i]);
						//	for (; points[cluster.m_start + j - 1].getX() > tmp.getX(); --j)
						//	{
						//		assert(j > 0);
						//		points[cluster.m_start + j] = points[cluster.m_start + j - 1];
						//	}
						//	points[cluster.m_start + j] = tmp;
						//}

						int count = cluster.m_count;
						for (int i = cluster.m_count - 1; i > 0; --i)
						{
							for (int j = i - 1; j >= 0; --j)
							{
								nd::VHACD::Vect3<double> error(points[cluster.m_start + j] - points[cluster.m_start + i]);
								double mag2 = error.Dot(error);
								if (mag2 < 1.0e-6)
								{
									points[cluster.m_start + j] = points[cluster.m_start + i];
									count--;
									break;
								}
							}
						}

						assert(baseCount <= cluster.m_start);
						for (int i = 0; i < count; ++i)
						{
							points[baseCount] = points[cluster.m_start + i];
							baseCount++;
						}
					}
					else
					{
						int firstSortAxis = 0;
						if ((variance2.getY() >= variance2.getX()) && (variance2.getY() >= variance2.getZ()))
						{
							firstSortAxis = 1;
						}
						else if ((variance2.getZ() >= variance2.getX()) && (variance2.getZ() >= variance2.getY()))
						{
							firstSortAxis = 2;
						}
						double axisVal = origin[firstSortAxis];

						int i0 = 0;
						int i1 = cluster.m_count - 1;

						const int start = cluster.m_start;
						while (i0 < i1)
						{
							while ((points[start + i0][firstSortAxis] <= axisVal) && (i0 < i1))
							{
								++i0;
							};

							while ((points[start + i1][firstSortAxis] > axisVal) && (i0 < i1))
							{
								--i1;
							}

							assert(i0 <= i1);
							if (i0 < i1)
							{
								Swap(points[start + i0], points[start + i1]);
								++i0;
								--i1;
							}
						}

						while ((points[start + i0][firstSortAxis] <= axisVal) && (i0 < cluster.m_count))
						{
							++i0;
						};

						#ifdef _DEBUG
						for (int i = 0; i < i0; ++i)
						{
							assert(points[start + i][firstSortAxis] <= axisVal);
						}

						for (int i = i0; i < cluster.m_count; ++i)
						{
							assert(points[start + i][firstSortAxis] > axisVal);
						}
						#endif

						nd::VHACD::Vect3<double> xc(0);
						nd::VHACD::Vect3<double> x2c(0);
						for (int i = 0; i < i0; ++i)
						{
							const nd::VHACD::Vect3<double>& x = points[start + i];
							xc += x;
							x2c += x.CWiseMul(x);
						}

						dCluster cluster_i1(cluster);
						cluster_i1.m_start = start + i0;
						cluster_i1.m_count = cluster.m_count - i0;
						cluster_i1.m_sum -= xc;
						cluster_i1.m_sum2 -= x2c;
						spliteStack[stack] = cluster_i1;
						assert(cluster_i1.m_count > 0);
						stack++;

						dCluster cluster_i0(cluster);
						cluster_i0.m_start = start;
						cluster_i0.m_count = i0;
						cluster_i0.m_sum = xc;
						cluster_i0.m_sum2 = x2c;
						assert(cluster_i0.m_count > 0);
						spliteStack[stack] = cluster_i0;
						stack++;
					}
				}
			}

			points.resize(baseCount);
			if (baseCount < 4)
			{
				return nullptr;
			}

			nd::VHACD::Vect3<double> sum(0);
			nd::VHACD::Vect3<double> sum2(0);
			nd::VHACD::Vect3<double> minP(1.0e15);
			nd::VHACD::Vect3<double> maxP(-1.0e15);
			class dTreeBox
			{
				public:
				nd::VHACD::Vect3<double> m_min;
				nd::VHACD::Vect3<double> m_max;
				nd::VHACD::Vect3<double> m_sum;
				nd::VHACD::Vect3<double> m_sum2;
				ConvexHullAABBTreeNode* m_parent;
				ConvexHullAABBTreeNode** m_child;
				int m_start;
				int m_count;
			};

			for (int i = 0; i < baseCount; ++i)
			{
				const nd::VHACD::Vect3<double>& p = points[i];
				sum += p;
				sum2 += p.CWiseMul(p);
				minP = minP.CWiseMin(p);
				maxP = maxP.CWiseMax(p);
			}
	
			dTreeBox treeBoxStack[128];
			treeBoxStack[0].m_start = 0;
			treeBoxStack[0].m_count = baseCount;
			treeBoxStack[0].m_sum = sum;
			treeBoxStack[0].m_sum2 = sum2;
			treeBoxStack[0].m_min = minP;
			treeBoxStack[0].m_max = maxP;
			treeBoxStack[0].m_child = nullptr;
			treeBoxStack[0].m_parent = nullptr;

			int stack = 1;
			ConvexHullAABBTreeNode* root = nullptr;
			while (stack)
			{
				stack--;
				dTreeBox box (treeBoxStack[stack]);
				if (box.m_count <= VHACD_CONVEXHULL_3D_VERTEX_CLUSTER_SIZE)
				{
					ConvexHull3dPointCluster* const clump = new (*memoryPool) ConvexHull3dPointCluster(box.m_parent);
					*memoryPool += sizeof(ConvexHull3dPointCluster);
					maxMemSize -= sizeof(ConvexHull3dPointCluster);
					assert(maxMemSize >= 0);
		
					assert(clump);
					clump->m_count = box.m_count;
					for (int i = 0; i < box.m_count; ++i)
					{
						clump->m_indices[i] = i + box.m_start;
					}
					clump->m_box[0] = box.m_min;
					clump->m_box[1] = box.m_max;

					if (box.m_child)
					{
						*box.m_child = clump;
					}

					if (!root)
					{
						root = clump;
					}
				}
				else
				{
					const nd::VHACD::Vect3<double> origin(box.m_sum * (1.0 / box.m_count));
					const nd::VHACD::Vect3<double> variance2(box.m_sum2 * (1.0 / box.m_count) - origin.CWiseMul(origin));

					int firstSortAxis = 0;
					if ((variance2.getY() >= variance2.getX()) && (variance2.getY() >= variance2.getZ()))
					{
						firstSortAxis = 1;
					}
					else if ((variance2.getZ() >= variance2.getX()) && (variance2.getZ() >= variance2.getY()))
					{
						firstSortAxis = 2;
					}
					double axisVal = origin[firstSortAxis];

					int i0 = 0;
					int i1 = box.m_count - 1;

					const int start = box.m_start;
					while (i0 < i1)
					{
						while ((points[start + i0][firstSortAxis] <= axisVal) && (i0 < i1))
						{
							++i0;
						};

						while ((points[start + i1][firstSortAxis] > axisVal) && (i0 < i1))
						{
							--i1;
						}

						assert(i0 <= i1);
						if (i0 < i1)
						{
							Swap(points[start + i0], points[start + i1]);
							++i0;
							--i1;
						}
					}

					while ((points[start + i0][firstSortAxis] <= axisVal) && (i0 < box.m_count))
					{
						++i0;
					};

					#ifdef _DEBUG
					for (int i = 0; i < i0; ++i)
					{
						assert(points[start + i][firstSortAxis] <= axisVal);
					}

					for (int i = i0; i < box.m_count; ++i)
					{
						assert(points[start + i][firstSortAxis] > axisVal);
					}
					#endif

					ConvexHullAABBTreeNode* const node = new (*memoryPool) ConvexHullAABBTreeNode(box.m_parent);
					*memoryPool += sizeof(ConvexHullAABBTreeNode);
					maxMemSize -= sizeof(ConvexHullAABBTreeNode);
					assert(maxMemSize >= 0);

					node->m_box[0] = box.m_min;
					node->m_box[1] = box.m_max;
					if (box.m_child)
					{
						*box.m_child = node;
					}

					if (!root)
					{
						root = node;
					}

					{
						nd::VHACD::Vect3<double> xc(0);
						nd::VHACD::Vect3<double> x2c(0);
						nd::VHACD::Vect3<double> p0(1.0e15);
						nd::VHACD::Vect3<double> p1(-1.0e15);
						for (int i = i0; i < box.m_count; ++i)
						{
							const nd::VHACD::Vect3<double>& p = points[start + i];
							xc += p;
							x2c += p.CWiseMul(p);
							p0 = p0.CWiseMin(p);
							p1 = p1.CWiseMax(p);
						}

						dTreeBox cluster_i1(box);
						cluster_i1.m_start = start + i0;
						cluster_i1.m_count = box.m_count - i0;
						cluster_i1.m_sum = xc;
						cluster_i1.m_sum2 = x2c;
						cluster_i1.m_min = p0;
						cluster_i1.m_max = p1;
						cluster_i1.m_parent = node;
						cluster_i1.m_child = &node->m_right;
						treeBoxStack[stack] = cluster_i1;
						assert(cluster_i1.m_count > 0);
						stack++;
					}

					{
						nd::VHACD::Vect3<double> xc(0);
						nd::VHACD::Vect3<double> x2c(0);
						nd::VHACD::Vect3<double> p0(1.0e15);
						nd::VHACD::Vect3<double> p1(-1.0e15);
						for (int i = 0; i < i0; ++i)
						{
							const nd::VHACD::Vect3<double>& p = points[start + i];
							xc += p;
							x2c += p.CWiseMul(p);
							p0 = p0.CWiseMin(p);
							p1 = p1.CWiseMax(p);
						}

						dTreeBox cluster_i0(box);
						cluster_i0.m_start = start;
						cluster_i0.m_count = i0;
						cluster_i0.m_min = p0;
						cluster_i0.m_max = p1;
						cluster_i0.m_sum = xc;
						cluster_i0.m_sum2 = x2c;
						cluster_i0.m_parent = node;
						cluster_i0.m_child = &node->m_left;
						assert(cluster_i0.m_count > 0);
						treeBoxStack[stack] = cluster_i0;
						stack++;
					}
				}
			}
	
			return root;
		}

        int ConvexHull::SupportVertex(ConvexHullAABBTreeNode** const treePointer,
                                      const std::vector<ConvexHullVertex>& points,
                                      const nd::VHACD::Vect3<double>& dirPlane,
                                      const bool removeEntry) const
		{
		#define VHACD_STACK_DEPTH_3D 64
			double aabbProjection[VHACD_STACK_DEPTH_3D];
			const ConvexHullAABBTreeNode *stackPool[VHACD_STACK_DEPTH_3D];

			nd::VHACD::Vect3<double> dir(dirPlane);

			int index = -1;
			int stack = 1;
			stackPool[0] = *treePointer;
			aabbProjection[0] = double(1.0e20f);
			double maxProj = double(-1.0e20f);
			int ix = (dir[0] > double(0.0f)) ? 1 : 0;
			int iy = (dir[1] > double(0.0f)) ? 1 : 0;
			int iz = (dir[2] > double(0.0f)) ? 1 : 0;
			while (stack)
			{
				stack--;
				double boxSupportValue = aabbProjection[stack];
				if (boxSupportValue > maxProj)
				{
					const ConvexHullAABBTreeNode* const me = stackPool[stack];

					if (me->m_left && me->m_right)
					{
						const nd::VHACD::Vect3<double> leftSupportPoint(me->m_left->m_box[ix].getX(),
                                                          me->m_left->m_box[iy].getY(),
                                                          me->m_left->m_box[iz].getZ());
						double leftSupportDist = leftSupportPoint.Dot(dir);

						const nd::VHACD::Vect3<double> rightSupportPoint(me->m_right->m_box[ix].getX(),
                                                           me->m_right->m_box[iy].getY(),
                                                           me->m_right->m_box[iz].getZ());
						double rightSupportDist = rightSupportPoint.Dot(dir);

						if (rightSupportDist >= leftSupportDist)
						{
							aabbProjection[stack] = leftSupportDist;
							stackPool[stack] = me->m_left;
							stack++;
							assert(stack < VHACD_STACK_DEPTH_3D);
							aabbProjection[stack] = rightSupportDist;
							stackPool[stack] = me->m_right;
							stack++;
							assert(stack < VHACD_STACK_DEPTH_3D);
						}
						else
						{
							aabbProjection[stack] = rightSupportDist;
							stackPool[stack] = me->m_right;
							stack++;
							assert(stack < VHACD_STACK_DEPTH_3D);
							aabbProjection[stack] = leftSupportDist;
							stackPool[stack] = me->m_left;
							stack++;
							assert(stack < VHACD_STACK_DEPTH_3D);
						}
					}
					else
					{
						ConvexHull3dPointCluster* const cluster = (ConvexHull3dPointCluster*)me;
						for (int i = 0; i < cluster->m_count; ++i)
						{
							const ConvexHullVertex& p = points[cluster->m_indices[i]];
							assert(p.getX() >= cluster->m_box[0].getX());
							assert(p.getX() <= cluster->m_box[1].getX());
							assert(p.getY() >= cluster->m_box[0].getY());
							assert(p.getY() <= cluster->m_box[1].getY());
							assert(p.getZ() >= cluster->m_box[0].getZ());
							assert(p.getZ() <= cluster->m_box[1].getZ());
							if (!p.m_mark)
							{
								//assert(p.m_w == double(0.0f));
								double dist = p.Dot(dir);
								if (dist > maxProj)
								{
									maxProj = dist;
									index = cluster->m_indices[i];
								}
							}
							else if (removeEntry)
							{
								cluster->m_indices[i] = cluster->m_indices[cluster->m_count - 1];
								cluster->m_count = cluster->m_count - 1;
								i--;
							}
						}

						if (cluster->m_count == 0)
						{
							ConvexHullAABBTreeNode* const parent = cluster->m_parent;
							if (parent)
							{
								ConvexHullAABBTreeNode* const sibling = (parent->m_left != cluster) ? parent->m_left : parent->m_right;
								assert(sibling != cluster);
								ConvexHullAABBTreeNode* const grandParent = parent->m_parent;
								if (grandParent)
								{
									sibling->m_parent = grandParent;
									if (grandParent->m_right == parent)
									{
										grandParent->m_right = sibling;
									}
									else
									{
										grandParent->m_left = sibling;
									}
								}
								else
								{
									sibling->m_parent = nullptr;
									*treePointer = sibling;
								}
							}
						}
					}
				}
			}

			assert(index != -1);
			return index;
		}

        double ConvexHull::TetrahedrumVolume(const nd::VHACD::Vect3<double>& p0,
                                             const nd::VHACD::Vect3<double>& p1,
                                             const nd::VHACD::Vect3<double>& p2,
                                             const nd::VHACD::Vect3<double>& p3) const
		{
			const nd::VHACD::Vect3<double> p1p0(p1 - p0);
			const nd::VHACD::Vect3<double> p2p0(p2 - p0);
			const nd::VHACD::Vect3<double> p3p0(p3 - p0);
			return p3p0.Dot(p1p0.Cross(p2p0));
		}

        int ConvexHull::InitVertexArray(std::vector<ConvexHullVertex>& points,
                                        void* const memoryPool,
                                        int maxMemSize)
		{
		#if 1
			ConvexHullAABBTreeNode* tree = BuildTreeOld(points, (char**)&memoryPool, maxMemSize);
		#else
			ConvexHullAABBTreeNode* tree = BuildTreeNew(points, (char**)&memoryPool, maxMemSize);
		#endif
			int count = int(points.size());
			if (count < 4)
			{
				m_points.resize(0);
				return 0;
			}
		
			m_points.resize(count);
			m_aabbP0 = tree->m_box[0];
			m_aabbP1 = tree->m_box[1];
	
			nd::VHACD::Vect3<double> boxSize(tree->m_box[1] - tree->m_box[0]);
            m_diag = boxSize.GetNorm();
			const ndNormalMap& normalMap = ndNormalMap::GetNormaMap();
	
            int index0 = SupportVertex(&tree,
                                       points,
                                       normalMap.m_normal[0]);
			m_points[0] = points[index0];
			points[index0].m_mark = 1;
	
			bool validTetrahedrum = false;
			nd::VHACD::Vect3<double> e1(0.0);
			for (int i = 1; i < normalMap.m_count; ++i)
			{
                int index = SupportVertex(&tree,
                                          points,
                                          normalMap.m_normal[i]);
				assert(index >= 0);
	
				e1 = points[index] - m_points[0];
				double error2 = e1.GetNormSquared();
				if (error2 > (double(1.0e-4f) * m_diag * m_diag))
				{
					m_points[1] = points[index];
					points[index].m_mark = 1;
					validTetrahedrum = true;
					break;
				}
			}
			if (!validTetrahedrum)
			{
				m_points.resize(0);
				assert(0);
				return count;
			}
	
			validTetrahedrum = false;
			nd::VHACD::Vect3<double> e2(0.0);
			nd::VHACD::Vect3<double> normal(0.0);
			for (int i = 2; i < normalMap.m_count; ++i)
			{
                int index = SupportVertex(&tree,
                                          points,
                                          normalMap.m_normal[i]);
				assert(index >= 0);
				e2 = points[index] - m_points[0];
				normal = e1.Cross(e2);
				double error2 = normal.GetNorm();
				if (error2 > (double(1.0e-4f) * m_diag * m_diag))
				{
					m_points[2] = points[index];
					points[index].m_mark = 1;
					validTetrahedrum = true;
					break;
				}
			}
	
			if (!validTetrahedrum)
			{
				m_points.resize(0);
				assert(0);
				return count;
			}
	
			// find the largest possible tetrahedron
			validTetrahedrum = false;
			nd::VHACD::Vect3<double> e3(0.0);
	
			index0 = SupportVertex(&tree, points, normal);
			e3 = points[index0] - m_points[0];
			double err2 = normal.Dot(e3);
			if (fabs(err2) > (double(1.0e-6f) * m_diag * m_diag))
			{
				// we found a valid tetrahedral, about and start build the hull by adding the rest of the points
				m_points[3] = points[index0];
				points[index0].m_mark = 1;
				validTetrahedrum = true;
			}
			if (!validTetrahedrum)
			{
				nd::VHACD::Vect3<double> n(-normal);
                int index = SupportVertex(&tree,
                                          points,
                                          n);
				e3 = points[index] - m_points[0];
				double error2 = normal.Dot(e3);
				if (fabs(error2) > (double(1.0e-6f) * m_diag * m_diag))
				{
					// we found a valid tetrahedral, about and start build the hull by adding the rest of the points
					m_points[3] = points[index];
					points[index].m_mark = 1;
					validTetrahedrum = true;
				}
			}
			if (!validTetrahedrum)
			{
				for (int i = 3; i < normalMap.m_count; ++i)
				{
                    int index = SupportVertex(&tree,
                                              points,
                                              normalMap.m_normal[i]);
					assert(index >= 0);
	
					//make sure the volume of the fist tetrahedral is no negative
					e3 = points[index] - m_points[0];
					double error2 = normal.Dot(e3);
					if (fabs(error2) > (double(1.0e-6f) * m_diag * m_diag))
					{
						// we found a valid tetrahedral, about and start build the hull by adding the rest of the points
						m_points[3] = points[index];
						points[index].m_mark = 1;
						validTetrahedrum = true;
						break;
					}
				}
			}
			if (!validTetrahedrum)
			{
				// the points do not form a convex hull
				m_points.resize(0);
				return count;
			}
	
			m_points.resize(4);
			double volume = TetrahedrumVolume(m_points[0], m_points[1], m_points[2], m_points[3]);
			if (volume > double(0.0f))
			{
				Swap(m_points[2], m_points[3]);
			}
			assert(TetrahedrumVolume(m_points[0], m_points[1], m_points[2], m_points[3]) < double(0.0f));
			return count;
		}

		ConvexHull::ndNode* ConvexHull::AddFace(int i0, int i1, int i2)
		{
			ndNode* const node = Append();
			ConvexHullFace& face = node->GetInfo();

			face.m_index[0] = i0;
			face.m_index[1] = i1;
			face.m_index[2] = i2;
			return node;
		}

        void ConvexHull::CalculateConvexHull3d(ConvexHullAABBTreeNode* vertexTree,
                                               std::vector<ConvexHullVertex>& points,
                                               int count,
                                               double distTol,
                                               int maxVertexCount)
		{
			distTol = fabs(distTol) * m_diag;
			ndNode* const f0Node = AddFace(0, 1, 2);
			ndNode* const f1Node = AddFace(0, 2, 3);
			ndNode* const f2Node = AddFace(2, 1, 3);
			ndNode* const f3Node = AddFace(1, 0, 3);

			ConvexHullFace* const f0 = &f0Node->GetInfo();
			ConvexHullFace* const f1 = &f1Node->GetInfo();
			ConvexHullFace* const f2 = &f2Node->GetInfo();
			ConvexHullFace* const f3 = &f3Node->GetInfo();

			f0->m_twin[0] = f3Node;
			f0->m_twin[1] = f2Node;
			f0->m_twin[2] = f1Node;

			f1->m_twin[0] = f0Node;
			f1->m_twin[1] = f2Node;
			f1->m_twin[2] = f3Node;

			f2->m_twin[0] = f0Node;
			f2->m_twin[1] = f3Node;
			f2->m_twin[2] = f1Node;

			f3->m_twin[0] = f0Node;
			f3->m_twin[1] = f1Node;
			f3->m_twin[2] = f2Node;
	
			List<ndNode*> boundaryFaces;
			boundaryFaces.Append(f0Node);
			boundaryFaces.Append(f1Node);
			boundaryFaces.Append(f2Node);
			boundaryFaces.Append(f3Node);

			m_points.resize(count);

			count -= 4;
			maxVertexCount -= 4;
			int currentIndex = 4;

			std::vector<ndNode*> stackPool;
			std::vector<ndNode*> coneListPool;
			std::vector<ndNode*> deleteListPool;

			stackPool.resize(1024 + count);
			coneListPool.resize(1024 + count);
			deleteListPool.resize(1024 + count);

			ndNode** const stack = &stackPool[0];
			ndNode** const coneList = &stackPool[0];
			ndNode** const deleteList = &deleteListPool[0];

			while (boundaryFaces.GetCount() && count && (maxVertexCount > 0))
			{
				// my definition of the optimal convex hull of a given vertex count,
				// is the convex hull formed by a subset of the input vertex that minimizes the volume difference
				// between the perfect hull formed from all input vertex and the hull of the sub set of vertex.
				// When using a priority heap this algorithms will generate the an optimal of a fix vertex count.
				// Since all Newton's tools do not have a limit on the point count of a convex hull, I can use either a stack or a queue.
				// a stack maximize construction speed, a Queue tend to maximize the volume of the generated Hull approaching a perfect Hull.
				// For now we use a queue.
				// For general hulls it does not make a difference if we use a stack, queue, or a priority heap.
				// perfect optimal hull only apply for when build hull of a limited vertex count.
				//
				// Also when building Hulls of a limited vertex count, this function runs in constant time.
				// yes that is correct, it does not makes a difference if you build a N point hull from 100 vertex
				// or from 100000 vertex input array.

				// using a queue (some what slower by better hull when reduced vertex count is desired)
				bool isvalid;
				ndNode* const faceNode = boundaryFaces.GetLast()->GetInfo();
				ConvexHullFace* const face = &faceNode->GetInfo();
				hullPlane planeEquation(face->GetPlaneEquation(&m_points[0], isvalid));

				int index = 0;
				double dist = 0;
				nd::VHACD::Vect3<double> p;
				if (isvalid)
				{
					index = SupportVertex(&vertexTree, points, planeEquation);
					p = points[index];
					dist = planeEquation.Evalue(p);
				}

				if (isvalid && (dist >= distTol) && (face->Evalue(&m_points[0], p) > double(0.0f)))
				{
					stack[0] = faceNode;

					int stackIndex = 1;
					int deletedCount = 0;

					while (stackIndex)
					{
						stackIndex--;
						ndNode* const node1 = stack[stackIndex];
						ConvexHullFace* const face1 = &node1->GetInfo();
			
						if (!face1->m_mark && (face1->Evalue(&m_points[0], p) > double(0.0f)))
						{
							#ifdef _DEBUG
							for (int i = 0; i < deletedCount; ++i)
							{
								assert(deleteList[i] != node1);
							}
							#endif
			
							deleteList[deletedCount] = node1;
							deletedCount++;
							assert(deletedCount < int(deleteListPool.size()));
							face1->m_mark = 1;
							for (int i = 0; i < 3; ++i)
							{
								ndNode* const twinNode = face1->m_twin[i];
								assert(twinNode);
								ConvexHullFace* const twinFace = &twinNode->GetInfo();
								if (!twinFace->m_mark)
								{
									stack[stackIndex] = twinNode;
									stackIndex++;
									assert(stackIndex < int(stackPool.size()));
								}
							}
						}
					}
			
					m_points[currentIndex] = points[index];
					points[index].m_mark = 1;
			
					int newCount = 0;
					for (int i = 0; i < deletedCount; ++i)
					{
						ndNode* const node1 = deleteList[i];
						ConvexHullFace* const face1 = &node1->GetInfo();
						assert(face1->m_mark == 1);
						for (int j0 = 0; j0 < 3; j0++)
						{
							ndNode* const twinNode = face1->m_twin[j0];
							ConvexHullFace* const twinFace = &twinNode->GetInfo();
							if (!twinFace->m_mark)
							{
								int j1 = (j0 == 2) ? 0 : j0 + 1;
								ndNode* const newNode = AddFace(currentIndex, face1->m_index[j0], face1->m_index[j1]);
								boundaryFaces.Addtop(newNode);
			
								ConvexHullFace* const newFace = &newNode->GetInfo();
								newFace->m_twin[1] = twinNode;
								for (int k = 0; k < 3; k++)
								{
									if (twinFace->m_twin[k] == node1)
									{
										twinFace->m_twin[k] = newNode;
									}
								}
								coneList[newCount] = newNode;
								newCount++;
								assert(newCount < int(coneListPool.size()));
							}
						}
					}
			
					for (int i = 0; i < newCount - 1; ++i)
					{
						ndNode* const nodeA = coneList[i];
						ConvexHullFace* const faceA = &nodeA->GetInfo();
						assert(faceA->m_mark == 0);
						for (int j = i + 1; j < newCount; j++) 
						{
							ndNode* const nodeB = coneList[j];
							ConvexHullFace* const faceB = &nodeB->GetInfo();
							assert(faceB->m_mark == 0);
							if (faceA->m_index[2] == faceB->m_index[1])
							{
								faceA->m_twin[2] = nodeB;
								faceB->m_twin[0] = nodeA;
								break;
							}
						}
			
						for (int j = i + 1; j < newCount; j++)
						{
							ndNode* const nodeB = coneList[j];
							ConvexHullFace* const faceB = &nodeB->GetInfo();
							assert(faceB->m_mark == 0);
							if (faceA->m_index[1] == faceB->m_index[2])
							{
								faceA->m_twin[0] = nodeB;
								faceB->m_twin[2] = nodeA;
								break;
							}
						}
					}
			
					for (int i = 0; i < deletedCount; ++i)
					{
						ndNode* const node = deleteList[i];
						boundaryFaces.Remove(node);
						Remove(node);
					}

					maxVertexCount--;
					currentIndex++;
					count--;
				}
				else
				{
					boundaryFaces.Remove(faceNode);
				}
			}
			m_points.resize(currentIndex);
		}
	}
}

//***********************************************************************************************
// End of ConvexHull generation code by Julio Jerez <jerezjulio0@gmail.com>
//***********************************************************************************************

// VertexIndex support
namespace VERTEX_INDEX
{

class KdTreeNode;

typedef std::vector<KdTreeNode*> KdTreeNodeVector;

enum Axes
{
    X_AXIS = 0,
    Y_AXIS = 1,
    Z_AXIS = 2
};

class KdTreeFindNode
{
public:
    KdTreeFindNode(void)
    {
        mNode = 0;
        mDistance = 0;
    }
    KdTreeNode* mNode;
    double mDistance;
};

class KdTreeNode;
class KdTreeNodeBundle;

class KdTree
{
public:
    KdTree() = default;

    ~KdTree() { reset(); }

    const VHACD::Vertex& getPosition(uint32_t index) const;

    uint32_t search(const nd::VHACD::Vect3<double>& pos,
                    double radius,
                    uint32_t maxObjects,
                    KdTreeFindNode* found) const;

    void reset();

    uint32_t add(const VHACD::Vertex& v);

    KdTreeNode* getNewNode(uint32_t index);

    uint32_t getNearest(const nd::VHACD::Vect3<double>& pos,
                        double radius,
                        bool& _found) const; // returns the nearest possible neighbor's index.

    const std::vector<VHACD::Vertex>& getVertices() const;
    std::vector<VHACD::Vertex>&& takeVertices();

    uint32_t getVcount(void) const;

private:
    KdTreeNode* mRoot = nullptr;
    KdTreeNodeBundle* mBundle = nullptr;
    KdTreeNodeBundle* mBundleHead = nullptr;

    std::vector<VHACD::Vertex> mVertices;
};

class KdTreeNode
{
public:
    KdTreeNode() = default;

    KdTreeNode(uint32_t index) : mIndex(index) {}

    ~KdTreeNode() = default;

    void add(KdTreeNode* node,
             Axes dim,
             const KdTree* iface)
    {
        Axes axis = X_AXIS;
        uint32_t idx = 0;
        switch (dim)
        {
        case X_AXIS:
            idx = 0;
            axis = Y_AXIS;
            break;
        case Y_AXIS:
            idx = 1;
            axis = Z_AXIS;
            break;
        case Z_AXIS:
            idx = 2;
            axis = X_AXIS;
            break;
        }

        const VHACD::Vertex& nodePosition = iface->getPosition(node->mIndex);
        const VHACD::Vertex& position = iface->getPosition(mIndex);
        if (nodePosition[idx] <= position[idx])
        {
            if (mLeft)
                mLeft->add(node, axis, iface);
            else
                mLeft = node;
        }
        else
        {
            if (mRight)
                mRight->add(node, axis, iface);
            else
                mRight = node;
        }
    }

    uint32_t getIndex() const
    {
        return mIndex;
    };

    void search(Axes axis,
                const nd::VHACD::Vect3<double>& pos,
                double radius,
                uint32_t& count,
                uint32_t maxObjects,
                KdTreeFindNode* found,
                const KdTree* iface)
    {
        const nd::VHACD::Vect3<double> position = iface->getPosition(mIndex);

        const nd::VHACD::Vect3<double> d = pos - position;

        KdTreeNode* search1 = 0;
        KdTreeNode* search2 = 0;

        uint32_t idx = 0;
        switch (axis)
        {
        case X_AXIS:
            idx = 0;
            axis = Y_AXIS;
            break;
        case Y_AXIS:
            idx = 1;
            axis = Z_AXIS;
            break;
        case Z_AXIS:
            idx = 2;
            axis = X_AXIS;
            break;
        }

        if (d[idx] <= 0) // JWR  if we are to the left
        {
            search1 = mLeft; // JWR  then search to the left
            if (-d[idx] < radius) // JWR  if distance to the right is less than our search radius, continue on the right
                                // as well.
                search2 = mRight;
        }
        else
        {
            search1 = mRight; // JWR  ok, we go down the left tree
            if (d[idx] < radius) // JWR  if the distance from the right is less than our search radius
                search2 = mLeft;
        }

        double r2 = radius * radius;
        double m = d.GetNormSquared();

        if (m < r2)
        {
            switch (count)
            {
            case 0:
                found[count].mNode = this;
                found[count].mDistance = m;
                break;
            case 1:
                if (m < found[0].mDistance)
                {
                    if (maxObjects == 1)
                    {
                        found[0].mNode = this;
                        found[0].mDistance = m;
                    }
                    else
                    {
                        found[1] = found[0];
                        found[0].mNode = this;
                        found[0].mDistance = m;
                    }
                }
                else if (maxObjects > 1)
                {
                    found[1].mNode = this;
                    found[1].mDistance = m;
                }
                break;
            default:
            {
                bool inserted = false;

                for (uint32_t i = 0; i < count; i++)
                {
                    if (m < found[i].mDistance) // if this one is closer than a pre-existing one...
                    {
                        // insertion sort...
                        uint32_t scan = count;
                        if (scan >= maxObjects)
                            scan = maxObjects - 1;
                        for (uint32_t j = scan; j > i; j--)
                        {
                            found[j] = found[j - 1];
                        }
                        found[i].mNode = this;
                        found[i].mDistance = m;
                        inserted = true;
                        break;
                    }
                }

                if (!inserted && count < maxObjects)
                {
                    found[count].mNode = this;
                    found[count].mDistance = m;
                }
            }
            break;
            }
            count++;
            if (count > maxObjects)
            {
                count = maxObjects;
            }
        }


        if (search1)
            search1->search(axis, pos, radius, count, maxObjects, found, iface);

        if (search2)
            search2->search(axis, pos, radius, count, maxObjects, found, iface);
    }

private:
    void setLeft(KdTreeNode* left)
    {
        mLeft = left;
    };
    void setRight(KdTreeNode* right)
    {
        mRight = right;
    };

    KdTreeNode* getLeft(void)
    {
        return mLeft;
    }
    KdTreeNode* getRight(void)
    {
        return mRight;
    }

    uint32_t mIndex = 0;
    KdTreeNode* mLeft = nullptr;
    KdTreeNode* mRight = nullptr;
};


#    define MAX_BUNDLE_SIZE                                                                                            \
        1024 // 1024 nodes at a time, to minimize memory allocation and guarantee that pointers are persistent.

class KdTreeNodeBundle
{
public:
    KdTreeNodeBundle() = default;

    bool isFull() const
    {
        return mIndex == MAX_BUNDLE_SIZE;
    }

    KdTreeNode* getNextNode()
    {
        assert(mIndex < MAX_BUNDLE_SIZE);
        KdTreeNode* ret = &mNodes[mIndex];
        mIndex++;
        return ret;
    }

    KdTreeNodeBundle* mNext = nullptr;
    uint32_t mIndex = 0;
    KdTreeNode mNodes[MAX_BUNDLE_SIZE];
};

const VHACD::Vertex& KdTree::getPosition(uint32_t index) const
{
    assert(index < mVertices.size());
    return mVertices[index];
}

uint32_t KdTree::search(const nd::VHACD::Vect3<double>& pos,
                        double radius,
                        uint32_t maxObjects,
                        KdTreeFindNode* found) const
{
    if (!mRoot)
        return 0;
    uint32_t count = 0;
    mRoot->search(X_AXIS, pos, radius, count, maxObjects, found, this);
    return count;
}

void KdTree::reset()
{
    mRoot = nullptr;
    mVertices.clear();
    KdTreeNodeBundle* bundle = mBundleHead;
    while (bundle)
    {
        KdTreeNodeBundle* next = bundle->mNext;
        delete bundle;
        bundle = next;
    }
    mBundle = nullptr;
    mBundleHead = nullptr;
}

uint32_t KdTree::add(const VHACD::Vertex& v)
{
    uint32_t ret = uint32_t(mVertices.size());
    mVertices.emplace_back(v);
    KdTreeNode* node = getNewNode(ret);
    if (mRoot)
    {
        mRoot->add(node,
                   X_AXIS,
                   this);
    }
    else
    {
        mRoot = node;
    }
    return ret;
}

KdTreeNode* KdTree::getNewNode(uint32_t index)
{
    if (mBundle == 0)
    {
        mBundle = new KdTreeNodeBundle;
        mBundleHead = mBundle;
    }
    if (mBundle->isFull())
    {
        KdTreeNodeBundle* bundle = new KdTreeNodeBundle;
        mBundle->mNext = bundle;
        mBundle = bundle;
    }
    KdTreeNode* node = mBundle->getNextNode();
    new (node) KdTreeNode(index);
    return node;
}

uint32_t KdTree::getNearest(const nd::VHACD::Vect3<double>& pos,
                            double radius,
                            bool& _found) const // returns the nearest possible neighbor's index.
{
    uint32_t ret = 0;

    _found = false;
    KdTreeFindNode found[1];
    uint32_t count = search(pos, radius, 1, found);
    if (count)
    {
        KdTreeNode* node = found[0].mNode;
        ret = node->getIndex();
        _found = true;
    }
    return ret;
}

const std::vector<VHACD::Vertex>& KdTree::getVertices() const
{
    return mVertices;
}

std::vector<VHACD::Vertex>&& KdTree::takeVertices()
{
    return std::move(mVertices);
}

uint32_t KdTree::getVcount() const
{
    return uint32_t(mVertices.size());
}

}; // end of namespace VERTEX_INDEX

//********************************************************************************************************************
// Prototypes for the handful of float math routines we use
//********************************************************************************************************************
namespace VHACD
{

// Compute centroid of a triangle mesh; takes area of each triangle into account weighted average
// bool fm_computeCentroid(uint32_t vcount, // number of input data points
//                         const VHACD::Vertex* points, // starting address of points array.
//                         uint32_t triangleCount,
//                         const VHACD::Triangle* indices,
//                         double* center);

// double fm_computeMeshVolume(const VHACD::Vertex* vertices,
//                             uint32_t tcount,
//                             const VHACD::Triangle* indices);

// void fm_inflateMinMax(double bmin[3],
//                       double bmax[3],
//                       double ratio);
// void fm_getAABB(uint32_t vcount,
//                 const double* points,
//                 uint32_t pstride,
//                 double bmin[3],
//                 double bmax[3]);
// bool fm_intersectAABB(const double bmin1[3],
//                       const double bmax1[3],
//                       const double bmin2[3],
//                       const double bmax2[3]);
// void fm_combineAABB(const double bmin1[3],
//                     const double bmax1[3],
//                     const double bmin2[3],
//                     const double bmax2[3],
//                     double bmin[3],
//                     double bmax[3]);
// double fm_volumeAABB(const double bmin[3],
//                      const double bmax[3]);

//********************************************************************************************************************
// Implementation of the handful of FloatMath methods we actually use
//********************************************************************************************************************

static double triangle_area_3d(const nd::VHACD::Vect3<double>& p1,
                               const nd::VHACD::Vect3<double>& p2,
                               const nd::VHACD::Vect3<double>& p3)

/**********************************************************************/

/*
Purpose:

TRIANGLE_AREA_3D computes the area of a triangle in 3D.

Modified:

22 April 1999

Author:

John Burkardt

Parameters:

Input, double X1, Y1, Z1, X2, Y2, Z2, X3, Y3, Z3, the (getX,getY,getZ)
coordinates of the corners of the triangle.

Output, double TRIANGLE_AREA_3D, the area of the triangle.
*/
{
    /*
    Find the projection of (P3-P1) onto (P2-P1).
    */
    double base = (p2 - p1).GetNorm();
    /*
    The height of the triangle is the length of (P3-P1) after its
    projection onto (P2-P1) has been subtracted.
    */
    double height;
    if (base == 0.0)
    {

        height = 0.0;
    }
    else
    {
        double dot = (p3 - p1).Dot(p2 - p1);
        double alpha = dot / (base * base);

        double a = p3.getX() - p1.getX() - alpha * (p2.getX() - p1.getX());
        double b = p3.getY() - p1.getY() - alpha * (p2.getY() - p1.getY());
        double c = p3.getZ() - p1.getZ() - alpha * (p2.getZ() - p1.getZ());

        height = std::sqrt(a * a + b * b + c * c);
    }

    return 0.5f * base * height;
}

double fm_computeArea(const nd::VHACD::Vect3<double>& p1,
                      const nd::VHACD::Vect3<double>& p2,
                      const nd::VHACD::Vect3<double>& p3)
{
    return triangle_area_3d(p1,
                            p2,
                            p3);
}

bool fm_computeCentroid(const std::vector<VHACD::Vertex>& points,
                        const std::vector<VHACD::Triangle>& indices,
                        nd::VHACD::Vect3<double>& center)

{
    bool ret = false;
    if (points.size())
    {
        center = nd::VHACD::Vect3<double>(0);

        nd::VHACD::Vect3<double> numerator(0);
        double denominator = 0;

        for (uint32_t i = 0; i < indices.size(); i++)
        {
            uint32_t i1 = indices[i].mI0;
            uint32_t i2 = indices[i].mI1;
            uint32_t i3 = indices[i].mI2;

            const nd::VHACD::Vect3<double>& p1 = points[i1];
            const nd::VHACD::Vect3<double>& p2 = points[i2];
            const nd::VHACD::Vect3<double>& p3 = points[i3];

            // Compute the average of the sum of the three positions
            nd::VHACD::Vect3<double> sum = (p1 + p2 + p3) / 3;

            // Compute the area of this triangle
            double area = fm_computeArea(p1,
                                         p2,
                                         p3);

            numerator += (sum * area);

            denominator += area;
        }
        double recip = 1 / denominator;
        center = numerator * recip;
        ret = true;
    }
    return ret;
}

inline double det(const VHACD::Vertex& p1,
                  const VHACD::Vertex& p2,
                  const VHACD::Vertex& p3)
{
    return p1.mX * p2.mY * p3.mZ +
           p2.mX * p3.mY * p1.mZ +
           p3.mX * p1.mY * p2.mZ -
           p1.mX * p3.mY * p2.mZ -
           p2.mX * p1.mY * p3.mZ -
           p3.mX * p2.mY * p1.mZ;
}

double fm_computeMeshVolume(const std::vector<VHACD::Vertex>& vertices,
                            const std::vector<VHACD::Triangle>& indices)
{
    double volume = 0;
    for (uint32_t i = 0; i < indices.size(); i++)
    {
        const VHACD::Vertex& p1 = vertices[indices[i].mI0];
        const VHACD::Vertex& p2 = vertices[indices[i].mI1];
        const VHACD::Vertex& p3 = vertices[indices[i].mI2];
        volume += det(p1,
                      p2,
                      p3); // compute the volume of the tetrahedran relative to the origin.
    }

    volume *= (1.0f / 6.0f);
    if (volume < 0)
        volume *= -1;
    return volume;
}

void fm_inflateMinMax(nd::VHACD::Vect3<double>& bmin,
                      nd::VHACD::Vect3<double>& bmax,
                      double ratio)
{
    double inflate = (bmin - bmax).GetNorm() * 0.5 * ratio;
    bmin -= inflate;
    bmax += inflate;
}

void fm_combineAABB(const nd::VHACD::Vect3<double>& bmin1,
                    const nd::VHACD::Vect3<double>& bmax1,
                    const nd::VHACD::Vect3<double>& bmin2,
                    const nd::VHACD::Vect3<double>& bmax2,
                    nd::VHACD::Vect3<double>& bmin,
                    nd::VHACD::Vect3<double>& bmax)
{
    bmin = bmin1.CWiseMin(bmin2);
    bmax = bmax1.CWiseMax(bmax2);
}

double fm_volumeAABB(const nd::VHACD::Vect3<double>& bmin,
                     const nd::VHACD::Vect3<double>& bmax)
{
    double dx = bmax.getX() - bmin.getX();
    double dy = bmax.getY() - bmin.getY();
    double dz = bmax.getZ() - bmin.getZ();
    return dx*dy*dz;
}

bool fm_intersectAABB(const nd::VHACD::Vect3<double>& bmin1,
                      const nd::VHACD::Vect3<double>& bmax1,
                      const nd::VHACD::Vect3<double>& bmin2,
                      const nd::VHACD::Vect3<double>& bmax2)
{
    if ((bmin1.getX() > bmax2.getX()) || (bmin2.getX() > bmax1.getX()))
        return false;
    if ((bmin1.getY() > bmax2.getY()) || (bmin2.getY() > bmax1.getY()))
        return false;
    if ((bmin1.getZ() > bmax2.getZ()) || (bmin2.getZ() > bmax1.getZ()))
        return false;
    return true;
}

void fm_getAABB(const std::vector<VHACD::Vertex>& points,
                nd::VHACD::Vect3<double>& bmin,
                nd::VHACD::Vect3<double>& bmax)
{
    bmin = points[0];
    bmax = points[0];

    for (uint32_t i = 1; i < points.size(); i++)
    {
        const VHACD::Vertex& p = points[i];
        bmin = bmin.CWiseMin(p);
        bmax = bmax.CWiseMax(p);
    }
}

class MyVertexIndex
{
public:
    MyVertexIndex(double granularity,
                  bool snapToGrid)
        : mSnapToGrid(snapToGrid)
        , mGranularity(granularity)
    {
    }

    double snapToGrid(double p)
    {
        double m = fmod(p, mGranularity);
        p -= m;
        return p;
    }

    uint32_t getIndex(const nd::VHACD::Vect3<double>& _p,
                      bool& newPos) // get index for a vector double
    {
        uint32_t ret;

        newPos = false;

        nd::VHACD::Vect3<double> p;

        if (mSnapToGrid)
        {
            p[0] = snapToGrid(_p[0]);
            p[1] = snapToGrid(_p[1]);
            p[2] = snapToGrid(_p[2]);
        }
        else
        {
            p[0] = _p[0];
            p[1] = _p[1];
            p[2] = _p[2];
        }

        bool found;
        ret = mKdTree.getNearest(p, mGranularity, found);
        if (!found)
        {
            newPos = true;
            ret = mKdTree.add(VHACD::Vertex(p.getX(), p.getY(), p.getZ()));
        }

        return ret;
    }

    const std::vector<VHACD::Vertex>& getVertices() const
    {
        return mKdTree.getVertices();
    }

    std::vector<VHACD::Vertex>&& takeVertices()
    {
        return std::move(mKdTree.takeVertices());
    }

    uint32_t getVcount() const
    {
        return mKdTree.getVcount();
    }

    bool saveAsObj(const char* fname,
                   uint32_t tcount,
                   uint32_t* indices)
    {
        bool ret = false;

        FILE* fph = fopen(fname, "wb");
        if (fph)
        {
            ret = true;

            const std::vector<VHACD::Vertex>& v = getVertices();
            for (uint32_t i = 0; i < v.size(); ++i)
            {
                fprintf(fph, "v %0.9f %0.9f %0.9f\r\n",
                        v[i].mX,
                        v[i].mY,
                        v[i].mZ);
            }

            for (uint32_t i = 0; i < tcount; i++)
            {
                uint32_t i1 = *indices++;
                uint32_t i2 = *indices++;
                uint32_t i3 = *indices++;
                fprintf(fph, "f %d %d %d\r\n",
                        i1 + 1,
                        i2 + 1,
                        i3 + 1);
            }
            fclose(fph);
        }

        return ret;
    }

private:
    bool mSnapToGrid : 1;
    double mGranularity;
    VERTEX_INDEX::KdTree mKdTree;
};

} // namespace VHACD

//********************************************************************************************************************
// Defining the Voxel class
//********************************************************************************************************************
namespace VHACD
{

#define VHACD_VOXEL_BITS 10
#define VHACD_VOXEL_BITS2 20
#define VHACD_VOXEL_BIT_MASK ((1<<VHACD_VOXEL_BITS)-1)

    /*
     * A wrapper class for 3 10 bit integers packed into a 32 bit integer
     * Layout is [PAD][X][Y][Z]
     * Pad is bits 31-30, X is 29-20, Y is 19-10, and Z is 9-0
     */
    class Voxel
    {
    public:
        Voxel() = default;

        inline Voxel(uint32_t index) : mVoxel(index) {}
        inline Voxel(uint32_t x, uint32_t y, uint32_t z)
            : mVoxel((x << VHACD_VOXEL_BITS2) | (y << VHACD_VOXEL_BITS) | z)
        {
        }

        inline bool operator==(const Voxel &v) const
        {
            return v.mVoxel == mVoxel;
        }

        inline void getVoxel(uint32_t &x,
                             uint32_t &y,
                             uint32_t &z) const
        {
            x = (mVoxel >> VHACD_VOXEL_BITS2);
            y = (mVoxel >> VHACD_VOXEL_BITS) & VHACD_VOXEL_BIT_MASK;
            z = mVoxel & VHACD_VOXEL_BIT_MASK;
        }

        inline uint32_t getX() const
        {
            return (mVoxel >> VHACD_VOXEL_BITS2);
        }

        inline uint32_t getY() const
        {
            return (mVoxel >> VHACD_VOXEL_BITS) & VHACD_VOXEL_BIT_MASK;
        }

        inline uint32_t getZ() const
        {
            return mVoxel & VHACD_VOXEL_BIT_MASK;
        }

        inline uint32_t getVoxelAddress() const
        {
            return mVoxel;
        }

        uint32_t mVoxel{0};
    };

    class VoxelHash
    {
    public:
        size_t operator() (const Voxel &p) const
        {
            return size_t(p.mVoxel); // xor the x,y,z location to compute a hash
        }
    };

    class VoxelPosition
    {
    public:
        VoxelPosition(void) { }
        VoxelPosition(double _x,double _y,double _z) : x(_x),y(_y),z(_z) {}
        double x;
        double y;
        double z;
    };

    using VoxelSet = std::unordered_set< Voxel, VoxelHash >;
    using VoxelPositionMap = std::unordered_map< Voxel,VoxelPosition, VoxelHash >;
    using VoxelVector = std::vector< Voxel >;
}

//********************************************************************************************************************
// Defining the SimpleMesh class
//********************************************************************************************************************
namespace VHACD
{

class SimpleMesh
{
public:
    std::vector<VHACD::Vertex> mVertices;
    std::vector<VHACD::Triangle> mIndices;
};

}

//******************************************************************************************
//  Declaration of the AABBTree class
//******************************************************************************************

namespace VHACD
{

/*======================== 0-tests ========================*/

#define VHACD_AXISTEST_X01(a, b, fa, fb)                                                                                     \
    p0 = a * v0[1] - b * v0[2];                                                                                        \
    p2 = a * v2[1] - b * v2[2];                                                                                        \
    if (p0 < p2)                                                                                                       \
    {                                                                                                                  \
        min = p0;                                                                                                      \
        max = p2;                                                                                                      \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        min = p2;                                                                                                      \
        max = p0;                                                                                                      \
    }                                                                                                                  \
    rad = fa * boxhalfsize[1] + fb * boxhalfsize[2];                                                                   \
    if (min > rad || max < -rad)                                                                                       \
        return 0;

#define VHACD_AXISTEST_X2(a, b, fa, fb)                                                                                      \
    p0 = a * v0[1] - b * v0[2];                                                                                        \
    p1 = a * v1[1] - b * v1[2];                                                                                        \
    if (p0 < p1)                                                                                                       \
    {                                                                                                                  \
        min = p0;                                                                                                      \
        max = p1;                                                                                                      \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        min = p1;                                                                                                      \
        max = p0;                                                                                                      \
    }                                                                                                                  \
    rad = fa * boxhalfsize[1] + fb * boxhalfsize[2];                                                                   \
    if (min > rad || max < -rad)                                                                                       \
        return 0;

/*======================== 1-tests ========================*/

#define VHACD_AXISTEST_Y02(a, b, fa, fb)                                                                                     \
    p0 = -a * v0[0] + b * v0[2];                                                                                       \
    p2 = -a * v2[0] + b * v2[2];                                                                                       \
    if (p0 < p2)                                                                                                       \
    {                                                                                                                  \
        min = p0;                                                                                                      \
        max = p2;                                                                                                      \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        min = p2;                                                                                                      \
        max = p0;                                                                                                      \
    }                                                                                                                  \
    rad = fa * boxhalfsize[0] + fb * boxhalfsize[2];                                                                   \
    if (min > rad || max < -rad)                                                                                       \
        return 0;

#define VHACD_AXISTEST_Y1(a, b, fa, fb)                                                                                      \
    p0 = -a * v0[0] + b * v0[2];                                                                                       \
    p1 = -a * v1[0] + b * v1[2];                                                                                       \
    if (p0 < p1)                                                                                                       \
    {                                                                                                                  \
        min = p0;                                                                                                      \
        max = p1;                                                                                                      \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        min = p1;                                                                                                      \
        max = p0;                                                                                                      \
    }                                                                                                                  \
    rad = fa * boxhalfsize[0] + fb * boxhalfsize[2];                                                                   \
    if (min > rad || max < -rad)                                                                                       \
        return 0;

/*======================== 2-tests ========================*/


#define VHACD_AXISTEST_Z12(a, b, fa, fb)                                                                                     \
    p1 = a * v1[0] - b * v1[1];                                                                                        \
    p2 = a * v2[0] - b * v2[1];                                                                                        \
    if (p2 < p1)                                                                                                       \
    {                                                                                                                  \
        min = p2;                                                                                                      \
        max = p1;                                                                                                      \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        min = p1;                                                                                                      \
        max = p2;                                                                                                      \
    }                                                                                                                  \
    rad = fa * boxhalfsize[0] + fb * boxhalfsize[1];                                                                   \
    if (min > rad || max < -rad)                                                                                       \
        return 0;

#define VHACD_AXISTEST_Z0(a, b, fa, fb)                                                                                      \
    p0 = a * v0[0] - b * v0[1];                                                                                        \
    p1 = a * v1[0] - b * v1[1];                                                                                        \
    if (p0 < p1)                                                                                                       \
    {                                                                                                                  \
        min = p0;                                                                                                      \
        max = p1;                                                                                                      \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        min = p1;                                                                                                      \
        max = p0;                                                                                                      \
    }                                                                                                                  \
    rad = fa * boxhalfsize[0] + fb * boxhalfsize[1];                                                                   \
    if (min > rad || max < -rad)                                                                                       \
        return 0;

inline bool IntersectRayAABB(const nd::VHACD::Vect3<double>& start,
                             const nd::VHACD::Vect3<double>& dir,
                             const nd::VHACD::Vect3<double>& min,
                             const nd::VHACD::Vect3<double>& max,
                             double& t,
                             nd::VHACD::Vect3<double>* /*normal*/)
{
    //! calculate candidate plane on each axis
    bool inside = true;
    nd::VHACD::Vect3<double> ta(-1.0);

    //! use unrolled loops
    for (uint32_t i = 0; i < 3; ++i)
    {
        if (start[i] < min[i])
        {
            if (dir[i] != 0.0f)
                ta[i] = (min[i] - start[i]) / dir[i];
            inside = false;
        }
        else if (start[i] > max[i])
        {
            if (dir[i] != 0.0f)
                ta[i] = (max[i] - start[i]) / dir[i];
            inside = false;
        }
    }

    //! if point inside all planes
    if (inside)
    {
        t = 0.0f;
        return true;
    }

    //! we now have t values for each of possible intersection planes
    //! find the maximum to get the intersection point
    uint32_t taxis;
    double tmax = ta.MaxCoeff(taxis);

    if (tmax < 0.0f)
        return false;

    //! check that the intersection point lies on the plane we picked
    //! we don't test the axis of closest intersection for precision reasons

    //! no eps for now
    double eps = 0.0f;

    nd::VHACD::Vect3<double> hit = start + dir * tmax;

    if ((hit.getX() < min.getX() - eps || hit.getX() > max.getX() + eps) && taxis != 0)
        return false;
    if ((hit.getY() < min.getY() - eps || hit.getY() > max.getY() + eps) && taxis != 1)
        return false;
    if ((hit.getZ() < min.getZ() - eps || hit.getZ() > max.getZ() + eps) && taxis != 2)
        return false;

    //! output results
    t = tmax;

    return true;
}

// Moller and Trumbore's method
inline bool IntersectRayTriTwoSided(const nd::VHACD::Vect3<double>& p,
                                    const nd::VHACD::Vect3<double>& dir,
                                    const nd::VHACD::Vect3<double>& a,
                                    const nd::VHACD::Vect3<double>& b,
                                    const nd::VHACD::Vect3<double>& c,
                                    double& t,
                                    double& u,
                                    double& v,
                                    double& w,
                                    double& sign,
                                    nd::VHACD::Vect3<double>* normal)
{
    nd::VHACD::Vect3<double> ab = b - a;
    nd::VHACD::Vect3<double> ac = c - a;
    nd::VHACD::Vect3<double> n = ab.Cross(ac);

    double d = -dir.Dot(n);
    double ood = 1.0f / d; // No need to check for division by zero here as infinity aritmetic will save us...
    nd::VHACD::Vect3<double> ap = p - a;

    t = (ap.Dot(n)) * ood;
    if (t < 0.0f)
        return false;

    nd::VHACD::Vect3<double> e = -dir.Cross(ap);
    v = (ac.Dot(e)) * ood;
    if (v < 0.0f || v > 1.0f) // ...here...
        return false;
    w = -(ab.Dot(e)) * ood;
    if (w < 0.0f || v + w > 1.0f) // ...and here
        return false;

    u = 1.0f - v - w;
    if (normal)
        *normal = n;

    sign = d;

    return true;
}

inline nd::VHACD::Vect3<double> ClosestPointToAABB(const nd::VHACD::Vect3<double>& p,
                                                   const nd::VHACD::Vect3<double>& lower,
                                                   const nd::VHACD::Vect3<double>& upper)
{
    nd::VHACD::Vect3<double> c;

    for (int i = 0; i < 3; ++i)
    {
        double v = p[i];
        if (v < lower[i])
            v = lower[i];
        if (v > upper[i])
            v = upper[i];
        c[i] = v;
    }

    return c;
}

// RTCD 5.1.5, page 142
inline nd::VHACD::Vect3<double> ClosestPointOnTriangle(const nd::VHACD::Vect3<double>& a,
                                                       const nd::VHACD::Vect3<double>& b,
                                                       const nd::VHACD::Vect3<double>& c,
                                                       const nd::VHACD::Vect3<double>& p,
                                                       double& v,
                                                       double& w)
{
    nd::VHACD::Vect3<double> ab = b - a;
    nd::VHACD::Vect3<double> ac = c - a;
    nd::VHACD::Vect3<double> ap = p - a;

    double d1 = ab.Dot(ap);
    double d2 = ac.Dot(ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
    {
        v = 0.0f;
        w = 0.0f;
        return a;
    }

    nd::VHACD::Vect3<double> bp = p - b;
    double d3 = ab.Dot(bp);
    double d4 = ac.Dot(bp);
    if (d3 >= 0.0f && d4 <= d3)
    {
        v = 1.0f;
        w = 0.0f;
        return b;
    }

    double vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        v = d1 / (d1 - d3);
        w = 0.0f;
        return a + v * ab;
    }

    nd::VHACD::Vect3<double> cp = p - c;
    double d5 = ab.Dot(cp);
    double d6 = ac.Dot(cp);
    if (d6 >= 0.0f && d5 <= d6)
    {
        v = 0.0f;
        w = 1.0f;
        return c;
    }

    double vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        v = 0.0f;
        w = d2 / (d2 - d6);
        return a + w * ac;
    }

    double va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        v = 1.0f - w;
        return b + w * (c - b);
    }

    double denom = 1.0f / (va + vb + vc);
    v = vb * denom;
    w = vc * denom;
    return a + ab * v + ac * w;
}


class AABBTreeImpl
{
public:
    AABBTreeImpl() = default;
    AABBTreeImpl(AABBTreeImpl&&) = default;
    AABBTreeImpl& operator=(AABBTreeImpl&&) = default;

    AABBTreeImpl(const std::vector<VHACD::Vertex>& vertices,
                 const std::vector<VHACD::Triangle>& indices);

    bool TraceRay(const nd::VHACD::Vect3<double>& start,
                  const nd::VHACD::Vect3<double>& dir,
                  double& outT,
                  double& u,
                  double& v,
                  double& w,
                  double& faceSign,
                  uint32_t& faceIndex) const;


    nd::VHACD::Vect3<double> GetCenter() const
    {
        return (m_nodes[0].m_minExtents + m_nodes[0].m_maxExtents) * 0.5f;
    }
    nd::VHACD::Vect3<double> GetMinExtents() const
    {
        return m_nodes[0].m_minExtents;
    }
    nd::VHACD::Vect3<double> GetMaxExtents() const
    {
        return m_nodes[0].m_maxExtents;
    }

    bool raycast(const nd::VHACD::Vect3<double>& start,
                 const nd::VHACD::Vect3<double>& dir,
                 double& outT,
                 double& u,
                 double& v,
                 double& w,
                 double& faceSign,
                 uint32_t& faceIndex) const
    {
        return TraceRay(start,
                        dir,
                        outT,
                        u,
                        v,
                        w,
                        faceSign,
                        faceIndex);
    }

    bool getClosestPointWithinDistance(const nd::VHACD::Vect3<double>& point,
                                       double maxDistance,
                                       nd::VHACD::Vect3<double>& closestPoint)
    {
        double dis, v, w;
        uint32_t faceIndex;
        bool hit = GetClosestPointWithinDistance(point,
                                                 maxDistance,
                                                 dis,
                                                 v,
                                                 w,
                                                 faceIndex,
                                                 closestPoint);
        return hit;
    }

private:
    struct Node
    {
        union
        {
            uint32_t m_children;
            uint32_t m_numFaces{ 0 };
        };

        uint32_t* m_faces{ nullptr };
        nd::VHACD::Vect3<double> m_minExtents{ 0.0 };
        nd::VHACD::Vect3<double> m_maxExtents{ 0.0 };
    };


    struct BoundsAABB
    {
        BoundsAABB() = default;
        BoundsAABB(const nd::VHACD::Vect3<double>& min,
                   const nd::VHACD::Vect3<double>& max)
            : m_min(min)
            , m_max(max)
        {
        }

        inline double GetVolume() const
        {
            nd::VHACD::Vect3<double> e = m_max - m_min;
            return (e.getX() * e.getY() * e.getZ());
        }

        inline double GetSurfaceArea() const
        {
            nd::VHACD::Vect3<double> e = m_max - m_min;
            return 2.0f * (e.getX() * e.getY() + e.getX() * e.getZ() + e.getY() * e.getZ());
        }

        inline void Union(const BoundsAABB& b)
        {
            m_min = m_min.CWiseMin(b.m_min);
            m_max = m_max.CWiseMax(b.m_max);
        }

        nd::VHACD::Vect3<double> m_min{ 0.0 };
        nd::VHACD::Vect3<double> m_max{ 0.0 };
    };

    // partition the objects and return the number of objects in the lower partition
    uint32_t PartitionMedian(Node& n,
                             uint32_t* faces,
                             uint32_t numFaces);
    uint32_t PartitionSAH(Node& n,
                          uint32_t* faces,
                          uint32_t numFaces);

    void Build();

    void BuildRecursive(uint32_t nodeIndex,
                        uint32_t* faces,
                        uint32_t numFaces);

    void TraceRecursive(uint32_t nodeIndex,
                        const nd::VHACD::Vect3<double>& start,
                        const nd::VHACD::Vect3<double>& dir,
                        double& outT,
                        double& u,
                        double& v,
                        double& w,
                        double& faceSign,
                        uint32_t& faceIndex) const;


    bool GetClosestPointWithinDistance(const nd::VHACD::Vect3<double>& point,
                                       const double maxDis,
                                       double& dis,
                                       double& v,
                                       double& w,
                                       uint32_t& faceIndex,
                                       nd::VHACD::Vect3<double>& closest) const;

    void GetClosestPointWithinDistanceSqRecursive(uint32_t nodeIndex,
                                                  const nd::VHACD::Vect3<double>& point,
                                                  double& outDisSq,
                                                  double& outV,
                                                  double& outW,
                                                  uint32_t& outFaceIndex,
                                                  nd::VHACD::Vect3<double>& closest) const;

    void CalculateFaceBounds(uint32_t* faces,
                             uint32_t numFaces,
                             nd::VHACD::Vect3<double>& outMinExtents,
                             nd::VHACD::Vect3<double>& outMaxExtents);

    uint32_t GetNumFaces() const
    {
        return uint32_t(m_indices->size());
    }

    uint32_t GetNumNodes() const
    {
        return uint32_t(m_nodes.size());
    }

    // track the next free node
    uint32_t m_freeNode;

    const std::vector<VHACD::Vertex>* m_vertices{nullptr};

    const std::vector<VHACD::Triangle>* m_indices{nullptr};

    std::vector<uint32_t> m_faces;
    std::vector<Node> m_nodes;
    std::vector<BoundsAABB> m_faceBounds;

    // stats
    uint32_t m_treeDepth = 0;
    uint32_t m_innerNodes = 0;
    uint32_t m_leafNodes = 0;

    uint32_t s_depth{0};
};

AABBTreeImpl::AABBTreeImpl(const std::vector<VHACD::Vertex>& vertices,
                           const std::vector<VHACD::Triangle>& indices)
    : m_vertices(&vertices)
    , m_indices(&indices)
{
    Build();
}

namespace
{

struct FaceSorter
{
    FaceSorter(const std::vector<VHACD::Vertex>& positions,
               const std::vector<VHACD::Triangle>& indices,
               uint32_t axis)
        : m_vertices(positions)
        , m_indices(indices)
        , m_axis(axis)
    {
    }

    inline bool operator()(uint32_t lhs, uint32_t rhs) const
    {
        double a = GetCentroid(lhs);
        double b = GetCentroid(rhs);

        if (a == b)
            return lhs < rhs;
        else
            return a < b;
    }

    inline double GetCentroid(uint32_t face) const
    {
        const nd::VHACD::Vect3<double>& a = m_vertices[m_indices[face].mI0];
        const nd::VHACD::Vect3<double>& b = m_vertices[m_indices[face].mI1];
        const nd::VHACD::Vect3<double>& c = m_vertices[m_indices[face].mI2];

        return (a[m_axis] + b[m_axis] + c[m_axis]) / 3.0f;
    }

    const std::vector<VHACD::Vertex>& m_vertices;
    const std::vector<VHACD::Triangle>& m_indices;
    uint32_t m_axis;
};


} // anonymous namespace

void AABBTreeImpl::CalculateFaceBounds(uint32_t* faces,
                                       uint32_t numFaces,
                                       nd::VHACD::Vect3<double>& outMinExtents,
                                       nd::VHACD::Vect3<double>& outMaxExtents)
{
    nd::VHACD::Vect3<double> minExtents(FLT_MAX);
    nd::VHACD::Vect3<double> maxExtents(-FLT_MAX);

    // calculate face bounds
    for (uint32_t i = 0; i < numFaces; ++i)
    {
        nd::VHACD::Vect3<double> a = (*m_vertices)[(*m_indices)[faces[i]].mI0];
        nd::VHACD::Vect3<double> b = (*m_vertices)[(*m_indices)[faces[i]].mI1];
        nd::VHACD::Vect3<double> c = (*m_vertices)[(*m_indices)[faces[i]].mI2];

        minExtents = a.CWiseMin(minExtents);
        maxExtents = a.CWiseMax(maxExtents);

        minExtents = b.CWiseMin(minExtents);
        maxExtents = b.CWiseMax(maxExtents);

        minExtents = c.CWiseMin(minExtents);
        maxExtents = c.CWiseMax(maxExtents);
    }

    outMinExtents = minExtents;
    outMaxExtents = maxExtents;
}

void AABBTreeImpl::Build()
{
    const uint32_t numFaces = uint32_t(m_indices->size());

    // build initial list of faces
    m_faces.reserve(numFaces);

    // calculate bounds of each face and store
    m_faceBounds.reserve(numFaces);

    std::vector<BoundsAABB> stack;
    for (uint32_t i = 0; i < numFaces; ++i)
    {
        BoundsAABB top;
        CalculateFaceBounds(&i,
                            1,
                            top.m_min,
                            top.m_max);

        m_faces.push_back(i);
        m_faceBounds.push_back(top);
    }

    m_nodes.reserve(uint32_t(numFaces * 1.5f));

    // allocate space for all the nodes
    m_freeNode = 1;

    // start building
    BuildRecursive(0,
                   m_faces.data(),
                   numFaces);

    assert(s_depth == 0);
}

// partion faces around the median face
uint32_t AABBTreeImpl::PartitionMedian(Node& n,
                                       uint32_t* faces,
                                       uint32_t numFaces)
{
    FaceSorter predicate(*m_vertices,
                         *m_indices,
                         (n.m_maxExtents - n.m_minExtents).LongestAxis());
    std::nth_element(faces,
                     faces + numFaces / 2,
                     faces + numFaces,
                     predicate);

    return numFaces / 2;
}

// partion faces based on the surface area heuristic
uint32_t AABBTreeImpl::PartitionSAH(Node& n,
                                    uint32_t* faces,
                                    uint32_t numFaces)
{
//    (n);
    uint32_t bestAxis = 0;
    uint32_t bestIndex = 0;
    double bestCost = FLT_MAX;

    for (uint32_t a = 0; a < 3; ++a)
    {
        // sort faces by centroids
        FaceSorter predicate(*m_vertices,
                             *m_indices,
                             a);
        std::sort(faces,
                  faces + numFaces,
                  predicate);

        // two passes over data to calculate upper and lower bounds
        std::vector<double> cumulativeLower(numFaces);
        std::vector<double> cumulativeUpper(numFaces);

        BoundsAABB lower;
        BoundsAABB upper;

        for (uint32_t i = 0; i < numFaces; ++i)
        {
            lower.Union(m_faceBounds[faces[i]]);
            upper.Union(m_faceBounds[faces[numFaces - i - 1]]);

            cumulativeLower[i] = lower.GetSurfaceArea();
            cumulativeUpper[numFaces - i - 1] = upper.GetSurfaceArea();
        }

        double invTotalSA = 1.0f / cumulativeUpper[0];

        // test all split positions
        for (uint32_t i = 0; i < numFaces - 1; ++i)
        {
            double pBelow = cumulativeLower[i] * invTotalSA;
            double pAbove = cumulativeUpper[i] * invTotalSA;

            double cost = 0.125f + (pBelow * i + pAbove * (numFaces - i));
            if (cost <= bestCost)
            {
                bestCost = cost;
                bestIndex = i;
                bestAxis = a;
            }
        }
    }

    // re-sort by best axis
    FaceSorter predicate(*m_vertices,
                         *m_indices,
                         bestAxis);
    std::sort(faces,
              faces + numFaces,
              predicate);

    return bestIndex + 1;
}

void AABBTreeImpl::BuildRecursive(uint32_t nodeIndex,
                                  uint32_t* faces,
                                  uint32_t numFaces)
{
    const uint32_t kMaxFacesPerLeaf = 6;

    // if we've run out of nodes allocate some more
    if (nodeIndex >= m_nodes.size())
    {
        uint32_t s = std::max(uint32_t(1.5f * m_nodes.size()), 512U);
        m_nodes.resize(s);
    }

    // a reference to the current node, need to be careful here as this reference may become invalid if array is resized
    Node& n = m_nodes[nodeIndex];

    // track max tree depth
    ++s_depth;
    m_treeDepth = std::max(m_treeDepth, s_depth);

    CalculateFaceBounds(faces,
                        numFaces,
                        n.m_minExtents,
                        n.m_maxExtents);

    // calculate bounds of faces and add node
    if (numFaces <= kMaxFacesPerLeaf)
    {
        n.m_faces = faces;
        n.m_numFaces = numFaces;

        ++m_leafNodes;
    }
    else
    {
        ++m_innerNodes;

        // face counts for each branch
        const uint32_t leftCount = PartitionMedian(n, faces, numFaces);
        // const uint32_t leftCount = PartitionSAH(n, faces, numFaces);
        const uint32_t rightCount = numFaces - leftCount;

        // alloc 2 nodes
        m_nodes[nodeIndex].m_children = m_freeNode;

        // allocate two nodes
        m_freeNode += 2;

        // split faces in half and build each side recursively
        BuildRecursive(m_nodes[nodeIndex].m_children + 0, faces, leftCount);
        BuildRecursive(m_nodes[nodeIndex].m_children + 1, faces + leftCount, rightCount);
    }

    --s_depth;
}

bool AABBTreeImpl::TraceRay(const nd::VHACD::Vect3<double>& start,
                            const nd::VHACD::Vect3<double>& dir,
                            double& outT,
                            double& u,
                            double& v,
                            double& w,
                            double& faceSign,
                            uint32_t& faceIndex) const
{
    outT = FLT_MAX;
    TraceRecursive(0,
                   start,
                   dir,
                   outT,
                   u,
                   v,
                   w,
                   faceSign,
                   faceIndex);
    return (outT != FLT_MAX);
}

void AABBTreeImpl::TraceRecursive(uint32_t nodeIndex,
                                  const nd::VHACD::Vect3<double>& start,
                                  const nd::VHACD::Vect3<double>& dir,
                                  double& outT,
                                  double& outU,
                                  double& outV,
                                  double& outW,
                                  double& faceSign,
                                  uint32_t& faceIndex) const
{
    const Node& node = m_nodes[nodeIndex];

    if (node.m_faces == NULL)
    {
        // find closest node
        const Node& leftChild = m_nodes[node.m_children + 0];
        const Node& rightChild = m_nodes[node.m_children + 1];

        double dist[2] = { FLT_MAX, FLT_MAX };

        IntersectRayAABB(start,
                         dir,
                         leftChild.m_minExtents,
                         leftChild.m_maxExtents,
                         dist[0],
                         nullptr);
        IntersectRayAABB(start,
                         dir,
                         rightChild.m_minExtents,
                         rightChild.m_maxExtents,
                         dist[1],
                         nullptr);

        uint32_t closest = 0;
        uint32_t furthest = 1;

        if (dist[1] < dist[0])
        {
            closest = 1;
            furthest = 0;
        }

        if (dist[closest] < outT)
            TraceRecursive(node.m_children + closest,
                           start,
                           dir,
                           outT,
                           outU,
                           outV,
                           outW,
                           faceSign,
                           faceIndex);

        if (dist[furthest] < outT)
            TraceRecursive(node.m_children + furthest,
                           start,
                           dir,
                           outT,
                           outU,
                           outV,
                           outW,
                           faceSign,
                           faceIndex);
    }
    else
    {
        double t, u, v, w, s;

        for (uint32_t i = 0; i < node.m_numFaces; ++i)
        {
            uint32_t indexStart = node.m_faces[i];

            const nd::VHACD::Vect3<double>& a = (*m_vertices)[(*m_indices)[indexStart].mI0];
            const nd::VHACD::Vect3<double>& b = (*m_vertices)[(*m_indices)[indexStart].mI1];
            const nd::VHACD::Vect3<double>& c = (*m_vertices)[(*m_indices)[indexStart].mI2];
            if (IntersectRayTriTwoSided(start, dir, a, b, c, t, u, v, w, s, NULL))
            {
                if (t < outT)
                {
                    outT = t;
                    outU = u;
                    outV = v;
                    outW = w;
                    faceSign = s;
                    faceIndex = node.m_faces[i];
                }
            }
        }
    }
}

bool AABBTreeImpl::GetClosestPointWithinDistance(const nd::VHACD::Vect3<double>& point,
                                                 const double maxDis,
                                                 double& dis,
                                                 double& v,
                                                 double& w,
                                                 uint32_t& faceIndex,
                                                 nd::VHACD::Vect3<double>& closest) const
{
    dis = maxDis;
    faceIndex = uint32_t(~0);
    double disSq = dis * dis;

    GetClosestPointWithinDistanceSqRecursive(0,
                                             point,
                                             disSq,
                                             v,
                                             w,
                                             faceIndex,
                                             closest);
    dis = sqrt(disSq);

    return (faceIndex < (~(static_cast<unsigned int>(0))));
}

void AABBTreeImpl::GetClosestPointWithinDistanceSqRecursive(uint32_t nodeIndex,
                                                            const nd::VHACD::Vect3<double>& point,
                                                            double& outDisSq,
                                                            double& outV,
                                                            double& outW,
                                                            uint32_t& outFaceIndex,
                                                            nd::VHACD::Vect3<double>& closestPoint) const
{
    const Node& node = m_nodes[nodeIndex];

    if (node.m_faces == nullptr)
    {
        // find closest node
        const Node& leftChild = m_nodes[node.m_children + 0];
        const Node& rightChild = m_nodes[node.m_children + 1];

        // double dist[2] = { FLT_MAX, FLT_MAX };
        nd::VHACD::Vect3<double> lp = ClosestPointToAABB(point,
                                                         leftChild.m_minExtents,
                                                         leftChild.m_maxExtents);
        nd::VHACD::Vect3<double> rp = ClosestPointToAABB(point,
                                                         rightChild.m_minExtents,
                                                         rightChild.m_maxExtents);


        uint32_t closest = 0;
        uint32_t furthest = 1;
        double dcSq = (point - lp).GetNormSquared();
        double dfSq = (point - rp).GetNormSquared();
        if (dfSq < dcSq)
        {
            closest = 1;
            furthest = 0;
            std::swap(dfSq, dcSq);
        }

        if (dcSq < outDisSq)
        {
            GetClosestPointWithinDistanceSqRecursive(node.m_children + closest,
                                                     point,
                                                     outDisSq,
                                                     outV,
                                                     outW,
                                                     outFaceIndex,
                                                     closestPoint);
        }

        if (dfSq < outDisSq)
        {
            GetClosestPointWithinDistanceSqRecursive(node.m_children + furthest,
                                                     point,
                                                     outDisSq,
                                                     outV,
                                                     outW,
                                                     outFaceIndex,
                                                     closestPoint);
        }
    }
    else
    {

        double v, w;
        for (uint32_t i = 0; i < node.m_numFaces; ++i)
        {
            uint32_t indexStart = node.m_faces[i];

            const nd::VHACD::Vect3<double>& a = (*m_vertices)[(*m_indices)[indexStart].mI0];
            const nd::VHACD::Vect3<double>& b = (*m_vertices)[(*m_indices)[indexStart].mI1];
            const nd::VHACD::Vect3<double>& c = (*m_vertices)[(*m_indices)[indexStart].mI2];

            nd::VHACD::Vect3<double> cp = ClosestPointOnTriangle(a, b, c, point, v, w);
            double disSq = (cp - point).GetNormSquared();

            if (disSq < outDisSq)
            {
                closestPoint = cp;
                outDisSq = disSq;
                outV = v;
                outW = w;
                outFaceIndex = node.m_faces[i];
            }
        }
    }
}

} // namespace aabbtree

//******************************************************************************************
//  Implementation of the RaycastMesh class
//******************************************************************************************
namespace VHACD
{

class MyRaycastMesh
{
public:
    MyRaycastMesh() = default;
    MyRaycastMesh(MyRaycastMesh&& rhs) = default;
    MyRaycastMesh& operator=(MyRaycastMesh&& rhs) = default;

    MyRaycastMesh(const std::vector<VHACD::Vertex>& vertices,
                  const std::vector<VHACD::Triangle>& indices)
        : mAABBTree(vertices, indices)
        , mVertices(&vertices)
        , mIndices(&indices)
    {
    }

    // Uses high speed AABB raycasting
    bool raycast(const nd::VHACD::Vect3<double>& start,
                 const nd::VHACD::Vect3<double>& dir,
                 double& outT,
                 double& u,
                 double& v,
                 double& w,
                 double& faceSign,
                 uint32_t& faceIndex) const
    {
        return mAABBTree.raycast(start,
                                 dir,
                                 outT,
                                 u,
                                 v,
                                 w,
                                 faceSign,
                                 faceIndex);
    }

    bool raycast(const nd::VHACD::Vect3<double>& start,
                 const nd::VHACD::Vect3<double>& to,
                 double& outT,
                 double& faceSign,
                 nd::VHACD::Vect3<double>& hitLocation) const
    {
        nd::VHACD::Vect3<double> dir = to - start;
        double distance = dir.Normalize();
        double u, v, w;
        uint32_t faceIndex;
        bool hit = mAABBTree.raycast(start,
                                     dir,
                                     outT,
                                     u,
                                     v,
                                     w,
                                     faceSign,
                                     faceIndex);
        if (hit)
        {
            hitLocation = start + dir * outT;
        }
        if (hit && outT > distance)
        {
            hit = false;
        }
        return hit;
    }

    bool getClosestPointWithinDistance(const nd::VHACD::Vect3<double> point,
                                       double maxDistance,
                                       nd::VHACD::Vect3<double>& closestPoint)
    {
        return mAABBTree.getClosestPointWithinDistance(point,
                                                       maxDistance,
                                                       closestPoint);
    }

    VHACD::AABBTreeImpl mAABBTree;
    const std::vector<VHACD::Vertex>* mVertices = nullptr;
    const std::vector<VHACD::Triangle>* mIndices = nullptr;
};

} // namespace VHACD

//*************************************************************************************************************
// Definition of the Volume class
//*************************************************************************************************************

namespace VHACD
{

enum class VoxelValue : uint8_t
{
    PRIMITIVE_UNDEFINED = 0,
    PRIMITIVE_OUTSIDE_SURFACE_TOWALK = 1,
    PRIMITIVE_OUTSIDE_SURFACE = 2,
    PRIMITIVE_INSIDE_SURFACE = 3,
    PRIMITIVE_ON_SURFACE = 4
};

class Volume
{
public:
    void Voxelize(const std::vector<VHACD::Vertex>& points,
                  const std::vector<VHACD::Triangle>& triangles,
                  const size_t dim,
                  FillMode fillMode,
                  MyRaycastMesh* raycastMesh);

    void raycastFill(MyRaycastMesh* raycastMesh);

    void SetVoxel(const size_t i, const size_t j, const size_t k, VoxelValue value)
    {
        assert(i < m_dim[0] || i >= 0);
        assert(j < m_dim[1] || j >= 0);
        assert(k < m_dim[2] || k >= 0);

        m_data[k + j * m_dim[2] + i * m_dim[1] * m_dim[2]] = value;
    }

    VoxelValue& GetVoxel(const size_t i, const size_t j, const size_t k)
    {
        assert(i < m_dim[0] || i >= 0);
        assert(j < m_dim[1] || j >= 0);
        assert(k < m_dim[2] || k >= 0);
        return m_data[k + j * m_dim[2] + i * m_dim[1] * m_dim[2]];
    }

    const VoxelValue& GetVoxel(const size_t i, const size_t j, const size_t k) const
    {
        assert(i < m_dim[0] || i >= 0);
        assert(j < m_dim[1] || j >= 0);
        assert(k < m_dim[2] || k >= 0);
        return m_data[k + j * m_dim[2] + i * m_dim[1] * m_dim[2]];
    }

    size_t GetNPrimitivesOnSurf() const
    {
        return m_numVoxelsOnSurface;
    }

    size_t GetNPrimitivesInsideSurf() const
    {
        return m_numVoxelsInsideSurface;
    }

    const VHACD::VoxelVector& getSurfaceVoxels() const
    {
        return mSurfaceVoxels;
    }

    const VHACD::VoxelVector& getInteriorVoxels() const
    {
        return mInteriorVoxels;
    }

    void addSurfaceVoxel(int32_t x, int32_t y, int32_t z)
    {
        VHACD::Voxel v(x, y, z);
        mSurfaceVoxels.push_back(v);
    }

    void addInteriorVoxel(int32_t x, int32_t y, int32_t z)
    {
        VHACD::Voxel v(x, y, z);
        mInteriorVoxels.push_back(v);
    }

    nd::VHACD::Vect3<double> m_minBB{ 0.0 };
    nd::VHACD::Vect3<double> m_maxBB{ 1.0 };
    double m_scale{ 1.0 };
    nd::VHACD::Vect3<uint32_t> m_dim{ 0 };
    size_t m_numVoxelsOnSurface{ 0 };
    size_t m_numVoxelsInsideSurface{ 0 };
    size_t m_numVoxelsOutsideSurface{ 0 };
    std::vector<VoxelValue> m_data;

private:
    void MarkOutsideSurface(const size_t i0,
                            const size_t j0,
                            const size_t k0,
                            const size_t i1,
                            const size_t j1,
                            const size_t k1);
    void FillOutsideSurface();

    void FillInsideSurface();

    void ComputeBB(const std::vector<VHACD::Vertex>& points);

    void Allocate();

    std::vector<VHACD::Voxel> mSurfaceVoxels;
    std::vector<VHACD::Voxel> mInteriorVoxels;
};

int32_t TriBoxOverlap(const nd::VHACD::Vect3<double>& boxcenter,
                      const nd::VHACD::Vect3<double>& boxhalfsize,
                      const nd::VHACD::Vect3<double>& triver0,
                      const nd::VHACD::Vect3<double>& triver1,
                      const nd::VHACD::Vect3<double>& triver2);

inline void Volume::ComputeBB(const std::vector<VHACD::Vertex>& points)
{
    nd::VHACD::Vect3<double> pt = points[0];
    m_maxBB = pt;
    m_minBB = pt;
    for (uint32_t v = 1; v < points.size(); ++v)
    {
        pt = points[v];
        for (int32_t i = 0; i < 3; ++i)
        {
            if (pt[i] < m_minBB[i])
                m_minBB[i] = pt[i];
            else if (pt[i] > m_maxBB[i])
                m_maxBB[i] = pt[i];
        }
    }
}

inline void Volume::Voxelize(const std::vector<VHACD::Vertex>& points,
                             const std::vector<VHACD::Triangle>& indices,
                             const size_t dim,
                             FillMode fillMode,
                             VHACD::MyRaycastMesh* raycastMesh)
{
    if (points.size() == 0)
    {
        return;
    }

    ComputeBB(points);

    nd::VHACD::Vect3<double> d = m_maxBB - m_minBB;
    double r;
    // Equal comparison is important here to avoid taking the last branch when d[0] == d[1] with d[2] being the smallest
    // dimension. That would lead to dimensions in i and j to be a lot bigger than expected and make the amount of
    // voxels in the volume totally unmanageable.
    if (d[0] >= d[1] && d[0] >= d[2])
    {
        r = d[0];
        m_dim[0] = uint32_t(dim);
        m_dim[1] = uint32_t(2 + static_cast<size_t>(dim * d[1] / d[0]));
        m_dim[2] = uint32_t(2 + static_cast<size_t>(dim * d[2] / d[0]));
    }
    else if (d[1] >= d[0] && d[1] >= d[2])
    {
        r = d[1];
        m_dim[1] = uint32_t(dim);
        m_dim[0] = uint32_t(2 + static_cast<size_t>(dim * d[0] / d[1]));
        m_dim[2] = uint32_t(2 + static_cast<size_t>(dim * d[2] / d[1]));
    }
    else
    {
        r = d[2];
        m_dim[2] = uint32_t(dim);
        m_dim[0] = uint32_t(2 + static_cast<size_t>(dim * d[0] / d[2]));
        m_dim[1] = uint32_t(2 + static_cast<size_t>(dim * d[1] / d[2]));
    }

    m_scale = r / (dim - 1);
    double invScale = (dim - 1) / r;

    Allocate();
    m_numVoxelsOnSurface = 0;
    m_numVoxelsInsideSurface = 0;
    m_numVoxelsOutsideSurface = 0;

    nd::VHACD::Vect3<double> p[3];
    size_t i, j, k;
    size_t i0, j0, k0;
    size_t i1, j1, k1;
    nd::VHACD::Vect3<double> boxcenter;
    nd::VHACD::Vect3<double> pt;
    const nd::VHACD::Vect3<double> boxhalfsize(0.5, 0.5, 0.5);
    for (size_t t = 0; t < indices.size(); ++t)
    {
        nd::VHACD::Vect3<uint32_t> tri = indices[t];
        for (int32_t c = 0; c < 3; ++c)
        {
            pt = points[tri[c]];

            p[c][0] = (pt[0] - m_minBB[0]) * invScale;
            p[c][1] = (pt[1] - m_minBB[1]) * invScale;
            p[c][2] = (pt[2] - m_minBB[2]) * invScale;

            i = static_cast<size_t>(p[c][0] + 0.5);
            j = static_cast<size_t>(p[c][1] + 0.5);
            k = static_cast<size_t>(p[c][2] + 0.5);

            assert(i < m_dim[0] && i >= 0 && j < m_dim[1] && j >= 0 && k < m_dim[2] && k >= 0);

            if (c == 0)
            {
                i0 = i1 = i;
                j0 = j1 = j;
                k0 = k1 = k;
            }
            else
            {
                i0 = Min(i0, i);
                j0 = Min(j0, j);
                k0 = Min(k0, k);
                i1 = Max(i1, i);
                j1 = Max(j1, j);
                k1 = Max(k1, k);
            }
        }
        if (i0 > 0)
            --i0;
        if (j0 > 0)
            --j0;
        if (k0 > 0)
            --k0;
        if (i1 < m_dim[0])
            ++i1;
        if (j1 < m_dim[1])
            ++j1;
        if (k1 < m_dim[2])
            ++k1;
        for (size_t i_id = i0; i_id < i1; ++i_id)
        {
            boxcenter[0] = uint32_t(i_id);
            for (size_t j_id = j0; j_id < j1; ++j_id)
            {
                boxcenter[1] = uint32_t(j_id);
                for (size_t k_id = k0; k_id < k1; ++k_id)
                {
                    boxcenter[2] = uint32_t(k_id);
                    int32_t res = TriBoxOverlap(boxcenter, boxhalfsize, p[0], p[1], p[2]);
                    VoxelValue& value = GetVoxel(i_id, j_id, k_id);
                    if (res == 1 && value == VoxelValue::PRIMITIVE_UNDEFINED)
                    {
                        value = VoxelValue::PRIMITIVE_ON_SURFACE;
                        ++m_numVoxelsOnSurface;
                        addSurfaceVoxel(int32_t(i_id),int32_t(j_id),int32_t(k_id));
                    }
                }
            }
        }
    }
    if (fillMode == FillMode::SURFACE_ONLY)
    {
        const size_t i0_local = m_dim[0];
        const size_t j0_local = m_dim[1];
        const size_t k0_local = m_dim[2];
        for (size_t i_id = 0; i_id < i0_local; ++i_id)
        {
            for (size_t j_id = 0; j_id < j0_local; ++j_id)
            {
                for (size_t k_id = 0; k_id < k0_local; ++k_id)
                {
                    const VoxelValue& voxel = GetVoxel(i_id, j_id, k_id);
                    if (voxel != VoxelValue::PRIMITIVE_ON_SURFACE)
                    {
                        SetVoxel(i_id, j_id, k_id, VoxelValue::PRIMITIVE_OUTSIDE_SURFACE);
                    }
                }
            }
        }
    }
    else if (fillMode == FillMode::FLOOD_FILL)
    {
        MarkOutsideSurface(0, 0, 0, m_dim[0], m_dim[1], 1);
        MarkOutsideSurface(0, 0, m_dim[2] - 1, m_dim[0], m_dim[1], m_dim[2]);
        MarkOutsideSurface(0, 0, 0, m_dim[0], 1, m_dim[2]);
        MarkOutsideSurface(0, m_dim[1] - 1, 0, m_dim[0], m_dim[1], m_dim[2]);
        MarkOutsideSurface(0, 0, 0, 1, m_dim[1], m_dim[2]);
        MarkOutsideSurface(m_dim[0] - 1, 0, 0, m_dim[0], m_dim[1], m_dim[2]);
        FillOutsideSurface();
        FillInsideSurface();
    }
    else if (fillMode == FillMode::RAYCAST_FILL)
    {
        raycastFill(raycastMesh);
    }

}

} // namespace VHACD

//*************************************************************************************************************
// Implementation of the Volume class
//*************************************************************************************************************

namespace VHACD
{

int32_t PlaneBoxOverlap(const nd::VHACD::Vect3<double>& normal,
                        const nd::VHACD::Vect3<double>& vert,
                        const nd::VHACD::Vect3<double>& maxbox)
{
    int32_t q;
    nd::VHACD::Vect3<double> vmin;
    nd::VHACD::Vect3<double> vmax;
    double v;
    for (q = 0; q <= 2; q++)
    {
        v = vert[q];
        if (normal[q] > 0.0)
        {
            vmin[q] = -maxbox[q] - v;
            vmax[q] = maxbox[q] - v;
        }
        else
        {
            vmin[q] = maxbox[q] - v;
            vmax[q] = -maxbox[q] - v;
        }
    }
    if (normal.Dot(vmin) > 0.0)
        return 0;
    if (normal.Dot(vmax) >= 0.0)
        return 1;
    return 0;
}

int32_t TriBoxOverlap(const nd::VHACD::Vect3<double>& boxcenter,
                      const nd::VHACD::Vect3<double>& boxhalfsize,
                      const nd::VHACD::Vect3<double>& triver0,
                      const nd::VHACD::Vect3<double>& triver1,
                      const nd::VHACD::Vect3<double>& triver2)
{
    /*    use separating axis theorem to test overlap between triangle and box */
    /*    need to test for overlap in these directions: */
    /*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
    /*       we do not even need to test these) */
    /*    2) normal of the triangle */
    /*    3) crossproduct(edge from tri, {x,y,z}-directin) */
    /*       this gives 3x3=9 more tests */

    nd::VHACD::Vect3<double> v0;
    nd::VHACD::Vect3<double> v1;
    nd::VHACD::Vect3<double> v2;
    double min, max, p0, p1, p2, rad, fex, fey, fez; // -NJMP- "d" local variable removed
    nd::VHACD::Vect3<double> normal;
    nd::VHACD::Vect3<double> e0;
    nd::VHACD::Vect3<double> e1;
    nd::VHACD::Vect3<double> e2;

    /* This is the fastest branch on Sun */
    /* move everything so that the boxcenter is in (0,0,0) */

    v0 = triver0 - boxcenter;
    v1 = triver1 - boxcenter;
    v2 = triver2 - boxcenter;

    /* compute triangle edges */
    e0 = v1 - v0; /* tri edge 0 */
    e1 = v2 - v1; /* tri edge 1 */
    e2 = v0 - v2; /* tri edge 2 */

    /* Bullet 3:  */
    /*  test the 9 tests first (this was faster) */
    fex = fabs(e0[0]);
    fey = fabs(e0[1]);
    fez = fabs(e0[2]);

    VHACD_AXISTEST_X01(e0[2], e0[1], fez, fey);
    VHACD_AXISTEST_Y02(e0[2], e0[0], fez, fex);
    VHACD_AXISTEST_Z12(e0[1], e0[0], fey, fex);

    fex = fabs(e1[0]);
    fey = fabs(e1[1]);
    fez = fabs(e1[2]);

    VHACD_AXISTEST_X01(e1[2], e1[1], fez, fey);
    VHACD_AXISTEST_Y02(e1[2], e1[0], fez, fex);
    VHACD_AXISTEST_Z0(e1[1], e1[0], fey, fex);

    fex = fabs(e2[0]);
    fey = fabs(e2[1]);
    fez = fabs(e2[2]);

    VHACD_AXISTEST_X2(e2[2], e2[1], fez, fey);
    VHACD_AXISTEST_Y1(e2[2], e2[0], fez, fex);
    VHACD_AXISTEST_Z12(e2[1], e2[0], fey, fex);

    /* Bullet 1: */
    /*  first test overlap in the {x,y,z}-directions */
    /*  find min, max of the triangle each direction, and test for overlap in */
    /*  that direction -- this is equivalent to testing a minimal AABB around */
    /*  the triangle against the AABB */

    /* test in 0-direction */
    min = std::min({v0.getX(), v1.getX(), v2.getX()});
    max = std::max({v0.getX(), v1.getX(), v2.getX()});
    if (min > boxhalfsize[0] || max < -boxhalfsize[0])
        return 0;

    /* test in 1-direction */
    min = std::min({v0.getY(), v1.getY(), v2.getY()});
    max = std::max({v0.getY(), v1.getY(), v2.getY()});
    if (min > boxhalfsize[1] || max < -boxhalfsize[1])
        return 0;

    /* test in getZ-direction */
    min = std::min({v0.getZ(), v1.getZ(), v2.getZ()});
    max = std::max({v0.getZ(), v1.getZ(), v2.getZ()});
    if (min > boxhalfsize[2] || max < -boxhalfsize[2])
        return 0;

    /* Bullet 2: */
    /*  test if the box intersects the plane of the triangle */
    /*  compute plane equation of triangle: normal*x+d=0 */
    normal = e0.Cross(e1);

    if (!PlaneBoxOverlap(normal, v0, boxhalfsize))
        return 0;
    return 1; /* box and triangle overlaps */
}

void Volume::Allocate()
{
    size_t size = m_dim[0] * m_dim[1] * m_dim[2];
    m_data = std::vector<VoxelValue>(size,
                                     VoxelValue::PRIMITIVE_UNDEFINED);
}

void Volume::MarkOutsideSurface(const size_t i0,
                                const size_t j0,
                                const size_t k0,
                                const size_t i1,
                                const size_t j1,
                                const size_t k1)
{
    for (size_t i = i0; i < i1; ++i)
    {
        for (size_t j = j0; j < j1; ++j)
        {
            for (size_t k = k0; k < k1; ++k)
            {
                VoxelValue& v = GetVoxel(i, j, k);
                if (v == VoxelValue::PRIMITIVE_UNDEFINED)
                {
                    v = VoxelValue::PRIMITIVE_OUTSIDE_SURFACE_TOWALK;
                }
            }
        }
    }
}

inline void WalkForward(int64_t start, int64_t end, VoxelValue* ptr, int64_t stride, int64_t maxDistance)
{
    for (int64_t i = start, count = 0; count < maxDistance && i < end && *ptr == VoxelValue::PRIMITIVE_UNDEFINED;
         ++i, ptr += stride, ++count)
    {
        *ptr = VoxelValue::PRIMITIVE_OUTSIDE_SURFACE_TOWALK;
    }
}

inline void WalkBackward(int64_t start, int64_t end, VoxelValue* ptr, int64_t stride, int64_t maxDistance)
{
    for (int64_t i = start, count = 0; count < maxDistance && i >= end && *ptr == VoxelValue::PRIMITIVE_UNDEFINED;
         --i, ptr -= stride, ++count)
    {
        *ptr = VoxelValue::PRIMITIVE_OUTSIDE_SURFACE_TOWALK;
    }
}

void Volume::FillOutsideSurface()
{
    size_t voxelsWalked = 0;
    const int64_t i0 = m_dim[0];
    const int64_t j0 = m_dim[1];
    const int64_t k0 = m_dim[2];

    // Avoid striding too far in each direction to stay in L1 cache as much as possible.
    // The cache size required for the walk is roughly (4 * walkDistance * 64) since
    // the k direction doesn't count as it's walking byte per byte directly in a cache lines.
    // ~16k is required for a walk distance of 64 in each directions.
    const size_t walkDistance = 64;

    // using the stride directly instead of calling GetVoxel for each iterations saves
    // a lot of multiplications and pipeline stalls due to data dependencies on imul.
    const size_t istride = &GetVoxel(1, 0, 0) - &GetVoxel(0, 0, 0);
    const size_t jstride = &GetVoxel(0, 1, 0) - &GetVoxel(0, 0, 0);
    const size_t kstride = &GetVoxel(0, 0, 1) - &GetVoxel(0, 0, 0);

    // It might seem counter intuitive to go over the whole voxel range multiple times
    // but since we do the run in memory order, it leaves us with far fewer cache misses
    // than a BFS algorithm and it has the additional benefit of not requiring us to
    // store and manipulate a fifo for recursion that might become huge when the number
    // of voxels is large.
    // This will outperform the BFS algorithm by several orders of magnitude in practice.
    do
    {
        voxelsWalked = 0;
        for (int64_t i = 0; i < i0; ++i)
        {
            for (int64_t j = 0; j < j0; ++j)
            {
                for (int64_t k = 0; k < k0; ++k)
                {
                    VoxelValue& voxel = GetVoxel(i, j, k);
                    if (voxel == VoxelValue::PRIMITIVE_OUTSIDE_SURFACE_TOWALK)
                    {
                        voxelsWalked++;
                        voxel = VoxelValue::PRIMITIVE_OUTSIDE_SURFACE;

                        // walk in each direction to mark other voxel that should be walked.
                        // this will generate a 3d pattern that will help the overall
                        // algorithm converge faster while remaining cache friendly.
                        WalkForward(k + 1, k0, &voxel + kstride, kstride, walkDistance);
                        WalkBackward(k - 1, 0, &voxel - kstride, kstride, walkDistance);

                        WalkForward(j + 1, j0, &voxel + jstride, jstride, walkDistance);
                        WalkBackward(j - 1, 0, &voxel - jstride, jstride, walkDistance);

                        WalkForward(i + 1, i0, &voxel + istride, istride, walkDistance);
                        WalkBackward(i - 1, 0, &voxel - istride, istride, walkDistance);
                    }
                }
            }
        }

        m_numVoxelsOutsideSurface += voxelsWalked;
    } while (voxelsWalked != 0);
}

#pragma warning(disable:4244)

void Volume::FillInsideSurface()
{
    const uint32_t i0 = uint32_t(m_dim[0]);
    const uint32_t j0 = uint32_t(m_dim[1]);
    const uint32_t k0 = uint32_t(m_dim[2]);

    size_t maxSize = i0*j0*k0;

    std::vector<uint32_t> temp;
    temp.reserve(maxSize);
    uint32_t count{ 0 };

    for (uint32_t i = 0; i < i0; ++i)
    {
        for (uint32_t j = 0; j < j0; ++j)
        {
            for (uint32_t k = 0; k < k0; ++k)
            {
                VoxelValue& v = GetVoxel(i, j, k);
                if (v == VoxelValue::PRIMITIVE_UNDEFINED)
                {
                    v = VoxelValue::PRIMITIVE_INSIDE_SURFACE;
                    uint32_t index = (i << VHACD_VOXEL_BITS2) | (j << VHACD_VOXEL_BITS) | k;
                    temp.push_back(index);
                    count++;
                    ++m_numVoxelsInsideSurface;
                }
            }
        }
    }

    if ( count )
    {
        mInteriorVoxels.resize(count);
        std::copy(temp.begin(),
                  temp.end(),
                  mInteriorVoxels.begin());
    }
}

void traceRay(VHACD::MyRaycastMesh* raycastMesh,
              const nd::VHACD::Vect3<double>& start,
              const nd::VHACD::Vect3<double>& dir,
              uint32_t& insideCount,
              uint32_t& outsideCount)
{
    double outT, u, v, w, faceSign;
    uint32_t faceIndex;
    bool hit = raycastMesh->raycast(start,
                                    dir,
                                    outT,
                                    u,
                                    v,
                                    w,
                                    faceSign,
                                    faceIndex);
    if (hit)
    {
        if (faceSign >= 0)
        {
            insideCount++;
        }
        else
        {
            outsideCount++;
        }
    }
}

void Volume::raycastFill(VHACD::MyRaycastMesh* raycastMesh)
{
    if (!raycastMesh)
    {
        return;
    }

    const uint32_t i0 = m_dim[0];
    const uint32_t j0 = m_dim[1];
    const uint32_t k0 = m_dim[2];

    size_t maxSize = i0 * j0*k0;

    std::vector<uint32_t> temp;
    temp.reserve(maxSize);
    uint32_t count{ 0 };
    m_numVoxelsInsideSurface = 0;
    for (uint32_t i = 0; i < i0; ++i)
    {
        for (uint32_t j = 0; j < j0; ++j)
        {
            for (uint32_t k = 0; k < k0; ++k)
            {
                VoxelValue& voxel = GetVoxel(i, j, k);
                if (voxel != VoxelValue::PRIMITIVE_ON_SURFACE)
                {
                    nd::VHACD::Vect3<double> start;

                    start[0] = float(i) * m_scale + m_minBB[0];
                    start[1] = float(j) * m_scale + m_minBB[1];
                    start[2] = float(k) * m_scale + m_minBB[2];

                    uint32_t insideCount = 0;
                    uint32_t outsideCount = 0;

                    nd::VHACD::Vect3<double> directions[6] = {
                        nd::VHACD::Vect3<double>( 1,  0,  0),
                        nd::VHACD::Vect3<double>(-1,  0,  0), // this was 1, 0, 0 in the original code, but looks wrong
                        nd::VHACD::Vect3<double>( 0,  1,  0),
                        nd::VHACD::Vect3<double>( 0, -1,  0),
                        nd::VHACD::Vect3<double>( 0,  0,  1),
                        nd::VHACD::Vect3<double>( 0,  0, -1)
                    };

                    for (uint32_t r = 0; r < 6; r++)
                    {
                        traceRay(raycastMesh, start, directions[r], insideCount, outsideCount);
                        // Early out if we hit the outside of the mesh
                        if (outsideCount)
                        {
                            break;
                        }
                        // Early out if we accumulated 3 inside hits
                        if (insideCount >= 3)
                        {
                            break;
                        }
                    }

                    if (outsideCount == 0 && insideCount >= 3)
                    {
                        voxel = VoxelValue::PRIMITIVE_INSIDE_SURFACE;
                        uint32_t index = (i << VHACD_VOXEL_BITS2) | (j << VHACD_VOXEL_BITS) | k;
                        temp.push_back(index);
                        count++;
                        m_numVoxelsInsideSurface++;
                    }
                    else
                    {
                        voxel = VoxelValue::PRIMITIVE_OUTSIDE_SURFACE;
                    }
                }
            }
        }
    }

    if (count)
    {
        mInteriorVoxels.resize(count);
        std::copy(temp.begin(),
                  temp.end(),
                  mInteriorVoxels.begin());
    }
}

} // namespace VHACD


//*************************************************************************************************************
// Implementation of the Voxelize class
//*************************************************************************************************************
namespace VHACD
{

class VoxelizeImpl
{
public:
    uint32_t voxelize(VHACD::MyRaycastMesh& raycastMesh,
                      const std::vector<VHACD::Vertex>& vertices,
                      const std::vector<VHACD::Triangle>& indices,
                      const uint32_t resolution,
                      FillMode fillMode)
    {
        double a = pow(resolution, 0.33);
        mDimensions = (a*1.5);
        // Minimum voxel resolution is 32x32x32
        mDimensions = Max(mDimensions, uint32_t(32));

        mVolume = VHACD::Volume();
        mVolume.Voxelize(vertices,
                         indices,
                         mDimensions,
                         fillMode,
                         &raycastMesh);

        return mDimensions;
    }

    double getScale() const
    {
        return mVolume.m_scale;;
    }

    nd::VHACD::Vect3<double> getBoundsMin() const
    {
        return mVolume.m_minBB;
    }

    nd::VHACD::Vect3<double> getBoundsMax() const
    {
        return mVolume.m_maxBB;
    }

    nd::VHACD::Vect3<uint32_t> getDimensions() const
    {
        return mVolume.m_dim;
    }

    VoxelValue getVoxel(uint32_t x,
                        uint32_t y,
                        uint32_t z) const
    {
        return mVolume.GetVoxel(x,y,z);;
    }

    void setVoxel(uint32_t x,
                  uint32_t y,
                  uint32_t z,
                  VoxelValue value)
    {
        mVolume.SetVoxel(x,y,z,value);
    }

    bool getVoxel(const nd::VHACD::Vect3<double>& pos,
                  uint32_t &x,
                  uint32_t &y,
                  uint32_t &z) const
    {
        bool ret = false;

        nd::VHACD::Vect3<double> bmin = getBoundsMin();
        nd::VHACD::Vect3<double> bmax = getBoundsMax();

        if ( pos[0] >= bmin[0] && pos[0] < bmax[0] &&
             pos[1] >= bmin[1] && pos[1] < bmax[1] &&
             pos[2] >= bmin[2] && pos[2] < bmax[2] )
        {
            double recipScale = 1.0 / getScale();
            x = uint32_t( (pos[0] - bmin[0]) * recipScale);
            y = uint32_t( (pos[1] - bmin[1]) * recipScale);
            z = uint32_t( (pos[2] - bmin[2]) * recipScale);
            ret = true;
        }
        return ret;
    }

    bool getSurfaceVoxels(VoxelVector &surfaceVoxels)
    {
        surfaceVoxels.clear();
        surfaceVoxels = mVolume.getSurfaceVoxels();

        return !surfaceVoxels.empty();
    }

    bool getInteriorVoxels(VoxelVector &interiorVoxels)
    {
        interiorVoxels.clear();
        interiorVoxels = mVolume.getInteriorVoxels();
        return !interiorVoxels.empty();
    }

    uint32_t        mDimensions{32};
    VHACD::Volume   mVolume;
};

} // namespace VHACD

//******************************************************************************************
//  ShrinkWrap helper class
//******************************************************************************************
// This is a code snippet which 'shrinkwraps' a convex hull
// to a source mesh.
//
// It is a somewhat complicated algorithm. It works as follows:
//
// * Step #1 : Compute the mean unit normal vector for each vertex in the convex hull
// * Step #2 : For each vertex in the conex hull we project is slightly outwards along the mean normal vector
// * Step #3 : We then raycast from this slightly extruded point back into the opposite direction of the mean normal vector
//             resulting in a raycast from slightly beyond the vertex in the hull into the source mesh we are trying
//             to 'shrink wrap' against
// * Step #4 : If the raycast fails we leave the original vertex alone
// * Step #5 : If the raycast hits a backface we leave the original vertex alone
// * Step #6 : If the raycast hits too far away (no more than a certain threshold distance) we live it alone
// * Step #7 : If the point we hit on the source mesh is not still within the convex hull, we reject it.
// * Step #8 : If all of the previous conditions are met, then we take the raycast hit location as the 'new position'
// * Step #9 : Once all points have been projected, if possible, we need to recompute the convex hull again based on these shrinkwrapped points
// * Step #10 : In theory that should work.. let's see...

//***********************************************************************************************
// QuickHull implementation
//***********************************************************************************************
namespace VHACD
{

//////////////////////////////////////////////////////////////////////////
// Quickhull base class holding the hull during construction
class QuickHullImpl
{
public:
    uint32_t computeConvexHull(const std::vector<VHACD::Vertex>& vertices,
                               uint32_t maxHullVertices)
    {
        mIndices.clear();

        nd::VHACD::ConvexHull ch(vertices,
                                 0.0001,
                                 maxHullVertices);

        auto& vlist = ch.GetVertexPool();
        if ( !vlist.empty() )
        {
            size_t vcount = vlist.size();
            mVertices.resize(vcount);
            std::copy(vlist.begin(),
                      vlist.end(),
                      mVertices.begin());
        }
        
		for (nd::VHACD::ConvexHull::ndNode* node = ch.GetFirst(); node; node = node->GetNext())
		{
			nd::VHACD::ConvexHullFace* const face = &node->GetInfo();
            mIndices.emplace_back(face->m_index[0],
                                  face->m_index[1],
                                  face->m_index[2]);
		}

        return uint32_t(mIndices.size());
    }

    const std::vector<VHACD::Vertex>& getVertices() const
    {
        return mVertices;
    }

    const std::vector<VHACD::Triangle>& getIndices() const
    {
        return mIndices;
    }

private:
    std::vector<VHACD::Vertex>   mVertices;
    std::vector<VHACD::Triangle> mIndices;
};

} // end of VHACD namespace

//******************************************************************************************
// Implementation of the ShrinkWrap class
//******************************************************************************************

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

namespace VHACD
{

class ShrinkWrapImpl
{
public:
    void shrinkWrap(SimpleMesh &sourceConvexHull,
                    VHACD::MyRaycastMesh &raycastMesh,
                    uint32_t maxHullVertexCount,
                    double distanceThreshold,
                    bool doShrinkWrap)
    {
        std::vector<VHACD::Vertex> verts; // New verts for the new convex hull
        verts.reserve(sourceConvexHull.mVertices.size());
        // Examine each vertex and see if it is within the voxel distance.
        // If it is, then replace the point with the shrinkwrapped / projected point
        for (uint32_t j = 0; j < sourceConvexHull.mVertices.size(); j++)
        {
            VHACD::Vertex& p = sourceConvexHull.mVertices[j];
            if (doShrinkWrap)
            {
                nd::VHACD::Vect3<double> closest;
                if (raycastMesh.getClosestPointWithinDistance(p, distanceThreshold, closest))
                {
                    p = closest;
                }
            }
            verts.emplace_back(p);
        }
        // Final step is to recompute the convex hull
        VHACD::QuickHullImpl qh;
        uint32_t tcount = qh.computeConvexHull(verts,
                                               maxHullVertexCount);
        if (tcount)
        {
            sourceConvexHull.mVertices = qh.getVertices();
            sourceConvexHull.mIndices = qh.getIndices();
        }
    }
};

}

//********************************************************************************************************************

#if !VHACD_DISABLE_THREADING

//********************************************************************************************************************
// Definition of the ThreadPool
//********************************************************************************************************************

namespace VHACD
{

class ThreadPool {
 public:
    ThreadPool() : ThreadPool(1) {};
    ThreadPool(int worker);
    ~ThreadPool();
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&& ... args)
        -> std::future< typename std::result_of< F( Args... ) >::type>;
 private:
    std::vector<std::thread> workers;
    std::deque<std::function<void()>> tasks;
    std::mutex task_mutex;
    std::condition_variable cv;
    bool closed;
    int count;
};

ThreadPool::ThreadPool(int worker) : closed(false), count(0)
{
    workers.reserve(worker);
    for(int i=0; i<worker; i++) 
    {
        workers.emplace_back(
            [this]
        {
                std::unique_lock<std::mutex> lock(this->task_mutex);
                while(true) 
                {
                    while (this->tasks.empty()) 
                    {
                        if (this->closed) 
                        {
                            return;
                        }
                        this->cv.wait(lock);
                    }
                    auto task = this->tasks.front();
                    this->tasks.pop_front();
                    lock.unlock();
                    task();
                    lock.lock();
                }
            }
        );
    }
}



template<typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&& ... args)
    -> std::future< typename std::result_of< F( Args... ) >::type>
{

    using return_type = typename std::result_of< F( Args...) >::type;
    auto task = std::make_shared<std::packaged_task<return_type()> > (
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    auto result = task->get_future();

    {
        std::unique_lock<std::mutex> lock(task_mutex);
        if (!closed) 
        {
            tasks.emplace_back([task]
            { 
                (*task)();
            });
            cv.notify_one();
        }
    }

    return result;
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(task_mutex);
        closed = true;
    }
    cv.notify_all();
    for (auto && worker : workers) 
    {
        worker.join();
    }
}


}
#endif


namespace VHACD
{


using AABBTreeVector = std::vector< VHACD::AABBTreeImpl* >;
using VoxelVector = std::vector< VHACD::Voxel >;
using ConvexHullVector = std::vector< IVHACD::ConvexHull *>;

enum class Stages
{
    COMPUTE_BOUNDS_OF_INPUT_MESH,
    REINDEXING_INPUT_MESH,
    CREATE_RAYCAST_MESH,
    VOXELIZING_INPUT_MESH,
    BUILD_INITIAL_CONVEX_HULL,
    PERFORMING_DECOMPOSITION,
    INITIALIZING_CONVEX_HULLS_FOR_MERGING,
    COMPUTING_COST_MATRIX,
    MERGING_CONVEX_HULLS,
    FINALIZING_RESULTS,
    NUM_STAGES
};

class VHACDCallbacks
{
public:
    virtual void progressUpdate(Stages stage,double stageProgress,const char *operation) = 0;
    virtual bool isCanceled(void) const = 0;

    virtual ~VHACDCallbacks() = default;
};

enum class SplitAxis
{
    X_AXIS_NEGATIVE,
    X_AXIS_POSITIVE,
    Y_AXIS_NEGATIVE,
    Y_AXIS_POSITIVE,
    Z_AXIS_NEGATIVE,
    Z_AXIS_POSITIVE,
};

// Maps from a voxel index to a vertex position index
using VoxelIndexMap = std::unordered_map< uint32_t, uint32_t >;

// This class represents a collection of voxels, the convex hull
// which surrounds them, and a triangle mesh representation of those voxels
class VoxelHull
{
public:

    // This method constructs a new VoxelHull based on a plane split of the parent
    // convex hull
    VoxelHull(const VoxelHull &parent,
              SplitAxis axis,
              uint32_t splitLoc) : mAxis(axis)
    {
        mDepth = parent.mDepth+1;
        mVoxelHullCount++;  // Each VoxelHull is given a unique index number to distinguish it from the rest. Primarily used for debugging
        mIndex = mVoxelHullCount;   
        mVoxels = parent.mVoxels;   // Copy the parent's voxels pointer
        mParams = parent.mParams;   // Copy the parent's parameters
        mVoxelScale = mVoxels->getScale();  // Get the scale of a single voxel
        mVoxelBmin = mVoxels->getBoundsMin();  // Get the 3d bounding volume minimum for this voxel space
        mVoxelBmax = mVoxels->getBoundsMax();  // Get the 3d bounding volume maximum for this voxel space
        mVoxelScaleHalf = mVoxelScale * 0.5;  // Compute half of the voxel size

        mVoxelAdjust = mVoxelBmin - mVoxelScaleHalf; // The adjustment to move from voxel coordinate to a 3d position

        // Default copy the voxel region from the parent, but values will
        // be adjusted next based on the split axis and location
        m1 = parent.m1;
        m2 = parent.m2;

        switch ( mAxis )
        {
            case SplitAxis::X_AXIS_NEGATIVE:
                m2.getX() = splitLoc;
                break;
            case SplitAxis::X_AXIS_POSITIVE:
                m1.getX() = splitLoc + 1;
                break;
            case SplitAxis::Y_AXIS_NEGATIVE:
                m2.getY() = splitLoc;
                break;
            case SplitAxis::Y_AXIS_POSITIVE:
                m1.getY() = splitLoc + 1;
                break;
            case SplitAxis::Z_AXIS_NEGATIVE:
                m2.getZ() = splitLoc;
                break;
            case SplitAxis::Z_AXIS_POSITIVE:
                m1.getZ() = splitLoc + 1;
                break;
        }
        // First, we copy all of the interior voxels from our parent
        // which intersect our region
        for (auto &i : parent.mInteriorVoxels)
        {
            uint32_t x, y, z;
            i.getVoxel(x, y, z);
            if ( x >= m1.getX() && x <= m2.getX() &&
                 y >= m1.getY() && y <= m2.getY() &&
                 z >= m1.getZ() && z <= m2.getZ() )
            {
                bool newSurface = false;
                switch ( mAxis )
                {
                    case SplitAxis::X_AXIS_NEGATIVE:
                        if ( x == splitLoc )
                        {
                            newSurface = true;
                        }
                        break;
                    case SplitAxis::X_AXIS_POSITIVE:
                        if ( x == m1.getX() )
                        {
                            newSurface = true;
                        }
                        break;
                    case SplitAxis::Y_AXIS_NEGATIVE:
                        if ( y == splitLoc )
                        {
                            newSurface = true;
                        }
                        break;
                    case SplitAxis::Y_AXIS_POSITIVE:
                        if ( y == m1.getY() )
                        {
                            newSurface = true;
                        }
                        break;
                    case SplitAxis::Z_AXIS_NEGATIVE:
                        if ( z == splitLoc )
                        {
                            newSurface = true;
                        }
                        break;
                    case SplitAxis::Z_AXIS_POSITIVE:
                        if ( z == m1.getZ() )
                        {
                            newSurface = true;
                        }
                        break;
                }
                // If his interior voxels lie directly on the split plane then
                // these become new surface voxels for our patch
                if ( newSurface )
                {
                    mNewSurfaceVoxels.push_back(i);
                }
                else
                {
                    mInteriorVoxels.push_back(i);
                }
            }
        }
        // Next we copy all of the surface voxels which intersect our region
        for (auto &i : parent.mSurfaceVoxels)
        {
            uint32_t x, y, z;
            i.getVoxel(x, y, z);
            if ( x >= m1.getX() && x <= m2.getX() &&
                 y >= m1.getY() && y <= m2.getY() &&
                 z >= m1.getZ() && z <= m2.getZ() )
            {
                mSurfaceVoxels.push_back(i);
            }
        }
        // Our parent's new surface voxels become our new surface voxels so long as they intersect our region
        for (auto &i : parent.mNewSurfaceVoxels)
        {
            uint32_t x, y, z;
            i.getVoxel(x, y, z);
            if ( x >= m1.getX() && x <= m2.getX() &&
                 y >= m1.getY() && y <= m2.getY() &&
                 z >= m1.getZ() && z <= m2.getZ() )
            {
                mNewSurfaceVoxels.push_back(i);
            }
        }

        // Recompute the min-max bounding box which would be different after the split occurs
        m1 = nd::VHACD::Vect3<uint32_t>(0x7FFFFFFF);
        m2 = nd::VHACD::Vect3<uint32_t>(0);
        for (auto &i : mSurfaceVoxels)
        {
            minMaxVoxelRegion(i);
        }
        for (auto &i : mNewSurfaceVoxels)
        {
            minMaxVoxelRegion(i);
        }
        for (auto &i : mInteriorVoxels)
        {
            minMaxVoxelRegion(i);
        }

        buildVoxelMesh();
        buildRaycastMesh(); // build a raycast mesh of the voxel mesh
        computeConvexHull();
    }

    // Helper method to refresh the min/max voxel bounding region
    void minMaxVoxelRegion(const Voxel &v)
    {
        uint32_t x, y, z;
        v.getVoxel(x, y, z);
        m1.getX() = Min(m1.getX(), x);
        m2.getX() = Max(m2.getX(), x);
        m1.getY() = Min(m1.getY(), y);
        m2.getY() = Max(m2.getY(), y);
        m1.getZ() = Min(m1.getZ(), z);
        m2.getZ() = Max(m2.getZ(), z);
    }


    // Here we construct the intitial convex hull around the
    // entire voxel set
    VoxelHull(VoxelizeImpl& voxels,
              const SimpleMesh &inputMesh,
              const IVHACD::Parameters &params,
              VHACDCallbacks *callbacks)
        : mVoxels(&voxels)
        , mParams(params)
        , mCallbacks(callbacks)
    {
        mVoxelHullCount++;
        mIndex = mVoxelHullCount;
        nd::VHACD::Vect3<uint32_t> dimensions = mVoxels->getDimensions();

        m2 = dimensions - 1;

        mVoxelScale = mVoxels->getScale();
        mVoxelBmin = mVoxels->getBoundsMin();
        mVoxelBmax = mVoxels->getBoundsMax();
        mVoxelScaleHalf = mVoxelScale * 0.5;

        mVoxelAdjust = mVoxelBmin - mVoxelScaleHalf;

        // Here we get a copy of all voxels which lie on the surface mesh
        mVoxels->getSurfaceVoxels(mSurfaceVoxels);
        // Now we get a copy of all voxels which are considered part of the 'interior' of the source mesh
        mVoxels->getInteriorVoxels(mInteriorVoxels);
        buildVoxelMesh();
        buildRaycastMesh(); // build a raycast mesh of the voxel mesh
//        saveVoxelMesh(inputMesh,true,false);
        computeConvexHull();
    }

    ~VoxelHull(void)
    {
        if ( mConvexHull )
        {
            delete mConvexHull;
            mConvexHull = nullptr;
        }
        delete mHullA;
        delete mHullB;
    }

    void buildRaycastMesh(void)
    {
        // Create a raycast mesh representation of the voxelized surface mesh
        if ( !mIndices.empty() )
        {
            mRaycastMesh = MyRaycastMesh(mVertices,
                                         mIndices);
        }
    }

    // We now compute the convex hull relative to a triangle mesh generated 
    // from the voxels
    void computeConvexHull(void)
    {
        if ( !mVertices.empty() )
        {
            // we compute the convex hull as follows...
            VHACD::QuickHullImpl qh;
            uint32_t tcount = qh.computeConvexHull(mVertices,
                                                   uint32_t(mVertices.size()));
            if ( tcount )
            {
                mConvexHull = new IVHACD::ConvexHull;

                const std::vector<VHACD::Vertex>& vertices = qh.getVertices();
                mConvexHull->m_points.resize(vertices.size());
                std::copy(vertices.begin(),
                          vertices.end(),
                          mConvexHull->m_points.begin());

                const std::vector<VHACD::Triangle>& indices = qh.getIndices();
                mConvexHull->m_triangles.resize(indices.size());
                std::copy(indices.begin(),
                          indices.end(),
                          mConvexHull->m_triangles.begin());

                VHACD::fm_computeCentroid(mConvexHull->m_points,
                                          mConvexHull->m_triangles,
                                          mConvexHull->m_center);
                mConvexHull->m_volume = VHACD::fm_computeMeshVolume(mConvexHull->m_points,
                                                                    mConvexHull->m_triangles);
            }
        }
        if ( mConvexHull )
        {
            mHullVolume = mConvexHull->m_volume;
        }
        // This is the volume of a single voxel
        double singleVoxelVolume = mVoxelScale * mVoxelScale * mVoxelScale;
        size_t voxelCount = mInteriorVoxels.size() + mNewSurfaceVoxels.size() + mSurfaceVoxels.size();
        mVoxelVolume = singleVoxelVolume * double(voxelCount);
        double diff = fabs(mHullVolume - mVoxelVolume);
        mVolumeError = (diff * 100) / mVoxelVolume;
    }

    // Returns true if this convex hull should be considered done
    bool isComplete(void)
    {
        bool ret = false;
        if ( mConvexHull == nullptr )
        {
            ret = true;
        }
        else if ( mVolumeError < mParams.m_minimumVolumePercentErrorAllowed )
        {
            ret = true;
        }
        else if ( mDepth > mParams.m_maxRecursionDepth )
        {
            ret = true;
        }
        else
        {
            // We compute the voxel width on all 3 axes and see if they are below the min threshold size
            nd::VHACD::Vect3<uint32_t> d = m2 - m1;
            if ( d.getX() <= mParams.m_minEdgeLength &&
                 d.getY() <= mParams.m_minEdgeLength &&
                 d.getZ() <= mParams.m_minEdgeLength )
            {
                ret = true;
            }
        }
        return ret;
    }

    
    // Convert a voxel position into it's correct double precision location
    inline nd::VHACD::Vect3<double> getPoint(const int32_t x,
                                             const int32_t y,
                                             const int32_t z,
                                             const double scale,
                                             const nd::VHACD::Vect3<double>& bmin) const
    {
        return nd::VHACD::Vect3<double>(x * scale + bmin.getX(),
                                        y * scale + bmin.getY(),
                                        z * scale + bmin.getZ());
    }

    // Sees if we have already got an index for this voxel position.
    // If the voxel position has already been indexed, we just return
    // that index value.
    // If not, then we convert it into the floating point position and
    // add it to the index map
    inline uint32_t getVertexIndex(const nd::VHACD::Vect3<int32_t>& p)
    {
        uint32_t ret = 0;
        uint32_t address = (p.getX() << 20) | (p.getY() << 10) | p.getZ();
        VoxelIndexMap::iterator found = mVoxelIndexMap.find(address);
        if ( found != mVoxelIndexMap.end() )
        {
            ret = (*found).second;
        }
        else
        {
            nd::VHACD::Vect3<double> vertex = getPoint(p.getX(),
                                                       p.getY(),
                                                       p.getZ(),
                                                       mVoxelScale,
                                                       mVoxelAdjust);
            ret = uint32_t(mVoxelIndexMap.size());
            mVoxelIndexMap[address] = ret;
            mVertices.emplace_back(vertex);
        }
        return ret;
    }

    // This method will convert the voxels into an actual indexed triangle mesh of boxes
    // This serves two purposes.
    // The primary purpose is so that when we compute a convex hull it considered all of the points
    // for each voxel, not just the center point. If you don't do this, then the hulls don't fit the
    // mesh accurately enough.
    // The second reason we convert it into a triangle mesh is so that we can do raycasting against it
    // to search for the best splitting plane fairly quickly. That algorithm will be discussed in the 
    // method which computes the best splitting plane.
    void buildVoxelMesh(void)
    {
        // When we build the triangle mesh we do *not* need the interior voxels, only the ones
        // which lie upon the logical surface of the mesh.
        // Each time we perform a plane split, voxels which are along the splitting plane become
        // 'new surface voxels'. 

        for (auto &i:mSurfaceVoxels)
        {
            addVoxelBox(i);
        }
        for (auto &i:mNewSurfaceVoxels)
        {
            addVoxelBox(i);
        }
    }

    // Convert a single voxel position into an actual 3d box mesh comprised
    // of 12 triangles
    inline void addVoxelBox(const Voxel &v)
    {
        // The voxel position of the upper left corner of the box
        nd::VHACD::Vect3<int32_t> bmin(v.getX(),
                                       v.getY(),
                                       v.getZ());
        // The voxel position of the lower right corner of the box
        nd::VHACD::Vect3<int32_t> bmax(bmin.getX() + 1,
                                       bmin.getY() + 1,
                                       bmin.getZ() + 1);

        // Build the set of 8 voxel positions representing
        // the coordinates of the box
        nd::VHACD::Vect3<int32_t> box[8];

        box[0] = nd::VHACD::Vect3<int32_t>(bmin.getX(), bmin.getY(), bmin.getZ());
        box[1] = nd::VHACD::Vect3<int32_t>(bmax.getX(), bmin.getY(), bmin.getZ());
        box[2] = nd::VHACD::Vect3<int32_t>(bmax.getX(), bmax.getY(), bmin.getZ());
        box[3] = nd::VHACD::Vect3<int32_t>(bmin.getX(), bmax.getY(), bmin.getZ());
        box[4] = nd::VHACD::Vect3<int32_t>(bmin.getX(), bmin.getY(), bmax.getZ());
        box[5] = nd::VHACD::Vect3<int32_t>(bmax.getX(), bmin.getY(), bmax.getZ());
        box[6] = nd::VHACD::Vect3<int32_t>(bmax.getX(), bmax.getY(), bmax.getZ());
        box[7] = nd::VHACD::Vect3<int32_t>(bmin.getX(), bmax.getY(), bmax.getZ());

        // Now add the 12 triangles comprising the 3d box
        addTri(box, 2, 1, 0);
        addTri(box, 3, 2, 0);

        addTri(box, 7, 2, 3);
        addTri(box, 7, 6, 2);

        addTri(box, 5, 1, 2);
        addTri(box, 5, 2, 6);

        addTri(box, 5, 4, 1);
        addTri(box, 4, 0, 1);

        addTri(box, 4, 6, 7);
        addTri(box, 4, 5, 6);

        addTri(box, 4, 7, 0);
        addTri(box, 7, 3, 0);
    }

    
    // Add the triangle represented by these 3 indices into the 'box' set of vertices
    // to the output mesh
    inline void addTri(const nd::VHACD::Vect3<int32_t>* box,
                       uint32_t i1, 
                       uint32_t i2, 
                       uint32_t i3)
    {
        addTriangle(box[i1], box[i2], box[i3]);
    }

    // Here we convert from voxel space to a 3d position, index it, and add
    // the triangle positions and indices for the output mesh
    inline void addTriangle(const nd::VHACD::Vect3<int32_t>& p1,
                            const nd::VHACD::Vect3<int32_t>& p2,
                            const nd::VHACD::Vect3<int32_t>& p3)
    {
        uint32_t i1 = getVertexIndex(p1);
        uint32_t i2 = getVertexIndex(p2);
        uint32_t i3 = getVertexIndex(p3);

        mIndices.emplace_back(i1, i2, i3);
    }

    // Used only for debugging. Saves the voxelized mesh to disk
    // Optionally saves the original source mesh as well for comparison
    void saveVoxelMesh(const SimpleMesh &inputMesh,
                       bool saveVoxelMesh,
                       bool saveSourceMesh)
    {
        char scratch[512];
        snprintf(scratch,sizeof(scratch),"voxel-mesh-%03d.obj", mIndex);
        FILE *fph = fopen(scratch,"wb");
        if ( fph )
        {
            uint32_t baseIndex = 1;
            if ( saveVoxelMesh )
            {
                for (size_t i = 0; i < mVertices.size(); i++)
                {
                    const VHACD::Vertex& p = mVertices[i];
                    fprintf(fph,
                            "v %0.9f %0.9f %0.9f\n",
                            p.mX,
                            p.mY,
                            p.mZ);
                    baseIndex++;
                }
                for (size_t i = 0; i < mIndices.size(); i++)
                {
                    const VHACD::Triangle& t = mIndices[i];
                    fprintf(fph,
                            "f %d %d %d\n",
                            t.mI0 + 1,
                            t.mI1 + 1,
                            t.mI2 + 1);
                }
            }
            if ( saveSourceMesh )
            {
                for (uint32_t i = 0; i < inputMesh.mVertices.size(); i++)
                {
                    const VHACD::Vertex& p = inputMesh.mVertices[i];
                    fprintf(fph,
                            "v %0.9f %0.9f %0.9f\n",
                            p.mX,
                            p.mY,
                            p.mZ);
                }
                for (uint32_t i = 0; i < inputMesh.mIndices.size(); i++)
                {
                    const VHACD::Triangle& idx = inputMesh.mIndices[i];
                    fprintf(fph,
                            "f %d %d %d\n",
                            idx.mI0 + baseIndex,
                            idx.mI1 + baseIndex,
                            idx.mI2 + baseIndex);
                }
            }
            fclose(fph);
        }
    }

    // When computing the split plane, we start by simply 
    // taking the midpoint of the longest side. However,
    // we can also search the surface and look for the greatest
    // spot of concavity and use that as the split location.
    // This will make the convex decomposition more efficient
    // as it will tend to cut across the greatest point of
    // concavity on the surface.
    SplitAxis computeSplitPlane(uint32_t &location)
    {
        SplitAxis ret = SplitAxis::X_AXIS_NEGATIVE;

        nd::VHACD::Vect3<uint32_t> d = m2 - m1;

        if ( d.getX() >= d.getY() && d.getX() >= d.getZ() )
        {
            ret = SplitAxis::X_AXIS_NEGATIVE;
            location = (m2.getX() + 1 + m1.getX()) / 2;
            uint32_t edgeLoc;
            if ( mParams.m_findBestPlane && findConcavityX(edgeLoc) )
            {
                location = edgeLoc;
            }
        }
        else if ( d.getY() >= d.getX() && d.getY() >= d.getZ() )
        {
            ret = SplitAxis::Y_AXIS_NEGATIVE;
            location = (m2.getY() + 1 + m1.getY()) / 2;
            uint32_t edgeLoc;
            if ( mParams.m_findBestPlane &&  findConcavityY(edgeLoc) )
            {
                location = edgeLoc;
            }
        }
        else
        {
            ret = SplitAxis::Z_AXIS_NEGATIVE;
            location = (m2.getZ() + 1 + m1.getZ()) / 2;
            uint32_t edgeLoc;
            if ( mParams.m_findBestPlane &&  findConcavityZ(edgeLoc) )
            {
                location = edgeLoc;
            }
        }

        return ret;
    }

    inline nd::VHACD::Vect3<double> getPosition(const nd::VHACD::Vect3<int32_t>& ip) const
    {
        return getPoint(ip.getX(),
                        ip.getY(),
                        ip.getZ(),
                        mVoxelScale,
                        mVoxelAdjust);
    }

    double raycast(const nd::VHACD::Vect3<int32_t>& p1,
                   const nd::VHACD::Vect3<int32_t>& p2) const
    {
        double ret;
        nd::VHACD::Vect3<double> from = getPosition(p1);
        nd::VHACD::Vect3<double> to = getPosition(p2);

        double outT;
        double faceSign;
        nd::VHACD::Vect3<double> hitLocation;
        if ( mRaycastMesh.raycast(from, to, outT, faceSign, hitLocation) )
        {
            ret = (from - hitLocation).GetNorm();
        }
        else
        {
            ret = 0; // if it doesn't hit anything, just assign it to zero.
        }

        return ret;
    }

//     bool findConcavity(uint32_t idx,
//                        uint32_t &splitLoc)
//     {
//         bool ret = false;
//
//         int32_t d = (m2[idx] - m1[idx]) + 1; // The length of the getX axis in voxel space
//
//         uint32_t idx1;
//         uint32_t idx2;
//         uint32_t idx3;
//         switch (idx)
//         {
//             case 0: // X
//                 idx1 = 0;
//                 idx2 = 1;
//                 idx3 = 2;
//                 break;
//             case 1: // Y
//                 idx1 = 1;
//                 idx2 = 0;
//                 idx3 = 2;
//                 break;
//             case 2:
//                 idx1 = 2;
//                 idx2 = 1;
//                 idx3 = 0;
//                 break;
//         }
//
//         // We will compute the edge error on the XY plane and the XZ plane
//         // searching for the greatest location of concavity
//         std::vector<double> edgeError1 = std::vector<double>(d);
//         std::vector<double> edgeError2 = std::vector<double>(d);
//
//         // Counter of number of voxel samples on the XY plane we have accumulated
//         uint32_t indexZ = 0;
//
//         // Compute Edge Error on the XY plane
//         for (int32_t x=(int32_t)m1[idx1]; x<=(int32_t)m2[idx1]; x++)
//         {
//             double errorTotal = 0;
//             // We now perform a raycast from the sides inward on the XY plane to
//             // determine the total error (distance of the surface from the sides)
//             // along this getX position.
//             for (int32_t y=(int32_t)m1[idx2]; y<=(int32_t)m2[idx2]; y++)
//             {
//                 IVec3 p1(x, y, m1[idx3] - 2);
//                 IVec3 p2(x, y, m2[idx3] + 2);
//
//                 double e1 = raycast(p1, p2);
//                 double e2 = raycast(p2, p1);
//
//                 errorTotal = errorTotal+e1+e2;
//             }
//             // The total amount of edge error along this voxel location
//             edgeError1[indexZ] = errorTotal;
//             indexZ++;
//         }
//
//         // Compute edge error along the XZ plane
//         uint32_t indexY = 0;
//
//         for (int32_t x=(int32_t)m1[idx1]; x<=(int32_t)m2[idx1]; x++)
//         {
//             double errorTotal = 0;
//
//             for (int32_t z=(int32_t)m1[idx3]; z<=(int32_t)m2[idx3]; z++)
//             {
//                 IVec3 p1(x, m1[idx2] - 2, z);
//                 IVec3 p2(x, m2[idx2] + 2, z);
//
//                 double e1 = raycast(p1, p2); // raycast from one side to the interior
//                 double e2 = raycast(p2, p1); // raycast from the other side to the interior
//
//                 errorTotal = errorTotal+e1+e2;
//             }
//             edgeError2[indexY] = errorTotal;
//             indexY++;
//         }
//
//
//         // we now compute the first derivitave to find the greatest spot of concavity on the XY plane
//         double maxDiff = 0;
//         uint32_t maxC = 0;
//         int32_t wid = (m2[idx1] - m1[idx1]) / 2 + 1;
//         for (uint32_t x=1; x<indexZ; x++)
//         {
//             if ( edgeError1[x] > 0 &&  edgeError1[x-1] > 0 )
//             {
//                 double diff = abs(edgeError1[x] - edgeError1[x-1]);
//                 if ( diff > maxDiff )
//                 {
//                     maxDiff = diff;
//                     maxC = x-1;
//                 }
//             }
//         }
//
//         // Now see if there is a greater concavity on the XZ plane
//         for (uint32_t x=1; x<indexY; x++)
//         {
//             if ( edgeError2[x] > 0 && edgeError2[x-1] > 0 )
//             {
//                 double diff = abs(edgeError2[x] - edgeError2[x-1]);
//                 if ( diff > maxDiff )
//                 {
//                     maxDiff = diff;
//                     maxC = x-1;
//                 }
//             }
//         }
//
//         splitLoc = maxC + m1[idx1];
//
//         // we do not allow an edge split if it is too close to the ends
//         if ( splitLoc > (m1[idx1] + 4) && splitLoc < (m2[idx1] - 4) )
//         {
//             ret = true;
//         }
//
//
//         return ret;
//     }

    // Finding the greatest area of concavity..
    bool findConcavityX(uint32_t &splitLoc)
    {
//         return findConcavity(0, splitLoc);
        bool ret = false;

        uint32_t dx = (m2.getX() - m1.getX()) + 1; // The length of the getX axis in voxel space

        // We will compute the edge error on the XY plane and the XZ plane
        // searching for the greatest location of concavity

        std::vector<double> edgeErrorZ = std::vector<double>(dx);
        std::vector<double> edgeErrorY = std::vector<double>(dx);

        // Counter of number of voxel samples on the XY plane we have accumulated
        uint32_t indexZ = 0;

        // Compute Edge Error on the XY plane
        for (uint32_t x = m1.getX(); x <= m2.getX(); x++)
        {
            double errorTotal = 0;
            // We now perform a raycast from the sides inward on the XY plane to
            // determine the total error (distance of the surface from the sides)
            // along this getX position.
            for (uint32_t y = m1.getY(); y <= m2.getY(); y++)
            {
                nd::VHACD::Vect3<int32_t> p1(x, y, m1.getZ() - 2);
                nd::VHACD::Vect3<int32_t> p2(x, y, m2.getZ() + 2);

                double e1 = raycast(p1, p2);
                double e2 = raycast(p2, p1);

                errorTotal = errorTotal + e1 + e2;
            }
            // The total amount of edge error along this voxel location
            edgeErrorZ[indexZ] = errorTotal;
            indexZ++;
        }

        // Compute edge error along the XZ plane
        uint32_t indexY = 0;

        for (uint32_t x = m1.getX(); x <= m2.getX(); x++)
        {
            double errorTotal = 0;

            for (uint32_t z = m1.getZ(); z <= m2.getZ(); z++)
            {
                nd::VHACD::Vect3<int32_t> p1(x, m1.getY() - 2, z);
                nd::VHACD::Vect3<int32_t> p2(x, m2.getY() + 2, z);

                double e1 = raycast(p1, p2); // raycast from one side to the interior
                double e2 = raycast(p2, p1); // raycast from the other side to the interior

                errorTotal = errorTotal + e1 + e2;
            }
            edgeErrorY[indexY] = errorTotal;
            indexY++;
        }


        // we now compute the first derivitave to find the greatest spot of concavity on the XY plane
        double maxDiff = 0;
        uint32_t maxC = 0;
        for (uint32_t x = 1; x < indexZ; x++)
        {
            if ( edgeErrorZ[x] > 0 &&  edgeErrorZ[x - 1] > 0 )
            {
                double diff = abs(edgeErrorZ[x] - edgeErrorZ[x - 1]);
                if ( diff > maxDiff )
                {
                    maxDiff = diff;
                    maxC = x-1;
                }
            }
        }
        // Now see if there is a greater concavity on the XZ plane
        for (uint32_t x = 1; x < indexY; x++)
        {
            if ( edgeErrorY[x] > 0 && edgeErrorY[x - 1] > 0 )
            {
                double diff = abs(edgeErrorY[x] - edgeErrorY[x - 1]);
                if ( diff > maxDiff )
                {
                    maxDiff = diff;
                    maxC = x-1;
                }
            }
        }

        splitLoc = maxC + m1.getX();

        // we do not allow an edge split if it is too close to the ends
        if ( splitLoc > (m1.getX() + 4) && splitLoc < (m2.getX() - 4) )
        {
            ret = true;
        }

        return ret;
    }


    // Finding the greatest area of concavity..
    bool findConcavityY(uint32_t &splitLoc)
    {
//         return findConcavity(1, splitLoc);
        bool ret = false;

        uint32_t dy = (m2.getY() - m1.getY()) + 1; // The length of the getX axis in voxel space

        // We will compute the edge error on the XY plane and the XZ plane
        // searching for the greatest location of concavity

        std::vector<double> edgeErrorZ = std::vector<double>(dy);
        std::vector<double> edgeErrorX = std::vector<double>(dy);

        // Counter of number of voxel samples on the XY plane we have accumulated
        uint32_t indexZ = 0;

        // Compute Edge Error on the XY plane
        for (uint32_t y = m1.getY(); y <= m2.getY(); y++)
        {
            double errorTotal = 0;
            // We now perform a raycast from the sides inward on the XY plane to
            // determine the total error (distance of the surface from the sides)
            // along this getX position.
            for (uint32_t x = m1.getX(); x <= m2.getX(); x++)
            {
                nd::VHACD::Vect3<int32_t> p1(x, y, m1.getZ() - 2);
                nd::VHACD::Vect3<int32_t> p2(x, y, m2.getZ() + 2);

                double e1 = raycast(p1, p2);
                double e2 = raycast(p2, p1);

                errorTotal = errorTotal + e1 + e2;
            }
            // The total amount of edge error along this voxel location
            edgeErrorZ[indexZ] = errorTotal;
            indexZ++;
        }

        // Compute edge error along the XZ plane
        uint32_t indexX = 0;

        for (uint32_t y = m1.getY(); y <= m2.getY(); y++)
        {
            double errorTotal = 0;

            for (uint32_t z = m1.getZ(); z <= m2.getZ(); z++)
            {
                nd::VHACD::Vect3<int32_t> p1(m1.getX() - 2, y, z);
                nd::VHACD::Vect3<int32_t> p2(m2.getX() + 2, y, z);

                double e1 = raycast(p1, p2); // raycast from one side to the interior
                double e2 = raycast(p2, p1); // raycast from the other side to the interior

                errorTotal = errorTotal + e1 + e2;
            }
            edgeErrorX[indexX] = errorTotal;
            indexX++;
        }

        // we now compute the first derivitave to find the greatest spot of concavity on the XY plane
        double maxDiff = 0;
        uint32_t maxC = 0;
        for (uint32_t y = 1; y < indexZ; y++)
        {
            if ( edgeErrorZ[y] > 0 && edgeErrorZ[y - 1] > 0 )
            {
                double diff = abs(edgeErrorZ[y] - edgeErrorZ[y - 1]);
                if ( diff > maxDiff )
                {
                    maxDiff = diff;
                    maxC = y-1;
                }
            }
        }
        // Now see if there is a greater concavity on the XZ plane
        for (uint32_t y = 1; y < indexX; y++)
        {
            if ( edgeErrorX[y] >0 &&  edgeErrorX[y - 1] > 0 )
            {
                double diff = abs(edgeErrorX[y] - edgeErrorX[y - 1]);
                if ( diff > maxDiff )
                {
                    maxDiff = diff;
                    maxC = y - 1;
                }
            }
        }

        splitLoc = maxC + m1.getY();

        // we do not allow an edge split if it is too close to the ends
        if ( splitLoc > (m1.getY() + 4) && splitLoc < (m2.getY() - 4) )
        {
            ret = true;
        }

        return ret;
    }

    // Finding the greatest area of concavity..
    bool findConcavityZ(uint32_t &splitLoc)
    {
//         return findConcavity(2, splitLoc);
        bool ret = false;

        int32_t dz = (m2.getZ() - m1.getZ()) + 1; // The length of the getX axis in voxel space

        // We will compute the edge error on the XY plane and the XZ plane
        // searching for the greatest location of concavity
        std::vector<double> edgeErrorX = std::vector<double>(dz);
        std::vector<double> edgeErrorY = std::vector<double>(dz);

        // Counter of number of voxel samples on the XY plane we have accumulated
        uint32_t indexX = 0;

        // Compute Edge Error on the XY plane
        for (uint32_t z = m1.getZ(); z <= m2.getZ(); z++)
        {
            double errorTotal = 0;
            // We now perform a raycast from the sides inward on the XY plane to
            // determine the total error (distance of the surface from the sides)
            // along this getX position.
            for (uint32_t y = m1.getY(); y <= m2.getY(); y++)
            {
                nd::VHACD::Vect3<int32_t> p1(m1.getX() - 2, y, z);
                nd::VHACD::Vect3<int32_t> p2(m2.getX() + 1, y, z);

                double e1 = raycast(p1, p2);
                double e2 = raycast(p2, p1);

                errorTotal = errorTotal + e1 + e2;
            }
            // The total amount of edge error along this voxel location
            edgeErrorX[indexX] = errorTotal;
            indexX++;
        }

        // Compute edge error along the XZ plane
        uint32_t indexY = 0;

        for (uint32_t z = m1.getZ(); z <= m2.getZ(); z++)
        {
            double errorTotal = 0;

            for (uint32_t x = m1.getX(); x <= m2.getX(); x++)
            {
                nd::VHACD::Vect3<int32_t> p1(x, m1.getY() - 2, z);
                nd::VHACD::Vect3<int32_t> p2(x, m2.getY() + 2, z);

                double e1 = raycast(p1, p2); // raycast from one side to the interior
                double e2 = raycast(p2, p1); // raycast from the other side to the interior

                errorTotal = errorTotal + e1 + e2;
            }
            edgeErrorY[indexY] = errorTotal;
            indexY++;
        }


        // we now compute the first derivitave to find the greatest spot of concavity on the XY plane
        double maxDiff = 0;
        uint32_t maxC = 0;
        for (uint32_t z = 1; z < indexX; z++)
        {
            if ( edgeErrorX[z] > 0 && edgeErrorX[z - 1] > 0 )
            {
                double diff = abs(edgeErrorX[z] - edgeErrorX[z - 1]);
                if ( diff > maxDiff )
                {
                    maxDiff = diff;
                    maxC = z - 1;
                }
            }
        }
        // Now see if there is a greater concavity on the XZ plane
        for (uint32_t z=1; z<indexY; z++)
        {
            if ( edgeErrorY[z] > 0 && edgeErrorY[z - 1] > 0 )
            {
                double diff = abs(edgeErrorY[z] - edgeErrorY[z - 1]);
                if ( diff > maxDiff )
                {
                    maxDiff = diff;
                    maxC = z-1;
                }
            }
        }

        splitLoc = maxC + m1.getX();

        // we do not allow an edge split if it is too close to the ends
        if ( splitLoc > (m1.getZ() + 4) && splitLoc < (m2.getZ() - 4) )
        {
            ret = true;
        }

        return ret;
    }

    // This operation is performed in a background thread.
    //It splits the voxels by a plane
    void performPlaneSplit(void)
    {
        if ( isComplete() )
        {
        }
        else
        {
            uint32_t splitLoc;
            SplitAxis axis = computeSplitPlane(splitLoc);
            switch ( axis )
            {
                case SplitAxis::X_AXIS_NEGATIVE:
                case SplitAxis::X_AXIS_POSITIVE:
                    // Split on the getX axis at this split location
                    mHullA = new VoxelHull(*this,SplitAxis::X_AXIS_NEGATIVE,splitLoc);
                    mHullB = new VoxelHull(*this,SplitAxis::X_AXIS_POSITIVE,splitLoc);
                    break;
                case SplitAxis::Y_AXIS_NEGATIVE:
                case SplitAxis::Y_AXIS_POSITIVE:
                    // Split on the 1 axis at this split location
                    mHullA = new VoxelHull(*this,SplitAxis::Y_AXIS_NEGATIVE,splitLoc);
                    mHullB = new VoxelHull(*this,SplitAxis::Y_AXIS_POSITIVE,splitLoc);
                    break;
                case SplitAxis::Z_AXIS_NEGATIVE:
                case SplitAxis::Z_AXIS_POSITIVE:
                    // Split on the getZ axis at this split location
                    mHullA = new VoxelHull(*this,SplitAxis::Z_AXIS_NEGATIVE,splitLoc);
                    mHullB = new VoxelHull(*this,SplitAxis::Z_AXIS_POSITIVE,splitLoc);
                    break;
            }
        }
    }

    void saveOBJ(const char *fname,
                 const VoxelHull *h)
    {
        FILE *fph = fopen(fname,"wb");
        if ( fph )
        {
            uint32_t baseIndex = 1;
            for (size_t i = 0; i < mVertices.size(); ++i)
            {
                const VHACD::Vertex& v = mVertices[i];
                fprintf(fph, "v %0.9f %0.9f %0.9f\n",
                        v.mX,
                        v.mY,
                        v.mZ);
            }

            for (size_t i = 0; i < mIndices.size(); ++i)
            {
                const VHACD::Triangle& t = mIndices[i];
                fprintf(fph, "f %d %d %d\n",
                        t.mI0 + baseIndex,
                        t.mI1 + baseIndex,
                        t.mI2 + baseIndex);
            }

            baseIndex += uint32_t(mVertices.size());

            for (size_t i = 0; i < h->mVertices.size(); ++i)
            {
                const VHACD::Vertex& v = h->mVertices[i];
                fprintf(fph, "v %0.9f %0.9f %0.9f\n",
                        v.mX,
                        v.mY + 0.1,
                        v.mZ);
            }

            for (size_t i = 0; i < h->mIndices.size(); ++i)
            {
                const VHACD::Triangle& t = h->mIndices[i];
                fprintf(fph, "f %d %d %d\n",
                        t.mI0 + baseIndex,
                        t.mI1 + baseIndex,
                        t.mI2 + baseIndex);
            }
            fclose(fph);
        }
    }

    void saveOBJ(const char *fname)
    {
        FILE *fph = fopen(fname, "wb");
        if ( fph )
        {
            printf("Saving '%s' with %d vertices and %d triangles\n",
                   fname,
                   uint32_t(mVertices.size()),
                   uint32_t(mIndices.size()));
            for (size_t i = 0; i < mVertices.size(); ++i)
            {
                const VHACD::Vertex& v = mVertices[i];
                fprintf(fph, "v %0.9f %0.9f %0.9f\n",
                        v.mX,
                        v.mY,
                        v.mZ);
            }

            for (size_t i = 0; i < mIndices.size(); ++i)
            {
                const VHACD::Triangle& t = mIndices[i];
                fprintf(fph, "f %d %d %d\n",
                        t.mI0 + 1,
                        t.mI1 + 1,
                        t.mI2 + 1);
            }
            fclose(fph);
        }
    }

    SplitAxis           mAxis{SplitAxis::X_AXIS_NEGATIVE};
    VoxelizeImpl        *mVoxels{nullptr};  // The voxelized data set
    double              mVoxelScale{0};     // Size of a single voxel
    double              mVoxelScaleHalf{0}; // 1/2 of the size of a single voxel
    nd::VHACD::Vect3<double> mVoxelAdjust;    // Minimum coordinates of the voxel space, with adjustment
    nd::VHACD::Vect3<double> mVoxelBmin;      // Minimum coordinates of the voxel space
    nd::VHACD::Vect3<double> mVoxelBmax;      // Maximum coordinates of the voxel space
    uint32_t            mDepth{0};          // How deep in the recursion of the binary tree this hull is
    uint32_t            mIndex{0};          // Each convex hull is given a unique id to distinguish it from the others
    double              mVolumeError{0};    // The percentage error from the convex hull volume vs. the voxel volume
    double              mVoxelVolume{0};    // The volume of the voxels
    double              mHullVolume{0};     // The volume of the enclosing convex hull

    IVHACD::ConvexHull  *mConvexHull{nullptr}; // The convex hull which encloses this set of voxels.
    VoxelVector         mSurfaceVoxels; // The voxels which are on the surface of the source mesh.
    VoxelVector         mNewSurfaceVoxels;    // Voxels which are on the surface as a result of a plane split
    VoxelVector         mInteriorVoxels;      // Voxels which are part of the interior of the hull

    VoxelHull           *mHullA{nullptr};   // hull resulting from one side of the plane split
    VoxelHull           *mHullB{nullptr};   // hull resulting from the other side of the plane split

    // Defines the coordinates this convex hull comprises within the voxel volume
    // of the entire source
    nd::VHACD::Vect3<uint32_t> m1{0};
    nd::VHACD::Vect3<uint32_t> m2{0};
    MyRaycastMesh           mRaycastMesh;
    VoxelIndexMap           mVoxelIndexMap; // Maps from a voxel coordinate space into a vertex index space
    std::vector<VHACD::Vertex> mVertices;
    std::vector<VHACD::Triangle> mIndices;
    static uint32_t         mVoxelHullCount;
    IVHACD::Parameters      mParams;
    VHACDCallbacks          *mCallbacks{nullptr};
};

uint32_t VoxelHull::mVoxelHullCount=0;

using VoxelHullVector = std::vector< VoxelHull * >;
using VoxelHullQueue = std::queue< VoxelHull * >;

class VHACDImpl;

// This class represents a single task to compute the volme error
// of two convex hulls combined
class CostTask
{
public:
    VHACDImpl   *mThis{nullptr};
    IVHACD::ConvexHull  *mHullA{nullptr};
    IVHACD::ConvexHull  *mHullB{nullptr};
    double      mConcavity{0}; // concavity of the two combineds
    std::future<void>   mFuture;
};

void computeMergeCostTask(void *ptr);

class HullPair
{
public:
    HullPair(void) { };
    HullPair(uint32_t hullA,uint32_t hullB,double concavity) : mHullA(hullA),mHullB(hullB),mConcavity(concavity)
    {
    }

    bool operator<(const HullPair &h) const
    {
        return mConcavity > h.mConcavity ? true : false;
    }

    uint32_t    mHullA{0};
    uint32_t    mHullB{0};
    double      mConcavity{0};
};

using HullPairQueue = std::priority_queue< HullPair >;
using HullMap = std::unordered_map< uint32_t, IVHACD::ConvexHull * >;

void jobCallback(void *userPtr);

// Don't consider more than 100,000 convex hulls.
#define VHACD_MAX_CONVEX_HULL_FRAGMENTS 100000

class VHACDImpl : public IVHACD, public VHACDCallbacks
{
public:
    VHACDImpl() = default;

    ~VHACDImpl() override
    {
        Clean();
    }

    void Cancel() override final
    {
        mCanceled = true;
    }

    bool Compute(const float* const points,
                 const uint32_t countPoints,
                 const uint32_t* const triangles,
                 const uint32_t countTriangles,
                 const Parameters& params) override final
    {
        std::vector<VHACD::Vertex> v;
        v.reserve(countPoints);
        for (uint32_t i = 0; i < countPoints; ++i)
        {
            v.emplace_back(points[i * 3 + 0],
                           points[i * 3 + 1],
                           points[i * 3 + 2]);
        }

        std::vector<VHACD::Triangle> t;
        t.reserve(countTriangles);
        for (uint32_t i = 0; i < countTriangles; ++i)
        {
            t.emplace_back(triangles[i * 3 + 0],
                           triangles[i * 3 + 1],
                           triangles[i * 3 + 2]);
        }

        return Compute(v, t, params);
    }

    bool Compute(const double* const points,
                 const uint32_t countPoints,
                 const uint32_t* const triangles,
                 const uint32_t countTriangles,
                 const Parameters& params) override final
    {
        std::vector<VHACD::Vertex> v;
        v.reserve(countPoints);
        for (uint32_t i = 0; i < countPoints; ++i)
        {
            v.emplace_back(points[i * 3 + 0],
                           points[i * 3 + 1],
                           points[i * 3 + 2]);
        }

        std::vector<VHACD::Triangle> t;
        t.reserve(countTriangles);
        for (uint32_t i = 0; i < countTriangles; ++i)
        {
            t.emplace_back(triangles[i * 3 + 0],
                           triangles[i * 3 + 1],
                           triangles[i * 3 + 2]);
        }

        return Compute(v, t, params);
    }

    bool Compute(const std::vector<VHACD::Vertex>& points,
                 const std::vector<VHACD::Triangle>& triangles,
                 const Parameters& params)
    {
        bool ret = false;

        mParams = params;
        mCanceled = false;

        Clean(); // release any previous results
#if !VHACD_DISABLE_THREADING
        if ( mParams.m_asyncACD )
        {
            mThreadPool = new ThreadPool(8);
        }
#endif
        copyInputMesh(points,
                      triangles);
        if ( !mCanceled )
        {
            // We now recursively perform convex decomposition until complete
            performConvexDecomposition();
        }

        if ( mCanceled )
        {
            Clean();
            ret = false;
            if ( mParams.m_logger )
            {
                mParams.m_logger->Log("VHACD operation canceled before it was complete.");
            }
        }
        else
        {
            ret = true;
        }
#if !VHACD_DISABLE_THREADING
        delete mThreadPool;
        mThreadPool = nullptr;
#endif
        return ret;
    }

    uint32_t GetNConvexHulls() const override final
    {
        return uint32_t(mConvexHulls.size());
    }

    bool GetConvexHull(const uint32_t index, ConvexHull& ch) const override final
    {
        bool ret = false;

        if ( index < uint32_t(mConvexHulls.size() ))
        {
            ch = *mConvexHulls[index];
            ret = true;
        }

        return ret;
    }

    void Clean() override final  // release internally allocated memory
    {
#if !VHACD_DISABLE_THREADING
        delete mThreadPool;
        mThreadPool = nullptr;
#endif

        mTrees.clear();

        for (auto &ch:mConvexHulls)
        {
            releaseConvexHull(ch);
        }
        mConvexHulls.clear();

        for (auto &ch:mHulls)
        {
            releaseConvexHull(ch.second);
        }
        mHulls.clear();

        for (auto &ch:mVoxelHulls)
        {
            delete ch;
        }
        mVoxelHulls.clear();

        for (auto &ch:mPendingHulls)
        {
            delete ch;
        }
        mPendingHulls.clear();

        mVertices.clear();
        mIndices.clear();
    }

    void Release(void) override final
    {
        delete this;
    }

    // Will compute the center of mass of the convex hull decomposition results and return it
    // in 'centerOfMass'.  Returns false if the center of mass could not be computed.
    bool ComputeCenterOfMass(double centerOfMass[3]) const override final
    {
        bool ret = false;

        return ret;
    }

    // In synchronous mode (non-multi-threaded) the state is always 'ready'
    // In asynchronous mode, this returns true if the background thread is not still actively computing
    // a new solution.  In an asynchronous config the 'IsReady' call will report any update or log
    // messages in the caller's current thread.
    bool IsReady(void) const override final
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
    uint32_t findNearestConvexHull(const double pos[3],
                                   double &distanceToHull) override final
    {
        uint32_t ret = 0; // The default return code is zero

        uint32_t hullCount = GetNConvexHulls();
        distanceToHull = 0;
        // First, make sure that we have valid and completed results
        if ( hullCount )
        {
            // See if we already have AABB trees created for each convex hull
            if ( mTrees.empty() )
            {
                // For each convex hull, we generate an AABB tree for fast closest point queries
                for (uint32_t i=0; i<hullCount; i++)
                {
                    VHACD::IVHACD::ConvexHull ch;
                    GetConvexHull(i,ch);
                    // Pass the triangle mesh to create an AABB tree instance based on it.
                    AABBTreeImpl *t = new AABBTreeImpl(ch.m_points,
                                                       ch.m_triangles);
                    // Save the AABB tree into the container 'mTrees'
                    mTrees.push_back(t);
                }
            }
            // We now compute the closest point to each convex hull and save the nearest one
            double closest = 1e99;
            for (uint32_t i=0; i<hullCount; i++)
            {
                AABBTreeImpl *t = mTrees[i];
                if ( t )
                {
                    nd::VHACD::Vect3<double> closestPoint;
                    if ( t->getClosestPointWithinDistance(*(const nd::VHACD::Vect3<double>*)pos,1e99,closestPoint))
                    {
                        double dx = pos[0] - closestPoint[0];
                        double dy = pos[1] - closestPoint[1];
                        double dz = pos[2] - closestPoint[2];
                        double distanceSquared = dx*dx + dy*dy + dz*dz;
                        if ( distanceSquared < closest )
                        {
                            closest = distanceSquared;
                            ret = i;
                        }
                    }
                }
            }
            distanceToHull = sqrt(closest); // compute the distance to the nearest convex hull
        }

        return ret;

    }

    // Take the source position, normalize it, and then convert it into an index position
    uint32_t getIndex(VHACD::MyVertexIndex& vi,
                      const VHACD::Vertex& p)
    {
        nd::VHACD::Vect3<double> pos;
        pos[0] = (p.mX - mCenter[0]) * mRecipScale;
        pos[1] = (p.mY - mCenter[1]) * mRecipScale;
        pos[2] = (p.mZ - mCenter[2]) * mRecipScale;
        bool newPos;
        uint32_t ret = vi.getIndex(pos,newPos);
        return ret;
    }


    // This copies the input mesh while scaling the input positions
    // to fit into a normalized unit cube. It also re-indexes all of the
    // vertex positions in case they weren't clean coming in. 
    void copyInputMesh(const std::vector<VHACD::Vertex>& points,
                       const std::vector<VHACD::Triangle>& triangles)
    {
        mVertices.clear();
        mIndices.clear();
        mIndices.reserve(triangles.size());

        // First we must find the bounding box of this input vertices and normalize them into a unit-cube
        nd::VHACD::Vect3<double> bmin(FLT_MAX);
        nd::VHACD::Vect3<double> bmax(-FLT_MAX);
        progressUpdate(Stages::COMPUTE_BOUNDS_OF_INPUT_MESH,
                       0,
                       "ComputingBounds");
        for (uint32_t i = 0; i < points.size(); i++)
        {
            const VHACD::Vertex& p = points[i];

            bmin = bmin.CWiseMin(p);
            bmax = bmax.CWiseMax(p);
        }
        progressUpdate(Stages::COMPUTE_BOUNDS_OF_INPUT_MESH,
                       100,
                       "ComputingBounds");

        mCenter = (bmax + bmin) * 0.5;

        nd::VHACD::Vect3<double> scale = bmax - bmin;
        mScale = Max(Max(scale.getX(), scale.getY()), scale.getZ());

        mRecipScale = mScale > 0 ? 1.0 / mScale : 0;

        {
            VHACD::MyVertexIndex vi = VHACD::MyVertexIndex(0.001, false);

            uint32_t dcount=0;

            for (uint32_t i = 0; i < triangles.size() && !mCanceled; ++i)
            {
                const VHACD::Triangle& t = triangles[i];
                const VHACD::Vertex& p1 = points[t.mI0];
                const VHACD::Vertex& p2 = points[t.mI1];
                const VHACD::Vertex& p3 = points[t.mI2];

                uint32_t i1 = getIndex(vi, p1);
                uint32_t i2 = getIndex(vi, p2);
                uint32_t i3 = getIndex(vi, p3);

                if ( i1 == i2 || i1 == i3 || i2 == i3 )
                {
                    dcount++;
                }
                else
                {
                    mIndices.emplace_back(i1, i2, i3);
                }
            }
            if ( dcount )
            {
                if ( mParams.m_logger )
                {
                    char scratch[512];
                    snprintf(scratch,
                             sizeof(scratch),
                             "Skipped %d degenerate triangles", dcount);
                    mParams.m_logger->Log(scratch);
                }
            }

            mVertices = vi.takeVertices();
        }

        // Create the raycast mesh
        if ( !mCanceled )
        {
            progressUpdate(Stages::CREATE_RAYCAST_MESH,
                           0,
                           "Building RaycastMesh");
            mRaycastMesh = VHACD::MyRaycastMesh(mVertices,
                                                mIndices);
            progressUpdate(Stages::CREATE_RAYCAST_MESH,
                           100,
                           "RaycastMesh completed");
        }
        if ( !mCanceled )
        {
            progressUpdate(Stages::VOXELIZING_INPUT_MESH,
                           0,
                           "Voxelizing Input Mesh");
            mVoxelize = VHACD::VoxelizeImpl();
            mVoxelize.voxelize(mRaycastMesh,
                               mVertices,
                               mIndices,
                               mParams.m_resolution,
                               mParams.m_fillMode);
            mVoxelScale = mVoxelize.getScale();
            mVoxelHalfScale = mVoxelScale * 0.5;
            mVoxelBmin = mVoxelize.getBoundsMin();
            mVoxelBmax = mVoxelize.getBoundsMax();
            progressUpdate(Stages::VOXELIZING_INPUT_MESH,
                           100,
                           "Voxelization complete");
        }

        mInputMesh.mVertices = mVertices;
        mInputMesh.mIndices = mIndices;
        if ( !mCanceled )
        {
            progressUpdate(Stages::BUILD_INITIAL_CONVEX_HULL,
                           0,
                           "Build initial ConvexHull");
            VoxelHull *vh = new VoxelHull(mVoxelize,
                                          mInputMesh,
                                          mParams,
                                          this);
            if ( vh->mConvexHull )
            {
                mOverallHullVolume = vh->mConvexHull->m_volume;
            }
            mPendingHulls.push_back(vh);
            progressUpdate(Stages::BUILD_INITIAL_CONVEX_HULL,
                           100,
                           "Initial ConvexHull complete");
        }
    }

    void scaleOutputConvexHull(ConvexHull &ch)
    {
        for (uint32_t i = 0; i < ch.m_points.size(); i++)
        {
            VHACD::Vertex& p = ch.m_points[i];
            p.mX = (p.mX * mScale) + mCenter[0];
            p.mY = (p.mY * mScale) + mCenter[1];
            p.mZ = (p.mZ * mScale) + mCenter[2];
        }
        ch.m_volume = computeConvexHullVolume(ch); // get the combined volume
        fm_getAABB(ch.m_points,
                   ch.mBmin,
                   ch.mBmax);
        fm_computeCentroid(ch.m_points,
                           ch.m_triangles,
                           ch.m_center);
    }

    void addCostToPriorityQueue(CostTask *task)
    {
        HullPair hp(task->mHullA->m_meshId,
                    task->mHullB->m_meshId,
                    task->mConcavity);
        mHullPairQueue.push(hp);
    }

    void releaseConvexHull(ConvexHull *ch)
    {
        if ( ch )
        {
            delete ch;
        }
    }

    void performConvexDecomposition(void)
    {
        {
            ScopedTime st("Convex Decompostion",
                          mParams.m_logger);
            double maxHulls = pow(2, mParams.m_maxRecursionDepth);
            // We recursively split convex hulls until we can
            // no longer recurse further.
            Timer t;

            while ( !mPendingHulls.empty() && !mCanceled )
            {
                size_t count = mPendingHulls.size() + mVoxelHulls.size();
                double e = t.peekElapsedSeconds();
                if ( e >= 0.1 )
                {
                    t.reset();
                    double stageProgress = (double(count) * 100.0) / maxHulls;
                    progressUpdate(Stages::PERFORMING_DECOMPOSITION,
                                   stageProgress,
                                   "Performing recursive decomposition of convex hulls");
                }
                // First we make a copy of the hulls we are processing
                VoxelHullVector oldList = mPendingHulls;
                // For each hull we want to split, we either
                // immediately perform the plane split or we post it as
                // a job to be performed in a background thread
                std::future<void> *futures = new std::future<void>[mPendingHulls.size()];
                uint32_t futureCount = 0;
                for (auto &i : mPendingHulls)
                {
                    if ( i->isComplete() || count > VHACD_MAX_CONVEX_HULL_FRAGMENTS )
                    {
                    }
                    else
                    {
#if !VHACD_DISABLE_THREADING
                        if ( mThreadPool )
                        {
                            futures[futureCount] = mThreadPool->enqueue([i]
                            {
                                jobCallback(i);
                            });
                            futureCount++;
                        }
                        else
#endif
                        {
                            i->performPlaneSplit();
                        }
                    }
                }
                // Wait for any outstanding jobs to complete in the background threads
                if ( futureCount )
                {
                    for (uint32_t i = 0; i < futureCount; i++)
                    {
                        futures[i].get();
                    }
                }
                delete []futures;
                // Now, we rebuild the pending convex hulls list by
                // adding the two children to the output list if
                // we need to recurse them further
                mPendingHulls.clear();
                for (auto &vh : oldList)
                {
                    if ( vh->isComplete() || count > VHACD_MAX_CONVEX_HULL_FRAGMENTS )
                    {
                        if ( vh->mConvexHull )
                        {
                            mVoxelHulls.push_back(vh);
                        }
                        else
                        {
                            delete vh;
                        }
                    }
                    else
                    {
                        if ( vh->mHullA )
                        {
                            mPendingHulls.push_back(vh->mHullA);
                        }
                        if ( vh->mHullB )
                        {
                            mPendingHulls.push_back(vh->mHullB);
                        }
                        vh->mHullA = nullptr;
                        vh->mHullB = nullptr;
                        delete vh;
                    }
                }
            }
        }

        if ( !mCanceled )
        {
            // Give each convex hull a unique guid
            mMeshId = 0;
            mHulls.clear();

            // Build the convex hull id map
            std::vector< ConvexHull *> hulls;

            progressUpdate(Stages::INITIALIZING_CONVEX_HULLS_FOR_MERGING,
                           0,
                           "Initializing ConvexHulls");

            for (auto &vh:mVoxelHulls)
            {
                if ( mCanceled )
                {
                    break;
                }
                ConvexHull *ch = copyConvexHull(*vh->mConvexHull);
                mMeshId++;
                ch->m_meshId = mMeshId;
                mHulls[mMeshId] = ch;
                // Compute the volume of the convex hull
                ch->m_volume = computeConvexHullVolume(*ch);
                // Compute the AABB of the convex hull
                fm_getAABB(ch->m_points,
                           ch->mBmin,
                           ch->mBmax);
                // Inflate the AABB by 10%
                fm_inflateMinMax(ch->mBmin,
                                 ch->mBmax,
                                 0.1);

                fm_computeCentroid(ch->m_points,
                                   ch->m_triangles,
                                   ch->m_center);

                delete vh;
                hulls.push_back(ch);
            }
            progressUpdate(Stages::INITIALIZING_CONVEX_HULLS_FOR_MERGING,
                           100,
                           "ConvexHull initialization complete");

            mVoxelHulls.clear();

            // here we merge convex hulls as needed until the match the
            // desired maximum hull count.
            size_t hullCount = hulls.size();

            if ( hullCount > mParams.m_maxConvexHulls && !mCanceled)
            {
                size_t costMatrixSize = ((hullCount*hullCount)-hullCount)>>1;
                CostTask *tasks = new CostTask[costMatrixSize];
                CostTask *task = tasks;
                ScopedTime st("Computing the Cost Matrix",
                              mParams.m_logger);
                // First thing we need to do is compute the cost matrix
                // This is computed as the volume error of any two convex hulls
                // combined
                progressUpdate(Stages::COMPUTING_COST_MATRIX,
                               0,
                               "Computing Hull Merge Cost Matrix");
                for (size_t i = 1; i < hullCount && !mCanceled; i++)
                {
                    ConvexHull *chA = hulls[i];

                    for (size_t j = 0; j < i && !mCanceled; j++)
                    {
                        ConvexHull *chB = hulls[j];

                        task->mHullA = chA;
                        task->mHullB = chB;
                        task->mThis = this;

                        if ( doFastCost(task) )
                        {
                        }
                        else
                        {
#if !VHACD_DISABLE_THREADING
                            if ( mThreadPool )
                            {
                                task->mFuture = mThreadPool->enqueue([task]
                                {
                                    computeMergeCostTask(task);
                                });
                            }
#endif
                            task++;
                        }
                    }
                }
                if ( !mCanceled )
                {
                    size_t taskCount = task - tasks;
#if !VHACD_DISABLE_THREADING
                    if ( mThreadPool )
                    {
                        if ( taskCount )
                        {
                            for (uint32_t i = 0; i < taskCount; i++)
                            {
                                tasks[i].mFuture.get();
                            }
                        }
                        for (size_t i = 0; i < taskCount; i++)
                        {
                            addCostToPriorityQueue(&tasks[i]);
                        }
                    }
                    else
#endif
                    {
                        task = tasks;
                        for (size_t i = 0; i < taskCount; i++)
                        {
                            performMergeCostTask(task);
                            addCostToPriorityQueue(task);
                            task++;
                        }
                    }
                    progressUpdate(Stages::COMPUTING_COST_MATRIX,
                                   100,
                                   "Finished cost matrix");
                }
                if ( !mCanceled )
                {
                    ScopedTime stMerging("Merging Convex Hulls",
                                         mParams.m_logger);
                    Timer t;
                    // Now that we know the cost to merge each hull, we can begin merging them.
                    bool cancel = false;

                    uint32_t maxMergeCount = uint32_t(mHulls.size()) - mParams.m_maxConvexHulls;
                    uint32_t startCount = uint32_t(mHulls.size());

                    while (   !cancel
                           && mHulls.size() > mParams.m_maxConvexHulls
                           && !mHullPairQueue.empty()
                           && !mCanceled)
                    {
                        double e = t.peekElapsedSeconds();
                        if ( e >= 0.1 )
                        {
                            t.reset();
                            uint32_t hullsProcessed = startCount - uint32_t(mHulls.size() );
                            double stageProgess = double(hullsProcessed * 100) / double(maxMergeCount);
                            progressUpdate(Stages::MERGING_CONVEX_HULLS,
                                           stageProgess,
                                           "Merging Convex Hulls");
                        }

                        HullPair hp = mHullPairQueue.top();
                        mHullPairQueue.pop();

                        // It is entirely possible that the hull pair queue can
                        // have references to convex hulls that are no longer valid 
                        // because they were previously merged. So we check for this
                        // and if either hull referenced in this pair no longer
                        // exists, then we skip it.

                        // Look up this pair of hulls by ID
                        ConvexHull *ch1 = getHull(hp.mHullA);
                        ConvexHull *ch2 = getHull(hp.mHullB);

                        // If both hulls are still valid, then we merge them, delete the old
                        // two hulls and recompute the cost matrix for the new combined hull
                        // we have created
                        if ( ch1 && ch2 )
                        {
                            // This is the convex hull which results from combining the
                            // vertices in the two source hulls 
                            ConvexHull *combinedHull = computeCombinedConvexHull(*ch1,
                                                                                 *ch2);
                            // The two old convex hulls are going to get removed
                            removeHull(hp.mHullA);
                            removeHull(hp.mHullB);

                            mMeshId++;
                            combinedHull->m_meshId = mMeshId;
                            CostTask *taskCost = tasks;

                            // Compute the cost between this new merged hull
                            // and all existing convex hulls and then 
                            // add that to the priority queue
                            for (auto &i : mHulls)
                            {
                                if ( mCanceled )
                                {
                                    break;
                                }
                                ConvexHull *secondHull = i.second;
                                taskCost->mHullA = combinedHull;
                                taskCost->mHullB = secondHull;
                                taskCost->mThis = this;
                                if ( doFastCost(taskCost) )
                                {
                                }
                                else
                                {
                                    taskCost++;
                                }
                            }
                            mHulls[combinedHull->m_meshId] = combinedHull;
                            // See how many merge cost tasks were posted
                            // If there are 8 or more and we are running asynchronously, then do them that way.
                            size_t tcount = taskCost - tasks;
#if !VHACD_DISABLE_THREADING
                            if ( mThreadPool && tcount >= 2 )
                            {
                                taskCost = tasks;
                                for (uint32_t i = 0; i < tcount; i++)
                                {
                                    taskCost->mFuture = mThreadPool->enqueue([taskCost]
                                    {
                                        computeMergeCostTask(taskCost);
                                    });
                                    taskCost++;
                                }
                                for (uint32_t i=0; i<tcount; i++)
                                {
                                    tasks[i].mFuture.get();
                                }
                            }
                            else
#endif
                            {
                                taskCost = tasks;
                                for (size_t i = 0; i < tcount; i++)
                                {
                                    performMergeCostTask(taskCost);
                                    taskCost++;
                                }
                            }
                            for (size_t i = 0; i < tcount; i++)
                            {
                                addCostToPriorityQueue(&tasks[i]);
                            }
                        }
                    }
                    // Ok...once we are done, we copy the results!
                    mMeshId -= 0;
                    progressUpdate(Stages::FINALIZING_RESULTS,
                                   0,
                                   "Finalizing results");
                    for (auto &i:mHulls)
                    {
                        if ( mCanceled )
                        {
                            break;
                        }
                        ConvexHull *ch = i.second;
                        // We now must reduce the convex hull 
                        if ( ch->m_points.size() > mParams.m_maxNumVerticesPerCH || mParams.m_shrinkWrap)
                        {
                            ConvexHull *reduce = computeReducedConvexHull(*ch,
                                                                          mParams.m_maxNumVerticesPerCH,
                                                                          mParams.m_shrinkWrap);
                            releaseConvexHull(ch);
                            ch = reduce;
                        }
                        scaleOutputConvexHull(*ch);
                        ch->m_meshId = mMeshId;
                        mMeshId++;
                        mConvexHulls.push_back(ch);
                    }
                    mHulls.clear(); // since the hulls were moved into the output list, we don't need to delete them from this container
                    progressUpdate(Stages::FINALIZING_RESULTS,
                                   100,
                                   "Finalized results complete");
                }
                delete []tasks;

            }
            else
            {
                progressUpdate(Stages::FINALIZING_RESULTS,
                               0,
                               "Finalizing results");
                mMeshId = 0;
                for (auto &ch:hulls)
                {
                    // We now must reduce the convex hull 
                    if ( ch->m_points.size() > mParams.m_maxNumVerticesPerCH  || mParams.m_shrinkWrap )
                    {
                        ConvexHull *reduce = computeReducedConvexHull(*ch,
                                                                      mParams.m_maxNumVerticesPerCH,
                                                                      mParams.m_shrinkWrap);
                        releaseConvexHull(ch);
                        ch = reduce;
                    }
                    scaleOutputConvexHull(*ch);
                    ch->m_meshId = mMeshId;
                    mMeshId++;
                    mConvexHulls.push_back(ch);
                }
                mHulls.clear();
                progressUpdate(Stages::FINALIZING_RESULTS,
                               100,
                               "Finalized results");
            }

        }
    }

    double computeConvexHullVolume(const ConvexHull &sm)
    {
        double totalVolume = 0;
        nd::VHACD::Vect3<double> bary(0, 0, 0);
        for (uint32_t i = 0; i < sm.m_points.size(); i++)
        {
            nd::VHACD::Vect3<double> p(sm.m_points[i]);
            bary += p;
        }
        bary /= double(sm.m_points.size());

        for (uint32_t i = 0; i < sm.m_triangles.size(); i++)
        {
            uint32_t i1 = sm.m_triangles[i].mI0;
            uint32_t i2 = sm.m_triangles[i].mI1;
            uint32_t i3 = sm.m_triangles[i].mI2;

            nd::VHACD::Vect3<double> ver0(sm.m_points[i1]);
            nd::VHACD::Vect3<double> ver1(sm.m_points[i2]);
            nd::VHACD::Vect3<double> ver2(sm.m_points[i3]);

            totalVolume += computeVolume4(ver0,
                                          ver1,
                                          ver2,
                                          bary);

        }
        totalVolume = totalVolume / 6.0;
        return totalVolume;
    }

    double computeVolume4(const nd::VHACD::Vect3<double>& a,
                          const nd::VHACD::Vect3<double>& b,
                          const nd::VHACD::Vect3<double>& c,
                          const nd::VHACD::Vect3<double>& d)
    {
        nd::VHACD::Vect3<double> ad = a - d;
        nd::VHACD::Vect3<double> bd = b - d;
        nd::VHACD::Vect3<double> cd = c - d;
        nd::VHACD::Vect3<double> bcd = bd.Cross(cd);
        double dot = ad.Dot(bcd);
        return dot;
    }

    double computeConcavity(double volumeSeparate,double volumeCombined,double volumeMesh)
    {
        return fabs(volumeSeparate - volumeCombined) / volumeMesh;
    }

    // See if we can compute the cost without having to actually merge convex hulls.
    // If the axis aligned bounding boxes (slightly inflated) of the two convex hulls
    // do not intersect, then we don't need to actually compute the merged convex hull
    // volume.
    inline bool doFastCost(CostTask *mt)
    {
        bool ret = false;

        ConvexHull *ch1 = mt->mHullA;
        ConvexHull *ch2 = mt->mHullB;

        double volume1 = ch1->m_volume;
        double volume2 = ch2->m_volume;
        double concavity = FLT_MAX;

        if ( !fm_intersectAABB(ch1->mBmin, ch1->mBmax, ch2->mBmin, ch2->mBmax))
        {
            nd::VHACD::Vect3<double> bmin;
            nd::VHACD::Vect3<double> bmax;
            fm_combineAABB(ch1->mBmin,
                           ch1->mBmax,
                           ch2->mBmin,
                           ch2->mBmax,
                           bmin,
                           bmax);
            double combinedVolume = fm_volumeAABB(bmin,
                                                  bmax);
            concavity = computeConcavity(volume1 + volume2,
                                         combinedVolume,
                                         mOverallHullVolume);
            HullPair hp(ch1->m_meshId,
                        ch2->m_meshId,
                        concavity);
            mHullPairQueue.push(hp);
            ret = true;
        }
        return ret;
    }

    void performMergeCostTask(CostTask *mt)
    {
        ConvexHull *ch1 = mt->mHullA;
        ConvexHull *ch2 = mt->mHullB;

        double volume1 = ch1->m_volume;
        double volume2 = ch2->m_volume;

        ConvexHull *combined = computeCombinedConvexHull(*ch1,
                                                         *ch2); // Build the combined convex hull
        double combinedVolume = computeConvexHullVolume(*combined); // get the combined volume
        mt->mConcavity = computeConcavity(volume1 + volume2,
                                          combinedVolume,
                                          mOverallHullVolume);
        releaseConvexHull(combined);
    }

    ConvexHull *computeReducedConvexHull(const ConvexHull &ch,
                                         uint32_t maxVerts,
                                         bool projectHullVertices)
    {
        SimpleMesh sourceConvexHull;

        sourceConvexHull.mVertices = ch.m_points;
        sourceConvexHull.mIndices = ch.m_triangles;

        ShrinkWrapImpl sw;
        sw.shrinkWrap(sourceConvexHull,
                      mRaycastMesh,
                      maxVerts,
                      mVoxelScale * 4,
                      projectHullVertices);

        ConvexHull *ret = new ConvexHull;

        ret->m_points = sourceConvexHull.mVertices;
        ret->m_triangles = sourceConvexHull.mIndices;

        fm_getAABB(ret->m_points,
                   ret->mBmin,
                   ret->mBmax);
        fm_inflateMinMax(ret->mBmin,
                         ret->mBmax,
                         0.1);
        fm_computeCentroid(ret->m_points,
                           ret->m_triangles,
                           ret->m_center);

        ret->m_volume = computeConvexHullVolume(*ret);

        // Return the convex hull 
        return ret;
    }

    // Take the points in convex hull A and the points in convex hull B and generate
    // a new convex hull on the combined set of points.
    // Once completed, we create a SimpleMesh instance to hold the triangle mesh
    // and we compute an inflated AABB for it.
    ConvexHull* computeCombinedConvexHull(const ConvexHull& sm1,
                                          const ConvexHull& sm2)
    {
        uint32_t vcount = uint32_t(sm1.m_points.size() + sm2.m_points.size()); // Total vertices from both hulls
        std::vector<VHACD::Vertex> vertices(vcount);
        auto it = std::copy(sm1.m_points.begin(),
                            sm1.m_points.end(),
                            vertices.begin());
        std::copy(sm2.m_points.begin(),
                  sm2.m_points.end(),
                  it);

        VHACD::QuickHullImpl qh;
        qh.computeConvexHull(vertices,
                             vcount);

        const std::vector<VHACD::Vertex>& hvertices = qh.getVertices();
        uint32_t hvcount = uint32_t(hvertices.size());
        const std::vector<VHACD::Triangle>& hindices = qh.getIndices();
        uint32_t htcount = uint32_t(hindices.size());

        ConvexHull *ret = new ConvexHull;
        ret->m_points.resize(hvcount);
        std::copy(hvertices.begin(),
                  hvertices.end(),
                  ret->m_points.begin());

        ret->m_triangles.resize(htcount);
        std::copy(hindices.begin(),
                  hindices.end(),
                  ret->m_triangles.begin());

        ret->m_volume = computeConvexHullVolume(*ret);

        fm_getAABB(hvertices,
                   ret->mBmin,
                   ret->mBmax);
        fm_inflateMinMax(ret->mBmin,
                         ret->mBmax,
                         0.1);
        fm_computeCentroid(ret->m_points,
                           ret->m_triangles,
                           ret->m_center);

        // Return the convex hull 
        return ret;
    }


    ConvexHull *getHull(uint32_t index)
    {
        ConvexHull *ret = nullptr;

        HullMap::iterator found = mHulls.find(index);
        if ( found != mHulls.end() )
        {
            ret = (*found).second;
        }

        return ret;
    }

    bool removeHull(uint32_t index)
    {
        bool ret = false;
        HullMap::iterator found = mHulls.find(index);
        if ( found != mHulls.end() )
        {
            ret = true;
            releaseConvexHull((*found).second);
            mHulls.erase(found);
        }
        return ret;
    }

    ConvexHull *copyConvexHull(const ConvexHull &source)
    {
        ConvexHull *ch = new ConvexHull;

        ch->mBmin = source.mBmin;
        ch->mBmax = source.mBmax;

        ch->m_center = source.m_center;

        ch->m_meshId = source.m_meshId;

        ch->m_points = source.m_points;

        ch->m_triangles = source.m_triangles;

        ch->m_volume = source.m_volume;

        return ch;
    }

    void progressUpdate(Stages stage,double stageProgress,
                        const char *operation)
    {
        if ( mParams.m_callback )
        {
            double overallProgress = (double(stage) * 100) / double(Stages::NUM_STAGES);
            const char *s = getStageName(stage);
            mParams.m_callback->Update(overallProgress,
                                       stageProgress,
                                       s,
                                       operation);
        }
    }

    const char *getStageName(Stages stage) const
    {
        const char *ret = "unknown";
        switch ( stage )
        {
            case Stages::COMPUTE_BOUNDS_OF_INPUT_MESH:
                ret = "COMPUTE_BOUNDS_OF_INPUT_MESH";
                break;
            case Stages::REINDEXING_INPUT_MESH:
                ret = "REINDEXING_INPUT_MESH";
                break;
            case Stages::CREATE_RAYCAST_MESH:
                ret = "CREATE_RAYCAST_MESH";
                break;
            case Stages::VOXELIZING_INPUT_MESH:
                ret = "VOXELIZING_INPUT_MESH";
                break;
            case Stages::BUILD_INITIAL_CONVEX_HULL:
                ret = "BUILD_INITIAL_CONVEX_HULL";
                break;
            case Stages::PERFORMING_DECOMPOSITION:
                ret = "PERFORMING_DECOMPOSITION";
                break;
            case Stages::INITIALIZING_CONVEX_HULLS_FOR_MERGING:
                ret = "INITIALIZING_CONVEX_HULLS_FOR_MERGING";
                break;
            case Stages::COMPUTING_COST_MATRIX:
                ret = "COMPUTING_COST_MATRIX";
                break;
            case Stages::MERGING_CONVEX_HULLS:
                ret = "MERGING_CONVEX_HULLS";
                break;
            case Stages::FINALIZING_RESULTS:
                ret = "FINALIZING_RESULTS";
                break;
            }
        return ret;
    }

    bool isCanceled(void) const override final
    {
        return mCanceled;
    }

    std::atomic<bool>                       mCanceled{false};
    Parameters                              mParams;                    // Convex decomposition parameters

    ConvexHullVector                        mConvexHulls;               // Finalized convex hulls
    VoxelHullVector                         mVoxelHulls;                // completed voxel hulls
    VoxelHullVector                         mPendingHulls;

    AABBTreeVector							mTrees;
    VHACD::MyRaycastMesh                    mRaycastMesh;
    VHACD::VoxelizeImpl                     mVoxelize;
    nd::VHACD::Vect3<double>                mCenter;
    double                                  mScale{1};
    double                                  mRecipScale{1};
    SimpleMesh                              mInputMesh;     // re-indexed and normalized input mesh
    std::vector<VHACD::Vertex>              mVertices;
    std::vector<VHACD::Triangle>            mIndices;

    double                                  mOverallHullVolume{0};
    double                                  mVoxelScale{0};
    double                                  mVoxelHalfScale{0};
    nd::VHACD::Vect3<double>                mVoxelBmin;
    nd::VHACD::Vect3<double>                mVoxelBmax;
    uint32_t                                mMeshId{0};
    HullPairQueue       mHullPairQueue;
#if !VHACD_DISABLE_THREADING
    ThreadPool              *mThreadPool{nullptr};
#endif
    HullMap             mHulls;

    double      mOverallProgress{0};
    double      mStageProgress{0};
    double      mOperationProgress{0};
};

void jobCallback(void *userPtr)
{
   VoxelHull *vh = static_cast<VoxelHull *>(userPtr);
   vh->performPlaneSplit();
}

void computeMergeCostTask(void *ptr)
{
    CostTask *ct = static_cast<CostTask *>(ptr);
    ct->mThis->performMergeCostTask(ct);
}

IVHACD* CreateVHACD(void)
{
    VHACDImpl *ret = new VHACDImpl;
    return static_cast< IVHACD *>(ret);
}

IVHACD* CreateVHACD(void);

#if !VHACD_DISABLE_THREADING

class LogMessage
{
public:
    double  mOverallProgress{-1};
    double  mStageProgress{-1};
    std::string mStage;
    std::string mOperation;
};

using LogMessageVector = std::vector< LogMessage >;

class MyHACD_API : public VHACD::IVHACD,
                   public VHACD::IVHACD::IUserCallback,
                   VHACD::IVHACD::IUserLogger,
                   VHACD::IVHACD::IUserTaskRunner
{
public:
    MyHACD_API()
    {
        mVHACD = VHACD::CreateVHACD();
    }

    ~MyHACD_API() override
    {
        Cancel();
        if ( mVHACD )
        {
            mVHACD->Release();
        }
    }

    void* StartTask(std::function<void()> func) override
    {
        return new std::thread(func);
    }

    void JoinTask(void* Task) override
    {
        std::thread* t = static_cast<std::thread*>(Task);
        t->join();
        delete t;
    }

    bool Compute(const float* const points,
                 const uint32_t countPoints,
                 const uint32_t* const triangles,
                 const uint32_t countTriangles,
                 const Parameters& params) override final
    {
        mVertices.reserve(countPoints * 3);
        for (uint32_t i = 0; i < countPoints; ++i)
        {
            mVertices.push_back(points[i * 3 + 0]);
            mVertices.push_back(points[i * 3 + 1]);
            mVertices.push_back(points[i * 3 + 2]);
        }

        mIndices.reserve(countTriangles * 3);
        for (uint32_t i = 0; i < countTriangles; ++i)
        {
            mIndices.push_back(triangles[i * 3 + 0]);
            mIndices.push_back(triangles[i * 3 + 1]);
            mIndices.push_back(triangles[i * 3 + 2]);
        }

        return Compute(params);
    }

    bool Compute(const double* const _points,
                 const uint32_t countPoints,
                 const uint32_t* const _triangles,
                 const uint32_t countTriangles,
                 const Parameters& _desc) override final
    {
        // We need to copy the input vertices and triangles into our own buffers so we can operate
        // on them safely from the background thread.
        mVertices.reserve(countPoints * 3);
        for (uint32_t i = 0; i < countPoints; ++i)
        {
            mVertices.push_back(_points[i * 3 + 0]);
            mVertices.push_back(_points[i * 3 + 1]);
            mVertices.push_back(_points[i * 3 + 2]);
        }

        mIndices.reserve(countTriangles * 3);
        for (uint32_t i = 0; i < countTriangles; ++i)
        {
            mIndices.push_back(_triangles[i * 3 + 0]);
            mIndices.push_back(_triangles[i * 3 + 1]);
            mIndices.push_back(_triangles[i * 3 + 2]);
        }

        return Compute(_desc);
    }

    bool Compute(const Parameters& _desc)
    {
        Cancel(); // if we previously had a solution running; cancel it.

        Parameters desc = _desc;
        mTaskRunner = _desc.m_taskRunner ? _desc.m_taskRunner : this;
        desc.m_taskRunner = mTaskRunner;

        mRunning = true;
        mTask = mTaskRunner->StartTask([this, desc]() {
            ComputeNow(mVertices, mIndices, desc);
            // If we have a user provided callback and the user did *not* call 'cancel' we notify him that the
            // task is completed. However..if the user selected 'cancel' we do not send a completed notification event.
            if (desc.m_callback && !mCancel)
            {
                desc.m_callback->NotifyVHACDComplete();
            }
            mRunning = false;
        });
        return true;
    }

    bool ComputeNow(const std::vector<double>& points,
                    const std::vector<uint32_t>& triangles,
                    const Parameters& _desc)
    {
        uint32_t ret = 0;

        Parameters desc;
        mCallback = _desc.m_callback;
        mLogger = _desc.m_logger;

        desc = _desc;
        // Set our intercepting callback interfaces if non-null
        desc.m_callback = _desc.m_callback ? this : nullptr;
        desc.m_logger = _desc.m_logger ? this : nullptr;

        // If not task runner provided, then use the default one
        if (desc.m_taskRunner == nullptr)
        {
            desc.m_taskRunner = this;
        }

        if (mVHACD)
        {
            bool ok = mVHACD->Compute(points.data(),
                                      uint32_t(points.size() / 3),
                                      triangles.data(),
                                      uint32_t(triangles.size() / 3),
                                      desc);
            if (ok)
            {
                ret = mVHACD->GetNConvexHulls();
            }
        }

        return ret ? true : false;
    }

    bool GetConvexHull(const uint32_t index,
                       VHACD::IVHACD::ConvexHull& ch) const override final
    {
        return mVHACD->GetConvexHull(index,ch);
    }

    void release() // release the HACD_API interface
    {
        delete this;
    }

    uint32_t getHullCount()
    {
        return mVHACD->GetNConvexHulls();
    }

    void Cancel() override final
    {
        // printf("Entered Cancel\n");
        mCancel = true;
        if (mVHACD)
        {
            // printf("Setting cancel on the V-HACD instance\n");
            mVHACD->Cancel(); // Set the cancel signal to the base VHACD
        }
        if (mTask)
        {
            // printf("JoinTask waiting for task to complete.\n");
            mTaskRunner->JoinTask(mTask); // Wait for the thread to fully exit before we delete the instance
            mTask = nullptr;
        }
        // printf("Leaving Cancel routine\n");
        mCancel = false; // clear the cancel semaphore
    }

    uint32_t GetNConvexHulls() const override final
    {
        processPendingMessages();
        return mVHACD->GetNConvexHulls();
    }

    void Clean() override final // release internally allocated memory
    {
        Cancel();
        mVHACD->Clean();
    }

    void Release() override final // release IVHACD
    {
        delete this;
    }

    void Update(const double overallProgress,
                const double stageProgress,
                const char* const stage,
                const char *operation) override final
    {
        mMessageMutex.lock();
        LogMessage m;
        m.mOperation = std::string(operation);
        m.mOverallProgress = overallProgress;
        m.mStageProgress = stageProgress;
        m.mStage = std::string(stage);
        mMessages.push_back(m);
        mHaveMessages = true;
        mMessageMutex.unlock();
    }

    void Log(const char* const msg) override final
    {
        mMessageMutex.lock();
        LogMessage m;
        m.mOperation = std::string(msg);
        mHaveMessages = true;
        mMessages.push_back(m);
        mMessageMutex.unlock();
    }

    bool IsReady() const override final
    {
        processPendingMessages();
        return !mRunning;
    }

    // As a convenience for the calling application we only send it update and log messages from it's own main
    // thread.  This reduces the complexity burden on the caller by making sure it only has to deal with log
    // messages in it's main application thread.
    void processPendingMessages() const
    {
        if (mCancel)
        {
            return;
        }
        if ( mHaveMessages )
        {
            mMessageMutex.lock();
            for (auto &i:mMessages)
            {
                if ( i.mOverallProgress == -1 )
                {
                    if ( mLogger )
                    {
                        mLogger->Log(i.mOperation.c_str());
                    }
                }
                else if ( mCallback )
                {
                    mCallback->Update(i.mOverallProgress,
                                      i.mStageProgress,
                                      i.mStage.c_str(),
                                      i.mOperation.c_str());
                }
            }
            mMessages.clear();
            mHaveMessages = false;
            mMessageMutex.unlock();
        }
    }

    // Will compute the center of mass of the convex hull decomposition results and return it
    // in 'centerOfMass'.  Returns false if the center of mass could not be computed.
    bool ComputeCenterOfMass(double centerOfMass[3]) const override
    {
        bool ret = false;

        centerOfMass[0] = 0;
        centerOfMass[1] = 0;
        centerOfMass[2] = 0;

        if (mVHACD && IsReady())
        {
            ret = mVHACD->ComputeCenterOfMass(centerOfMass);
        }
        return ret;
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
    uint32_t findNearestConvexHull(const double pos[3],
                                   double &distanceToHull) override final
	{
		uint32_t ret = 0; // The default return code is zero

		distanceToHull = 0;
		// First, make sure that we have valid and completed results
		if (mVHACD && IsReady() )
		{
			ret = mVHACD->findNearestConvexHull(pos,distanceToHull);
		}

		return ret;
	}

private:
    std::vector<double> mVertices;
    std::vector<uint32_t> mIndices;
    VHACD::IVHACD::IUserCallback* mCallback{ nullptr };
    VHACD::IVHACD::IUserLogger* mLogger{ nullptr };
    VHACD::IVHACD* mVHACD{ nullptr };
    VHACD::IVHACD::IUserTaskRunner* mTaskRunner{ nullptr };
    void* mTask{ nullptr };
    std::atomic<bool> mRunning{ false };
    std::atomic<bool> mCancel{ false };

    // Thread safe caching mechanism for messages and update status.
    // This is so that caller always gets messages in his own thread
    // Member variables are marked as 'mutable' since the message dispatch function
    // is called from const query methods.
    mutable std::mutex mMessageMutex;
    mutable LogMessageVector    mMessages;
    mutable std::atomic<bool>   mHaveMessages{false};
};

IVHACD* CreateVHACD_ASYNC(void)
{
    MyHACD_API* m = new MyHACD_API;
    return static_cast<IVHACD*>(m);
}
#endif

};

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC__

#endif // ENABLE_VHACD_IMPLEMENTATION

#endif // VHACD_H
