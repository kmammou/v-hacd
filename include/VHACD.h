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
        uint32_t m_nPoints{0};                  // Total number of vertices in the convex hull
        double* m_points{nullptr};              // Points in the convex hull as doubles x1,y1,z1,x2,y2,z2...

        uint32_t m_nTriangles{0};               // Total number of triangles in the convex hull
        uint32_t* m_triangles{nullptr};         // Triangle indices

        double m_volume{0};                     // The volume of the convex hull
        double m_center[3]{0,0,0};              // The centroid of the convex hull
        uint32_t    m_meshId{0};                // A unique id for this convex hull
        double  mBmin[3];                       // Bounding box minimum of the AABB
        double  mBmax[3];                       // Bounding box maximum of he AABB
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
    virtual bool GetConvexHull(const uint32_t index, ConvexHull& ch) const = 0;

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
    virtual uint32_t findNearestConvexHull(const double pos[3],double &distanceToHull) = 0;

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

#include <chrono>
#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <algorithm>
#include <condition_variable>
#include <unordered_set>
#include <unordered_map>
#include <future>
#include <memory>
#include <array>
#include <deque>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100 4189 4456 4701 4702 4127 4996)
#endif

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


}

//***********************************************************************************************
// ConvexHull generation code by Julio Jerez <jerezjulio0@gmail.com>
//***********************************************************************************************

namespace nd 
{
	namespace VHACD 
	{
		//!    Vector dim 3.
		template <typename T>
		class Vect3 {
		public:
			T& operator[](size_t i) { return m_data[i]; }
			const T& operator[](size_t i) const { return m_data[i]; }
			T& getX();
			T& getY();
			T& getZ();
			const T& getX() const;
			const T& getY() const;
			const T& getZ() const;
			void Normalize();
			T GetNorm() const;
			void operator=(const Vect3& rhs);
			void operator+=(const Vect3& rhs);
			void operator-=(const Vect3& rhs);
			void operator-=(T a);
			void operator+=(T a);
			void operator/=(T a);
			void operator*=(T a);
			Vect3 operator^(const Vect3& rhs) const;
			T operator*(const Vect3& rhs) const;
			Vect3 operator+(const Vect3& rhs) const;
			Vect3 operator-(const Vect3& rhs) const;
			Vect3 operator-() const;
			Vect3 operator*(T rhs) const;
			Vect3 operator/(T rhs) const;
			bool operator<(const Vect3& rhs) const;
			bool operator>(const Vect3& rhs) const;
			Vect3();
			Vect3(T a);
			Vect3(T x, T y, T z);
			Vect3(const Vect3& rhs);
			/*virtual*/ ~Vect3(void);

			// Compute the center of this bounding box and return the diagonal length
			T GetCenter(const Vect3 &bmin, const Vect3 &bmax)
			{
				getX() = (bmin.getX() + bmax.getX())*0.5;
				getY() = (bmin.getY() + bmax.getY())*0.5;
				getZ() = (bmin.getZ() + bmax.getZ())*0.5;
				T dx = bmax.getX() - bmin.getX();
				T dy = bmax.getY() - bmin.getY();
				T dz = bmax.getZ() - bmin.getZ();
				T diagonal = T(sqrt(dx*dx + dy*dy + dz*dz));
				return diagonal;
			}

			// Update the min/max values relative to this point
			void UpdateMinMax(Vect3 &bmin, Vect3 &bmax) const
			{
				if (getX() < bmin.getX())
				{
					bmin.getX() = getX();
				}
				if (getY() < bmin.getY())
				{
					bmin.getY() = getY();
				}
				if (getZ() < bmin.getZ())
				{
					bmin.getZ() = getZ();
				}
				if (getX() > bmax.getX())
				{
					bmax.getX() = getX();
				}
				if (getX() > bmax.getX())
				{
					bmax.getX() = getX();
				}
				if (getY() > bmax.getY())
				{
					bmax.getY() = getY();
				}
				if (getZ() > bmax.getZ())
				{
					bmax.getZ() = getZ();
				}
			}

			// Returns the squared distance between these two points
			T GetDistanceSquared(const Vect3 &p) const
			{
				T dx = getX() - p.getX();
				T dy = getY() - p.getY();
				T dz = getZ() - p.getZ();
				return dx*dx + dy*dy + dz*dz;
			}

			T GetDistance(const Vect3 &p) const
			{
				return sqrt(GetDistanceSquared(p));
			}

			// Returns the raw vector data as a pointer
			T* GetData(void)
			{
				return m_data;
			}
		private:
			T m_data[3];
		};
		//!    Vector dim 2.
		template <typename T>
		class Vec2 {
		public:
			T& operator[](size_t i) { return m_data[i]; }
			const T& operator[](size_t i) const { return m_data[i]; }
			T& getX();
			T& getY();
			const T& getX() const;
			const T& getY() const;
			void Normalize();
			T GetNorm() const;
			void operator=(const Vec2& rhs);
			void operator+=(const Vec2& rhs);
			void operator-=(const Vec2& rhs);
			void operator-=(T a);
			void operator+=(T a);
			void operator/=(T a);
			void operator*=(T a);
			T operator^(const Vec2& rhs) const;
			T operator*(const Vec2& rhs) const;
			Vec2 operator+(const Vec2& rhs) const;
			Vec2 operator-(const Vec2& rhs) const;
			Vec2 operator-() const;
			Vec2 operator*(T rhs) const;
			Vec2 operator/(T rhs) const;
			Vec2();
			Vec2(T a);
			Vec2(T x, T y);
			Vec2(const Vec2& rhs);
			/*virtual*/ ~Vec2(void);

		private:
			T m_data[2];
		};

		template <typename T>
		const bool Colinear(const Vect3<T>& a, const Vect3<T>& b, const Vect3<T>& c);
		template <typename T>
		const T ComputeVolume4(const Vect3<T>& a, const Vect3<T>& b, const Vect3<T>& c, const Vect3<T>& d);
	}
}

namespace nd
{
	namespace VHACD
	{
		template <typename T>
		inline Vect3<T> operator*(T lhs, const Vect3<T> & rhs)
		{
			return Vect3<T>(lhs * rhs.getX(), lhs * rhs.getY(), lhs * rhs.getZ());
		}
		template <typename T>
		inline T & Vect3<T>::getX()
		{
			return m_data[0];
		}
		template <typename T>
		inline  T &    Vect3<T>::getY()
		{
			return m_data[1];
		}
		template <typename T>
		inline  T &    Vect3<T>::getZ()
		{
			return m_data[2];
		}
		template <typename T>
		inline  const T & Vect3<T>::getX() const
		{
			return m_data[0];
		}
		template <typename T>
		inline  const T & Vect3<T>::getY() const
		{
			return m_data[1];
		}
		template <typename T>
		inline  const T & Vect3<T>::getZ() const
		{
			return m_data[2];
		}
		template <typename T>
		inline  void Vect3<T>::Normalize()
		{
			T n = sqrt(m_data[0] * m_data[0] + m_data[1] * m_data[1] + m_data[2] * m_data[2]);
			if (n != 0.0) (*this) /= n;
		}
		template <typename T>
		inline  T Vect3<T>::GetNorm() const
		{
			return sqrt(m_data[0] * m_data[0] + m_data[1] * m_data[1] + m_data[2] * m_data[2]);
		}
		template <typename T>
		inline  void Vect3<T>::operator= (const Vect3 & rhs)
		{
			this->m_data[0] = rhs.m_data[0];
			this->m_data[1] = rhs.m_data[1];
			this->m_data[2] = rhs.m_data[2];
		}
		template <typename T>
		inline  void Vect3<T>::operator+=(const Vect3 & rhs)
		{
			this->m_data[0] += rhs.m_data[0];
			this->m_data[1] += rhs.m_data[1];
			this->m_data[2] += rhs.m_data[2];
		}
		template <typename T>
		inline void Vect3<T>::operator-=(const Vect3 & rhs)
		{
			this->m_data[0] -= rhs.m_data[0];
			this->m_data[1] -= rhs.m_data[1];
			this->m_data[2] -= rhs.m_data[2];
		}
		template <typename T>
		inline void Vect3<T>::operator-=(T a)
		{
			this->m_data[0] -= a;
			this->m_data[1] -= a;
			this->m_data[2] -= a;
		}
		template <typename T>
		inline void Vect3<T>::operator+=(T a)
		{
			this->m_data[0] += a;
			this->m_data[1] += a;
			this->m_data[2] += a;
		}
		template <typename T>
		inline void Vect3<T>::operator/=(T a)
		{
			this->m_data[0] /= a;
			this->m_data[1] /= a;
			this->m_data[2] /= a;
		}
		template <typename T>
		inline void Vect3<T>::operator*=(T a)
		{
			this->m_data[0] *= a;
			this->m_data[1] *= a;
			this->m_data[2] *= a;
		}
		template <typename T>
		inline Vect3<T> Vect3<T>::operator^ (const Vect3<T> & rhs) const
		{
			return Vect3<T>(m_data[1] * rhs.m_data[2] - m_data[2] * rhs.m_data[1],
				m_data[2] * rhs.m_data[0] - m_data[0] * rhs.m_data[2],
				m_data[0] * rhs.m_data[1] - m_data[1] * rhs.m_data[0]);
		}
		template <typename T>
		inline T Vect3<T>::operator*(const Vect3<T> & rhs) const
		{
			return (m_data[0] * rhs.m_data[0] + m_data[1] * rhs.m_data[1] + m_data[2] * rhs.m_data[2]);
		}
		template <typename T>
		inline Vect3<T> Vect3<T>::operator+(const Vect3<T> & rhs) const
		{
			return Vect3<T>(m_data[0] + rhs.m_data[0], m_data[1] + rhs.m_data[1], m_data[2] + rhs.m_data[2]);
		}
		template <typename T>
		inline  Vect3<T> Vect3<T>::operator-(const Vect3<T> & rhs) const
		{
			return Vect3<T>(m_data[0] - rhs.m_data[0], m_data[1] - rhs.m_data[1], m_data[2] - rhs.m_data[2]);
		}
		template <typename T>
		inline  Vect3<T> Vect3<T>::operator-() const
		{
			return Vect3<T>(-m_data[0], -m_data[1], -m_data[2]);
		}

		template <typename T>
		inline Vect3<T> Vect3<T>::operator*(T rhs) const
		{
			return Vect3<T>(rhs * this->m_data[0], rhs * this->m_data[1], rhs * this->m_data[2]);
		}
		template <typename T>
		inline Vect3<T> Vect3<T>::operator/ (T rhs) const
		{
			return Vect3<T>(m_data[0] / rhs, m_data[1] / rhs, m_data[2] / rhs);
		}
		template <typename T>
		inline Vect3<T>::Vect3(T a)
		{
			m_data[0] = m_data[1] = m_data[2] = a;
		}
		template <typename T>
		inline Vect3<T>::Vect3(T x, T y, T z)
		{
			m_data[0] = x;
			m_data[1] = y;
			m_data[2] = z;
		}
		template <typename T>
		inline Vect3<T>::Vect3(const Vect3 & rhs)
		{
			m_data[0] = rhs.m_data[0];
			m_data[1] = rhs.m_data[1];
			m_data[2] = rhs.m_data[2];
		}
		template <typename T>
		inline Vect3<T>::~Vect3(void) {};

		template <typename T>
		inline Vect3<T>::Vect3() {}

		template<typename T>
		inline const bool Colinear(const Vect3<T> & a, const Vect3<T> & b, const Vect3<T> & c)
		{
			return  ((c.getZ() - a.getZ()) * (b.getY() - a.getY()) - (b.getZ() - a.getZ()) * (c.getY() - a.getY()) == 0.0 /*EPS*/) &&
				((b.getZ() - a.getZ()) * (c.getX() - a.getX()) - (b.getX() - a.getX()) * (c.getZ() - a.getZ()) == 0.0 /*EPS*/) &&
				((b.getX() - a.getX()) * (c.getY() - a.getY()) - (b.getY() - a.getY()) * (c.getX() - a.getX()) == 0.0 /*EPS*/);
		}

		template<typename T>
		inline const T ComputeVolume4(const Vect3<T> & a, const Vect3<T> & b, const Vect3<T> & c, const Vect3<T> & d)
		{
			return (a - d) * ((b - d) ^ (c - d));
		}

		template <typename T>
		inline bool Vect3<T>::operator<(const Vect3 & rhs) const
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
		inline  bool Vect3<T>::operator>(const Vect3 & rhs) const
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
		inline Vec2<T> operator*(T lhs, const Vec2<T> & rhs)
		{
			return Vec2<T>(lhs * rhs.getX(), lhs * rhs.getY());
		}
		template <typename T>
		inline T & Vec2<T>::getX()
		{
			return m_data[0];
		}
		template <typename T>
		inline  T &    Vec2<T>::getY()
		{
			return m_data[1];
		}
		template <typename T>
		inline  const T & Vec2<T>::getX() const
		{
			return m_data[0];
		}
		template <typename T>
		inline  const T & Vec2<T>::getY() const
		{
			return m_data[1];
		}
		template <typename T>
		inline  void Vec2<T>::Normalize()
		{
			T n = sqrt(m_data[0] * m_data[0] + m_data[1] * m_data[1]);
			if (n != 0.0) (*this) /= n;
		}
		template <typename T>
		inline  T Vec2<T>::GetNorm() const
		{
			return sqrt(m_data[0] * m_data[0] + m_data[1] * m_data[1]);
		}
		template <typename T>
		inline  void Vec2<T>::operator= (const Vec2 & rhs)
		{
			this->m_data[0] = rhs.m_data[0];
			this->m_data[1] = rhs.m_data[1];
		}
		template <typename T>
		inline  void Vec2<T>::operator+=(const Vec2 & rhs)
		{
			this->m_data[0] += rhs.m_data[0];
			this->m_data[1] += rhs.m_data[1];
		}
		template <typename T>
		inline void Vec2<T>::operator-=(const Vec2 & rhs)
		{
			this->m_data[0] -= rhs.m_data[0];
			this->m_data[1] -= rhs.m_data[1];
		}
		template <typename T>
		inline void Vec2<T>::operator-=(T a)
		{
			this->m_data[0] -= a;
			this->m_data[1] -= a;
		}
		template <typename T>
		inline void Vec2<T>::operator+=(T a)
		{
			this->m_data[0] += a;
			this->m_data[1] += a;
		}
		template <typename T>
		inline void Vec2<T>::operator/=(T a)
		{
			this->m_data[0] /= a;
			this->m_data[1] /= a;
		}
		template <typename T>
		inline void Vec2<T>::operator*=(T a)
		{
			this->m_data[0] *= a;
			this->m_data[1] *= a;
		}
		template <typename T>
		inline T Vec2<T>::operator^ (const Vec2<T> & rhs) const
		{
			return m_data[0] * rhs.m_data[1] - m_data[1] * rhs.m_data[0];
		}
		template <typename T>
		inline T Vec2<T>::operator*(const Vec2<T> & rhs) const
		{
			return (m_data[0] * rhs.m_data[0] + m_data[1] * rhs.m_data[1]);
		}
		template <typename T>
		inline Vec2<T> Vec2<T>::operator+(const Vec2<T> & rhs) const
		{
			return Vec2<T>(m_data[0] + rhs.m_data[0], m_data[1] + rhs.m_data[1]);
		}
		template <typename T>
		inline  Vec2<T> Vec2<T>::operator-(const Vec2<T> & rhs) const
		{
			return Vec2<T>(m_data[0] - rhs.m_data[0], m_data[1] - rhs.m_data[1]);
		}
		template <typename T>
		inline  Vec2<T> Vec2<T>::operator-() const
		{
			return Vec2<T>(-m_data[0], -m_data[1]);
		}

		template <typename T>
		inline Vec2<T> Vec2<T>::operator*(T rhs) const
		{
			return Vec2<T>(rhs * this->m_data[0], rhs * this->m_data[1]);
		}
		template <typename T>
		inline Vec2<T> Vec2<T>::operator/ (T rhs) const
		{
			return Vec2<T>(m_data[0] / rhs, m_data[1] / rhs);
		}
		template <typename T>
		inline Vec2<T>::Vec2(T a)
		{
			m_data[0] = m_data[1] = a;
		}
		template <typename T>
		inline Vec2<T>::Vec2(T x, T y)
		{
			m_data[0] = x;
			m_data[1] = y;
		}
		template <typename T>
		inline Vec2<T>::Vec2(const Vec2 & rhs)
		{
			m_data[0] = rhs.m_data[0];
			m_data[1] = rhs.m_data[1];
		}
		template <typename T>
		inline Vec2<T>::~Vec2(void) {};

		template <typename T>
		inline Vec2<T>::Vec2() {}

		/*
		  InsideTriangle decides if a point P is Inside of the triangle
		  defined by A, B, C.
		*/
		template<typename T>
		inline const bool InsideTriangle(const Vec2<T> & a, const Vec2<T> & b, const Vec2<T> & c, const Vec2<T> & p)
		{
			T ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
			T cCROSSap, bCROSScp, aCROSSbp;
			ax = c.getX() - b.getX();  ay = c.getY() - b.getY();
			bx = a.getX() - c.getX();  by = a.getY() - c.getY();
			cx = b.getX() - a.getX();  cy = b.getY() - a.getY();
			apx = p.getX() - a.getX();  apy = p.getY() - a.getY();
			bpx = p.getX() - b.getX();  bpy = p.getY() - b.getY();
			cpx = p.getX() - c.getX();  cpy = p.getY() - c.getY();
			aCROSSbp = ax*bpy - ay*bpx;
			cCROSSap = cx*apy - cy*apx;
			bCROSScp = bx*cpy - by*cpx;
			return ((aCROSSbp >= 0.0) && (bCROSScp >= 0.0) && (cCROSSap >= 0.0));
		}
	}
}


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

		class hullVector : public VHACD::Vect3<double>
		{
			public:
			hullVector()
				:Vect3<double>(0, 0, 0)
			{
			}

			hullVector(double x)
				:Vect3<double>(x, x, x)
			{
			}

			hullVector(const hullVector& x)
				:Vect3<double>(x.getX(), x.getY(), x.getZ())
			{
			}

			hullVector(double x, double y, double z, double)
				:Vect3<double>(x, y, z)
			{
			}

			hullVector GetMin(const hullVector& p) const
			{
				return hullVector(
					getX() < p.getX() ? getX() : p.getX(),
					getY() < p.getY() ? getY() : p.getY(),
					getZ() < p.getZ() ? getZ() : p.getZ(), 0.0);
			}

			hullVector GetMax(const hullVector& p) const
			{
				return hullVector(
					getX() > p.getX() ? getX() : p.getX(),
					getY() > p.getY() ? getY() : p.getY(),
					getZ() > p.getZ() ? getZ() : p.getZ(), 0.0);
			}

			hullVector Scale(double s) const
			{
				return hullVector(getX() * s, getY() * s, getZ() * s, 0.0);
			}

			inline hullVector operator+(const hullVector & rhs) const
			{
				return hullVector(getX() + rhs.getX(), getY() + rhs.getY(), getZ() + rhs.getZ(), 0.0f);
			}

			inline hullVector operator-(const hullVector & rhs) const
			{
				return hullVector(getX() - rhs.getX(), getY() - rhs.getY(), getZ() - rhs.getZ(), 0.0f);
			}

			inline hullVector operator*(const hullVector & rhs) const
			{
				return hullVector(getX() * rhs.getX(), getY() * rhs.getY(), getZ() * rhs.getZ(), 0.0f);
			}

			inline double DotProduct(const hullVector & rhs) const
			{
				return getX() * rhs.getX() + getY() * rhs.getY() + getZ() * rhs.getZ();
			}

			inline hullVector CrossProduct(const hullVector & rhs) const
			{
				return hullVector(getY() * rhs.getZ() - getZ() * rhs.getY(), getZ() * rhs.getX() - getX() * rhs.getZ(), getX() * rhs.getY() - getY() * rhs.getX(), 0.0);
			}

			inline hullVector operator= (const Vect3 & rhs)
			{
				getX() = rhs.getX();
				getY() = rhs.getY();
				getZ() = rhs.getZ();
				return *this;
			}
		};

		class hullPlane : public hullVector
		{
			public:
			hullPlane(double x, double y, double z, double w)
				:hullVector(x, y, z, 0.0)
				, m_w(w)
			{
			}

			hullPlane(const hullVector &P0, const hullVector &P1, const hullVector &P2)
				:hullVector((P1 - P0).CrossProduct(P2 - P0))
			{
				m_w = -DotProduct(P0);
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

			inline hullVector operator*(const hullVector & rhs) const
			{
				return hullVector(getX() * rhs.getX(), getY() * rhs.getY(), getZ() * rhs.getZ(), 0.0f);
			}

			double Evalue(const hullVector &point) const
			{
				return DotProduct(point) + m_w;
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
		inline T Max(T A, T B)
		{
			return (A > B) ? A : B;
		}

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
			double Evalue(const hullVector* const pointArray, const hullVector& point) const;
			hullPlane GetPlaneEquation(const hullVector* const pointArray, bool& isValid) const;

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
			ConvexHull(const double* const vertexCloud, int strideInBytes, int count, double distTol, int maxVertexCount = 0x7fffffff);
			~ConvexHull();

			const std::vector<hullVector>& GetVertexPool() const;

			private:
			void BuildHull(const double* const vertexCloud, int strideInBytes, int count, double distTol, int maxVertexCount);

			void GetUniquePoints(std::vector<ConvexHullVertex>& points);
			int InitVertexArray(std::vector<ConvexHullVertex>& points, void* const memoryPool, int maxMemSize);

			ConvexHullAABBTreeNode* BuildTreeNew(std::vector<ConvexHullVertex>& points, char** const memoryPool, int& maxMemSize) const;
			ConvexHullAABBTreeNode* BuildTreeOld(std::vector<ConvexHullVertex>& points, char** const memoryPool, int& maxMemSize);
			ConvexHullAABBTreeNode* BuildTreeRecurse(ConvexHullAABBTreeNode* const parent, ConvexHullVertex* const points, int count, int baseIndex, char** const memoryPool, int& maxMemSize) const;

			ndNode* AddFace(int i0, int i1, int i2);

			void CalculateConvexHull3d(ConvexHullAABBTreeNode* vertexTree, std::vector<ConvexHullVertex>& points, int count, double distTol, int maxVertexCount);

			int SupportVertex(ConvexHullAABBTreeNode** const tree, const std::vector<ConvexHullVertex>& points, const hullVector& dir, const bool removeEntry = true) const;
			double TetrahedrumVolume(const hullVector& p0, const hullVector& p1, const hullVector& p2, const hullVector& p3) const;

			hullVector m_aabbP0;
			hullVector m_aabbP1;
			double m_diag;
			std::vector<hullVector> m_points;
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

		hullPlane ConvexHullFace::GetPlaneEquation(const hullVector* const pointArray, bool& isvalid) const
		{
			const hullVector& p0 = pointArray[m_index[0]];
			const hullVector& p1 = pointArray[m_index[1]];
			const hullVector& p2 = pointArray[m_index[2]];
			hullPlane plane(p0, p1, p2);

			isvalid = false;
			double mag2 = plane.DotProduct(plane);
			if (mag2 > 1.0e-16f)
			{
				isvalid = true;
				plane = plane.Scale(1.0f / sqrt(mag2));
			}
			return plane;
		}

		double ConvexHullFace::Evalue(const hullVector* const pointArray, const hullVector& point) const
		{
			const hullVector& p0 = pointArray[m_index[0]];
			const hullVector& p1 = pointArray[m_index[1]];
			const hullVector& p2 = pointArray[m_index[2]];

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

		class ConvexHullVertex : public hullVector
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

			hullVector m_box[2];
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
				:m_count(sizeof(m_normal) / sizeof(m_normal[0]))
			{
				hullVector p0(double(1.0f), double(0.0f), double(0.0f), double(0.0f));
				hullVector p1(double(-1.0f), double(0.0f), double(0.0f), double(0.0f));
				hullVector p2(double(0.0f), double(1.0f), double(0.0f), double(0.0f));
				hullVector p3(double(0.0f), double(-1.0f), double(0.0f), double(0.0f));
				hullVector p4(double(0.0f), double(0.0f), double(1.0f), double(0.0f));
				hullVector p5(double(0.0f), double(0.0f), double(-1.0f), double(0.0f));

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

			void TessellateTriangle(int level, const hullVector& p0, const hullVector& p1, const hullVector& p2, int& count)
			{
				if (level) 
				{
					assert(fabs(p0.DotProduct(p0) - double(1.0f)) < double(1.0e-4f));
					assert(fabs(p1.DotProduct(p1) - double(1.0f)) < double(1.0e-4f));
					assert(fabs(p2.DotProduct(p2) - double(1.0f)) < double(1.0e-4f));
					hullVector p01(p0 + p1);
					hullVector p12(p1 + p2);
					hullVector p20(p2 + p0);

					p01 = p01.Scale(1.0 / sqrt(p01.DotProduct(p01)));
					p12 = p12.Scale(1.0 / sqrt(p12.DotProduct(p12)));
					p20 = p20.Scale(1.0 / sqrt(p20.DotProduct(p20)));

					assert(fabs(p01.DotProduct(p01) - double(1.0f)) < double(1.0e-4f));
					assert(fabs(p12.DotProduct(p12) - double(1.0f)) < double(1.0e-4f));
					assert(fabs(p20.DotProduct(p20) - double(1.0f)) < double(1.0e-4f));

					TessellateTriangle(level - 1, p0, p01, p20, count);
					TessellateTriangle(level - 1, p1, p12, p01, count);
					TessellateTriangle(level - 1, p2, p20, p12, count);
					TessellateTriangle(level - 1, p01, p12, p20, count);
				}
				else 
				{
					hullPlane n(p0, p1, p2);
					n = n.Scale(double(1.0f) / sqrt(n.DotProduct(n)));
					n.m_w = double(0.0f);
					int index = dBitReversal(count, sizeof(m_normal) / sizeof(m_normal[0]));
					m_normal[index] = n;
					count++;
					assert(count <= int(sizeof(m_normal) / sizeof(m_normal[0])));
				}
			}

			hullVector m_normal[128];
			int m_count;
		};

		ConvexHull::ConvexHull(const double* const vertexCloud, int strideInBytes, int count, double distTol, int maxVertexCount)
			:List<ConvexHullFace>()
			,m_aabbP0(0)
			,m_aabbP1(0)
			,m_diag()
			,m_points()
		{
			m_points.resize(0);
			if (count >= 4)
			{
				BuildHull(vertexCloud, strideInBytes, count, distTol, maxVertexCount);
			}
		}

		ConvexHull::~ConvexHull()
		{
		}

		const std::vector<hullVector>& ConvexHull::GetVertexPool() const
		{
			return m_points;
		}


		void ConvexHull::BuildHull(const double* const vertexCloud, int strideInBytes, int count, double distTol, int maxVertexCount)
		{
			int treeCount = count / (VHACD_CONVEXHULL_3D_VERTEX_CLUSTER_SIZE >> 1);
			if (treeCount < 4)
			{
				treeCount = 4;
			}
			treeCount *= 2;

			std::vector<ConvexHullVertex> points(count);
			std::vector<ConvexHull3dPointCluster> treePool(treeCount + 256);
			points.resize(count);
			treePool.resize(treeCount + 256);

			const int stride = int(strideInBytes / sizeof(double));
			for (int i = 0; i < count; ++i)
			{
				int index = i * stride;
				hullVector& vertex = points[i];
				vertex = hullVector(vertexCloud[index], vertexCloud[index + 1], vertexCloud[index + 2], double(0.0f));
				points[i].m_mark = 0;
			}
			count = InitVertexArray(points, &treePool[0], sizeof (ConvexHull3dPointCluster) * int (treePool.size()));

			if (m_points.size() >= 4)
			{
				CalculateConvexHull3d(&treePool[0], points, count, distTol, maxVertexCount);
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

		ConvexHullAABBTreeNode* ConvexHull::BuildTreeRecurse(ConvexHullAABBTreeNode* const parent, ConvexHullVertex* const points, int count, int baseIndex, char** memoryPool, int& maxMemSize) const
		{
			ConvexHullAABBTreeNode* tree = nullptr;

			assert(count);
			hullVector minP(double(1.0e15f));
			hullVector maxP(-double(1.0e15f));
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

					const hullVector& p = points[i];
					minP = minP.GetMin(p);
					maxP = maxP.GetMax(p);
				}

				clump->m_left = nullptr;
				clump->m_right = nullptr;
				tree = clump;
			}
			else
			{
				hullVector median(0);
				hullVector varian(0);
				for (int i = 0; i < count; ++i)
				{
					const hullVector& p = points[i];
					minP = minP.GetMin(p);
					maxP = maxP.GetMax(p);
					median += p;
					varian += p * p;
				}

				varian = varian.Scale(double(count)) - median * median;
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
				hullVector center(median.Scale(double(1.0f) / double(count)));

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
			tree->m_box[0] = minP - hullVector(double(1.0e-3f));
			tree->m_box[1] = maxP + hullVector(double(1.0e-3f));
			return tree;
		}

		ConvexHullAABBTreeNode* ConvexHull::BuildTreeOld(std::vector<ConvexHullVertex>& points, char** const memoryPool, int& maxMemSize)
		{
			GetUniquePoints(points);
			int count = int(points.size());
			if (count < 4)
			{
				return nullptr;
			}
			return BuildTreeRecurse(nullptr, &points[0], count, 0, memoryPool, maxMemSize);
		}

		ConvexHullAABBTreeNode* ConvexHull::BuildTreeNew(std::vector<ConvexHullVertex>& points, char** const memoryPool, int& maxMemSize) const
		{
			class dCluster
			{
				public:
				hullVector m_sum;
				hullVector m_sum2;
				int m_start;
				int m_count;
			};

			dCluster firstCluster;
			firstCluster.m_start = 0;
			firstCluster.m_count = int (points.size());
			firstCluster.m_sum = hullVector(0);
			firstCluster.m_sum2 = hullVector(0);

			for (int i = 0; i < firstCluster.m_count; ++i)
			{
				const hullVector& p = points[i];
				firstCluster.m_sum += p;
				firstCluster.m_sum2 += p * p;
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

					const hullVector origin(cluster.m_sum.Scale(1.0f / cluster.m_count));
					const hullVector variance2(cluster.m_sum2.Scale(1.0f / cluster.m_count) - origin * origin);
					double maxVariance2 = Max(Max(variance2.getX(), variance2.getY()), variance2.getZ());

					if ((cluster.m_count <= clusterSize) || (stack > (sizeof(spliteStack) / sizeof(spliteStack[0]) - 4)) || (maxVariance2 < 1.e-4f))
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
								hullVector error(points[cluster.m_start + j] - points[cluster.m_start + i]);
								double mag2 = error.DotProduct(error);
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

						hullVector xc(0);
						hullVector x2c(0);
						for (int i = 0; i < i0; ++i)
						{
							const hullVector& x = points[start + i];
							xc += x;
							x2c += x * x;
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

			hullVector sum(0);
			hullVector sum2(0);
			hullVector minP(double(1.0e15f));
			hullVector maxP(-double(1.0e15f));
			class dTreeBox
			{
				public:
				hullVector m_min;
				hullVector m_max;
				hullVector m_sum;
				hullVector m_sum2;
				ConvexHullAABBTreeNode* m_parent;
				ConvexHullAABBTreeNode** m_child;
				int m_start;
				int m_count;
			};

			for (int i = 0; i < baseCount; ++i)
			{
				const hullVector& p = points[i];
				sum += p;
				sum2 += p * p;
				minP = minP.GetMin(p);
				maxP = maxP.GetMax(p);
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
					const hullVector origin(box.m_sum.Scale(1.0f / box.m_count));
					const hullVector variance2(box.m_sum2.Scale(1.0f / box.m_count) - origin * origin);

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
						hullVector xc(0);
						hullVector x2c(0);
						hullVector p0(double(1.0e15f));
						hullVector p1(-double(1.0e15f));
						for (int i = i0; i < box.m_count; ++i)
						{
							const hullVector& p = points[start + i];
							xc += p;
							x2c += p * p;
							p0 = p0.GetMin(p);
							p1 = p1.GetMax(p);
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
						hullVector xc(0);
						hullVector x2c(0);
						hullVector p0(double(1.0e15f));
						hullVector p1(-double(1.0e15f));
						for (int i = 0; i < i0; ++i)
						{
							const hullVector& p = points[start + i];
							xc += p;
							x2c += p * p;
							p0 = p0.GetMin(p);
							p1 = p1.GetMax(p);
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

		int ConvexHull::SupportVertex(ConvexHullAABBTreeNode** const treePointer, const std::vector<ConvexHullVertex>& points, const hullVector& dirPlane, const bool removeEntry) const
		{
		#define VHACD_STACK_DEPTH_3D 64
			double aabbProjection[VHACD_STACK_DEPTH_3D];
			const ConvexHullAABBTreeNode *stackPool[VHACD_STACK_DEPTH_3D];

			hullVector dir(dirPlane);

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
						const hullVector leftSupportPoint(me->m_left->m_box[ix].getX(), me->m_left->m_box[iy].getY(), me->m_left->m_box[iz].getZ(), 0.0f);
						double leftSupportDist = leftSupportPoint.DotProduct(dir);

						const hullVector rightSupportPoint(me->m_right->m_box[ix].getX(), me->m_right->m_box[iy].getY(), me->m_right->m_box[iz].getZ(), 0.0f);
						double rightSupportDist = rightSupportPoint.DotProduct(dir);

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
								double dist = p.DotProduct(dir);
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

		double ConvexHull::TetrahedrumVolume(const hullVector& p0, const hullVector& p1, const hullVector& p2, const hullVector& p3) const
		{
			const hullVector p1p0(p1 - p0);
			const hullVector p2p0(p2 - p0);
			const hullVector p3p0(p3 - p0);
			return p3p0.DotProduct(p1p0.CrossProduct(p2p0));
		}

		int ConvexHull::InitVertexArray(std::vector<ConvexHullVertex>& points, void* const memoryPool, int maxMemSize)
		{
		#if 1
			ConvexHullAABBTreeNode* tree = BuildTreeOld(points, (char**)&memoryPool, maxMemSize);
		#else
			ConvexHullAABBTreeNode* tree = BuildTreeNew(points, (char**)&memoryPool, maxMemSize);
		#endif
			int count = int (points.size());
			if (count < 4)
			{
				m_points.resize(0);
				return 0;
			}
		
			m_points.resize(count);
			m_aabbP0 = tree->m_box[0];
			m_aabbP1 = tree->m_box[1];
	
			hullVector boxSize(tree->m_box[1] - tree->m_box[0]);
			m_diag = double(sqrt(boxSize.DotProduct(boxSize)));
			const ndNormalMap& normalMap = ndNormalMap::GetNormaMap();
	
			int index0 = SupportVertex(&tree, points, normalMap.m_normal[0]);
			m_points[0] = points[index0];
			points[index0].m_mark = 1;
	
			bool validTetrahedrum = false;
			hullVector e1(0.0);
			for (int i = 1; i < normalMap.m_count; ++i)
			{
				int index = SupportVertex(&tree, points, normalMap.m_normal[i]);
				assert(index >= 0);
	
				e1 = points[index] - m_points[0];
				double error2 = e1.DotProduct(e1);
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
			hullVector e2(0.0);
			hullVector normal(0.0);
			for (int i = 2; i < normalMap.m_count; ++i)
			{
				int index = SupportVertex(&tree, points, normalMap.m_normal[i]);
				assert(index >= 0);
				e2 = points[index] - m_points[0];
				normal = e1.CrossProduct(e2);
				double error2 = sqrt(normal.DotProduct(normal));
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
			hullVector e3(0.0);
	
			index0 = SupportVertex(&tree, points, normal);
			e3 = points[index0] - m_points[0];
			double err2 = normal.DotProduct(e3);
			if (fabs(err2) > (double(1.0e-6f) * m_diag * m_diag))
			{
				// we found a valid tetrahedral, about and start build the hull by adding the rest of the points
				m_points[3] = points[index0];
				points[index0].m_mark = 1;
				validTetrahedrum = true;
			}
			if (!validTetrahedrum)
			{
				hullVector n(normal.Scale(double(-1.0f)));
				int index = SupportVertex(&tree, points, n);
				e3 = points[index] - m_points[0];
				double error2 = normal.DotProduct(e3);
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
					int index = SupportVertex(&tree, points, normalMap.m_normal[i]);
					assert(index >= 0);
	
					//make sure the volume of the fist tetrahedral is no negative
					e3 = points[index] - m_points[0];
					double error2 = normal.DotProduct(e3);
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

		void ConvexHull::CalculateConvexHull3d(ConvexHullAABBTreeNode* vertexTree, std::vector<ConvexHullVertex>& points, int count, double distTol, int maxVertexCount)
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
				hullVector p;
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

class KdTreeInterface
{
public:
    virtual const double* getPositionDouble(uint32_t index) const = 0;
    virtual const float* getPositionFloat(uint32_t index) const = 0;
};

class KdTreeNode
{
public:
    KdTreeNode(void)
    {
        mIndex = 0;
        mLeft = 0;
        mRight = 0;
    }

    KdTreeNode(uint32_t index)
    {
        mIndex = index;
        mLeft = 0;
        mRight = 0;
    };

    ~KdTreeNode(void)
    {
    }


    void addDouble(KdTreeNode* node, Axes dim, const KdTreeInterface* iface)
    {
        const double* nodePosition = iface->getPositionDouble(node->mIndex);
        const double* position = iface->getPositionDouble(mIndex);
        switch (dim)
        {
        case X_AXIS:
            if (nodePosition[0] <= position[0])
            {
                if (mLeft)
                    mLeft->addDouble(node, Y_AXIS, iface);
                else
                    mLeft = node;
            }
            else
            {
                if (mRight)
                    mRight->addDouble(node, Y_AXIS, iface);
                else
                    mRight = node;
            }
            break;
        case Y_AXIS:
            if (nodePosition[1] <= position[1])
            {
                if (mLeft)
                    mLeft->addDouble(node, Z_AXIS, iface);
                else
                    mLeft = node;
            }
            else
            {
                if (mRight)
                    mRight->addDouble(node, Z_AXIS, iface);
                else
                    mRight = node;
            }
            break;
        case Z_AXIS:
            if (nodePosition[2] <= position[2])
            {
                if (mLeft)
                    mLeft->addDouble(node, X_AXIS, iface);
                else
                    mLeft = node;
            }
            else
            {
                if (mRight)
                    mRight->addDouble(node, X_AXIS, iface);
                else
                    mRight = node;
            }
            break;
        }
    }


    void addFloat(KdTreeNode* node, Axes dim, const KdTreeInterface* iface)
    {
        const float* nodePosition = iface->getPositionFloat(node->mIndex);
        const float* position = iface->getPositionFloat(mIndex);
        switch (dim)
        {
        case X_AXIS:
            if (nodePosition[0] <= position[0])
            {
                if (mLeft)
                    mLeft->addFloat(node, Y_AXIS, iface);
                else
                    mLeft = node;
            }
            else
            {
                if (mRight)
                    mRight->addFloat(node, Y_AXIS, iface);
                else
                    mRight = node;
            }
            break;
        case Y_AXIS:
            if (nodePosition[1] <= position[1])
            {
                if (mLeft)
                    mLeft->addFloat(node, Z_AXIS, iface);
                else
                    mLeft = node;
            }
            else
            {
                if (mRight)
                    mRight->addFloat(node, Z_AXIS, iface);
                else
                    mRight = node;
            }
            break;
        case Z_AXIS:
            if (nodePosition[2] <= position[2])
            {
                if (mLeft)
                    mLeft->addFloat(node, X_AXIS, iface);
                else
                    mLeft = node;
            }
            else
            {
                if (mRight)
                    mRight->addFloat(node, X_AXIS, iface);
                else
                    mRight = node;
            }
            break;
        }
    }


    uint32_t getIndex(void) const
    {
        return mIndex;
    };

    void search(Axes axis,
                const double* pos,
                double radius,
                uint32_t& count,
                uint32_t maxObjects,
                KdTreeFindNode* found,
                const KdTreeInterface* iface)
    {

        const double* position = iface->getPositionDouble(mIndex);

        double dx = pos[0] - position[0];
        double dy = pos[1] - position[1];
        double dz = pos[2] - position[2];

        KdTreeNode* search1 = 0;
        KdTreeNode* search2 = 0;

        switch (axis)
        {
        case X_AXIS:
            if (dx <= 0) // JWR  if we are to the left
            {
                search1 = mLeft; // JWR  then search to the left
                if (-dx < radius) // JWR  if distance to the right is less than our search radius, continue on the right
                                  // as well.
                    search2 = mRight;
            }
            else
            {
                search1 = mRight; // JWR  ok, we go down the left tree
                if (dx < radius) // JWR  if the distance from the right is less than our search radius
                    search2 = mLeft;
            }
            axis = Y_AXIS;
            break;
        case Y_AXIS:
            if (dy <= 0)
            {
                search1 = mLeft;
                if (-dy < radius)
                    search2 = mRight;
            }
            else
            {
                search1 = mRight;
                if (dy < radius)
                    search2 = mLeft;
            }
            axis = Z_AXIS;
            break;
        case Z_AXIS:
            if (dz <= 0)
            {
                search1 = mLeft;
                if (-dz < radius)
                    search2 = mRight;
            }
            else
            {
                search1 = mRight;
                if (dz < radius)
                    search2 = mLeft;
            }
            axis = X_AXIS;
            break;
        }

        double r2 = radius * radius;
        double m = dx * dx + dy * dy + dz * dz;

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

    void search(Axes axis,
                const float* pos,
                float radius,
                uint32_t& count,
                uint32_t maxObjects,
                KdTreeFindNode* found,
                const KdTreeInterface* iface)
    {

        const float* position = iface->getPositionFloat(mIndex);

        float dx = pos[0] - position[0];
        float dy = pos[1] - position[1];
        float dz = pos[2] - position[2];

        KdTreeNode* search1 = 0;
        KdTreeNode* search2 = 0;

        switch (axis)
        {
        case X_AXIS:
            if (dx <= 0) // JWR  if we are to the left
            {
                search1 = mLeft; // JWR  then search to the left
                if (-dx < radius) // JWR  if distance to the right is less than our search radius, continue on the right
                                  // as well.
                    search2 = mRight;
            }
            else
            {
                search1 = mRight; // JWR  ok, we go down the left tree
                if (dx < radius) // JWR  if the distance from the right is less than our search radius
                    search2 = mLeft;
            }
            axis = Y_AXIS;
            break;
        case Y_AXIS:
            if (dy <= 0)
            {
                search1 = mLeft;
                if (-dy < radius)
                    search2 = mRight;
            }
            else
            {
                search1 = mRight;
                if (dy < radius)
                    search2 = mLeft;
            }
            axis = Z_AXIS;
            break;
        case Z_AXIS:
            if (dz <= 0)
            {
                search1 = mLeft;
                if (-dz < radius)
                    search2 = mRight;
            }
            else
            {
                search1 = mRight;
                if (dz < radius)
                    search2 = mLeft;
            }
            axis = X_AXIS;
            break;
        }

        float r2 = radius * radius;
        float m = dx * dx + dy * dy + dz * dz;

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

    uint32_t mIndex;
    KdTreeNode* mLeft;
    KdTreeNode* mRight;
};


#    define MAX_BUNDLE_SIZE                                                                                            \
        1024 // 1024 nodes at a time, to minimize memory allocation and guarantee that pointers are persistent.

class KdTreeNodeBundle
{
public:
    KdTreeNodeBundle(void)
    {
        mNext = 0;
        mIndex = 0;
    }

    bool isFull(void) const
    {
        return (bool)(mIndex == MAX_BUNDLE_SIZE);
    }

    KdTreeNode* getNextNode(void)
    {
        assert(mIndex < MAX_BUNDLE_SIZE);
        KdTreeNode* ret = &mNodes[mIndex];
        mIndex++;
        return ret;
    }

    KdTreeNodeBundle* mNext;
    uint32_t mIndex;
    KdTreeNode mNodes[MAX_BUNDLE_SIZE];
};


typedef std::vector<double> DoubleVector;
typedef std::vector<float> FloatVector;

class KdTree : public KdTreeInterface
{
public:
    KdTree(void)
    {
        mRoot = 0;
        mBundle = 0;
        mVcount = 0;
        mUseDouble = false;
    }

    virtual ~KdTree(void)
    {
        reset();
    }

    const double* getPositionDouble(uint32_t index) const
    {
        assert(mUseDouble);
        assert(index < mVcount);
        return &mVerticesDouble[index * 3];
    }

    const float* getPositionFloat(uint32_t index) const
    {
        assert(!mUseDouble);
        assert(index < mVcount);
        return &mVerticesFloat[index * 3];
    }

    uint32_t search(const double* pos, double radius, uint32_t maxObjects, KdTreeFindNode* found) const
    {
        assert(mUseDouble);
        if (!mRoot)
            return 0;
        uint32_t count = 0;
        mRoot->search(X_AXIS, pos, radius, count, maxObjects, found, this);
        return count;
    }

    uint32_t search(const float* pos, float radius, uint32_t maxObjects, KdTreeFindNode* found) const
    {
        assert(!mUseDouble);
        if (!mRoot)
            return 0;
        uint32_t count = 0;
        mRoot->search(X_AXIS, pos, radius, count, maxObjects, found, this);
        return count;
    }

    void reset(void)
    {
        mRoot = 0;
        mVerticesDouble.clear();
        mVerticesFloat.clear();
        KdTreeNodeBundle* bundle = mBundleHead;
        while (bundle)
        {
            KdTreeNodeBundle* next = bundle->mNext;
            delete bundle;
            bundle = next;
        }
        mBundle = 0;
        mBundleHead = 0;
        mVcount = 0;
    }

    uint32_t add(double x, double y, double z)
    {
        assert(mUseDouble);
        uint32_t ret = mVcount;
        mVerticesDouble.push_back(x);
        mVerticesDouble.push_back(y);
        mVerticesDouble.push_back(z);
        mVcount++;
        KdTreeNode* node = getNewNode(ret);
        if (mRoot)
        {
            mRoot->addDouble(node, X_AXIS, this);
        }
        else
        {
            mRoot = node;
        }
        return ret;
    }

    uint32_t add(float x, float y, float z)
    {
        assert(!mUseDouble);
        uint32_t ret = mVcount;
        mVerticesFloat.push_back(x);
        mVerticesFloat.push_back(y);
        mVerticesFloat.push_back(z);
        mVcount++;
        KdTreeNode* node = getNewNode(ret);
        if (mRoot)
        {
            mRoot->addFloat(node, X_AXIS, this);
        }
        else
        {
            mRoot = node;
        }
        return ret;
    }

    KdTreeNode* getNewNode(uint32_t index)
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

    uint32_t getNearest(const double* pos, double radius, bool& _found) const // returns the nearest possible neighbor's
                                                                              // index.
    {
        assert(mUseDouble);
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

    uint32_t getNearest(const float* pos, float radius, bool& _found) const // returns the nearest possible neighbor's
                                                                            // index.
    {
        assert(!mUseDouble);
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

    const double* getVerticesDouble(void) const
    {
        assert(mUseDouble);
        const double* ret = 0;
        if (!mVerticesDouble.empty())
        {
            ret = &mVerticesDouble[0];
        }
        return ret;
    }

    const float* getVerticesFloat(void) const
    {
        assert(!mUseDouble);
        const float* ret = 0;
        if (!mVerticesFloat.empty())
        {
            ret = &mVerticesFloat[0];
        }
        return ret;
    }

    uint32_t getVcount(void) const
    {
        return mVcount;
    };

    void setUseDouble(bool useDouble)
    {
        mUseDouble = useDouble;
    }

private:
    bool mUseDouble;
    KdTreeNode* mRoot;
    KdTreeNodeBundle* mBundle;
    KdTreeNodeBundle* mBundleHead = NULL;

    uint32_t mVcount;
    DoubleVector mVerticesDouble;
    FloatVector mVerticesFloat;
};

}; // end of namespace VERTEX_INDEX

//********************************************************************************************************************
// Prototypes for the handful of float math routines we use
//********************************************************************************************************************
namespace VHACD
{

double fm_normalize(double n[3]); // normalize this vector and return the distance

                                  // Compute centroid of a triangle mesh; takes area of each triangle into account
// weighted average
bool fm_computeCentroid(uint32_t vcount, // number of input data points
                        const double* points, // starting address of points array.
                        uint32_t triangleCount,
                        const uint32_t* indices,
                        double* center);

double fm_computeMeshVolume(const double* vertices, uint32_t tcount, const uint32_t* indices);

void fm_initMinMax(double bmin[3], double bmax[3]);
void fm_minmax(const double p[3], double bmin[3], double bmax[3]); // accumulate to a min-max value
void fm_inflateMinMax(double bmin[3], double bmax[3], double ratio);
void fm_getAABB(uint32_t vcount, const double* points, uint32_t pstride, double bmin[3], double bmax[3]);
bool fm_intersectAABB(const double bmin1[3], const double bmax1[3], const double bmin2[3], const double bmax2[3]);
void fm_combineAABB(const double bmin1[3], const double bmax1[3], const double bmin2[3], const double bmax2[3],double bmin[3],double bmax[3]);
double fm_volumeAABB(const double bmin[3],const double bmax[3]);

class fm_VertexIndex
{
public:
    virtual uint32_t getIndex(const float pos[3], bool& newPos) = 0; // get welded index for this float vector[3]
    virtual uint32_t getIndex(const double pos[3], bool& newPos) = 0; // get welded index for this double vector[3]
    virtual const float* getVerticesFloat(void) const = 0;
    virtual const double* getVerticesDouble(void) const = 0;
    virtual const float* getVertexFloat(uint32_t index) const = 0;
    virtual const double* getVertexDouble(uint32_t index) const = 0;
    virtual uint32_t getVcount(void) const = 0;
    virtual bool isDouble(void) const = 0;
    virtual bool saveAsObj(const char* fname, uint32_t tcount, uint32_t* indices) = 0;
};

fm_VertexIndex* fm_createVertexIndex(double granularity, bool snapToGrid); // create an indexed vertex system for
                                                                           // doubles
fm_VertexIndex* fm_createVertexIndex(float granularity, bool snapToGrid); // create an indexed vertext system for floats
void fm_releaseVertexIndex(fm_VertexIndex* vindex);

//********************************************************************************************************************
// Implementation of the handful of FloatMath methods we actually use
//********************************************************************************************************************
double fm_normalize(double* n) // normalize this vector
{
    double dist = (double)sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (dist > 0.0000001f)
    {
        double mag = 1.0f / dist;
        n[0] *= mag;
        n[1] *= mag;
        n[2] *= mag;
    }
    else
    {
        n[0] = 1;
        n[1] = 0;
        n[2] = 0;
    }

    return dist;
}

static double enorm0_3d(double x0, double y0, double z0, double x1, double y1, double z1)

/**********************************************************************/

/*
Purpose:

ENORM0_3D computes the Euclidean norm of (P1-P0) in 3D.

Modified:

18 April 1999

Author:

John Burkardt

Parameters:

Input, double X0, Y0, Z0, X1, Y1, Z1, the coordinates of the points
P0 and P1.

Output, double ENORM0_3D, the Euclidean norm of (P1-P0).
*/
{
    double value;

    value = (double)sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0) + (z1 - z0) * (z1 - z0));

    return value;
}


static double triangle_area_3d(double x1, double y1, double z1, double x2, double y2, double z2, double x3, double y3, double z3)

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
    double a;
    double alpha;
    double area;
    double b;
    double base;
    double c;
    double dot;
    double height;
    /*
    Find the projection of (P3-P1) onto (P2-P1).
    */
    dot = (x2 - x1) * (x3 - x1) + (y2 - y1) * (y3 - y1) + (z2 - z1) * (z3 - z1);

    base = enorm0_3d(x1, y1, z1, x2, y2, z2);
    /*
    The height of the triangle is the length of (P3-P1) after its
    projection onto (P2-P1) has been subtracted.
    */
    if (base == 0.0)
    {

        height = 0.0;
    }
    else
    {

        alpha = dot / (base * base);

        a = x3 - x1 - alpha * (x2 - x1);
        b = y3 - y1 - alpha * (y2 - y1);
        c = z3 - z1 - alpha * (z2 - z1);

        height = (double)sqrt(a * a + b * b + c * c);
    }

    area = 0.5f * base * height;

    return area;
}


double fm_computeArea(const double* p1, const double* p2, const double* p3)
{
    double ret = 0;

    ret = triangle_area_3d(p1[0], p1[1], p1[2], p2[0], p2[1], p2[2], p3[0], p3[1], p3[2]);

    return ret;
}



bool fm_computeCentroid(uint32_t vcount, // number of input data points
                        const double* points, // starting address of points array.
                        uint32_t triCount,
                        const uint32_t* indices,
                        double* center)

{
    bool ret = false;
    if (vcount)
    {
        center[0] = 0;
        center[1] = 0;
        center[2] = 0;

        double numerator[3] = { 0, 0, 0 };
        double denomintaor = 0;

        for (uint32_t i = 0; i < triCount; i++)
        {
            uint32_t i1 = indices[i * 3 + 0];
            uint32_t i2 = indices[i * 3 + 1];
            uint32_t i3 = indices[i * 3 + 2];

            const double* p1 = &points[i1 * 3];
            const double* p2 = &points[i2 * 3];
            const double* p3 = &points[i3 * 3];

            // Compute the sum of the three positions
            double sum[3];
            sum[0] = p1[0] + p2[0] + p3[0];
            sum[1] = p1[1] + p2[1] + p3[1];
            sum[2] = p1[2] + p2[2] + p3[2];

            // Compute the average of the three positions
            sum[0] = sum[0] / 3;
            sum[1] = sum[1] / 3;
            sum[2] = sum[2] / 3;

            // Compute the area of this triangle
            double area = fm_computeArea(p1, p2, p3);

            numerator[0] += (sum[0] * area);
            numerator[1] += (sum[1] * area);
            numerator[2] += (sum[2] * area);

            denomintaor += area;
        }
        double recip = 1 / denomintaor;
        center[0] = numerator[0] * recip;
        center[1] = numerator[1] * recip;
        center[2] = numerator[2] * recip;
        ret = true;
    }
    return ret;
}

inline double det(const double* p1, const double* p2, const double* p3)
{
    return p1[0] * p2[1] * p3[2] + p2[0] * p3[1] * p1[2] + p3[0] * p1[1] * p2[2] - p1[0] * p3[1] * p2[2] -
           p2[0] * p1[1] * p3[2] - p3[0] * p2[1] * p1[2];
}


double fm_computeMeshVolume(const double* vertices, uint32_t tcount, const uint32_t* indices)
{
    double volume = 0;

    for (uint32_t i = 0; i < tcount; i++, indices += 3)
    {
        const double* p1 = &vertices[indices[0] * 3];
        const double* p2 = &vertices[indices[1] * 3];
        const double* p3 = &vertices[indices[2] * 3];
        volume += det(p1, p2, p3); // compute the volume of the tetrahedran relative to the origin.
    }

    volume *= (1.0f / 6.0f);
    if (volume < 0)
        volume *= -1;
    return volume;
}

void fm_initMinMax(double bmin[3], double bmax[3])
{
    bmin[0] = FLT_MAX;
    bmin[1] = FLT_MAX;
    bmin[2] = FLT_MAX;

    bmax[0] = -FLT_MAX;
    bmax[1] = -FLT_MAX;
    bmax[2] = -FLT_MAX;
}

void fm_minmax(const double* p, double* bmin, double* bmax) // accmulate to a min-max value
{

    if (p[0] < bmin[0])
        bmin[0] = p[0];
    if (p[1] < bmin[1])
        bmin[1] = p[1];
    if (p[2] < bmin[2])
        bmin[2] = p[2];

    if (p[0] > bmax[0])
        bmax[0] = p[0];
    if (p[1] > bmax[1])
        bmax[1] = p[1];
    if (p[2] > bmax[2])
        bmax[2] = p[2];
}

double fm_distance(const double* p1, const double* p2)
{
    double dx = p1[0] - p2[0];
    double dy = p1[1] - p2[1];
    double dz = p1[2] - p2[2];

    return (double)sqrt(dx * dx + dy * dy + dz * dz);
}

void fm_inflateMinMax(double bmin[3], double bmax[3], double ratio)
{
    double inflate = fm_distance(bmin, bmax) * 0.5f * ratio;

    bmin[0] -= inflate;
    bmin[1] -= inflate;
    bmin[2] -= inflate;

    bmax[0] += inflate;
    bmax[1] += inflate;
    bmax[2] += inflate;
}

void fm_combineAABB(const double bmin1[3], const double bmax1[3], const double bmin2[3], const double bmax2[3],double bmin[3],double bmax[3])
{
    bmin[0] = bmin1[0];
    bmin[1] = bmin1[1];
    bmin[2] = bmin1[2];

    bmax[0] = bmax1[0];
    bmax[1] = bmax1[1];
    bmax[2] = bmax1[2];

    for (uint32_t i=0; i<3; i++)
    {
        if ( bmin2[i] < bmin[i])
        {
            bmin[i] = bmin2[i];
        }
        if ( bmax2[i] > bmax[i])
        {
            bmax[i] = bmax2[i];
        }
    }
}

double fm_volumeAABB(const double bmin[3],const double bmax[3])
{
    double dx = bmax[0] - bmin[0];
    double dy = bmax[1] - bmin[1];
    double dz = bmax[2] - bmin[2];
    return dx*dy*dz;
}

bool fm_intersectAABB(const double* bmin1, const double* bmax1, const double* bmin2, const double* bmax2)
{
    if ((bmin1[0] > bmax2[0]) || (bmin2[0] > bmax1[0]))
        return false;
    if ((bmin1[1] > bmax2[1]) || (bmin2[1] > bmax1[1]))
        return false;
    if ((bmin1[2] > bmax2[2]) || (bmin2[2] > bmax1[2]))
        return false;
    return true;
}

void fm_getAABB(uint32_t vcount, const double* points, uint32_t pstride, double* bmin, double* bmax)
{

    const uint8_t* source = (const uint8_t*)points;

    bmin[0] = points[0];
    bmin[1] = points[1];
    bmin[2] = points[2];

    bmax[0] = points[0];
    bmax[1] = points[1];
    bmax[2] = points[2];


    for (uint32_t i = 1; i < vcount; i++)
    {
        source += pstride;
        const double* p = (const double*)source;

        if (p[0] < bmin[0])
            bmin[0] = p[0];
        if (p[1] < bmin[1])
            bmin[1] = p[1];
        if (p[2] < bmin[2])
            bmin[2] = p[2];

        if (p[0] > bmax[0])
            bmax[0] = p[0];
        if (p[1] > bmax[1])
            bmax[1] = p[1];
        if (p[2] > bmax[2])
            bmax[2] = p[2];
    }
}


class MyVertexIndex : public fm_VertexIndex
{
public:
    MyVertexIndex(double granularity, bool snapToGrid)
    {
        mDoubleGranularity = granularity;
        mFloatGranularity = (float)granularity;
        mSnapToGrid = snapToGrid;
        mUseDouble = true;
        mKdTree.setUseDouble(true);
    }

    MyVertexIndex(float granularity, bool snapToGrid)
    {
        mDoubleGranularity = granularity;
        mFloatGranularity = (float)granularity;
        mSnapToGrid = snapToGrid;
        mUseDouble = false;
        mKdTree.setUseDouble(false);
    }

    virtual ~MyVertexIndex(void)
    {
    }


    double snapToGrid(double p)
    {
        double m = fmod(p, mDoubleGranularity);
        p -= m;
        return p;
    }

    float snapToGrid(float p)
    {
        float m = fmodf(p, mFloatGranularity);
        p -= m;
        return p;
    }

    uint32_t getIndex(const float* _p, bool& newPos) // get index for a vector float
    {
        uint32_t ret;

        if (mUseDouble)
        {
            double p[3];
            p[0] = _p[0];
            p[1] = _p[1];
            p[2] = _p[2];
            return getIndex(p, newPos);
        }

        newPos = false;

        float p[3];

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
        ret = mKdTree.getNearest(p, mFloatGranularity, found);
        if (!found)
        {
            newPos = true;
            ret = mKdTree.add(p[0], p[1], p[2]);
        }


        return ret;
    }

    uint32_t getIndex(const double* _p, bool& newPos) // get index for a vector double
    {
        uint32_t ret;

        if (!mUseDouble)
        {
            float p[3];
            p[0] = (float)_p[0];
            p[1] = (float)_p[1];
            p[2] = (float)_p[2];
            return getIndex(p, newPos);
        }

        newPos = false;

        double p[3];

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
        ret = mKdTree.getNearest(p, mDoubleGranularity, found);
        if (!found)
        {
            newPos = true;
            ret = mKdTree.add(p[0], p[1], p[2]);
        }


        return ret;
    }

    const float* getVerticesFloat(void) const
    {
        const float* ret = 0;

        assert(!mUseDouble);

        ret = mKdTree.getVerticesFloat();

        return ret;
    }

    const double* getVerticesDouble(void) const
    {
        const double* ret = 0;

        assert(mUseDouble);

        ret = mKdTree.getVerticesDouble();

        return ret;
    }

    const float* getVertexFloat(uint32_t index) const
    {
        const float* ret = 0;
        assert(!mUseDouble);
#    ifdef _DEBUG
        uint32_t vcount = mKdTree.getVcount();
        assert(index < vcount);
#    endif
        ret = mKdTree.getVerticesFloat();
        ret = &ret[index * 3];
        return ret;
    }

    const double* getVertexDouble(uint32_t index) const
    {
        const double* ret = 0;
        assert(mUseDouble);
#    ifdef _DEBUG
        uint32_t vcount = mKdTree.getVcount();
        assert(index < vcount);
#    endif
        ret = mKdTree.getVerticesDouble();
        ret = &ret[index * 3];

        return ret;
    }

    uint32_t getVcount(void) const
    {
        return mKdTree.getVcount();
    }

    bool isDouble(void) const
    {
        return mUseDouble;
    }


    bool saveAsObj(const char* fname, uint32_t tcount, uint32_t* indices)
    {
        bool ret = false;


        FILE* fph = fopen(fname, "wb");
        if (fph)
        {
            ret = true;

            uint32_t vcount = getVcount();
            if (mUseDouble)
            {
                const double* v = getVerticesDouble();
                for (uint32_t i = 0; i < vcount; i++)
                {
                    fprintf(fph, "v %0.9f %0.9f %0.9f\r\n", (float)v[0], (float)v[1], (float)v[2]);
                    v += 3;
                }
            }
            else
            {
                const float* v = getVerticesFloat();
                for (uint32_t i = 0; i < vcount; i++)
                {
                    fprintf(fph, "v %0.9f %0.9f %0.9f\r\n", v[0], v[1], v[2]);
                    v += 3;
                }
            }

            for (uint32_t i = 0; i < tcount; i++)
            {
                uint32_t i1 = *indices++;
                uint32_t i2 = *indices++;
                uint32_t i3 = *indices++;
                fprintf(fph, "f %d %d %d\r\n", i1 + 1, i2 + 1, i3 + 1);
            }
            fclose(fph);
        }

        return ret;
    }

private:
    bool mUseDouble : 1;
    bool mSnapToGrid : 1;
    double mDoubleGranularity;
    float mFloatGranularity;
    VERTEX_INDEX::KdTree mKdTree;
};

fm_VertexIndex* fm_createVertexIndex(double granularity, bool snapToGrid) // create an indexed vertex system for doubles
{
    MyVertexIndex* ret = new MyVertexIndex(granularity, snapToGrid);
    return static_cast<fm_VertexIndex*>(ret);
}

fm_VertexIndex* fm_createVertexIndex(float granularity, bool snapToGrid) // create an indexed vertext system for floats
{
    MyVertexIndex* ret = new MyVertexIndex(granularity, snapToGrid);
    return static_cast<fm_VertexIndex*>(ret);
}

void fm_releaseVertexIndex(fm_VertexIndex* vindex)
{
    MyVertexIndex* m = static_cast<MyVertexIndex*>(vindex);
    delete m;
}


}

//********************************************************************************************************************
// Defining the Voxel class
//********************************************************************************************************************
namespace VHACD
{

enum class VoxelFillMode
{
    eFloodFill, // This is the default behavior, after the voxelization step it uses a flood fill to determine 'inside'
                // from 'outside'. However, meshes with holes can fail and create hollow results.
    eSurfaceOnly, // Only consider the 'surface', will create 'skins' with hollow centers.
    eRaycastFill // Uses raycasting to determine inside from outside.
};


#define VHACD_VOXEL_BITS 10
#define VHACD_VOXEL_BITS2 20
#define VHACD_VOXEL_BIT_MASK ((1<<VHACD_VOXEL_BITS)-1)

    class Voxel
    {
    public:
        Voxel(void)
        {
        }
        inline Voxel(uint32_t index) : mVoxel(index)
        {
        }
        inline Voxel(uint32_t x, uint32_t y, uint32_t z)
        {
            mVoxel = (x<<VHACD_VOXEL_BITS2)|(y<<VHACD_VOXEL_BITS)|z;
        }
        inline bool operator==(const Voxel &v) const
        {
            return v.mVoxel == mVoxel;
        }

        inline void getVoxel(uint32_t &x,uint32_t &y,uint32_t &z) const
        {
            x = (mVoxel>>VHACD_VOXEL_BITS2);
            y = (mVoxel>>VHACD_VOXEL_BITS)&VHACD_VOXEL_BIT_MASK;
            z = mVoxel & VHACD_VOXEL_BIT_MASK;
        }

        inline void getVoxel(int32_t &x, int32_t &y, int32_t &z) const
        {
            x = (int32_t) (mVoxel >> VHACD_VOXEL_BITS2);
            y = (int32_t)((mVoxel >> VHACD_VOXEL_BITS)&VHACD_VOXEL_BIT_MASK);
            z = (int32_t)(mVoxel & VHACD_VOXEL_BIT_MASK);
        }

        inline uint32_t getX(void) const
        {
            return (mVoxel>>VHACD_VOXEL_BITS2);
        }

        inline uint32_t getY(void) const
        {
            return (mVoxel>>VHACD_VOXEL_BITS)&VHACD_VOXEL_BIT_MASK;
        }

        inline uint32_t getZ(void) const
        {
            return mVoxel & VHACD_VOXEL_BIT_MASK;
        }

        inline uint32_t getVoxelAddress(void) const
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
        VoxelPosition(double _x,double _y,double _z) : x(_x),y(_y),z(_z)
        {
        }
        double x;
        double y;
        double z;
    };

    using VoxelSet = std::unordered_set< Voxel, VoxelHash >;
    using VoxelPositionMap = std::unordered_map< Voxel,VoxelPosition, VoxelHash >;
    using VoxelVector = std::vector< Voxel >;

}

// Defining the vector class
namespace VHACD
{
//!    Vector dim 3.
template <typename T>
class Vec3
{
public:
    T& operator[](size_t i)
    {
        return m_data[i];
    }
    const T& operator[](size_t i) const
    {
        return m_data[i];
    }
    T& getX();
    T& getY();
    T& getZ();
    const T& getX() const;
    const T& getY() const;
    const T& getZ() const;
    T Normalize();
    T GetNorm() const;
    void operator=(const Vec3& rhs);
    void operator+=(const Vec3& rhs);
    void operator-=(const Vec3& rhs);
    void operator-=(T a);
    void operator+=(T a);
    void operator/=(T a);
    void operator*=(T a);
    Vec3 operator^(const Vec3& rhs) const;
    T operator*(const Vec3& rhs) const;
    Vec3 operator+(const Vec3& rhs) const;
    Vec3 operator-(const Vec3& rhs) const;
    Vec3 operator-() const;
    Vec3 operator*(T rhs) const;
    Vec3 operator/(T rhs) const;
    bool operator<(const Vec3& rhs) const;
    bool operator>(const Vec3& rhs) const;
    Vec3();
    Vec3(T a);
    Vec3(T x, T y, T z);
    Vec3(const Vec3& rhs);
    /*virtual*/ ~Vec3(void);

    // Compute the center of this bounding box and return the diagonal length
    T GetCenter(const Vec3& bmin, const Vec3& bmax)
    {
        getX() = (bmin.getX() + bmax.getX()) * 0.5;
        getY() = (bmin.getY() + bmax.getY()) * 0.5;
        getZ() = (bmin.getZ() + bmax.getZ()) * 0.5;
        T dx = bmax.getX() - bmin.getX();
        T dy = bmax.getY() - bmin.getY();
        T dz = bmax.getZ() - bmin.getZ();
        T diagonal = T(sqrt(dx * dx + dy * dy + dz * dz));
        return diagonal;
    }

    // Update the min/max values relative to this point
    void UpdateMinMax(Vec3& bmin, Vec3& bmax) const
    {
        if (getX() < bmin.getX())
        {
            bmin.getX() = getX();
        }
        if (getY() < bmin.getY())
        {
            bmin.getY() = getY();
        }
        if (getZ() < bmin.getZ())
        {
            bmin.getZ() = getZ();
        }
        if (getX() > bmax.getX())
        {
            bmax.getX() = getX();
        }
        if (getX() > bmax.getX())
        {
            bmax.getX() = getX();
        }
        if (getY() > bmax.getY())
        {
            bmax.getY() = getY();
        }
        if (getZ() > bmax.getZ())
        {
            bmax.getZ() = getZ();
        }
    }

    // Returns the squared distance between these two points
    T GetDistanceSquared(const Vec3& p) const
    {
        T dx = getX() - p.getX();
        T dy = getY() - p.getY();
        T dz = getZ() - p.getZ();
        return dx * dx + dy * dy + dz * dz;
    }

    T GetDistance(const Vec3& p) const
    {
        return sqrt(GetDistanceSquared(p));
    }

    // Returns the raw vector data as a pointer
    T* GetData(void)
    {
        return m_data;
    }

private:
    T m_data[3];
};
//!    Vector dim 2.
template <typename T>
class Vec2
{
public:
    T& operator[](size_t i)
    {
        return m_data[i];
    }
    const T& operator[](size_t i) const
    {
        return m_data[i];
    }
    T& getX();
    T& getY();
    const T& getX() const;
    const T& getY() const;
    void Normalize();
    T GetNorm() const;
    void operator=(const Vec2& rhs);
    void operator+=(const Vec2& rhs);
    void operator-=(const Vec2& rhs);
    void operator-=(T a);
    void operator+=(T a);
    void operator/=(T a);
    void operator*=(T a);
    T operator^(const Vec2& rhs) const;
    T operator*(const Vec2& rhs) const;
    Vec2 operator+(const Vec2& rhs) const;
    Vec2 operator-(const Vec2& rhs) const;
    Vec2 operator-() const;
    Vec2 operator*(T rhs) const;
    Vec2 operator/(T rhs) const;
    Vec2();
    Vec2(T a);
    Vec2(T x, T y);
    Vec2(const Vec2& rhs);
    /*virtual*/ ~Vec2(void);

private:
    T m_data[2];
};

template <typename T>
const bool Colinear(const Vec3<T>& a, const Vec3<T>& b, const Vec3<T>& c);
template <typename T>
const T ComputeVolume4(const Vec3<T>& a, const Vec3<T>& b, const Vec3<T>& c, const Vec3<T>& d);
}

namespace VHACD
{
template <typename T>
inline Vec3<T> operator*(T lhs, const Vec3<T>& rhs)
{
    return Vec3<T>(lhs * rhs.getX(), lhs * rhs.getY(), lhs * rhs.getZ());
}
template <typename T>
inline T& Vec3<T>::getX()
{
    return m_data[0];
}
template <typename T>
inline T& Vec3<T>::getY()
{
    return m_data[1];
}
template <typename T>
inline T& Vec3<T>::getZ()
{
    return m_data[2];
}
template <typename T>
inline const T& Vec3<T>::getX() const
{
    return m_data[0];
}
template <typename T>
inline const T& Vec3<T>::getY() const
{
    return m_data[1];
}
template <typename T>
inline const T& Vec3<T>::getZ() const
{
    return m_data[2];
}
template <typename T>
inline T Vec3<T>::Normalize()
{
    T n = sqrt(m_data[0] * m_data[0] + m_data[1] * m_data[1] + m_data[2] * m_data[2]);
    if (n != 0.0)
        (*this) /= n;
    return n;
}
template <typename T>
inline T Vec3<T>::GetNorm() const
{
    return sqrt(m_data[0] * m_data[0] + m_data[1] * m_data[1] + m_data[2] * m_data[2]);
}
template <typename T>
inline void Vec3<T>::operator=(const Vec3& rhs)
{
    this->m_data[0] = rhs.m_data[0];
    this->m_data[1] = rhs.m_data[1];
    this->m_data[2] = rhs.m_data[2];
}
template <typename T>
inline void Vec3<T>::operator+=(const Vec3& rhs)
{
    this->m_data[0] += rhs.m_data[0];
    this->m_data[1] += rhs.m_data[1];
    this->m_data[2] += rhs.m_data[2];
}
template <typename T>
inline void Vec3<T>::operator-=(const Vec3& rhs)
{
    this->m_data[0] -= rhs.m_data[0];
    this->m_data[1] -= rhs.m_data[1];
    this->m_data[2] -= rhs.m_data[2];
}
template <typename T>
inline void Vec3<T>::operator-=(T a)
{
    this->m_data[0] -= a;
    this->m_data[1] -= a;
    this->m_data[2] -= a;
}
template <typename T>
inline void Vec3<T>::operator+=(T a)
{
    this->m_data[0] += a;
    this->m_data[1] += a;
    this->m_data[2] += a;
}
template <typename T>
inline void Vec3<T>::operator/=(T a)
{
    this->m_data[0] /= a;
    this->m_data[1] /= a;
    this->m_data[2] /= a;
}
template <typename T>
inline void Vec3<T>::operator*=(T a)
{
    this->m_data[0] *= a;
    this->m_data[1] *= a;
    this->m_data[2] *= a;
}
template <typename T>
inline Vec3<T> Vec3<T>::operator^(const Vec3<T>& rhs) const
{
    return Vec3<T>(m_data[1] * rhs.m_data[2] - m_data[2] * rhs.m_data[1],
                   m_data[2] * rhs.m_data[0] - m_data[0] * rhs.m_data[2],
                   m_data[0] * rhs.m_data[1] - m_data[1] * rhs.m_data[0]);
}
template <typename T>
inline T Vec3<T>::operator*(const Vec3<T>& rhs) const
{
    return (m_data[0] * rhs.m_data[0] + m_data[1] * rhs.m_data[1] + m_data[2] * rhs.m_data[2]);
}
template <typename T>
inline Vec3<T> Vec3<T>::operator+(const Vec3<T>& rhs) const
{
    return Vec3<T>(m_data[0] + rhs.m_data[0], m_data[1] + rhs.m_data[1], m_data[2] + rhs.m_data[2]);
}
template <typename T>
inline Vec3<T> Vec3<T>::operator-(const Vec3<T>& rhs) const
{
    return Vec3<T>(m_data[0] - rhs.m_data[0], m_data[1] - rhs.m_data[1], m_data[2] - rhs.m_data[2]);
}
template <typename T>
inline Vec3<T> Vec3<T>::operator-() const
{
    return Vec3<T>(-m_data[0], -m_data[1], -m_data[2]);
}

template <typename T>
inline Vec3<T> Vec3<T>::operator*(T rhs) const
{
    return Vec3<T>(rhs * this->m_data[0], rhs * this->m_data[1], rhs * this->m_data[2]);
}
template <typename T>
inline Vec3<T> Vec3<T>::operator/(T rhs) const
{
    return Vec3<T>(m_data[0] / rhs, m_data[1] / rhs, m_data[2] / rhs);
}
template <typename T>
inline Vec3<T>::Vec3(T a)
{
    m_data[0] = m_data[1] = m_data[2] = a;
}
template <typename T>
inline Vec3<T>::Vec3(T x, T y, T z)
{
    m_data[0] = x;
    m_data[1] = y;
    m_data[2] = z;
}
template <typename T>
inline Vec3<T>::Vec3(const Vec3& rhs)
{
    m_data[0] = rhs.m_data[0];
    m_data[1] = rhs.m_data[1];
    m_data[2] = rhs.m_data[2];
}
template <typename T>
inline Vec3<T>::~Vec3(void){};

template <typename T>
inline Vec3<T>::Vec3()
{
}

template <typename T>
inline const bool Colinear(const Vec3<T>& a, const Vec3<T>& b, const Vec3<T>& c)
{
    return ((c.getZ() - a.getZ()) * (b.getY() - a.getY()) - (b.getZ() - a.getZ()) * (c.getY() - a.getY()) == 0.0 /*EPS*/) &&
           ((b.getZ() - a.getZ()) * (c.getX() - a.getX()) - (b.getX() - a.getX()) * (c.getZ() - a.getZ()) == 0.0 /*EPS*/) &&
           ((b.getX() - a.getX()) * (c.getY() - a.getY()) - (b.getY() - a.getY()) * (c.getX() - a.getX()) == 0.0 /*EPS*/);
}

template <typename T>
inline const T ComputeVolume4(const Vec3<T>& a, const Vec3<T>& b, const Vec3<T>& c, const Vec3<T>& d)
{
    return (a - d) * ((b - d) ^ (c - d));
}

template <typename T>
inline bool Vec3<T>::operator<(const Vec3& rhs) const
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
inline bool Vec3<T>::operator>(const Vec3& rhs) const
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
inline Vec2<T> operator*(T lhs, const Vec2<T>& rhs)
{
    return Vec2<T>(lhs * rhs.getX(), lhs * rhs.getY());
}
template <typename T>
inline T& Vec2<T>::getX()
{
    return m_data[0];
}
template <typename T>
inline T& Vec2<T>::getY()
{
    return m_data[1];
}
template <typename T>
inline const T& Vec2<T>::getX() const
{
    return m_data[0];
}
template <typename T>
inline const T& Vec2<T>::getY() const
{
    return m_data[1];
}
template <typename T>
inline void Vec2<T>::Normalize()
{
    T n = sqrt(m_data[0] * m_data[0] + m_data[1] * m_data[1]);
    if (n != 0.0)
        (*this) /= n;
}
template <typename T>
inline T Vec2<T>::GetNorm() const
{
    return sqrt(m_data[0] * m_data[0] + m_data[1] * m_data[1]);
}
template <typename T>
inline void Vec2<T>::operator=(const Vec2& rhs)
{
    this->m_data[0] = rhs.m_data[0];
    this->m_data[1] = rhs.m_data[1];
}
template <typename T>
inline void Vec2<T>::operator+=(const Vec2& rhs)
{
    this->m_data[0] += rhs.m_data[0];
    this->m_data[1] += rhs.m_data[1];
}
template <typename T>
inline void Vec2<T>::operator-=(const Vec2& rhs)
{
    this->m_data[0] -= rhs.m_data[0];
    this->m_data[1] -= rhs.m_data[1];
}
template <typename T>
inline void Vec2<T>::operator-=(T a)
{
    this->m_data[0] -= a;
    this->m_data[1] -= a;
}
template <typename T>
inline void Vec2<T>::operator+=(T a)
{
    this->m_data[0] += a;
    this->m_data[1] += a;
}
template <typename T>
inline void Vec2<T>::operator/=(T a)
{
    this->m_data[0] /= a;
    this->m_data[1] /= a;
}
template <typename T>
inline void Vec2<T>::operator*=(T a)
{
    this->m_data[0] *= a;
    this->m_data[1] *= a;
}
template <typename T>
inline T Vec2<T>::operator^(const Vec2<T>& rhs) const
{
    return m_data[0] * rhs.m_data[1] - m_data[1] * rhs.m_data[0];
}
template <typename T>
inline T Vec2<T>::operator*(const Vec2<T>& rhs) const
{
    return (m_data[0] * rhs.m_data[0] + m_data[1] * rhs.m_data[1]);
}
template <typename T>
inline Vec2<T> Vec2<T>::operator+(const Vec2<T>& rhs) const
{
    return Vec2<T>(m_data[0] + rhs.m_data[0], m_data[1] + rhs.m_data[1]);
}
template <typename T>
inline Vec2<T> Vec2<T>::operator-(const Vec2<T>& rhs) const
{
    return Vec2<T>(m_data[0] - rhs.m_data[0], m_data[1] - rhs.m_data[1]);
}
template <typename T>
inline Vec2<T> Vec2<T>::operator-() const
{
    return Vec2<T>(-m_data[0], -m_data[1]);
}

template <typename T>
inline Vec2<T> Vec2<T>::operator*(T rhs) const
{
    return Vec2<T>(rhs * this->m_data[0], rhs * this->m_data[1]);
}
template <typename T>
inline Vec2<T> Vec2<T>::operator/(T rhs) const
{
    return Vec2<T>(m_data[0] / rhs, m_data[1] / rhs);
}
template <typename T>
inline Vec2<T>::Vec2(T a)
{
    m_data[0] = m_data[1] = a;
}
template <typename T>
inline Vec2<T>::Vec2(T x, T y)
{
    m_data[0] = x;
    m_data[1] = y;
}
template <typename T>
inline Vec2<T>::Vec2(const Vec2& rhs)
{
    m_data[0] = rhs.m_data[0];
    m_data[1] = rhs.m_data[1];
}
template <typename T>
inline Vec2<T>::~Vec2(void){};

template <typename T>
inline Vec2<T>::Vec2()
{
}

/*
  InsideTriangle decides if a point P is Inside of the triangle
  defined by A, B, C.
*/
template <typename T>
inline const bool InsideTriangle(const Vec2<T>& a, const Vec2<T>& b, const Vec2<T>& c, const Vec2<T>& p)
{
    T ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
    T cCROSSap, bCROSScp, aCROSSbp;
    ax = c.getX() - b.getX();
    ay = c.getY() - b.getY();
    bx = a.getX() - c.getX();
    by = a.getY() - c.getY();
    cx = b.getX() - a.getX();
    cy = b.getY() - a.getY();
    apx = p.getX() - a.getX();
    apy = p.getY() - a.getY();
    bpx = p.getX() - b.getX();
    bpy = p.getY() - b.getY();
    cpx = p.getX() - c.getX();
    cpy = p.getY() - c.getY();
    aCROSSbp = ax * bpy - ay * bpx;
    cCROSSap = cx * apy - cy * apx;
    bCROSScp = bx * cpy - by * cpx;
    return ((aCROSSbp >= 0.0) && (bCROSScp >= 0.0) && (cCROSSap >= 0.0));
}
}





//********************************************************************************************************************
// Defining the SimpleMesh class
//********************************************************************************************************************
namespace VHACD
{

class SimpleMesh
{
public:
    SimpleMesh(void)
    {
    }

    SimpleMesh(const SimpleMesh &sm)
    {
        mOwnAllocation = true; // we allocated this

        mVertexCount = sm.mVertexCount; // assign the vertex count
        if (mVertexCount)
        {
            mVertices = new double[mVertexCount * 3];
            memcpy(mVertices, sm.mVertices, sizeof(double)*mVertexCount * 3); // allocate and copy the vertices
        }
        mTriangleCount = sm.mTriangleCount; // assign the triangle count
        if (mTriangleCount)
        {
            mIndices = new uint32_t[mTriangleCount * 3];
            memcpy(mIndices, sm.mIndices, sizeof(uint32_t)*mTriangleCount * 3); // allocate and copy the vertices
        }

        mBmin[0] = sm.mBmin[0];
        mBmin[1] = sm.mBmin[1];
        mBmin[2] = sm.mBmin[2];

        mBmax[0] = sm.mBmax[0];
        mBmax[1] = sm.mBmax[1];
        mBmax[2] = sm.mBmax[2];

        mCenter[0] = sm.mCenter[0];
        mCenter[1] = sm.mCenter[1];
        mCenter[2] = sm.mCenter[2];

        mVolume = sm.mVolume;

        mMeshId = sm.mMeshId;
    }

    SimpleMesh(uint32_t vcount,const double *vertices,uint32_t tcount,const uint32_t *indices)
    {
        mOwnAllocation = true;
        mVertexCount = vcount;
        if ( mVertexCount )
        {
            mVertices = new double[mVertexCount*3];
            memcpy(mVertices,vertices,sizeof(double)*mVertexCount*3);
        }
        mTriangleCount = tcount;
        if ( mTriangleCount )
        {
            mIndices = new uint32_t[mTriangleCount*3];
            memcpy(mIndices,indices,sizeof(uint32_t)*mTriangleCount*3);
        }
    }

    ~SimpleMesh(void)
    {
        releaseMeshData();
    }

    void releaseMeshData(void)
    {
        if ( mOwnAllocation )
        {
            delete []mVertices;
            delete []mIndices;
            mVertices = nullptr;
            mIndices = nullptr;
            mVertexCount = 0;
            mTriangleCount = 0;
            mOwnAllocation = false;
        }
    }

    bool        mOwnAllocation{false}; // true if we allocated these buffers and are therefore responsible for deleting them
    uint32_t    mMeshId{0}; // optional id to uniquely identify this mesh
    uint32_t    mVertexCount{0};
    uint32_t    mTriangleCount{0};
    double       *mVertices{nullptr};
    uint32_t    *mIndices{nullptr};
    double       mCenter[3];
    double      mVolume{0};
    double  mBmin[3];
    double  mBmax[3];
};

}

//******************************************************************************************
//  Declaration of the AABBTree class
//******************************************************************************************

namespace VHACD
{

class AABBTree
{
public:
    static AABBTree* create(const double* vertices, uint32_t numVerts, const uint32_t* indices, uint32_t numFaces);

    virtual bool raycast(const double* start,
                         const double* dir,
                         double& outT,
                         double& u,
                         double& v,
                         double& w,
                         double& faceSign,
                         uint32_t& faceIndex) const = 0;


    virtual bool getClosestPointWithinDistance(const double* point, double maxDistance, double* closestPoint) = 0;

    virtual void release(void) = 0;

protected:
    virtual ~AABBTree(void)
    {
    }
};

} 

namespace VHACD
{

template <typename T>
inline T Min(T a, T b)
{
    return a < b ? a : b;
}

template <typename T>
inline T Max(T a, T b)
{
    return a > b ? a : b;
}

#define VHACD_PI 3.141592653589f
const double k2Pi = 2.0f * VHACD_PI;
const double kInvPi = 1.0f / VHACD_PI;
const double kInv2Pi = 0.5f / VHACD_PI;
const double kDegToRad = VHACD_PI / 180.0f;
const double kRadToDeg = 180.0f / VHACD_PI;

inline double DegToRad(double t)
{
    return t * kDegToRad;
}

inline double RadToDeg(double t)
{
    return t * kRadToDeg;
}

inline double Sin(double theta)
{
    return sin(theta);
}

inline double Cos(double theta)
{
    return cos(theta);
}

inline void SinCos(double theta, double& s, double& c)
{
    // no optimizations yet
    s = sin(theta);
    c = cos(theta);
}

inline double Tan(double theta)
{
    return tan(theta);
}

inline double Sqrt(double x)
{
    return sqrt(x);
}

inline double ASin(double theta)
{
    return asin(theta);
}

inline double ACos(double theta)
{
    return acos(theta);
}

inline double ATan(double theta)
{
    return atan(theta);
}

inline double ATan2(double x, double y)
{
    return atan2(x, y);
}

inline double Abs(double x)
{
    return fabs(x);
}

inline double Pow(double b, double e)
{
    return pow(b, e);
}

inline double Sgn(double x)
{
    return (x < 0.0f ? -1.0f : 1.0f);
}

inline double Sign(double x)
{
    return x < 0.0f ? -1.0f : 1.0f;
}

inline double Mod(double x, double y)
{
    return fmod(x, y);
}

template <typename T>
inline void Swap(T& a, T& b)
{
    T tmp = a;
    a = b;
    b = tmp;
}

template <typename T>
inline T Clamp(T a, T low, T high)
{
    if (low > high)
        Swap(low, high);

    return Max(low, Min(a, high));
}

template <typename V, typename T>
inline V Lerp(const V& start, const V& end, const T& t)
{
    return start + (end - start) * t;
}

inline double InvSqrt(double x)
{
    return 1.0f / sqrt(x);
}

// round towards +infinity
inline int Round(double f)
{
    return int(f + 0.5f);
}

template <typename T>
T Normalize(const T& v)
{
    T a(v);
    a /= Length(v);
    return a;
}

template <typename T>
inline typename T::value_type LengthSq(const T v)
{
    return Dot(v, v);
}

template <typename T>
inline typename T::value_type Length(const T& v)
{
    typename T::value_type lSq = LengthSq(v);
    if (lSq)
        return Sqrt(LengthSq(v));
    else
        return 0.0f;
}

// this is mainly a helper function used by script
template <typename T>
inline typename T::value_type Distance(const T& v1, const T& v2)
{
    return Length(v1 - v2);
}

template <typename T>
inline T SafeNormalize(const T& v, const T& fallback = T())
{
    double l = LengthSq(v);
    if (l > 0.0f)
    {
        return v * InvSqrt(l);
    }
    else
        return fallback;
}

template <typename T>
inline T Sqr(T x)
{
    return x * x;
}

template <typename T>
inline T Cube(T x)
{
    return x * x * x;
}


template <typename T = double>
class XVector3
{
public:
    typedef T value_type;

    XVector3() : x(0.0f), y(0.0f), z(0.0f){};
    XVector3(T a) : x(a), y(a), z(a){};
    XVector3(const T* p) : x(p[0]), y(p[1]), z(p[2]){};
    XVector3(T x_, T y_, T z_) : x(x_), y(y_), z(z_)
    {
    }

    operator T*()
    {
        return &x;
    }
    operator const T*() const
    {
        return &x;
    };

    void Set(T x_, T y_, T z_)
    {
        x = x_;
        y = y_;
        z = z_;
    }

    XVector3<T> operator*(T scale) const
    {
        XVector3<T> r(*this);
        r *= scale;
        return r;
    }
    XVector3<T> operator/(T scale) const
    {
        XVector3<T> r(*this);
        r /= scale;
        return r;
    }
    XVector3<T> operator+(const XVector3<T>& v) const
    {
        XVector3<T> r(*this);
        r += v;
        return r;
    }
    XVector3<T> operator-(const XVector3<T>& v) const
    {
        XVector3<T> r(*this);
        r -= v;
        return r;
    }
    XVector3<T> operator/(const XVector3<T>& v) const
    {
        XVector3<T> r(*this);
        r /= v;
        return r;
    }
    XVector3<T> operator*(const XVector3<T>& v) const
    {
        XVector3<T> r(*this);
        r *= v;
        return r;
    }

    XVector3<T>& operator*=(T scale)
    {
        x *= scale;
        y *= scale;
        z *= scale;
        return *this;
    }
    XVector3<T>& operator/=(T scale)
    {
        T s(1.0f / scale);
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }
    XVector3<T>& operator+=(const XVector3<T>& v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    XVector3<T>& operator-=(const XVector3<T>& v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }
    XVector3<T>& operator/=(const XVector3<T>& v)
    {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        return *this;
    }
    XVector3<T>& operator*=(const XVector3<T>& v)
    {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        return *this;
    }

    bool operator!=(const XVector3<T>& v) const
    {
        return (x != v.x || y != v.y || z != v.z);
    }

    // negate
    XVector3<T> operator-() const
    {
        return XVector3<T>(-x, -y, -z);
    }


    T x, y, z;
};

typedef XVector3<double> Vector3;

// lhs scalar scale
template <typename T>
XVector3<T> operator*(T lhs, const XVector3<T>& rhs)
{
    XVector3<T> r(rhs);
    r *= lhs;
    return r;
}

template <typename T>
bool operator==(const XVector3<T>& lhs, const XVector3<T>& rhs)
{
    return (lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z);
}

template <typename T>
typename T::value_type Dot3(const T& v1, const T& v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline double Dot3(const double* v1, const double* v2)
{
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}


template <typename T>
inline T Dot(const XVector3<T>& v1, const XVector3<T>& v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline Vector3 Cross(const Vector3& b, const Vector3& c)
{
    return Vector3(b.y * c.z - b.z * c.y, b.z * c.x - b.x * c.z, b.x * c.y - b.y * c.x);
}

template <typename T>
inline XVector3<T> Max(const XVector3<T>& a, const XVector3<T>& b)
{
    return XVector3<T>(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z));
}

template <typename T>
inline XVector3<T> Min(const XVector3<T>& a, const XVector3<T>& b)
{
    return XVector3<T>(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z));
}

template <typename T>
inline XVector3<T> Abs(const XVector3<T>& a)
{
    return XVector3<T>(Abs(a.x), Abs(a.y), Abs(a.z));
}

template <typename T>
inline int LongestAxis(const XVector3<T>& v)
{
    if (v.x > v.y && v.x > v.z)
        return 0;
    else
        return (v.y > v.z) ? 1 : 2;
}

struct Bounds
{
    inline Bounds() : lower(FLT_MAX), upper(-FLT_MAX)
    {
    }

    inline Bounds(const Vector3& lower, const Vector3& upper) : lower(lower), upper(upper)
    {
    }

    inline Vector3 GetCenter() const
    {
        return 0.5 * (lower + upper);
    }

    inline Vector3 GetEdges() const
    {
        return upper - lower;
    }

    inline void Expand(double r)
    {
        lower -= Vector3(r);
        upper += Vector3(r);
    }

    inline void Expand(const Vector3& r)
    {
        lower -= r;
        upper += r;
    }

    inline bool Empty() const
    {
        return lower.x >= upper.x || lower.y >= upper.y || lower.z >= upper.z;
    }

    inline bool Overlaps(const Vector3& p) const
    {
        if (p.x < lower.x || p.y < lower.y || p.z < lower.z || p.x > upper.x || p.y > upper.y || p.z > upper.z)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    inline bool Overlaps(const Bounds& b) const
    {
        if (lower.x > b.upper.x || lower.y > b.upper.y || lower.z > b.upper.z || upper.x < b.lower.x ||
            upper.y < b.lower.y || upper.z < b.lower.z)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    Vector3 lower;
    Vector3 upper;
};

inline Bounds Union(const Bounds& a, const Vector3& b)
{
    return Bounds(Min(a.lower, b), Max(a.upper, b));
}

inline Bounds Union(const Bounds& a, const Bounds& b)
{
    return Bounds(Min(a.lower, b.lower), Max(a.upper, b.upper));
}

inline Bounds Intersection(const Bounds& a, const Bounds& b)
{
    return Bounds(Max(a.lower, b.lower), Min(a.upper, b.upper));
}



#define VHACD_CROSS(dest, v1, v2)                                                                                            \
    dest[0] = v1[1] * v2[2] - v1[2] * v2[1];                                                                           \
    dest[1] = v1[2] * v2[0] - v1[0] * v2[2];                                                                           \
    dest[2] = v1[0] * v2[1] - v1[1] * v2[0];

#define VHACD_DOT(v1, v2) (v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2])

#define VHACD_SUB(dest, v1, v2)                                                                                              \
    dest[0] = v1[0] - v2[0];                                                                                           \
    dest[1] = v1[1] - v2[1];                                                                                           \
    dest[2] = v1[2] - v2[2];

#define VHACD_FINDMINMAX(x0, x1, x2, min, max)                                                                               \
    min = max = x0;                                                                                                    \
    if (x1 < min)                                                                                                      \
        min = x1;                                                                                                      \
    if (x1 > max)                                                                                                      \
        max = x1;                                                                                                      \
    if (x2 < min)                                                                                                      \
        min = x2;                                                                                                      \
    if (x2 > max)                                                                                                      \
        max = x2;


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

int planeBoxOverlap(double normal[3], double vert[3], double maxbox[3]) // -NJMP-
{

    int q;

    double vmin[3], vmax[3], v;

    for (q = 0; q <= 2; q++)
    {
        v = vert[q]; // -NJMP-

        if (normal[q] > 0.0f)
        {
            vmin[q] = -maxbox[q] - v; // -NJMP-
            vmax[q] = maxbox[q] - v; // -NJMP-
        }
        else
        {
            vmin[q] = maxbox[q] - v; // -NJMP-
            vmax[q] = -maxbox[q] - v; // -NJMP-
        }
    }

    if (VHACD_DOT(normal, vmin) > 0.0f)
        return 0; // -NJMP-
    if (VHACD_DOT(normal, vmax) >= 0.0f)
        return 1; // -NJMP-

    return 0;
}


int triBoxOverlap(double boxcenter[3], double boxhalfsize[3], double triverts[3][3])

{

    /*    use separating axis theorem to test overlap between triangle and box */
    /*    need to test for overlap in these directions: */
    /*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
    /*       we do not even need to test these) */
    /*    2) normal of the triangle */
    /*    3) crossproduct(edge from tri, {x,y,z}-directin) */
    /*       this gives 3x3=9 more tests */

    double v0[3], v1[3], v2[3];

    //   double axis[3];

    double min, max, p0, p1, p2, rad, fex, fey, fez; // -NJMP- "d" local variable removed

    double normal[3], e0[3], e1[3], e2[3];


    /* This is the fastest branch on Sun */

    /* move everything so that the boxcenter is in (0,0,0) */

    VHACD_SUB(v0, triverts[0], boxcenter);

    VHACD_SUB(v1, triverts[1], boxcenter);

    VHACD_SUB(v2, triverts[2], boxcenter);


    /* compute triangle edges */

    VHACD_SUB(e0, v1, v0); /* tri edge 0 */

    VHACD_SUB(e1, v2, v1); /* tri edge 1 */

    VHACD_SUB(e2, v0, v2); /* tri edge 2 */


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

    VHACD_FINDMINMAX(v0[0], v1[0], v2[0], min, max);

    if (min > boxhalfsize[0] || max < -boxhalfsize[0])
        return 0;


    /* test in 1-direction */

    VHACD_FINDMINMAX(v0[1], v1[1], v2[1], min, max);

    if (min > boxhalfsize[1] || max < -boxhalfsize[1])
        return 0;


    /* test in 2-direction */

    VHACD_FINDMINMAX(v0[2], v1[2], v2[2], min, max);

    if (min > boxhalfsize[2] || max < -boxhalfsize[2])
        return 0;


    /* Bullet 2: */

    /*  test if the box intersects the plane of the triangle */

    /*  compute plane equation of triangle: normal*x+d=0 */

    VHACD_CROSS(normal, e0, e1);

    // -NJMP- (line removed here)

    if (!planeBoxOverlap(normal, v0, boxhalfsize))
        return 0; // -NJMP-


    return 1; /* box and triangle overlaps */
}


bool TriangleBoxOverlap(Vector3 lower, Vector3 upper, Vector3 a, Vector3 b, Vector3 c)
{
    Vector3 center = (lower + upper) * 0.5f;
    Vector3 halfEdges = (upper - lower) * 0.5f;

    double m[3][3];
    (Vector3&)m[0] = a;
    (Vector3&)m[1] = b;
    (Vector3&)m[2] = c;

    return triBoxOverlap((double*)&center.x, (double*)&halfEdges.x, m) ? true : false;
}

inline double minf(const double a, const double b)
{
    return a < b ? a : b;
}
inline double maxf(const double a, const double b)
{
    return a > b ? a : b;
}

inline bool IntersectRayAABBFast(const Vector3& pos, const Vector3& rcp_dir, const Vector3& min, const Vector3& max, double& t)
{

    double l1 = (min.x - pos.x) * rcp_dir.x, l2 = (max.x - pos.x) * rcp_dir.x, lmin = minf(l1, l2), lmax = maxf(l1, l2);

    l1 = (min.y - pos.y) * rcp_dir.y;
    l2 = (max.y - pos.y) * rcp_dir.y;
    lmin = maxf(minf(l1, l2), lmin);
    lmax = minf(maxf(l1, l2), lmax);

    l1 = (min.z - pos.z) * rcp_dir.z;
    l2 = (max.z - pos.z) * rcp_dir.z;
    lmin = maxf(minf(l1, l2), lmin);
    lmax = minf(maxf(l1, l2), lmax);

    // return ((lmax > 0.f) & (lmax >= lmin));
    // return ((lmax > 0.f) & (lmax > lmin));
    bool hit = ((lmax >= 0.f) & (lmax >= lmin));
    if (hit)
        t = lmin;
    return hit;
}

inline bool IntersectRayAABB(
    const Vector3& start, const Vector3& dir, const Vector3& min, const Vector3& max, double& t, Vector3* /*normal*/)
{
    //! calculate candidate plane on each axis
    double tx = -1.0f, ty = -1.0f, tz = -1.0f;
    bool inside = true;

    //! use unrolled loops

    //! x
    if (start.x < min.x)
    {
        if (dir.x != 0.0f)
            tx = (min.x - start.x) / dir.x;
        inside = false;
    }
    else if (start.x > max.x)
    {
        if (dir.x != 0.0f)
            tx = (max.x - start.x) / dir.x;
        inside = false;
    }

    //! y
    if (start.y < min.y)
    {
        if (dir.y != 0.0f)
            ty = (min.y - start.y) / dir.y;
        inside = false;
    }
    else if (start.y > max.y)
    {
        if (dir.y != 0.0f)
            ty = (max.y - start.y) / dir.y;
        inside = false;
    }

    //! z
    if (start.z < min.z)
    {
        if (dir.z != 0.0f)
            tz = (min.z - start.z) / dir.z;
        inside = false;
    }
    else if (start.z > max.z)
    {
        if (dir.z != 0.0f)
            tz = (max.z - start.z) / dir.z;
        inside = false;
    }

    //! if point inside all planes
    if (inside)
    {
        t = 0.0f;
        return true;
    }

    //! we now have t values for each of possible intersection planes
    //! find the maximum to get the intersection point
    double tmax = tx;
    int taxis = 0;

    if (ty > tmax)
    {
        tmax = ty;
        taxis = 1;
    }
    if (tz > tmax)
    {
        tmax = tz;
        taxis = 2;
    }

    if (tmax < 0.0f)
        return false;

    //! check that the intersection point lies on the plane we picked
    //! we don't test the axis of closest intersection for precision reasons

    //! no eps for now
    double eps = 0.0f;

    Vector3 hit = start + dir * tmax;

    if ((hit.x < min.x - eps || hit.x > max.x + eps) && taxis != 0)
        return false;
    if ((hit.y < min.y - eps || hit.y > max.y + eps) && taxis != 1)
        return false;
    if ((hit.z < min.z - eps || hit.z > max.z + eps) && taxis != 2)
        return false;

    //! output results
    t = tmax;

    return true;
}

// Moller and Trumbore's method
inline bool IntersectRayTriTwoSided(const Vector3& p,
                                    const Vector3& dir,
                                    const Vector3& a,
                                    const Vector3& b,
                                    const Vector3& c,
                                    double& t,
                                    double& u,
                                    double& v,
                                    double& w,
                                    double& sign,
                                    Vector3* normal)
{
    Vector3 ab = b - a;
    Vector3 ac = c - a;
    Vector3 n = Cross(ab, ac);

    double d = Dot(-dir, n);
    double ood = 1.0f / d; // No need to check for division by zero here as infinity aritmetic will save us...
    Vector3 ap = p - a;

    t = Dot(ap, n) * ood;
    if (t < 0.0f)
        return false;

    Vector3 e = Cross(-dir, ap);
    v = Dot(ac, e) * ood;
    if (v < 0.0f || v > 1.0f) // ...here...
        return false;
    w = -Dot(ab, e) * ood;
    if (w < 0.0f || v + w > 1.0f) // ...and here
        return false;

    u = 1.0f - v - w;
    if (normal)
        *normal = n;

    sign = d;

    return true;
}

inline Vector3 ClosestPointToAABB(const Vector3& p, const Vector3& lower, const Vector3& upper)
{
    Vector3 c;

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

inline double DistanceToAABB(const Vector3& p, const Vector3& lower, const Vector3& upper)
{
    Vector3 cp = ClosestPointToAABB(p, lower, upper);

    return Length(p - cp);
}

// RTCD 5.1.5, page 142
inline Vector3 ClosestPointOnTriangle(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& p, double& v, double& w)
{
    Vector3 ab = b - a;
    Vector3 ac = c - a;
    Vector3 ap = p - a;

    double d1 = Dot(ab, ap);
    double d2 = Dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
    {
        v = 0.0f;
        w = 0.0f;
        return a;
    }

    Vector3 bp = p - b;
    double d3 = Dot(ab, bp);
    double d4 = Dot(ac, bp);
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

    Vector3 cp = p - c;
    double d5 = Dot(ab, cp);
    double d6 = Dot(ac, cp);
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


class AABBTreeImpl : public AABBTree
{
public:
    AABBTreeImpl(const AABBTreeImpl&);
    AABBTreeImpl& operator=(const AABBTreeImpl&);

    AABBTreeImpl(const Vector3* vertices, uint32_t numVerts, const uint32_t* indices, uint32_t numFaces);

    bool TraceRay(const Vector3& start,
                  const Vector3& dir,
                  double& outT,
                  double& u,
                  double& v,
                  double& w,
                  double& faceSign,
                  uint32_t& faceIndex) const;


    Vector3 GetCenter() const
    {
        return (m_nodes[0].m_minExtents + m_nodes[0].m_maxExtents) * 0.5f;
    }
    Vector3 GetMinExtents() const
    {
        return m_nodes[0].m_minExtents;
    }
    Vector3 GetMaxExtents() const
    {
        return m_nodes[0].m_maxExtents;
    }

    virtual bool raycast(const double* start,
                         const double* dir,
                         double& outT,
                         double& u,
                         double& v,
                         double& w,
                         double& faceSign,
                         uint32_t& faceIndex) const final
    {
        bool ret = TraceRay(*(const Vector3*)start, *(const Vector3*)dir, outT, u, v, w, faceSign, faceIndex);
        return ret;
    }

    virtual void release(void) final
    {
        delete this;
    }

private:
    struct Node
    {
        Node() : m_numFaces(0), m_faces(NULL), m_minExtents(0.0f), m_maxExtents(0.0f)
        {
        }

        union
        {
            uint32_t m_children;
            uint32_t m_numFaces;
        };

        uint32_t* m_faces;
        Vector3 m_minExtents;
        Vector3 m_maxExtents;
    };


    struct BoundsAABB
    {
        BoundsAABB() : m_min(0.0f), m_max(0.0f)
        {
        }

        BoundsAABB(const Vector3& min, const Vector3& max) : m_min(min), m_max(max)
        {
        }

        inline double GetVolume() const
        {
            Vector3 e = m_max - m_min;
            return (e.x * e.y * e.z);
        }

        inline double GetSurfaceArea() const
        {
            Vector3 e = m_max - m_min;
            return 2.0f * (e.x * e.y + e.x * e.z + e.y * e.z);
        }

        inline void Union(const BoundsAABB& b)
        {
            m_min = Min(m_min, b.m_min);
            m_max = Max(m_max, b.m_max);
        }

        Vector3 m_min;
        Vector3 m_max;
    };

    typedef std::vector<uint32_t> IndexArray;
    typedef std::vector<Vector3> PositionArray;
    typedef std::vector<Node> NodeArray;
    typedef std::vector<uint32_t> FaceArray;
    typedef std::vector<BoundsAABB> FaceBoundsArray;

    // partition the objects and return the number of objects in the lower partition
    uint32_t PartitionMedian(Node& n, uint32_t* faces, uint32_t numFaces);
    uint32_t PartitionSAH(Node& n, uint32_t* faces, uint32_t numFaces);

    void Build();

    void BuildRecursive(uint32_t nodeIndex, uint32_t* faces, uint32_t numFaces);

    void TraceRecursive(uint32_t nodeIndex,
                        const Vector3& start,
                        const Vector3& dir,
                        double& outT,
                        double& u,
                        double& v,
                        double& w,
                        double& faceSign,
                        uint32_t& faceIndex) const;


    bool GetClosestPointWithinDistance(
        const Vector3& point, const double maxDis, double& dis, double& v, double& w, uint32_t& faceIndex, Vector3& closest) const;

    virtual bool getClosestPointWithinDistance(const double* point, double maxDistance, double* closestPoint) final
    {
        double dis, v, w;
        uint32_t faceIndex;
        bool hit =
            GetClosestPointWithinDistance(*(const Vector3*)point, maxDistance, dis, v, w, faceIndex, *(Vector3*)closestPoint);
        return hit;
    }

    void GetClosestPointWithinDistanceSqRecursive(uint32_t nodeIndex,
                                                  const Vector3& point,
                                                  double& outDisSq,
                                                  double& outV,
                                                  double& outW,
                                                  uint32_t& outFaceIndex,
                                                  Vector3& closest) const;

    void CalculateFaceBounds(uint32_t* faces, uint32_t numFaces, Vector3& outMinExtents, Vector3& outMaxExtents);

    uint32_t GetNumFaces() const
    {
        return m_numFaces;
    }

    uint32_t GetNumNodes() const
    {
        return uint32_t(m_nodes.size());
    }

    // track the next free node
    uint32_t m_freeNode;

    const Vector3* m_vertices;
    const uint32_t m_numVerts;

    const uint32_t* m_indices;
    const uint32_t m_numFaces;

    FaceArray m_faces;
    NodeArray m_nodes;
    FaceBoundsArray m_faceBounds;

    // stats
    uint32_t m_treeDepth;
    uint32_t m_innerNodes;
    uint32_t m_leafNodes;

    uint32_t s_depth{0};
};


AABBTreeImpl::AABBTreeImpl(const Vector3* vertices, uint32_t numVerts, const uint32_t* indices, uint32_t numFaces)
    : m_vertices(vertices), m_numVerts(numVerts), m_indices(indices), m_numFaces(numFaces)
{
    // build stats
    m_treeDepth = 0;
    m_innerNodes = 0;
    m_leafNodes = 0;

    Build();
}

namespace
{

struct FaceSorter
{
    FaceSorter(const Vector3* positions, const uint32_t* indices, uint32_t n, uint32_t axis)
        : m_vertices(positions), m_indices(indices), m_numIndices(n), m_axis(axis)
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
        const Vector3& a = m_vertices[m_indices[face * 3 + 0]];
        const Vector3& b = m_vertices[m_indices[face * 3 + 1]];
        const Vector3& c = m_vertices[m_indices[face * 3 + 2]];

        return (a[m_axis] + b[m_axis] + c[m_axis]) / 3.0f;
    }

    const Vector3* m_vertices;
    const uint32_t* m_indices;
    uint32_t m_numIndices;
    uint32_t m_axis;
};


} // anonymous namespace

void AABBTreeImpl::CalculateFaceBounds(uint32_t* faces, uint32_t numFaces, Vector3& outMinExtents, Vector3& outMaxExtents)
{
    Vector3 minExtents(FLT_MAX);
    Vector3 maxExtents(-FLT_MAX);

    // calculate face bounds
    for (uint32_t i = 0; i < numFaces; ++i)
    {
        Vector3 a = Vector3(m_vertices[m_indices[faces[i] * 3 + 0]]);
        Vector3 b = Vector3(m_vertices[m_indices[faces[i] * 3 + 1]]);
        Vector3 c = Vector3(m_vertices[m_indices[faces[i] * 3 + 2]]);

        minExtents = Min(a, minExtents);
        maxExtents = Max(a, maxExtents);

        minExtents = Min(b, minExtents);
        maxExtents = Max(b, maxExtents);

        minExtents = Min(c, minExtents);
        maxExtents = Max(c, maxExtents);
    }

    outMinExtents = minExtents;
    outMaxExtents = maxExtents;
}

void AABBTreeImpl::Build()
{
    assert(m_numFaces * 3);

    // const double startTime = GetSeconds();

    const uint32_t numFaces = m_numFaces;

    // build initial list of faces
    m_faces.reserve(numFaces);

    // calculate bounds of each face and store
    m_faceBounds.reserve(numFaces);

    std::vector<BoundsAABB> stack;
    for (uint32_t i = 0; i < numFaces; ++i)
    {
        BoundsAABB top;
        CalculateFaceBounds(&i, 1, top.m_min, top.m_max);

        m_faces.push_back(i);
        m_faceBounds.push_back(top);
    }

    m_nodes.reserve(uint32_t(numFaces * 1.5f));

    // allocate space for all the nodes
    m_freeNode = 1;

    // start building
    BuildRecursive(0, &m_faces[0], numFaces);

    assert(s_depth == 0);
}

// partion faces around the median face
uint32_t AABBTreeImpl::PartitionMedian(Node& n, uint32_t* faces, uint32_t numFaces)
{
    FaceSorter predicate(&m_vertices[0], &m_indices[0], m_numFaces * 3, LongestAxis(n.m_maxExtents - n.m_minExtents));
    std::nth_element(faces, faces + numFaces / 2, faces + numFaces, predicate);

    return numFaces / 2;
}

// partion faces based on the surface area heuristic
uint32_t AABBTreeImpl::PartitionSAH(Node& n, uint32_t* faces, uint32_t numFaces)
{
//    (n);
    uint32_t bestAxis = 0;
    uint32_t bestIndex = 0;
    double bestCost = FLT_MAX;

    for (uint32_t a = 0; a < 3; ++a)
    {
        // sort faces by centroids
        FaceSorter predicate(&m_vertices[0], &m_indices[0], m_numFaces * 3, a);
        std::sort(faces, faces + numFaces, predicate);

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
    FaceSorter predicate(&m_vertices[0], &m_indices[0], m_numFaces * 3, bestAxis);
    std::sort(faces, faces + numFaces, predicate);

    return bestIndex + 1;
}

void AABBTreeImpl::BuildRecursive(uint32_t nodeIndex, uint32_t* faces, uint32_t numFaces)
{
    const uint32_t kMaxFacesPerLeaf = 6;

    // if we've run out of nodes allocate some more
    if (nodeIndex >= m_nodes.size())
    {
        uint32_t s = std::max(uint32_t(1.5f * m_nodes.size()), 512U);

        // cout << "Resizing tree, current size: " << m_nodes.size()*sizeof(Node) << " new size: " << s*sizeof(Node) <<
        // endl;

        m_nodes.resize(s);
    }

    // a reference to the current node, need to be careful here as this reference may become invalid if array is resized
    Node& n = m_nodes[nodeIndex];

    // track max tree depth
    ++s_depth;
    m_treeDepth = std::max(m_treeDepth, s_depth);

    CalculateFaceBounds(faces, numFaces, n.m_minExtents, n.m_maxExtents);

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

struct StackEntry
{
    uint32_t m_node;
    double m_dist;
};


bool AABBTreeImpl::TraceRay(const Vector3& start,
                            const Vector3& dir,
                            double& outT,
                            double& u,
                            double& v,
                            double& w,
                            double& faceSign,
                            uint32_t& faceIndex) const
{
    outT = FLT_MAX;
    TraceRecursive(0, start, dir, outT, u, v, w, faceSign, faceIndex);
    return (outT != FLT_MAX);
}

void AABBTreeImpl::TraceRecursive(uint32_t nodeIndex,
                                  const Vector3& start,
                                  const Vector3& dir,
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

        IntersectRayAABB(start, dir, leftChild.m_minExtents, leftChild.m_maxExtents, dist[0], NULL);
        IntersectRayAABB(start, dir, rightChild.m_minExtents, rightChild.m_maxExtents, dist[1], NULL);

        uint32_t closest = 0;
        uint32_t furthest = 1;

        if (dist[1] < dist[0])
        {
            closest = 1;
            furthest = 0;
        }

        if (dist[closest] < outT)
            TraceRecursive(node.m_children + closest, start, dir, outT, outU, outV, outW, faceSign, faceIndex);

        if (dist[furthest] < outT)
            TraceRecursive(node.m_children + furthest, start, dir, outT, outU, outV, outW, faceSign, faceIndex);
    }
    else
    {
        Vector3 normal;
        double t, u, v, w, s;

        for (uint32_t i = 0; i < node.m_numFaces; ++i)
        {
            uint32_t indexStart = node.m_faces[i] * 3;

            const Vector3& a = m_vertices[m_indices[indexStart + 0]];
            const Vector3& b = m_vertices[m_indices[indexStart + 1]];
            const Vector3& c = m_vertices[m_indices[indexStart + 2]];
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

bool AABBTreeImpl::GetClosestPointWithinDistance(
    const Vector3& point, const double maxDis, double& dis, double& v, double& w, uint32_t& faceIndex, Vector3& closest) const
{
    dis = maxDis;
    faceIndex = uint32_t(~0);
    double disSq = dis * dis;

    GetClosestPointWithinDistanceSqRecursive(0, point, disSq, v, w, faceIndex, closest);
    dis = sqrt(disSq);

    return (faceIndex < (~((unsigned int)0)));
}

void AABBTreeImpl::GetClosestPointWithinDistanceSqRecursive(uint32_t nodeIndex,
                                                            const Vector3& point,
                                                            double& outDisSq,
                                                            double& outV,
                                                            double& outW,
                                                            uint32_t& outFaceIndex,
                                                            Vector3& closestPoint) const
{
    const Node& node = m_nodes[nodeIndex];

    if (node.m_faces == NULL)
    {
        // find closest node
        const Node& leftChild = m_nodes[node.m_children + 0];
        const Node& rightChild = m_nodes[node.m_children + 1];

        // double dist[2] = { FLT_MAX, FLT_MAX };
        Vector3 lp = ClosestPointToAABB(point, leftChild.m_minExtents, leftChild.m_maxExtents);
        Vector3 rp = ClosestPointToAABB(point, rightChild.m_minExtents, rightChild.m_maxExtents);


        uint32_t closest = 0;
        uint32_t furthest = 1;
        double dcSq = LengthSq(point - lp);
        double dfSq = LengthSq(point - rp);
        if (dfSq < dcSq)
        {
            closest = 1;
            furthest = 0;
            std::swap(dfSq, dcSq);
        }

        if (dcSq < outDisSq)
        {
            GetClosestPointWithinDistanceSqRecursive(
                node.m_children + closest, point, outDisSq, outV, outW, outFaceIndex, closestPoint);
        }

        if (dfSq < outDisSq)
        {
            GetClosestPointWithinDistanceSqRecursive(
                node.m_children + furthest, point, outDisSq, outV, outW, outFaceIndex, closestPoint);
        }
    }
    else
    {

        double v, w;
        for (uint32_t i = 0; i < node.m_numFaces; ++i)
        {
            uint32_t indexStart = node.m_faces[i] * 3;

            const Vector3& a = m_vertices[m_indices[indexStart + 0]];
            const Vector3& b = m_vertices[m_indices[indexStart + 1]];
            const Vector3& c = m_vertices[m_indices[indexStart + 2]];

            Vector3 cp = ClosestPointOnTriangle(a, b, c, point, v, w);
            double disSq = LengthSq(cp - point);

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

AABBTree* AABBTree::create(const double* vertices, uint32_t numVerts, const uint32_t* indices, uint32_t numFaces)
{
    auto ret = new AABBTreeImpl((const Vector3*)vertices, numVerts, indices, numFaces);
    return static_cast<AABBTree*>(ret);
}


} // namespace aabbtree



//******************************************************************************************
//  Declaration of the RaycastMesh class
//******************************************************************************************

namespace VHACD
{

// Very simple brute force raycast against a triangle mesh.  Tests every triangle; no hierachy.
// Does a deep copy, always does calculations with full double float precision
class RaycastMesh
{
public:
    static RaycastMesh* createRaycastMesh(uint32_t vcount, // The number of vertices in the source triangle mesh
                                          const double* vertices, // The array of vertex positions in the format
                                                                  // x1,y1,z1..x2,y2,z2.. etc.
                                          uint32_t tcount, // The number of triangles in the source triangle mesh
                                          const uint32_t* indices); // The triangle indices in the format of i1,i2,i3
                                                                    // ... i4,i5,i6, ...

    static RaycastMesh* createRaycastMesh(uint32_t vcount, // The number of vertices in the source triangle mesh
                                          const float* vertices, // The array of vertex positions in the format
                                                                 // x1,y1,z1..x2,y2,z2.. etc.
                                          uint32_t tcount, // The number of triangles in the source triangle mesh
                                          const uint32_t* indices); // The triangle indices in the format of i1,i2,i3
                                                                    // ... i4,i5,i6, ...


    // Uses high speed AABB raycasting
    virtual bool raycast(const double* start,
                         const double* dir,
                         double& outT,
                         double& u,
                         double& v,
                         double& w,
                         double& faceSign,
                         uint32_t& faceIndex) const = 0;

    virtual bool raycast(
        const double* start, const double* to, double& outT, double& faceSign, double* hitLocation) const = 0;

    virtual bool getClosestPointWithinDistance(const double* point, double maxDistance, double* closestPoint) = 0;

    virtual void release(void) = 0;

protected:
    virtual ~RaycastMesh(void){};
};

}

//******************************************************************************************
//  Implementation of the RaycastMesh class
//******************************************************************************************
namespace VHACD
{

class MyRaycastMesh : public VHACD::RaycastMesh
{
public:
    template <class T>
    MyRaycastMesh(uint32_t vcount, const T* vertices, uint32_t tcount, const uint32_t* indices)
    {
        mVcount = vcount;
        mVertices = new double[mVcount * 3];
        for (uint32_t i = 0; i < mVcount; i++)
        {
            mVertices[i * 3 + 0] = vertices[0];
            mVertices[i * 3 + 1] = vertices[1];
            mVertices[i * 3 + 2] = vertices[2];
            vertices += 3;
        }
        mTcount = tcount;
        mIndices = new uint32_t[mTcount * 3];
        for (uint32_t i = 0; i < mTcount; i++)
        {
            mIndices[i * 3 + 0] = indices[0];
            mIndices[i * 3 + 1] = indices[1];
            mIndices[i * 3 + 2] = indices[2];
            indices += 3;
        }
        mAABBTree = VHACD::AABBTree::create(mVertices, mTcount, mIndices, mTcount);
    }


    ~MyRaycastMesh(void)
    {
        delete[] mVertices;
        delete[] mIndices;
        mAABBTree->release();
    }

    virtual void release(void)
    {
        delete this;
    }

    // Uses high speed AABB raycasting
    virtual bool raycast(const double* start,
                         const double* dir,
                         double& outT,
                         double& u,
                         double& v,
                         double& w,
                         double& faceSign,
                         uint32_t& faceIndex) const final
    {
        return mAABBTree->raycast(start, dir, outT, u, v, w, faceSign, faceIndex);
    }

    virtual bool raycast(const double* start, const double* to, double& outT, double& faceSign, double* hitLocation) const final
    {
        double dir[3];
        dir[0] = to[0] - start[0];
        dir[1] = to[1] - start[1];
        dir[2] = to[2] - start[2];
        double distance = VHACD::fm_normalize(dir);
        double u, v, w;
        uint32_t faceIndex;
        bool hit = mAABBTree->raycast(start, dir, outT, u, v, w, faceSign, faceIndex);
        if (hit && hitLocation)
        {
            hitLocation[0] = start[0] + dir[0] * outT;
            hitLocation[1] = start[1] + dir[1] * outT;
            hitLocation[2] = start[2] + dir[2] * outT;
        }
        if (hit && outT > distance)
        {
            hit = false;
        }
        return hit;
    }

    virtual bool getClosestPointWithinDistance(const double* point, double maxDistance, double* closestPoint) final
    {
        return mAABBTree->getClosestPointWithinDistance(point, maxDistance, closestPoint);
    }


    VHACD::AABBTree* mAABBTree{ nullptr };
    uint32_t mVcount;
    double* mVertices;
    uint32_t mTcount;
    uint32_t* mIndices;
};

}; // namespace RAYCAST_MESH


namespace VHACD
{

RaycastMesh* RaycastMesh::createRaycastMesh(uint32_t vcount, // The number of vertices in the source triangle mesh
                                            const double* vertices, // The array of vertex positions in the format
                                                                    // x1,y1,z1..x2,y2,z2.. etc.
                                            uint32_t tcount, // The number of triangles in the source triangle mesh
                                            const uint32_t* indices) // The triangle indices in the format of i1,i2,i3
                                                                     // ... i4,i5,i6, ...
{
    MyRaycastMesh* m = new MyRaycastMesh(vcount, vertices, tcount, indices);
    return static_cast<RaycastMesh*>(m);
}

RaycastMesh* RaycastMesh::createRaycastMesh(uint32_t vcount, // The number of vertices in the source triangle mesh
                                            const float* vertices, // The array of vertex positions in the format
                                                                   // x1,y1,z1..x2,y2,z2.. etc.
                                            uint32_t tcount, // The number of triangles in the source triangle mesh
                                            const uint32_t* indices) // The triangle indices in the format of i1,i2,i3
                                                                     // ... i4,i5,i6, ...
{
    MyRaycastMesh* m = new MyRaycastMesh(vcount, vertices, tcount, indices);
    return static_cast<RaycastMesh*>(m);
}


} // namespace VHACD


//*************************************************************************************************************
// Definition of the Volume class
//*************************************************************************************************************

namespace VHACD
{

class RaycastMesh;

enum VOXEL_VALUE
{
    PRIMITIVE_UNDEFINED = 0,
    PRIMITIVE_OUTSIDE_SURFACE_TOWALK = 1,
    PRIMITIVE_OUTSIDE_SURFACE = 2,
    PRIMITIVE_INSIDE_SURFACE = 3,
    PRIMITIVE_ON_SURFACE = 4
};


struct VoxelEntry
{
public:
    short m_coord[3];
    short m_data;
};

//!
class Volume
{
public:
    Volume(void);
    //! Destructor.
    ~Volume(void);

    //! Voxelize
    void Voxelize(const double* const points,
                  const uint32_t nPoints,
                  const int32_t* const triangles,
                  const uint32_t nTriangles,
                  const size_t dim,
                  VoxelFillMode fillMode,
                  RaycastMesh* raycastMesh);

    void raycastFill(RaycastMesh* raycastMesh);

    void SetVoxel(const size_t i, const size_t j, const size_t k, unsigned char value)
    {
        assert(i < m_dim[0] || i >= 0);
        assert(j < m_dim[1] || j >= 0);
        assert(k < m_dim[2] || k >= 0);

        m_data[k + j * m_dim[2] + i * m_dim[1] * m_dim[2]] = value;
    }

    unsigned char& GetVoxel(const size_t i, const size_t j, const size_t k)
    {
        assert(i < m_dim[0] || i >= 0);
        assert(j < m_dim[1] || j >= 0);
        assert(k < m_dim[2] || k >= 0);
        return m_data[k + j * m_dim[2] + i * m_dim[1] * m_dim[2]];
    }

    const unsigned char& GetVoxel(const size_t i, const size_t j, const size_t k) const
    {
        assert(i < m_dim[0] || i >= 0);
        assert(j < m_dim[0] || j >= 0);
        assert(k < m_dim[0] || k >= 0);
        return m_data[k + j * m_dim[2] + i * m_dim[1] * m_dim[2]];
    }

    const size_t GetNPrimitivesOnSurf() const
    {
        return m_numVoxelsOnSurface;
    }

    const size_t GetNPrimitivesInsideSurf() const
    {
        return m_numVoxelsInsideSurface;
    }

    void AlignToPrincipalAxes(double (&rot)[3][3]) const;

    const VHACD::VoxelVector& getSurfaceVoxels(void) const
    {
        return mSurfaceVoxels;
    }

    const VHACD::VoxelVector& getInteriorVoxels(void) const
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

    Vec3<double> m_minBB;
    Vec3<double> m_maxBB;
    double m_scale;
    size_t m_dim[3]; //>! dim
    size_t m_numVoxelsOnSurface;
    size_t m_numVoxelsInsideSurface;
    size_t m_numVoxelsOutsideSurface;
    unsigned char* m_data;

//private:
    void MarkOutsideSurface(
        const size_t i0, const size_t j0, const size_t k0, const size_t i1, const size_t j1, const size_t k1);
    void FillOutsideSurface();

    void FillInsideSurface();

    void ComputeBB(const double* const points, const uint32_t nPoints);

    void Allocate();
    void Free();

    VHACD::VoxelVector   mSurfaceVoxels;
    VHACD::VoxelVector   mInteriorVoxels;
};

int32_t TriBoxOverlap(const Vec3<double>& boxcenter,
                      const Vec3<double>& boxhalfsize,
                      const Vec3<double>& triver0,
                      const Vec3<double>& triver1,
                      const Vec3<double>& triver2);

inline void ComputeAlignedPoint(const double* const points, const uint32_t idx, Vec3<double>& pt)
{
    pt[0] = points[idx + 0];
    pt[1] = points[idx + 1];
    pt[2] = points[idx + 2];
}

inline void Volume::ComputeBB(const double* const points, const uint32_t nPoints)
{
    Vec3<double> pt;
    ComputeAlignedPoint(points, 0, pt);
    m_maxBB = pt;
    m_minBB = pt;
    for (uint32_t v = 1; v < nPoints; ++v)
    {
        ComputeAlignedPoint(points, v * 3, pt);
        for (int32_t i = 0; i < 3; ++i)
        {
            if (pt[i] < m_minBB[i])
                m_minBB[i] = pt[i];
            else if (pt[i] > m_maxBB[i])
                m_maxBB[i] = pt[i];
        }
    }
}

inline void Volume::Voxelize(const double* const points,
                             const uint32_t nPoints,
                             const int32_t* const triangles,
                             const uint32_t nTriangles,
                             const size_t dim,
                             VoxelFillMode fillMode,
                             RaycastMesh* raycastMesh)
{
    if (nPoints == 0)
    {
        return;
    }

    ComputeBB(points, nPoints);

    double d[3] = { m_maxBB[0] - m_minBB[0], m_maxBB[1] - m_minBB[1], m_maxBB[2] - m_minBB[2] };
    double r;
    // Equal comparison is important here to avoid taking the last branch when d[0] == d[1] with d[2] being the smallest
    // dimension. That would lead to dimensions in i and j to be a lot bigger than expected and make the amount of
    // voxels in the volume totally unmanageable.
    if (d[0] >= d[1] && d[0] >= d[2])
    {
        r = d[0];
        m_dim[0] = dim;
        m_dim[1] = 2 + static_cast<size_t>(dim * d[1] / d[0]);
        m_dim[2] = 2 + static_cast<size_t>(dim * d[2] / d[0]);
    }
    else if (d[1] >= d[0] && d[1] >= d[2])
    {
        r = d[1];
        m_dim[1] = dim;
        m_dim[0] = 2 + static_cast<size_t>(dim * d[0] / d[1]);
        m_dim[2] = 2 + static_cast<size_t>(dim * d[2] / d[1]);
    }
    else
    {
        r = d[2];
        m_dim[2] = dim;
        m_dim[0] = 2 + static_cast<size_t>(dim * d[0] / d[2]);
        m_dim[1] = 2 + static_cast<size_t>(dim * d[1] / d[2]);
    }

    m_scale = r / (dim - 1);
    double invScale = (dim - 1) / r;

    Allocate();
    m_numVoxelsOnSurface = 0;
    m_numVoxelsInsideSurface = 0;
    m_numVoxelsOutsideSurface = 0;

    Vec3<double> p[3];
    size_t i, j, k;
    size_t i0, j0, k0;
    size_t i1, j1, k1;
    Vec3<double> boxcenter;
    Vec3<double> pt;
    const Vec3<double> boxhalfsize(0.5, 0.5, 0.5);
    for (size_t t = 0, ti = 0; t < nTriangles; ++t, ti += 3)
    {
        Vec3<int32_t> tri(triangles[ti + 0], triangles[ti + 1], triangles[ti + 2]);
        for (int32_t c = 0; c < 3; ++c)
        {
            ComputeAlignedPoint(points, tri[c] * 3, pt);

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
                if (i < i0)
                    i0 = i;
                if (j < j0)
                    j0 = j;
                if (k < k0)
                    k0 = k;
                if (i > i1)
                    i1 = i;
                if (j > j1)
                    j1 = j;
                if (k > k1)
                    k1 = k;
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
           boxcenter[0] = (double)i_id;
            for (size_t j_id = j0; j_id < j1; ++j_id)
            {
                boxcenter[1] = (double)j_id;
                for (size_t k_id = k0; k_id < k1; ++k_id)
                {
                    boxcenter[2] = (double)k_id;
                    int32_t res = TriBoxOverlap(boxcenter, boxhalfsize, p[0], p[1], p[2]);
                    unsigned char& value = GetVoxel(i_id, j_id, k_id);
                    if (res == 1 && value == PRIMITIVE_UNDEFINED)
                    {
                        value = PRIMITIVE_ON_SURFACE;
                        ++m_numVoxelsOnSurface;
                        addSurfaceVoxel(int32_t(i_id),int32_t(j_id),int32_t(k_id));
                    }
                }
            }
        }
    }
    if (fillMode == VoxelFillMode::eSurfaceOnly)
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
                    const unsigned char& voxel = GetVoxel(i_id, j_id, k_id);
                    if (voxel != PRIMITIVE_ON_SURFACE)
                    {
                        SetVoxel(i_id, j_id, k_id, PRIMITIVE_OUTSIDE_SURFACE);
                    }
                }
            }
        }
    }
    else if (fillMode == VoxelFillMode::eFloodFill)
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
    else if (fillMode == VoxelFillMode::eRaycastFill)
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



int32_t PlaneBoxOverlap(const Vec3<double>& normal, const Vec3<double>& vert, const Vec3<double>& maxbox)
{
    int32_t q;
    Vec3<double> vmin, vmax;
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
    if (normal * vmin > 0.0)
        return 0;
    if (normal * vmax >= 0.0)
        return 1;
    return 0;
}

int32_t TriBoxOverlap(const Vec3<double>& boxcenter,
                      const Vec3<double>& boxhalfsize,
                      const Vec3<double>& triver0,
                      const Vec3<double>& triver1,
                      const Vec3<double>& triver2)
{
    /*    use separating axis theorem to test overlap between triangle and box */
    /*    need to test for overlap in these directions: */
    /*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
    /*       we do not even need to test these) */
    /*    2) normal of the triangle */
    /*    3) crossproduct(edge from tri, {x,y,z}-directin) */
    /*       this gives 3x3=9 more tests */

    Vec3<double> v0, v1, v2;
    double min, max, p0, p1, p2, rad, fex, fey, fez; // -NJMP- "d" local variable removed
    Vec3<double> normal, e0, e1, e2;

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
    VHACD_FINDMINMAX(v0[0], v1[0], v2[0], min, max);
    if (min > boxhalfsize[0] || max < -boxhalfsize[0])
        return 0;

    /* test in 1-direction */
    VHACD_FINDMINMAX(v0[1], v1[1], v2[1], min, max);
    if (min > boxhalfsize[1] || max < -boxhalfsize[1])
        return 0;

    /* test in getZ-direction */
    VHACD_FINDMINMAX(v0[2], v1[2], v2[2], min, max);
    if (min > boxhalfsize[2] || max < -boxhalfsize[2])
        return 0;

    /* Bullet 2: */
    /*  test if the box intersects the plane of the triangle */
    /*  compute plane equation of triangle: normal*x+d=0 */
    normal = e0 ^ e1;

    if (!PlaneBoxOverlap(normal, v0, boxhalfsize))
        return 0;
    return 1; /* box and triangle overlaps */
}

// Slightly modified version of  Stan Melax's code for 3x3 matrix diagonalization (Thanks Stan!)
// source: http://www.melax.com/diag.html?attredirects=0
void Diagonalize(const double (&A)[3][3], double (&Q)[3][3], double (&D)[3][3])
{
    // A must be a symmetric matrix.
    // returns Q and D such that
    // Diagonal matrix D = QT * A * Q;  and  A = Q*D*QT
    const int32_t maxsteps = 24; // certainly wont need that many.
    int32_t k0, k1, k2;
    double o[3], m[3];
    double q[4] = { 0.0, 0.0, 0.0, 1.0 };
    double jr[4];
    double sqw, sqx, sqy, sqz;
    double tmp1, tmp2, mq;
    double AQ[3][3];
    double thet, sgn, t, c;
    for (int32_t i = 0; i < maxsteps; ++i)
    {
        // quat to matrix
        sqx = q[0] * q[0];
        sqy = q[1] * q[1];
        sqz = q[2] * q[2];
        sqw = q[3] * q[3];
        Q[0][0] = (sqx - sqy - sqz + sqw);
        Q[1][1] = (-sqx + sqy - sqz + sqw);
        Q[2][2] = (-sqx - sqy + sqz + sqw);
        tmp1 = q[0] * q[1];
        tmp2 = q[2] * q[3];
        Q[1][0] = 2.0 * (tmp1 + tmp2);
        Q[0][1] = 2.0 * (tmp1 - tmp2);
        tmp1 = q[0] * q[2];
        tmp2 = q[1] * q[3];
        Q[2][0] = 2.0 * (tmp1 - tmp2);
        Q[0][2] = 2.0 * (tmp1 + tmp2);
        tmp1 = q[1] * q[2];
        tmp2 = q[0] * q[3];
        Q[2][1] = 2.0 * (tmp1 + tmp2);
        Q[1][2] = 2.0 * (tmp1 - tmp2);

        // AQ = A * Q
        AQ[0][0] = Q[0][0] * A[0][0] + Q[1][0] * A[0][1] + Q[2][0] * A[0][2];
        AQ[0][1] = Q[0][1] * A[0][0] + Q[1][1] * A[0][1] + Q[2][1] * A[0][2];
        AQ[0][2] = Q[0][2] * A[0][0] + Q[1][2] * A[0][1] + Q[2][2] * A[0][2];
        AQ[1][0] = Q[0][0] * A[0][1] + Q[1][0] * A[1][1] + Q[2][0] * A[1][2];
        AQ[1][1] = Q[0][1] * A[0][1] + Q[1][1] * A[1][1] + Q[2][1] * A[1][2];
        AQ[1][2] = Q[0][2] * A[0][1] + Q[1][2] * A[1][1] + Q[2][2] * A[1][2];
        AQ[2][0] = Q[0][0] * A[0][2] + Q[1][0] * A[1][2] + Q[2][0] * A[2][2];
        AQ[2][1] = Q[0][1] * A[0][2] + Q[1][1] * A[1][2] + Q[2][1] * A[2][2];
        AQ[2][2] = Q[0][2] * A[0][2] + Q[1][2] * A[1][2] + Q[2][2] * A[2][2];
        // D = Qt * AQ
        D[0][0] = AQ[0][0] * Q[0][0] + AQ[1][0] * Q[1][0] + AQ[2][0] * Q[2][0];
        D[0][1] = AQ[0][0] * Q[0][1] + AQ[1][0] * Q[1][1] + AQ[2][0] * Q[2][1];
        D[0][2] = AQ[0][0] * Q[0][2] + AQ[1][0] * Q[1][2] + AQ[2][0] * Q[2][2];
        D[1][0] = AQ[0][1] * Q[0][0] + AQ[1][1] * Q[1][0] + AQ[2][1] * Q[2][0];
        D[1][1] = AQ[0][1] * Q[0][1] + AQ[1][1] * Q[1][1] + AQ[2][1] * Q[2][1];
        D[1][2] = AQ[0][1] * Q[0][2] + AQ[1][1] * Q[1][2] + AQ[2][1] * Q[2][2];
        D[2][0] = AQ[0][2] * Q[0][0] + AQ[1][2] * Q[1][0] + AQ[2][2] * Q[2][0];
        D[2][1] = AQ[0][2] * Q[0][1] + AQ[1][2] * Q[1][1] + AQ[2][2] * Q[2][1];
        D[2][2] = AQ[0][2] * Q[0][2] + AQ[1][2] * Q[1][2] + AQ[2][2] * Q[2][2];
        o[0] = D[1][2];
        o[1] = D[0][2];
        o[2] = D[0][1];
        m[0] = fabs(o[0]);
        m[1] = fabs(o[1]);
        m[2] = fabs(o[2]);

        k0 = (m[0] > m[1] && m[0] > m[2]) ? 0 : (m[1] > m[2]) ? 1 : 2; // index of largest element of offdiag
        k1 = (k0 + 1) % 3;
        k2 = (k0 + 2) % 3;
        if (o[k0] == 0.0)
        {
            break; // diagonal already
        }
        thet = (D[k2][k2] - D[k1][k1]) / (2.0 * o[k0]);
        sgn = (thet > 0.0) ? 1.0 : -1.0;
        thet *= sgn; // make it positive
        t = sgn / (thet + ((thet < 1.E6) ? sqrt(thet * thet + 1.0) : thet)); // sign(T)/(|T|+sqrt(T^2+1))
        c = 1.0 / sqrt(t * t + 1.0); //  c= 1/(t^2+1) , t=s/c
        if (c == 1.0)
        {
            break; // no room for improvement - reached machine precision.
        }
        jr[0] = jr[1] = jr[2] = jr[3] = 0.0;
        jr[k0] = sgn * sqrt((1.0 - c) / 2.0); // using 1/2 angle identity sin(a/2) = sqrt((1-cos(a))/2)
        jr[k0] *= -1.0; // since our quat-to-matrix convention was for v*M instead of M*v
        jr[3] = sqrt(1.0 - jr[k0] * jr[k0]);
        if (jr[3] == 1.0)
        {
            break; // reached limits of floating point precision
        }
        q[0] = (q[3] * jr[0] + q[0] * jr[3] + q[1] * jr[2] - q[2] * jr[1]);
        q[1] = (q[3] * jr[1] - q[0] * jr[2] + q[1] * jr[3] + q[2] * jr[0]);
        q[2] = (q[3] * jr[2] + q[0] * jr[1] - q[1] * jr[0] + q[2] * jr[3]);
        q[3] = (q[3] * jr[3] - q[0] * jr[0] - q[1] * jr[1] - q[2] * jr[2]);
        mq = sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
        q[0] /= mq;
        q[1] /= mq;
        q[2] /= mq;
        q[3] /= mq;
    }
}
Volume::Volume(void)
{
    m_dim[0] = m_dim[1] = m_dim[2] = 0;
    m_minBB[0] = m_minBB[1] = m_minBB[2] = 0.0;
    m_maxBB[0] = m_maxBB[1] = m_maxBB[2] = 1.0;
    m_numVoxelsOnSurface = 0;
    m_numVoxelsInsideSurface = 0;
    m_numVoxelsOutsideSurface = 0;
    m_scale = 1.0;
    m_data = 0;
}
Volume::~Volume(void)
{
    delete[] m_data;
}
void Volume::Allocate()
{
    delete[] m_data;
    size_t size = m_dim[0] * m_dim[1] * m_dim[2];
    m_data = new unsigned char[size];
    memset(m_data, PRIMITIVE_UNDEFINED, sizeof(unsigned char) * size);
}
void Volume::Free()
{
    delete[] m_data;
    m_data = 0;
}

void Volume::MarkOutsideSurface(
    const size_t i0, const size_t j0, const size_t k0, const size_t i1, const size_t j1, const size_t k1)
{
    for (size_t i = i0; i < i1; ++i)
    {
        for (size_t j = j0; j < j1; ++j)
        {
            for (size_t k = k0; k < k1; ++k)
            {
                unsigned char& v = GetVoxel(i, j, k);
                if (v == PRIMITIVE_UNDEFINED)
                {
                    v = PRIMITIVE_OUTSIDE_SURFACE_TOWALK;
                }
            }
        }
    }
}

inline void WalkForward(int64_t start, int64_t end, unsigned char* ptr, int64_t stride, int64_t maxDistance)
{
    for (int64_t i = start, count = 0; count < maxDistance && i < end && *ptr == PRIMITIVE_UNDEFINED;
         ++i, ptr += stride, ++count)
    {
        *ptr = PRIMITIVE_OUTSIDE_SURFACE_TOWALK;
    }
}

inline void WalkBackward(int64_t start, int64_t end, unsigned char* ptr, int64_t stride, int64_t maxDistance)
{
    for (int64_t i = start, count = 0; count < maxDistance && i >= end && *ptr == PRIMITIVE_UNDEFINED;
         --i, ptr -= stride, ++count)
    {
        *ptr = PRIMITIVE_OUTSIDE_SURFACE_TOWALK;
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
                    unsigned char& voxel = GetVoxel(i, j, k);
                    if (voxel == PRIMITIVE_OUTSIDE_SURFACE_TOWALK)
                    {
                        voxelsWalked++;
                        voxel = PRIMITIVE_OUTSIDE_SURFACE;

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
#if 1

    size_t maxSize = i0*j0*k0;

    uint32_t *temp = new uint32_t[maxSize];
    uint32_t *dest = temp;

    for (uint32_t i = 0; i < i0; ++i)
    {
        for (uint32_t j = 0; j < j0; ++j)
        {
            for (uint32_t k = 0; k < k0; ++k)
            {
                unsigned char& v = GetVoxel((size_t)i, (size_t)j, (size_t)k);
                if (v == PRIMITIVE_UNDEFINED)
                {
                    v = PRIMITIVE_INSIDE_SURFACE;
                    uint32_t index = (i<<VHACD_VOXEL_BITS2) | (j<<VHACD_VOXEL_BITS) | k;
                    *dest++ = index;
                    ++m_numVoxelsInsideSurface;
                }
            }
        }
    }

    uint32_t count = dest - temp;
    if ( count )
    {
        mInteriorVoxels.resize(count);
        memcpy(&mInteriorVoxels[0],temp,sizeof(uint32_t)*count);
    }
    delete []temp;
#endif
}

void traceRay(RaycastMesh* raycastMesh, const double* start, const double* dir, uint32_t& insideCount, uint32_t& outsideCount)
{
    double outT, u, v, w, faceSign;
    uint32_t faceIndex;
    bool hit = raycastMesh->raycast(start, dir, outT, u, v, w, faceSign, faceIndex);
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

inline void initVec3(double* dest, uint32_t vindex, double x, double y, double z)
{
    dest[vindex * 3 + 0] = x;
    dest[vindex * 3 + 1] = y;
    dest[vindex * 3 + 2] = z;
}

void Volume::raycastFill(RaycastMesh* raycastMesh)
{
    if (!raycastMesh)
    {
        return;
    }

    const uint32_t i0 = (uint32_t)m_dim[0];
    const uint32_t j0 = (uint32_t)m_dim[1];
    const uint32_t k0 = (uint32_t)m_dim[2];

    size_t maxSize = i0 * j0*k0;

    uint32_t *temp = new uint32_t[maxSize];
    uint32_t *dest = temp;
    m_numVoxelsInsideSurface = 0;
    for (uint32_t i = 0; i < i0; ++i)
    {
        for (uint32_t j = 0; j < j0; ++j)
        {
            for (uint32_t k = 0; k < k0; ++k)
            {
                unsigned char& voxel = GetVoxel(i, j, k);
                if (voxel != VHACD::PRIMITIVE_ON_SURFACE)
                {
                    double start[3];

                    start[0] = float(i) * m_scale + m_minBB[0];
                    start[1] = float(j) * m_scale + m_minBB[1];
                    start[2] = float(k) * m_scale + m_minBB[2];

                    uint32_t insideCount = 0;
                    uint32_t outsideCount = 0;

                    double directions[6 * 3];

                    initVec3(directions, 0, 1, 0, 0);
                    initVec3(directions, 1, 1, 0, 0);
                    initVec3(directions, 2, 0, 1, 0);
                    initVec3(directions, 3, 0, -1, 0);
                    initVec3(directions, 4, 0, 0, 1);
                    initVec3(directions, 5, 0, 0, -1);

                    for (uint32_t r = 0; r < 6; r++)
                    {
                        traceRay(raycastMesh, start, &directions[r * 3], insideCount, outsideCount);
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
                        voxel = VHACD::PRIMITIVE_INSIDE_SURFACE;
                        uint32_t index = (i << VHACD_VOXEL_BITS2) | (j << VHACD_VOXEL_BITS) | k;
                        *dest++ = index;
                        m_numVoxelsInsideSurface++;
                    }
                    else
                    {
                        voxel = VHACD::PRIMITIVE_OUTSIDE_SURFACE;
                    }
                }
            }
        }
    }
    uint32_t count = dest - temp;
    if (count)
    {
        mInteriorVoxels.resize(count);
        memcpy(&mInteriorVoxels[0], temp, sizeof(uint32_t)*count);
    }
    delete[]temp;
}

}

//*************************************************************************************************************
// Definition of the Voxelize class
//*************************************************************************************************************
namespace VHACD
{

class RaycastMesh;

enum class VoxelValue : uint8_t
{
    PRIMITIVE_UNDEFINED = 0,
    PRIMITIVE_OUTSIDE_SURFACE_TOWALK = 1,
    PRIMITIVE_OUTSIDE_SURFACE = 2,
    PRIMITIVE_INSIDE_SURFACE = 3,
    PRIMITIVE_ON_SURFACE = 4
};


class Voxelize
{
public:
    static Voxelize *create(void);

    virtual uint32_t voxelize(VHACD::RaycastMesh *raycastMesh,
        const double* const vertices,
        const uint32_t vertexCount,
        const uint32_t* indices,
        const uint32_t triangleCount,
        const uint32_t resolution,
        VoxelFillMode fillMode) = 0;

    virtual double getScale(void) const = 0;

    virtual bool getBoundsMin(double bmin[3]) const = 0;
    virtual bool getBoundsMax(double bmax[3]) const = 0;
    virtual bool getDimensions(uint32_t dim[3]) const = 0;
    virtual uint8_t getVoxel(uint32_t x,uint32_t y,uint32_t z) const = 0;
    virtual void setVoxel(uint32_t x,uint32_t y,uint32_t z,uint8_t value) = 0;

    virtual bool getVoxel(const double *pos,uint32_t &x,uint32_t &y, uint32_t &z) const = 0;

    virtual bool getSurfaceVoxels(VoxelVector &surfaceVoxels) = 0;
    virtual bool getInteriorVoxels(VoxelVector &interiorVoxels) = 0;

    virtual void release(void) = 0;
protected:
    virtual ~Voxelize(void)
    {
    }
};

}

//*************************************************************************************************************
// Implementation of the Voxelize class
//*************************************************************************************************************
namespace VHACD
{

class VoxelizeImpl : public Voxelize
{
public:
    VoxelizeImpl(void)
    {
    }

    virtual ~VoxelizeImpl(void)
    {
        delete mVolume;
    }

    virtual uint32_t voxelize(VHACD::RaycastMesh *raycastMesh,
        const double* const vertices,
        const uint32_t vertexCount,
        const uint32_t* indices,
        const uint32_t triangleCount,
        const uint32_t resolution,VoxelFillMode fillMode) final
    {
        double a = pow((double)(resolution), 0.33);
        mDimensions = (uint32_t)(a*1.5);
        // Minimum voxel resolution is 32x32x32
        if (mDimensions < 32)
        {
            mDimensions = 32;
        }

        delete mVolume;
        mVolume = new VHACD::Volume;

        mVolume->Voxelize(vertices, vertexCount, (const int32_t *)indices, triangleCount, mDimensions, fillMode, raycastMesh);

        return mDimensions;
    }

    virtual void release(void) final
    {
        delete this;
    }

    virtual double getScale(void) const final
    {
        double ret = 1;

        if ( mVolume )
        {
            ret = mVolume->m_scale;
        }

        return ret;
    }

    virtual bool getBoundsMin(double bmin[3]) const final
    {
        bool ret = false;

        if ( mVolume )
        {
            bmin[0] = mVolume->m_minBB[0];
            bmin[1] = mVolume->m_minBB[1];
            bmin[2] = mVolume->m_minBB[2];
            ret = true;
        }

        return ret;
    }

    virtual bool getBoundsMax(double bmax[3]) const final
    {
        bool ret = false;

        if (mVolume)
        {
            bmax[0] = mVolume->m_maxBB[0];
            bmax[1] = mVolume->m_maxBB[1];
            bmax[2] = mVolume->m_maxBB[2];
            ret = true;
        }

        return ret;
    }

    virtual bool getDimensions(uint32_t dim[3]) const final
    {
        bool ret = false;

        if ( mVolume )
        {
            dim[0] = uint32_t(mVolume->m_dim[0]);
            dim[1] = uint32_t(mVolume->m_dim[1]);
            dim[2] = uint32_t(mVolume->m_dim[2]);
            ret = true;
        }

        return ret;
    }

    virtual uint8_t getVoxel(uint32_t x, uint32_t y, uint32_t z) const final
    {
        uint8_t ret = 0;

        if ( mVolume )
        {
            ret = mVolume->GetVoxel(x,y,z);
        }

        return ret;
    }

    virtual void setVoxel(uint32_t x, uint32_t y, uint32_t z, uint8_t value) final
    {
        if ( mVolume )
        {
            mVolume->SetVoxel(x,y,z,value);
        }
    }

    virtual bool getVoxel(const double *pos,uint32_t &x,uint32_t &y, uint32_t &z) const  final
    {
        bool ret = false;

        double bmin[3];
        double bmax[3];

        getBoundsMin(bmin);
        getBoundsMax(bmax);

        if ( pos[0] >= bmin[0] && pos[0] < bmax[0] &&
             pos[1] >= bmin[1] && pos[1] < bmax[1] &&
             pos[2] >= bmin[2] && pos[2] < bmax[2] )
        {
            double recipScale = 1.0 / getScale();
            x = uint32_t( (pos[0] - bmin[0])*recipScale);
            y = uint32_t( (pos[1] - bmin[1])*recipScale);
            z = uint32_t( (pos[2] - bmin[2])*recipScale);
            ret = true;
        }
        return ret;
    }

    virtual bool getSurfaceVoxels(VoxelVector &surfaceVoxels) final
    {
        bool ret = false;

        if ( mVolume )
        {
            surfaceVoxels.clear();
            surfaceVoxels = mVolume->getSurfaceVoxels();
            ret = !surfaceVoxels.empty();
        }

        return ret;
    }

    virtual bool getInteriorVoxels(VoxelVector &interiorVoxels) final
    {
        bool ret = false;

        if (mVolume)
        {
            interiorVoxels.clear();
            interiorVoxels = mVolume->getInteriorVoxels();
            ret = !interiorVoxels.empty();
        }

        return ret;
    }

    uint32_t        mDimensions{32};
    VHACD::Volume	*mVolume{nullptr};
};

Voxelize *Voxelize::create(void)
{
    auto ret = new VoxelizeImpl;
    return static_cast< Voxelize *>(ret);
}


}











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
#include <stdint.h>

namespace VHACD
{

class SimpleMesh;
class RaycastMesh;


class ShrinkWrap
{
public:
    static ShrinkWrap *create(void);

    virtual void shrinkWrap(SimpleMesh &sourceConvexHull,
        RaycastMesh &raycastMesh,
        uint32_t maxHullVertexCount,
        double distanceThreshold,
        bool doShrinkWrap) = 0;

    virtual void release(void) = 0;
};

}

//***********************************************************************************************
// QuickHull definition
//***********************************************************************************************
namespace VHACD
{

class HullPoints
{
public:
    uint32_t    mVertexCount{0};                        // Number of input vertices
    const double *mVertices{nullptr};                    // array of input vertices
    uint32_t    mMaxHullVertices{60};                   // Maximum number of vertices in the output convex hull
};

class QuickHull
{
public:
    static QuickHull *create(void);

    virtual uint32_t computeConvexHull(const HullPoints &hp) = 0;
    virtual const double *getVertices(uint32_t &vcount) const = 0;
    virtual const uint32_t *getIndices(uint32_t &tcount) const = 0;

    virtual void release(void) = 0;
protected:
    virtual ~QuickHull(void) { };
};

}

//***********************************************************************************************
// QuickHull implementation
//***********************************************************************************************
namespace VHACD
{

//////////////////////////////////////////////////////////////////////////
// Quickhull base class holding the hull during construction
class QuickHullImpl : public QuickHull
{
public:

    QuickHullImpl(void)
    {
    }

    ~QuickHullImpl()
    {
    }

    virtual void release(void) final
    {
        delete this;
    }

    virtual uint32_t computeConvexHull(const HullPoints &hp) final
    {
        mTriangleCount = 0;
        mIndices.clear();

        nd::VHACD::ConvexHull ch(hp.mVertices,sizeof(double)*3,hp.mVertexCount,0.0001,hp.mMaxHullVertices);

        auto &vlist = ch.GetVertexPool();
        if ( !vlist.empty() )
        {
            size_t vcount = vlist.size();
            mVertices.resize(vcount*3);
            memcpy(&mVertices[0],&vlist[0],sizeof(double)*3*vcount);
        }
        
		for (nd::VHACD::ConvexHull::ndNode* node = ch.GetFirst(); node; node = node->GetNext())
		{
			nd::VHACD::ConvexHullFace* const face = &node->GetInfo();
            mIndices.push_back(face->m_index[0]);
            mIndices.push_back(face->m_index[1]);
            mIndices.push_back(face->m_index[2]);
            mTriangleCount++;
		}

        return mTriangleCount;
    }

    virtual const double *getVertices(uint32_t &vcount) const final
    {
        const double *ret = nullptr;

        vcount = (uint32_t)mVertices.size()/3;

        if ( vcount )
        {
            ret = &mVertices[0];
        }

        return ret;
    }

    virtual const uint32_t *getIndices(uint32_t &tcount) const final
    {
        const uint32_t *ret = nullptr;
        tcount = mTriangleCount;
        if ( mTriangleCount )
        {
            ret = &mIndices[0];
        }

        return ret;
    }

private:
    std::vector< double >   mVertices;
    uint32_t                mTriangleCount{0};
    std::vector<uint32_t>   mIndices;
};

QuickHull *QuickHull::create(void)
{
    auto ret = new QuickHullImpl;
    return static_cast< QuickHull *>(ret);
}



} // end of VHACD namespace




//******************************************************************************************
// Implementation of the ShrinkWrap class
//******************************************************************************************

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

namespace VHACD
{

class SVec3
{
public:
    SVec3(void) { };

    SVec3(double _x,double _y,double _z) : x(_x), y(_y), z(_z)
    {
    }

    double x;
    double y;
    double z;
};



class ShrinkWrapImpl : public ShrinkWrap
{
public:
    ShrinkWrapImpl(void)
    {
    }

    virtual ~ShrinkWrapImpl(void)
    {
    }

    virtual void shrinkWrap(SimpleMesh &sourceConvexHull, 
                            RaycastMesh &raycastMesh, 
                            uint32_t maxHullVertexCount,
                            double distanceThreshold,
                            bool doShrinkWrap) final
    {
        std::vector< SVec3 > verts; // New verts for the new convex hull
        // Examine each vertex and see if it is within the voxel distance.
        // If it is, then replace the point with the shrinkwrapped / projected point
        for (uint32_t j = 0; j < sourceConvexHull.mVertexCount; j++)
        {
            double *p = &sourceConvexHull.mVertices[j * 3];
            if (doShrinkWrap)
            {
                double closest[3];
                if (raycastMesh.getClosestPointWithinDistance(p, distanceThreshold, closest))
                {
                    p[0] = closest[0];
                    p[1] = closest[1];
                    p[2] = closest[2];
                }
            }
            SVec3 point(p[0], p[1], p[2]);
            verts.push_back(point);
        }
        // Final step is to recompute the convex hull
        VHACD::QuickHull *qh = VHACD::QuickHull::create();
        VHACD::HullPoints hp;
        hp.mVertexCount = (uint32_t)verts.size();
        hp.mVertices = &verts[0].x;
        hp.mMaxHullVertices = maxHullVertexCount;
        uint32_t tcount = qh->computeConvexHull(hp);
        if (tcount)
        {
            delete[]sourceConvexHull.mVertices;
            delete[]sourceConvexHull.mIndices;

            sourceConvexHull.mVertices = nullptr;
            sourceConvexHull.mIndices = nullptr;

            const double *vtx = qh->getVertices(sourceConvexHull.mVertexCount);
            const uint32_t *idx = qh->getIndices(sourceConvexHull.mTriangleCount);

            sourceConvexHull.mVertices = new double[sourceConvexHull.mVertexCount * 3];
            sourceConvexHull.mIndices = new uint32_t[sourceConvexHull.mTriangleCount * 3];
            memcpy(sourceConvexHull.mVertices, vtx, sizeof(double)*sourceConvexHull.mVertexCount * 3);
            memcpy(sourceConvexHull.mIndices, idx, sizeof(uint32_t)*sourceConvexHull.mTriangleCount * 3);
        }
        qh->release();
    }

    virtual void release(void) final
    {
        delete this;
    }
};

ShrinkWrap *ShrinkWrap::create(void)
{
    auto ret = new ShrinkWrapImpl;
    return static_cast< ShrinkWrap *>(ret);
}


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


using AABBTreeVector = std::vector< VHACD::AABBTree *>;
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

class IVec3
{
public:
    IVec3(void) { };
    IVec3(int32_t _x,int32_t _y,int32_t _z) : x(_x), y(_y), z(_z)
    {
    }
    int32_t x{0};
    int32_t y{0};
    int32_t z{0};
};


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
        mVoxels->getBoundsMin(mVoxelBmin);  // Get the 3d bounding volume minimum for this voxel space
        mVoxels->getBoundsMax(mVoxelBmax);  // Get the 3d bounding volume maximum for this voxel space
        mVoxelScaleHalf = mVoxelScale*0.5;  // Compute half of the voxel size

        mVoxelAdjust[0] = mVoxelBmin[0]-mVoxelScaleHalf;    // The adjustment to move from voxel coordinate to a 3d position
        mVoxelAdjust[1] = mVoxelBmin[1]-mVoxelScaleHalf;
        mVoxelAdjust[2] = mVoxelBmin[2]-mVoxelScaleHalf;

        // Default copy the voxel region from the parent, but values will
        // be adjusted next based on the split axis and location
        mX1 = parent.mX1;
        mX2 = parent.mX2;
        mY1 = parent.mY1;
        mY2 = parent.mY2;
        mZ1 = parent.mZ1;
        mZ2 = parent.mZ2;

        switch ( mAxis )
        {
            case SplitAxis::X_AXIS_NEGATIVE:
                mX2 = splitLoc;
                break;
            case SplitAxis::X_AXIS_POSITIVE:
                mX1 = splitLoc+1;
                break;
            case SplitAxis::Y_AXIS_NEGATIVE:
                mY2 = splitLoc;
                break;
            case SplitAxis::Y_AXIS_POSITIVE:
                mY1 = splitLoc+1;
                break;
            case SplitAxis::Z_AXIS_NEGATIVE:
                mZ2 = splitLoc;
                break;
            case SplitAxis::Z_AXIS_POSITIVE:
                mZ1 = splitLoc+1;
                break;
        }
        // First, we copy all of the interior voxels from our parent
        // which intersect our region
        for (auto &i:parent.mInteriorVoxels)
        {
            uint32_t x,y,z;
            i.getVoxel(x,y,z);
            if ( x >= mX1 && x <= mX2 &&
                 y >= mY1 && y <= mY2 &&
                 z >= mZ1 && z <= mZ2 )
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
                        if ( x == mX1 )
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
                        if ( y == mY1 )
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
                        if ( z == mZ1 )
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
        for (auto &i:parent.mSurfaceVoxels)
        {
            uint32_t x,y,z;
            i.getVoxel(x,y,z);
            if ( x >= mX1 && x <= mX2 &&
                 y >= mY1 && y <= mY2 &&
                 z >= mZ1 && z <= mZ2 )
            {
                mSurfaceVoxels.push_back(i);
            }
        }
        // Our parent's new surface voxels become our new surface voxels so long as they intersect our region
        for (auto &i:parent.mNewSurfaceVoxels)
        {
            uint32_t x,y,z;
            i.getVoxel(x,y,z);
            if ( x >= mX1 && x <= mX2 &&
                 y >= mY1 && y <= mY2 &&
                 z >= mZ1 && z <= mZ2 )
            {
                mNewSurfaceVoxels.push_back(i);
            }
        }

        // Recompute the min-max bounding box which would be different after the split occurs
        mX1 = 0x7FFFFFFF;
        mY1 = 0x7FFFFFFF;
        mZ1 = 0x7FFFFFFF;
        mX2 = 0;
        mY2 = 0;
        mZ2 = 0;
        for (auto &i:mSurfaceVoxels)
        {
            minMaxVoxelRegion(i);
        }
        for (auto &i:mNewSurfaceVoxels)
        {
            minMaxVoxelRegion(i);
        }
        for (auto &i:mInteriorVoxels)
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
        uint32_t x,y,z;
        v.getVoxel(x,y,z);
        if ( x < mX1 ) mX1 = x;
        if ( x > mX2 ) mX2 = x;
        if ( y < mY1 ) mY1 = y;
        if ( y > mY2 ) mY2 = y;
        if ( z < mZ1 ) mZ1 = z;
        if ( z > mZ2 ) mZ2 = z;
    }


    // Here we construct the intitial convex hull around the
    // entire voxel set
    VoxelHull(Voxelize *voxels,
              const SimpleMesh &inputMesh,
              const IVHACD::Parameters &params,
              VHACDCallbacks *callbacks) : mVoxels(voxels), mParams(params), mCallbacks(callbacks)
    {
        mVoxelHullCount++;
        mIndex = mVoxelHullCount;
        uint32_t dimensions[3];
        mVoxels->getDimensions(dimensions);

        mX2 = dimensions[0]-1;
        mY2 = dimensions[1]-1;
        mZ2 = dimensions[2]-1;

        mVoxelScale = mVoxels->getScale();
        mVoxels->getBoundsMin(mVoxelBmin);
        mVoxels->getBoundsMax(mVoxelBmax);
        mVoxelScaleHalf = mVoxelScale*0.5;

        mVoxelAdjust[0] = mVoxelBmin[0]-mVoxelScaleHalf;
        mVoxelAdjust[1] = mVoxelBmin[1]-mVoxelScaleHalf;
        mVoxelAdjust[2] = mVoxelBmin[2]-mVoxelScaleHalf;

        // Here we get a copy of all voxels which lie on the surface mesh
        mVoxels->getSurfaceVoxels(mSurfaceVoxels);
        // Now we get a copy of all voxels which are considered part of the 'interior' of the source mesh
        mVoxels->getInteriorVoxels(mInteriorVoxels);
        buildVoxelMesh();
        buildRaycastMesh(); // build a raycast mesh of the voxel mesh
//        saveVoxelMesh(inputMesh,true,false);
        computeConvexHull();
    }

    virtual ~VoxelHull(void)
    {
        if ( mConvexHull )
        {
            delete []mConvexHull->m_points;
            delete []mConvexHull->m_triangles;
            delete mConvexHull;
            mConvexHull = nullptr;
        }
        delete mHullA;
        delete mHullB;
        VHACD_SAFE_RELEASE(mRaycastMesh);
    }

    void buildRaycastMesh(void)
    {
        // Create a raycast mesh representation of the voxelized surface mesh
        if ( !mIndices.empty() )
        {
            mRaycastMesh = RaycastMesh::createRaycastMesh(uint32_t(mVertices.size())/3,&mVertices[0],uint32_t(mIndices.size())/3,&mIndices[0]);
        }
    }

    // We now compute the convex hull relative to a triangle mesh generated 
    // from the voxels
    void computeConvexHull(void)
    {
        if ( !mVertices.empty() )
        {
            // we compute the convex hull as follows...
            VHACD::QuickHull *qh = VHACD::QuickHull::create();

            VHACD::HullPoints hp;
            hp.mVertexCount = (uint32_t)mVertices.size()/3;
            hp.mVertices = &mVertices[0];
            hp.mMaxHullVertices = hp.mVertexCount;
            uint32_t tcount = qh->computeConvexHull(hp);
            if ( tcount )
            {
                mConvexHull = new IVHACD::ConvexHull;

                const double *vertices = qh->getVertices(mConvexHull->m_nPoints);
                mConvexHull->m_points = new double[mConvexHull->m_nPoints*3];
                memcpy(mConvexHull->m_points,vertices,sizeof(double)*mConvexHull->m_nPoints*3);

                const uint32_t *indices = qh->getIndices(mConvexHull->m_nTriangles);
                mConvexHull->m_triangles = new uint32_t[mConvexHull->m_nTriangles*3];
                memcpy(mConvexHull->m_triangles,indices,mConvexHull->m_nTriangles*sizeof(uint32_t)*3);

                VHACD::fm_computeCentroid(mConvexHull->m_nPoints,mConvexHull->m_points,mConvexHull->m_nTriangles,mConvexHull->m_triangles, mConvexHull->m_center);
                mConvexHull->m_volume = VHACD::fm_computeMeshVolume(mConvexHull->m_points,mConvexHull->m_nTriangles,mConvexHull->m_triangles);
            }
            VHACD_SAFE_RELEASE(qh);
        }
        if ( mConvexHull )
        {
            mHullVolume = mConvexHull->m_volume;
        }
        // This is the volume of a single voxel
        double singleVoxelVolume = mVoxelScale*mVoxelScale*mVoxelScale;
        size_t voxelCount = mInteriorVoxels.size() + mNewSurfaceVoxels.size() + mSurfaceVoxels.size();
        mVoxelVolume = singleVoxelVolume * double(voxelCount);
        double diff = fabs(mHullVolume-mVoxelVolume);
        mVolumeError = (diff*100)/mVoxelVolume;
    }

    // Returns true if this convex hull should be considered done
    bool isComplete(void)
    {
        bool ret = false;
        if ( mRaycastMesh == nullptr )
        {
            ret = true;
        }
        else if ( mConvexHull == nullptr )
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
            uint32_t dx = mX2-mX1;
            uint32_t dy = mY2-mY1;
            uint32_t dz = mZ2-mZ1;
            if ( dx <= mParams.m_minEdgeLength &&
                 dy <= mParams.m_minEdgeLength &&
                 dz <= mParams.m_minEdgeLength )
            {
                ret = true;
            }
        }
        return ret;
    }

    
    // Convert a voxel position into it's correct double precision location
    inline void getPoint(const int32_t x,
                  const int32_t y,
                  const int32_t z,
                  const double scale,
                  const double *bmin,
                  double *dest) const
    {
        dest[0] = (double(x)*scale + bmin[0]);
        dest[1] = (double(y)*scale + bmin[1]);
        dest[2] = (double(z)*scale + bmin[2]);
    }

    // Sees if we have already got an index for this voxel position.
    // If the voxel position has already been indexed, we just return
    // that index value.
    // If not, then we convert it into the floating point position and
    // add it to the index map
    inline uint32_t getVertexIndex(const IVec3 &p)
    {
        uint32_t ret = 0;
        uint32_t address = (p.x<<20)|(p.y<<10)|p.z;
        VoxelIndexMap::iterator found = mVoxelIndexMap.find(address);
        if ( found != mVoxelIndexMap.end() )
        {
            ret = (*found).second;
        }
        else
        {
            double vertex[3];
            getPoint(p.x,p.y,p.z,mVoxelScale,mVoxelAdjust,vertex);
            ret = (uint32_t)mVoxelIndexMap.size();
            mVoxelIndexMap[address] = ret;
            mVertices.push_back(vertex[0]);
            mVertices.push_back(vertex[1]);
            mVertices.push_back(vertex[2]);
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
        IVec3 bmin(v.getX(),v.getY(),v.getZ());
        // The voxel position of the lower right corner of the box
        IVec3 bmax(bmin.x+1,bmin.y+1,bmin.z+1);

        // Build the set of 8 voxel positions representing
        // the coordinates of the box
        IVec3 box[8];

        box[0] = IVec3(bmin.x, bmin.y, bmin.z);
        box[1] = IVec3(bmax.x, bmin.y, bmin.z);
        box[2] = IVec3(bmax.x, bmax.y, bmin.z);
        box[3] = IVec3(bmin.x, bmax.y, bmin.z);
        box[4] = IVec3(bmin.x, bmin.y, bmax.z);
        box[5] = IVec3(bmax.x, bmin.y, bmax.z);
        box[6] = IVec3(bmax.x, bmax.y, bmax.z);
        box[7] = IVec3(bmin.x, bmax.y, bmax.z);

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
    inline void addTri(const IVec3 *box, 
                       uint32_t i1, 
                       uint32_t i2, 
                       uint32_t i3)
    {
        const IVec3 &p1 = box[i1];
        const IVec3 &p2 = box[i2];
        const IVec3 &p3 = box[i3];
        addTriangle(box[i1],box[i2],box[i3]);
    }

    // Here we convert from voxel space to a 3d position, index it, and add
    // the triangle positions and indices for the output mesh
    inline void addTriangle(const IVec3 &p1,const IVec3 &p2,const IVec3 &p3)
    {
        uint32_t i1 = getVertexIndex(p1);
        uint32_t i2 = getVertexIndex(p2);
        uint32_t i3 = getVertexIndex(p3);

        mIndices.push_back(i1);
        mIndices.push_back(i2);
        mIndices.push_back(i3);
    }

    // Used only for debugging. Saves the voxelized mesh to disk
    // Optionally saves the original source mesh as well for comparison
    void saveVoxelMesh(const SimpleMesh &inputMesh,bool saveVoxelMesh,bool saveSourceMesh)
    {
        char scratch[512];
        snprintf(scratch,sizeof(scratch),"voxel-mesh-%03d.obj", mIndex);
        FILE *fph = fopen(scratch,"wb");
        if ( fph )
        {
            uint32_t baseIndex = 1;
            if ( saveVoxelMesh )
            {
                for (size_t i=0; i<mVertices.size()/3; i++)
                {
                    const double *p = &mVertices[i*3];
                    fprintf(fph,"v %0.9f %0.9f %0.9f\n", p[0], p[1], p[2]);
                    baseIndex++;
                }
                for (size_t i=0; i<mIndices.size()/3; i++)
                {
                    const uint32_t *idx = &mIndices[i*3];
                    fprintf(fph,"f %d %d %d\n", idx[0]+1, idx[1]+1, idx[2]+1);
                }
            }
            if ( saveSourceMesh )
            {
                for (uint32_t i=0; i<inputMesh.mVertexCount; i++)
                {
                    const double *p = &inputMesh.mVertices[i*3];
                    fprintf(fph,"v %0.9f %0.9f %0.9f\n", p[0], p[1], p[2]);
                }
                for (uint32_t i=0; i<inputMesh.mTriangleCount; i++)
                {
                    const uint32_t *idx = &inputMesh.mIndices[i*3];
                    fprintf(fph,"f %d %d %d\n", idx[0]+baseIndex, idx[1]+baseIndex, idx[2]+baseIndex);
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

        uint32_t dx = mX2-mX1;
        uint32_t dy = mY2-mY1;
        uint32_t dz = mZ2-mZ1;

        if ( dx >= dy && dx >= dz )
        {
            ret = SplitAxis::X_AXIS_NEGATIVE;
            location = (mX2+1+mX1)/2;
            uint32_t edgeLoc;
            if ( mParams.m_findBestPlane && findConcavityX(edgeLoc) )
            {
                location = edgeLoc;
            }
        }
        else if ( dy >= dx && dy >= dz )
        {
            ret = SplitAxis::Y_AXIS_NEGATIVE;
            location = (mY2 + 1 + mY1) / 2;
            uint32_t edgeLoc;
            if ( mParams.m_findBestPlane &&  findConcavityY(edgeLoc) )
            {
                location = edgeLoc;
            }
        }
        else
        {
            ret = SplitAxis::Z_AXIS_NEGATIVE;
            location = (mZ2 + 1 + mZ1) / 2;
            uint32_t edgeLoc;
            if ( mParams.m_findBestPlane &&  findConcavityZ(edgeLoc) )
            {
                location = edgeLoc;
            }
        }

        return ret;
    }

    inline void getDoublePosition(const IVec3 &ip,double *p) const
    {
        getPoint(ip.x,ip.y,ip.z,mVoxelScale,mVoxelAdjust,p);
    }

    double raycast(const IVec3 &p1,const IVec3& p2) const
    {
        double ret;
        double from[3];
        double to[3];
        getDoublePosition(p1,from);
        getDoublePosition(p2,to);

        double outT;
        double faceSign;
        double hitLocation[3];
        if ( mRaycastMesh->raycast(from,to,outT,faceSign,hitLocation) )
        {
            ret = fm_distance(from,hitLocation);
        }
        else
        {
            ret = 0; // if it doesn't hit anything, just assign it to zero.
        }

        return ret;
    }

    // Finding the greatest area of concavity..
    bool findConcavityX(uint32_t &splitLoc)
    {
        bool ret = false;

        int32_t dx = (mX2-mX1)+1; // The length of the getX axis in voxel space

        // We will compute the edge error on the XY plane and the XZ plane
        // searching for the greatest location of concavity

        double *edgeErrorZ = new double[dx];
        double *edgeErrorY = new double[dx];

        // Counter of number of voxel samples on the XY plane we have accumulated
        uint32_t indexZ = 0;

        // Compute Edge Error on the XY plane
        for (int32_t x=(int32_t)mX1; x<=(int32_t)mX2; x++)
        {

            double errorTotal = 0;
            // We now perform a raycast from the sides inward on the XY plane to
            // determine the total error (distance of the surface from the sides)
            // along this getX position.
            for (int32_t y=(int32_t)mY1; y<=(int32_t)mY2; y++)
            {
                IVec3 p1(x,y,mZ1-2);
                IVec3 p2(x,y,mZ2+2);

                double e1 = raycast(p1,p2);
                double e2 = raycast(p2,p1);

                errorTotal = errorTotal+e1+e2;
            }
            // The total amount of edge error along this voxel location
            edgeErrorZ[indexZ] = errorTotal;
            indexZ++;
        }

        // Compute edge error along the XZ plane
        uint32_t indexY = 0;

        for (int32_t x=(int32_t)mX1; x<=(int32_t)mX2; x++)
        {

            double errorTotal = 0;

            for (int32_t z=(int32_t)mZ1; z<=(int32_t)mZ2; z++)
            {
                IVec3 p1(x,mY1-2,z);
                IVec3 p2(x,mY2+2,z);

                double e1 = raycast(p1,p2); // raycast from one side to the interior
                double e2 = raycast(p2,p1); // raycast from the other side to the interior

                errorTotal = errorTotal+e1+e2;
            }
            edgeErrorY[indexY] = errorTotal;
            indexY++;
        }


        // we now compute the first derivitave to find the greatest spot of concavity on the XY plane
        double maxDiff = 0;
        uint32_t maxC = 0;
        int32_t wid = (mX2-mX1)/2+1;
        for (uint32_t x=1; x<indexZ; x++)
        {
            if ( edgeErrorZ[x] > 0 &&  edgeErrorZ[x-1] > 0 )
            {
                double diff = abs(edgeErrorZ[x] - edgeErrorZ[x-1]);
                if ( diff > maxDiff )
                {
                    maxDiff = diff;
                    maxC = x-1;
                }
            }
        }
        // Now see if there is a greater concavity on the XZ plane
        for (uint32_t x=1; x<indexY; x++)
        {
            if ( edgeErrorY[x] > 0 && edgeErrorY[x-1] > 0 )
            {
                double diff = abs(edgeErrorY[x] - edgeErrorY[x-1]);
                if ( diff > maxDiff )
                {
                    maxDiff = diff;
                    maxC = x-1;
                }
            }
        }


        delete []edgeErrorZ;
        delete []edgeErrorY;

        splitLoc = maxC+mX1;

        // we do not allow an edge split if it is too close to the ends
        if ( splitLoc > (mX1+4) && splitLoc < (mX2-4) )
        {
            ret = true;
        }


        return ret;
    }


    // Finding the greatest area of concavity..
    bool findConcavityY(uint32_t &splitLoc)
    {
        bool ret = false;

        int32_t dy = (mY2-mY1)+1; // The length of the getX axis in voxel space

        // We will compute the edge error on the XY plane and the XZ plane
        // searching for the greatest location of concavity

        double *edgeErrorZ = new double[dy];
        double *edgeErrorX = new double[dy];

        // Counter of number of voxel samples on the XY plane we have accumulated
        uint32_t indexZ = 0;

        // Compute Edge Error on the XY plane
        for (int32_t y=(int32_t)mY1; y<=(int32_t)mY2; y++)
        {

            double errorTotal = 0;
            // We now perform a raycast from the sides inward on the XY plane to
            // determine the total error (distance of the surface from the sides)
            // along this getX position.
            for (int32_t x=(int32_t)mX1; x<=(int32_t)mX2; x++)
            {
                IVec3 p1(x,y,mZ1-2);
                IVec3 p2(x,y,mZ2+2);

                double e1 = raycast(p1,p2);
                double e2 = raycast(p2,p1);

                errorTotal = errorTotal+e1+e2;
            }
            // The total amount of edge error along this voxel location
            edgeErrorZ[indexZ] = errorTotal;
            indexZ++;
        }

        // Compute edge error along the XZ plane
        uint32_t indexX = 0;

        for (int32_t y=(int32_t)mY1; y<=(int32_t)mY2; y++)
        {

            double errorTotal = 0;

            for (int32_t z=(int32_t)mZ1; z<=(int32_t)mZ2; z++)
            {
                IVec3 p1(mX1-2,y,z);
                IVec3 p2(mX2+2,y,z);

                double e1 = raycast(p1,p2); // raycast from one side to the interior
                double e2 = raycast(p2,p1); // raycast from the other side to the interior

                errorTotal = errorTotal+e1+e2;
            }
            edgeErrorX[indexX] = errorTotal;
            indexX++;
        }

        // we now compute the first derivitave to find the greatest spot of concavity on the XY plane
        double maxDiff = 0;
        uint32_t maxC = 0;
        int32_t wid = (mY2-mY1)/2+1;
        for (uint32_t y=1; y<indexZ; y++)
        {
            if ( edgeErrorZ[y] > 0 && edgeErrorZ[y-1] > 0 )
            {
                double diff = abs(edgeErrorZ[y] - edgeErrorZ[y-1]);
                if ( diff > maxDiff )
                {
                    maxDiff = diff;
                    maxC = y-1;
                }
            }
        }
        // Now see if there is a greater concavity on the XZ plane
        for (uint32_t y=1; y<indexX; y++)
        {
            if ( edgeErrorX[y] >0 &&  edgeErrorX[y-1] > 0 )
            {
                double diff = abs(edgeErrorX[y] - edgeErrorX[y-1]);
                if ( diff > maxDiff )
                {
                    maxDiff = diff;
                    maxC = y-1;
                }
            }
        }

        delete []edgeErrorZ;
        delete []edgeErrorX;

        splitLoc = maxC+mY1;

        // we do not allow an edge split if it is too close to the ends
        if ( splitLoc > (mY1+4) && splitLoc < (mY2-4) )
        {
            ret = true;
        }


        return ret;
    }


    // Finding the greatest area of concavity..
    bool findConcavityZ(uint32_t &splitLoc)
    {
        bool ret = false;

        int32_t dz = (mZ2-mZ1)+1; // The length of the getX axis in voxel space

        // We will compute the edge error on the XY plane and the XZ plane
        // searching for the greatest location of concavity

        double *edgeErrorX = new double[dz];
        double *edgeErrorY = new double[dz];

        // Counter of number of voxel samples on the XY plane we have accumulated
        uint32_t indexX = 0;

        // Compute Edge Error on the XY plane
        for (int32_t z=(int32_t)mZ1; z<=(int32_t)mZ2; z++)
        {

            double errorTotal = 0;
            // We now perform a raycast from the sides inward on the XY plane to
            // determine the total error (distance of the surface from the sides)
            // along this getX position.
            for (int32_t y=(int32_t)mY1; y<=(int32_t)mY2; y++)
            {
                IVec3 p1(mX1-2,y,z);
                IVec3 p2(mX2+1,y,z);

                double e1 = raycast(p1,p2);
                double e2 = raycast(p2,p1);

                errorTotal = errorTotal+e1+e2;
            }
            // The total amount of edge error along this voxel location
            edgeErrorX[indexX] = errorTotal;
            indexX++;
        }

        // Compute edge error along the XZ plane
        uint32_t indexY = 0;

        for (int32_t z=(int32_t)mZ1; z<=(int32_t)mZ2; z++)
        {

            double errorTotal = 0;

            for (int32_t x=(int32_t)mX1; x<=(int32_t)mX2; x++)
            {
                IVec3 p1(x,mY1-2,z);
                IVec3 p2(x,mY2+2,z);

                double e1 = raycast(p1,p2); // raycast from one side to the interior
                double e2 = raycast(p2,p1); // raycast from the other side to the interior

                errorTotal = errorTotal+e1+e2;
            }
            edgeErrorY[indexY] = errorTotal;
            indexY++;
        }


        // we now compute the first derivitave to find the greatest spot of concavity on the XY plane
        double maxDiff = 0;
        uint32_t maxC = 0;
        int32_t wid = (mZ2 - mZ1)/2+1;
        for (uint32_t z=1; z<indexX; z++)
        {
            if ( edgeErrorX[z] > 0 && edgeErrorX[z-1] > 0 )
            {
                double diff = abs(edgeErrorX[z] - edgeErrorX[z-1]);
                if ( diff > maxDiff )
                {
                    maxDiff = diff;
                    maxC = z-1;
                }
            }
        }
        // Now see if there is a greater concavity on the XZ plane
        for (uint32_t z=1; z<indexY; z++)
        {
            if ( edgeErrorY[z] > 0 &&- edgeErrorY[z-1] > 0 )
            {
                double diff = abs(edgeErrorY[z] - edgeErrorY[z-1]);
                if ( diff > maxDiff )
                {
                    maxDiff = diff;
                    maxC = z-1;
                }
            }
        }


        delete []edgeErrorX;
        delete []edgeErrorY;

        splitLoc = maxC+mX1;

        // we do not allow an edge split if it is too close to the ends
        if ( splitLoc > (mZ1+4) && splitLoc < (mZ2-4) )
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

    void saveOBJ(const char *fname,VoxelHull *h)
    {
        FILE *fph = fopen(fname,"wb");
        if ( fph )
        {
            uint32_t baseIndex = 1;
            size_t vcount = mVertices.size()/3;
            size_t tcount = mIndices.size()/3;

            for (size_t i=0; i<vcount; i++)
            {
                fprintf(fph,"v %0.9f %0.9f %0.9f\n", 
                    mVertices[i*3+0],
                    mVertices[i*3+1],
                    mVertices[i*3+2]);
            }
            for (size_t i=0; i<tcount; i++)
            {
                fprintf(fph,"f %d %d %d\n", 
                    mIndices[i*3+0]+baseIndex,
                    mIndices[i*3+1]+baseIndex,
                    mIndices[i*3+2]+baseIndex);
            }

            baseIndex+=uint32_t(vcount);

            vcount = h->mVertices.size()/3;
            tcount = h->mIndices.size()/3;

            for (size_t i=0; i<vcount; i++)
            {
                fprintf(fph,"v %0.9f %0.9f %0.9f\n", 
                    h->mVertices[i*3+0],
                    h->mVertices[i*3+1]+0.1,
                    h->mVertices[i*3+2]);
            }

            for (size_t i=0; i<tcount; i++)
            {
                fprintf(fph,"f %d %d %d\n", 
                    h->mIndices[i*3+0]+baseIndex,
                    h->mIndices[i*3+1]+baseIndex,
                    h->mIndices[i*3+2]+baseIndex);
            }
            fclose(fph);
        }
    }

    void saveOBJ(const char *fname)
    {
        FILE *fph = fopen(fname,"wb");
        if ( fph )
        {
            size_t vcount = mVertices.size()/3;
            size_t tcount = mIndices.size()/3;
            printf("Saving '%s' with %d vertices and %d triangles\n", fname, uint32_t(vcount),uint32_t(tcount));
            for (size_t i=0; i<vcount; i++)
            {
                fprintf(fph,"v %0.9f %0.9f %0.9f\n", mVertices[i*3+0],mVertices[i*3+1],mVertices[i*3+2]);
            }
            for (size_t i=0; i<tcount; i++)
            {
                fprintf(fph,"f %d %d %d\n", mIndices[i*3+0]+1,mIndices[i*3+1]+1,mIndices[i*3+2]+1);
            }
            fclose(fph);
        }
    }

    SplitAxis           mAxis{SplitAxis::X_AXIS_NEGATIVE};
    Voxelize            *mVoxels{nullptr};  // The voxelized data set
    double              mVoxelScale{0};     // Size of a single voxel
    double              mVoxelScaleHalf{0}; // 1/2 of the size of a single voxel
    double              mVoxelAdjust[3];    // Minimum coordinates of the voxel space, with adjustment
    double              mVoxelBmin[3];      // Minimum coordinates of the voxel space
    double              mVoxelBmax[3];      // Maximum coordinates of the voxel space
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
    uint32_t                mX1{0};
    uint32_t                mY1{0};
    uint32_t                mZ1{0};
    uint32_t                mX2{0};
    uint32_t                mY2{0};
    uint32_t                mZ2{0};
    RaycastMesh             *mRaycastMesh{nullptr};
    VoxelIndexMap           mVoxelIndexMap; // Maps from a voxel coordinate space into a vertex index space
    std::vector< double >   mVertices;  // Vertices for the voxelized mesh
    std::vector<uint32_t>   mIndices;   // indices for the voxelized mesh
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

class Vec3d
{
public:
    Vec3d(void)
    {
    }
    Vec3d(double _x,double _y,double _z) : x(_x), y(_y), z(_z)
    {
    }
    Vec3d(const double *v)
    {
        x = v[0];
        y = v[1];
        z = v[2];
    }

    // The multiply operator appears to be a dot-product
    inline double operator*(const Vec3d& rhs) const
    {
        return (x * rhs.x + y * rhs.y + z * rhs.z);
    }

    inline Vec3d operator-(const Vec3d& rhs) const
    {
        return Vec3d(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    inline Vec3d operator+(const Vec3d& rhs) const
    {
        return Vec3d(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    inline void operator+=(const Vec3d& rhs) 
    {
        x+=rhs.x;
        y+=rhs.y;
        z+=rhs.z;
    }

    inline void operator*=(const double v)
    {
        x = x*v;
        y = y*v;
        z = z*v;
    }

    inline void operator/=(const double v)
    {
        x = x / v;
        y = y / v;
        z = z / v;
    }

    inline Vec3d operator^(const Vec3d& rhs) const
    {
        return Vec3d(y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x);
    }

    double x;
    double y;
    double z;
};

void jobCallback(void *userPtr);

// Don't consider more than 100,000 convex hulls.
#define VHACD_MAX_CONVEX_HULL_FRAGMENTS 100000

class VHACDImpl : public IVHACD, public VHACDCallbacks
{
public:
    VHACDImpl(void)
    {
    }

    virtual ~VHACDImpl(void)
    {
        Clean();
    }

    virtual void Cancel() final
    {
        mCanceled = true;
    }

    virtual bool Compute(const float* const points,
                         const uint32_t countPoints,
                         const uint32_t* const triangles,
                         const uint32_t countTriangles,
                         const Parameters& params) final
    {
        bool ret = false;

        double *dpoints = new double[countPoints*3];
        for (uint32_t i=0; i<countPoints*3; i++)
        {
            dpoints[i] = points[i];
        }
        ret = Compute(dpoints,countPoints,triangles,countTriangles,params);
        delete []dpoints;

        return ret;
    }


   
    virtual bool Compute(const double* const points,
                         const uint32_t countPoints,
                         const uint32_t* const triangles,
                         const uint32_t countTriangles,
                         const Parameters& params) final
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
        copyInputMesh(points,countPoints,triangles,countTriangles);
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

    virtual uint32_t GetNConvexHulls() const final
    {
        return uint32_t(mConvexHulls.size());
    }

    virtual bool GetConvexHull(const uint32_t index, ConvexHull& ch) const final
    {
        bool ret = false;

        if ( index < uint32_t(mConvexHulls.size() ))
        {
            ch = *mConvexHulls[index];
            ret = true;
        }

        return ret;
    }

    virtual void Clean(void) final  // release internally allocated memory
    {
#if !VHACD_DISABLE_THREADING
        delete mThreadPool;
        mThreadPool = nullptr;
#endif
        VHACD_SAFE_RELEASE(mRaycastMesh);
        VHACD_SAFE_RELEASE(mVoxelize);

        for (auto &i:mTrees)
        {
            VHACD_SAFE_RELEASE(i);
        }
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

    virtual void Release(void) final
    {
        delete this;
    }

    // Will compute the center of mass of the convex hull decomposition results and return it
    // in 'centerOfMass'.  Returns false if the center of mass could not be computed.
    virtual bool ComputeCenterOfMass(double centerOfMass[3]) const final
    {
        bool ret = false;

        return ret;
    }

    // In synchronous mode (non-multi-threaded) the state is always 'ready'
    // In asynchronous mode, this returns true if the background thread is not still actively computing
    // a new solution.  In an asynchronous config the 'IsReady' call will report any update or log
    // messages in the caller's current thread.
    virtual bool IsReady(void) const final
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
    virtual uint32_t findNearestConvexHull(const double pos[3],double &distanceToHull) final
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
				AABBTree *t  = AABBTree::create(ch.m_points,ch.m_nPoints,ch.m_triangles,ch.m_nTriangles);
				// Save the AABB tree into the container 'mTrees'
				mTrees.push_back(t);
			}
		}
		// We now compute the closest point to each convex hull and save the nearest one
		double closest = 1e99;
		for (uint32_t i=0; i<hullCount; i++)
		{
			AABBTree *t = mTrees[i];
			if ( t )
			{
				double closestPoint[3];
				if ( t->getClosestPointWithinDistance(pos,1e99,closestPoint))
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
    uint32_t getIndex(VHACD::fm_VertexIndex *vi,const double *p)
    {
        double pos[3];
        pos[0] = (p[0]-mCenter[0])*mRecipScale;
        pos[1] = (p[1]-mCenter[1])*mRecipScale;
        pos[2] = (p[2]-mCenter[2])*mRecipScale;
        bool newPos;
        uint32_t ret = vi->getIndex(pos,newPos);
        return ret;
    }


    // This copies the input mesh while scaling the input positions
    // to fit into a normalized unit cube. It also re-indexes all of the
    // vertex positions in case they weren't clean coming in. 
    void copyInputMesh(const double* const points,
                         const uint32_t countPoints,
                         const uint32_t* const triangles,
                         const uint32_t countTriangles)
    {
        mVertices.clear();
        mIndices.clear();
        mIndices.reserve(countTriangles*3);

        // First we must find the bounding box of this input vertices and normalize them into a unit-cube
        double bmin[3];
        double bmax[3];
        VHACD::fm_initMinMax(bmin,bmax);
        progressUpdate(Stages::COMPUTE_BOUNDS_OF_INPUT_MESH,0,"ComputingBounds");
        for (uint32_t i=0; i<countPoints; i++)
        {
            const double *p = &points[i*3];
            VHACD::fm_minmax(p,bmin,bmax);
        }
        progressUpdate(Stages::COMPUTE_BOUNDS_OF_INPUT_MESH,100,"ComputingBounds");

        mCenter[0] = (bmax[0]+bmin[0])*0.5;
        mCenter[1] = (bmax[1]+bmin[1])*0.5;
        mCenter[2] = (bmax[2]+bmin[2])*0.5;

        double scaleX = bmax[0] - bmin[0];
        double scaleY = bmax[1] - bmin[1];
        double scaleZ = bmax[2] - bmin[2];

        double scale = scaleX;

        if ( scaleY > scale )
        {
            scale = scaleY;
        }

        if ( scaleZ > scale )
        {
            scale = scaleZ;
        }

        mScale = scale;

        mRecipScale = mScale > 0 ? 1.0 / mScale : 0;

        {
            VHACD::fm_VertexIndex *vi = VHACD::fm_createVertexIndex(0.001,false);

            uint32_t dcount=0;

            for (uint32_t i=0; i<countTriangles && !mCanceled; i++)
            {
                uint32_t i1 = triangles[i*3+0];
                uint32_t i2 = triangles[i*3+1];
                uint32_t i3 = triangles[i*3+2];

                const double *p1 = &points[i1*3];
                const double *p2 = &points[i2*3];
                const double *p3 = &points[i3*3];

                i1 = getIndex(vi,p1);
                i2 = getIndex(vi,p2);
                i3 = getIndex(vi,p3);

                if ( i1 == i2 || i1 == i3 || i2 == i3 )
                {
                    dcount++;
                }
                else
                {
                    mIndices.push_back(i1);
                    mIndices.push_back(i2);
                    mIndices.push_back(i3);
                }
            }
            if ( dcount )
            {
                if ( mParams.m_logger )
                {
                    char scratch[512];
                    snprintf(scratch,sizeof(scratch),"Skipped %d degeneratate triangles", dcount);
                    mParams.m_logger->Log(scratch);
                }
            }
            uint32_t vcount = vi->getVcount();
            mVertices.resize(vcount*3);
            memcpy(&mVertices[0],vi->getVerticesDouble(),sizeof(double)*vcount*3);


            VHACD::fm_releaseVertexIndex(vi);
        }

        // Create the raycast mesh
        if ( !mCanceled )
        {
            progressUpdate(Stages::CREATE_RAYCAST_MESH,0,"Building RaycastMesh");
            mRaycastMesh = RaycastMesh::createRaycastMesh(uint32_t(mVertices.size())/3,&mVertices[0],uint32_t(mIndices.size())/3,&mIndices[0]);
            progressUpdate(Stages::CREATE_RAYCAST_MESH,100,"RaycastMesh completed");
        }
        if ( !mCanceled )
        {
            progressUpdate(Stages::VOXELIZING_INPUT_MESH,0,"Voxelizing Input Mesh");
            mVoxelize = Voxelize::create();
            mVoxelize->voxelize(mRaycastMesh,&mVertices[0],uint32_t(mVertices.size())/3,&mIndices[0],uint32_t(mIndices.size())/3,mParams.m_resolution,(VoxelFillMode)mParams.m_fillMode);
            mVoxelScale = mVoxelize->getScale();
            mVoxelHalfScale = mVoxelScale*0.5;
            mVoxelize->getBoundsMin(mVoxelBmin);
            mVoxelize->getBoundsMax(mVoxelBmax);
            progressUpdate(Stages::VOXELIZING_INPUT_MESH,100,"Voxelization complete");
        }


        mInputMesh.mVertexCount = (uint32_t)mVertices.size()/3;
        mInputMesh.mVertices = &mVertices[0];
        mInputMesh.mTriangleCount = (uint32_t)mIndices.size()/3;
        mInputMesh.mIndices = &mIndices[0];
        if ( !mCanceled )
        {
            progressUpdate(Stages::BUILD_INITIAL_CONVEX_HULL,0,"Build initial ConvexHull");
            VoxelHull *vh = new VoxelHull(mVoxelize,mInputMesh,mParams,this);
            if ( vh->mConvexHull )
            {
                mOverallHullVolume = vh->mConvexHull->m_volume;
            }
            mPendingHulls.push_back(vh);
            progressUpdate(Stages::BUILD_INITIAL_CONVEX_HULL,100,"Initial ConvexHull complete");
        }
    }

    void scaleOutputConvexHull(ConvexHull &ch)
    {
        for (uint32_t i=0; i<ch.m_nPoints; i++)
        {
            double *p = &ch.m_points[i*3];
            p[0] = (p[0]*mScale)+mCenter[0];
            p[1] = (p[1]*mScale)+mCenter[1];
            p[2] = (p[2]*mScale)+mCenter[2];
        }
        ch.m_volume = computeConvexHullVolume(ch); // get the combined volume
        fm_getAABB(ch.m_nPoints,ch.m_points,sizeof(double)*3,ch.mBmin,ch.mBmax);
        fm_computeCentroid(ch.m_nPoints,ch.m_points,ch.m_nTriangles,ch.m_triangles,ch.m_center);
    }

    void addCostToPriorityQueue(CostTask *task)
    {
        HullPair hp(task->mHullA->m_meshId,task->mHullB->m_meshId,task->mConcavity);
        mHullPairQueue.push(hp);
    }

    void releaseConvexHull(ConvexHull *ch)
    {
        if ( ch )
        {
            delete []ch->m_points;
            delete []ch->m_triangles;
            delete ch;
        }
    }

    void performConvexDecomposition(void)
    {
        {
            ScopedTime st("Convex Decompostion",mParams.m_logger);
            double maxHulls = pow(2,mParams.m_maxRecursionDepth);
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
                    {
                        double stageProgress = (double(count)*100.0) / maxHulls;
                        progressUpdate(Stages::PERFORMING_DECOMPOSITION,stageProgress,"Performing recursive decomposition of convex hulls");
                    }
                }
                // First we make a copy of the hulls we are processing
                VoxelHullVector oldList = mPendingHulls;
                // For each hull we want to split, we either
                // immediately perform the plane split or we post it as
                // a job to be performed in a background thread
                std::future<void> *futures = new std::future<void>[mPendingHulls.size()];
                uint32_t futureCount = 0;
                for (auto &i:mPendingHulls)
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
                    for (uint32_t i=0; i<futureCount; i++)
                    {
                        futures[i].get();
                    }
                }
                delete []futures;
                // Now, we rebuild the pending convex hulls list by
                // adding the two children to the output list if
                // we need to recurse them further
                mPendingHulls.clear();
                for (auto &vh:oldList)
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

            progressUpdate(Stages::INITIALIZING_CONVEX_HULLS_FOR_MERGING,0,"Initializing ConvexHulls");

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
                fm_getAABB(ch->m_nPoints,ch->m_points,sizeof(double)*3,ch->mBmin,ch->mBmax);
                // Inflate the AABB by 10%
                fm_inflateMinMax(ch->mBmin,ch->mBmax,0.1);

                fm_computeCentroid(ch->m_nPoints,ch->m_points,ch->m_nTriangles,ch->m_triangles,ch->m_center);

                delete vh;
                hulls.push_back(ch);
            }
            progressUpdate(Stages::INITIALIZING_CONVEX_HULLS_FOR_MERGING,100,"ConvexHull initialization complete");

            mVoxelHulls.clear();

            // here we merge convex hulls as needed until the match the
            // desired maximum hull count.
            size_t hullCount = hulls.size();

            if ( hullCount > mParams.m_maxConvexHulls && !mCanceled)
            {
                size_t costMatrixSize = ((hullCount*hullCount)-hullCount)>>1;
                CostTask *tasks = new CostTask[costMatrixSize];
                CostTask *task = tasks;
                ScopedTime st("Computing the Cost Matrix",mParams.m_logger);
                // First thing we need to do is compute the cost matrix
                // This is computed as the volume error of any two convex hulls
                // combined
                progressUpdate(Stages::COMPUTING_COST_MATRIX,0,"Computing Hull Merge Cost Matrix");
                for (size_t i=1; i<hullCount && !mCanceled; i++)
                {
                    ConvexHull *chA = hulls[i];

                    for (size_t j=0; j<i && !mCanceled; j++)
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
                            for (uint32_t i=0; i<taskCount; i++)
                            {
                                tasks[i].mFuture.get();
                            }
                        }
                        for (size_t i=0; i<taskCount; i++)
                        {
                            addCostToPriorityQueue(&tasks[i]);
                        }
                    }
                    else
#endif
                    {
                        task = tasks;
                        for (size_t i=0; i<taskCount; i++)
                        {
                            performMergeCostTask(task);
                            addCostToPriorityQueue(task);
                            task++;
                        }
                    }
                    progressUpdate(Stages::COMPUTING_COST_MATRIX,100,"Finished cost matrix");
                }
                if ( !mCanceled )
                {
                    ScopedTime stMerging("Merging Convex Hulls",mParams.m_logger);
                    Timer t;
                    // Now that we know the cost to merge each hull, we can begin merging them.
                    bool cancel = false;

                    uint32_t maxMergeCount = uint32_t(mHulls.size()) - mParams.m_maxConvexHulls;
                    uint32_t startCount = uint32_t(mHulls.size());

                    while ( !cancel && mHulls.size() > mParams.m_maxConvexHulls && !mHullPairQueue.empty() && !mCanceled)
                    {
                        double e = t.peekElapsedSeconds();
                        if ( e >= 0.1 )
                        {
                            t.reset();
                            uint32_t hullsProcessed = startCount - uint32_t(mHulls.size() );
                            double stageProgess = double(hullsProcessed*100) / double(maxMergeCount);
                            progressUpdate(Stages::MERGING_CONVEX_HULLS,stageProgess,"Merging Convex Hulls");
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
                            ConvexHull *combinedHull = computeCombinedConvexHull(*ch1,*ch2);
                            // The two old convex hulls are going to get removed
                            removeHull(hp.mHullA);
                            removeHull(hp.mHullB);

                            mMeshId++;
                            combinedHull->m_meshId = mMeshId;
                            CostTask *taskCost = tasks;

                            // Compute the cost between this new merged hull
                            // and all existing convex hulls and then 
                            // add that to the priority queue
                            for (auto &i:mHulls)
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
                                for (uint32_t i=0; i<tcount; i++)
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
                                for (size_t i=0; i<tcount; i++)
                                {
                                    performMergeCostTask(taskCost);
                                    taskCost++;
                                }
                            }
                            for (size_t i=0; i<tcount; i++)
                            {
                                addCostToPriorityQueue(&tasks[i]);
                            }
                        }
                    }
                    // Ok...once we are done, we copy the results!
                    mMeshId -= 0;
                    progressUpdate(Stages::FINALIZING_RESULTS,0,"Finalizing results");
                    for (auto &i:mHulls)
                    {
                        if ( mCanceled )
                        {
                            break;
                        }
                        ConvexHull *ch = i.second;
                        // We now must reduce the convex hull 
                        if ( ch->m_nPoints > mParams.m_maxNumVerticesPerCH || mParams.m_shrinkWrap)
                        {
                            ConvexHull *reduce = computeReducedConvexHull(*ch,mParams.m_maxNumVerticesPerCH,mParams.m_shrinkWrap);
                            releaseConvexHull(ch);
                            ch = reduce;
                        }
                        scaleOutputConvexHull(*ch);
                        ch->m_meshId = mMeshId;
                        mMeshId++;
                        mConvexHulls.push_back(ch);
                    }
                    mHulls.clear(); // since the hulls were moved into the output list, we don't need to delete them from this container
                    progressUpdate(Stages::FINALIZING_RESULTS,100,"Finalized results complete");
                }
                delete []tasks;

            }
            else
            {
                progressUpdate(Stages::FINALIZING_RESULTS,0,"Finalizing results");
                mMeshId = 0;
                for (auto &ch:hulls)
                {
                    // We now must reduce the convex hull 
                    if ( ch->m_nPoints > mParams.m_maxNumVerticesPerCH  || mParams.m_shrinkWrap )
                    {
                        ConvexHull *reduce = computeReducedConvexHull(*ch,mParams.m_maxNumVerticesPerCH,mParams.m_shrinkWrap);
                        releaseConvexHull(ch);
                        ch = reduce;
                    }
                    scaleOutputConvexHull(*ch);
                    ch->m_meshId = mMeshId;
                    mMeshId++;
                    mConvexHulls.push_back(ch);
                }
                mHulls.clear();
                progressUpdate(Stages::FINALIZING_RESULTS,100,"Finalized results");
            }

        }
    }

    double computeConvexHullVolume(const ConvexHull &sm)
    {
        double totalVolume = 0;
        Vec3d bary(0,0,0);
        for (uint32_t i=0; i<sm.m_nPoints; i++)
        {
            Vec3d p(&sm.m_points[i*3]);
            bary+=p;
        }
        bary/=double(sm.m_nPoints);

        for (uint32_t i=0; i<sm.m_nTriangles; i++)
        {
            uint32_t i1 = sm.m_triangles[i*3+0];
            uint32_t i2 = sm.m_triangles[i*3+1];
            uint32_t i3 = sm.m_triangles[i*3+2];

            Vec3d ver0(&sm.m_points[i1*3]);
            Vec3d ver1(&sm.m_points[i2*3]);
            Vec3d ver2(&sm.m_points[i3*3]);

            totalVolume+=computeVolume4(ver0,ver1,ver2,bary);

        }
        totalVolume = totalVolume / 6.0;
        return totalVolume;
    }

    double computeVolume4(const Vec3d &a,const Vec3d &b,const Vec3d &c,const Vec3d &d)
    {
        Vec3d ad = a - d;
        Vec3d bd = b - d;
        Vec3d cd = c - d;
        Vec3d bcd = bd ^ cd;
        double dot = ad * bcd;
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
            double bmin[3];
            double bmax[3];
            fm_combineAABB(ch1->mBmin, ch1->mBmax, ch2->mBmin, ch2->mBmax, bmin, bmax);
            double combinedVolume = fm_volumeAABB(bmin, bmax);
            concavity = computeConcavity(volume1 + volume2, combinedVolume, mOverallHullVolume);
            HullPair hp(ch1->m_meshId, ch2->m_meshId,concavity);
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
        double concavity = FLT_MAX;

        ConvexHull *combined = computeCombinedConvexHull(*ch1, *ch2); // Build the combined convex hull
        double combinedVolume = computeConvexHullVolume(*combined); // get the combined volume
        mt->mConcavity = computeConcavity(volume1 + volume2, combinedVolume, mOverallHullVolume);
        releaseConvexHull(combined);
    }

    ConvexHull *computeReducedConvexHull(const ConvexHull &ch,uint32_t maxVerts,bool projectHullVertices)
    {

        SimpleMesh sourceConvexHull;

        sourceConvexHull.mVertexCount = ch.m_nPoints;
        sourceConvexHull.mTriangleCount = ch.m_nTriangles;
        sourceConvexHull.mVertices = new double[sourceConvexHull.mVertexCount*3];
        memcpy(sourceConvexHull.mVertices,ch.m_points,sizeof(double)*3*sourceConvexHull.mVertexCount);
        sourceConvexHull.mIndices = new uint32_t[sourceConvexHull.mTriangleCount*3];
        memcpy(sourceConvexHull.mIndices,ch.m_triangles,sizeof(uint32_t)*3*sourceConvexHull.mTriangleCount);

        ShrinkWrap *sw = ShrinkWrap::create();
        sw->shrinkWrap(sourceConvexHull,*mRaycastMesh,maxVerts,mVoxelScale*4,projectHullVertices);

        ConvexHull *ret = new ConvexHull;

        ret->m_nPoints = sourceConvexHull.mVertexCount;
        ret->m_nTriangles = sourceConvexHull.mTriangleCount;
        ret->m_points = sourceConvexHull.mVertices;
        sourceConvexHull.mVertices = nullptr;
        ret->m_triangles = sourceConvexHull.mIndices;
        sourceConvexHull.mIndices = nullptr;

         fm_getAABB(ret->m_nPoints,ret->m_points,sizeof(double)*3,ret->mBmin,ret->mBmax);
        fm_inflateMinMax(ret->mBmin,ret->mBmax,0.1);
        fm_computeCentroid(ret->m_nPoints,ret->m_points,ret->m_nTriangles,ret->m_triangles,ret->m_center);

        ret->m_volume = computeConvexHullVolume(*ret);


        sw->release();

        // Return the convex hull 
        return ret;
    }

    // Take the points in convex hull A and the points in convex hull B and generate
    // a new convex hull on the combined set of points.
    // Once completed, we create a SimpleMesh instance to hold the triangle mesh
    // and we compute an inflated AABB for it.
    ConvexHull *computeCombinedConvexHull(const ConvexHull &sm1,const ConvexHull &sm2)
    {
        uint32_t vcount = sm1.m_nPoints+sm2.m_nPoints; // Total vertices from both hulls
        double *vertices = new double[vcount*3];  // Allocate memory for that many vertices
        memcpy(vertices,sm1.m_points,sizeof(double)*3*sm1.m_nPoints); // Copy the vertices from the first hull
        double *df = vertices+(sm1.m_nPoints*3); // Get a pointer to where to write the vertices for the second hull.
        memcpy(df,sm2.m_points,sizeof(double)*3*sm2.m_nPoints); // Copy the vertices from the second hull

        VHACD::QuickHull *qh = VHACD::QuickHull::create();
        VHACD::HullPoints hp;
        hp.mVertexCount = vcount;
        hp.mVertices = vertices;
        hp.mMaxHullVertices = vcount;
        qh->computeConvexHull(hp);

        uint32_t hvcount,htcount;
        const double *hvertices = qh->getVertices(hvcount);
        const uint32_t *hindices = qh->getIndices(htcount);

        ConvexHull *ret = new ConvexHull;
        ret->m_nPoints = hvcount;
        ret->m_nTriangles = htcount;
        ret->m_points = new double[hvcount*3];
        memcpy(ret->m_points,hvertices,sizeof(double)*hvcount*3);
        ret->m_triangles = new uint32_t[htcount*3];
        memcpy(ret->m_triangles,hindices,sizeof(uint32_t)*htcount*3);
        ret->m_volume = computeConvexHullVolume(*ret);

        fm_getAABB(hvcount,hvertices,sizeof(double)*3,ret->mBmin,ret->mBmax);
        fm_inflateMinMax(ret->mBmin,ret->mBmax,0.1);
        fm_computeCentroid(ret->m_nPoints,ret->m_points,ret->m_nTriangles,ret->m_triangles,ret->m_center);

        qh->release();
        delete []vertices;

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

        ch->mBmin[0] = source.mBmin[0];
        ch->mBmin[1] = source.mBmin[1];
        ch->mBmin[2] = source.mBmin[2];

        ch->mBmax[0] = source.mBmax[0];
        ch->mBmax[1] = source.mBmax[1];
        ch->mBmax[2] = source.mBmax[2];

        ch->m_center[0] = source.m_center[0];
        ch->m_center[1] = source.m_center[1];
        ch->m_center[2] = source.m_center[2];

        ch->m_meshId = source.m_meshId;

        ch->m_nPoints = source.m_nPoints;
        ch->m_points = new double[ch->m_nPoints*3];
        memcpy(ch->m_points,source.m_points,sizeof(double)*ch->m_nPoints*3);

        ch->m_nTriangles = source.m_nTriangles;
        ch->m_triangles = new uint32_t[ch->m_nTriangles*3];
        memcpy(ch->m_triangles,source.m_triangles,sizeof(uint32_t)*ch->m_nTriangles*3);

        ch->m_volume = source.m_volume;

        return ch;
    }

    void progressUpdate(Stages stage,double stageProgress,const char *operation)
    {
        if ( mParams.m_callback )
        {
            double overallProgress = (double(stage)*100) / double(Stages::NUM_STAGES);
            const char *s = getStageName(stage);
            mParams.m_callback->Update(overallProgress,stageProgress,s,operation);
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

    virtual bool isCanceled(void) const final
    {
        return mCanceled;
    }

    std::atomic<bool>                       mCanceled{false};
    Parameters                              mParams;                    // Convex decomposition parameters

    ConvexHullVector                        mConvexHulls;               // Finalized convex hulls
    VoxelHullVector                         mVoxelHulls;                // completed voxel hulls
    VoxelHullVector                         mPendingHulls;

    AABBTreeVector							mTrees;
    RaycastMesh                             *mRaycastMesh{nullptr};
    Voxelize                                *mVoxelize{nullptr};
    double                                  mCenter[3];
    double                                  mScale{1};
    double                                  mRecipScale{1};
    SimpleMesh                              mInputMesh;     // re-indexed and normalized input mesh
    std::vector< double >                   mVertices;
    std::vector< uint32_t >                 mIndices;

    double                                  mOverallHullVolume{0};
    double                                  mVoxelScale{0};
    double                                  mVoxelHalfScale{0};
    double                                  mVoxelBmin[3];
    double                                  mVoxelBmax[3];
    uint32_t                                mMeshId{0};
    HullPairQueue       mHullPairQueue;
#if !VHACD_DISABLE_THREADING
    ThreadPool              *mThreadPool{nullptr};
#endif
    HullMap             mHulls;

    // 
    double      mOverallProgress{0};
    double      mStageProgress{0};
    double      mOperationProgress{0};
};

void jobCallback(void *userPtr)
{
   VoxelHull *vh = (VoxelHull *)userPtr;
   vh->performPlaneSplit();
}

void computeMergeCostTask(void *ptr)
{
    CostTask *ct = (CostTask *)ptr;
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
    MyHACD_API(void)
    {
        mVHACD = VHACD::CreateVHACD();
    }

    virtual ~MyHACD_API(void)
    {
        releaseHACD();
        Cancel();
        if ( mVHACD )
        {
            mVHACD->Release();
        }
    }

    virtual void* StartTask(std::function<void()> func) override
    {
        return new std::thread(func);
    }

    virtual void JoinTask(void* Task) override
    {
        std::thread* t = (std::thread*)Task;
        t->join();
        delete t;
    }

    virtual bool Compute(const double* const _points,
                         const uint32_t countPoints,
                         const uint32_t* const _triangles,
                         const uint32_t countTriangles,
                         const Parameters& _desc) final
    {
        Cancel(); // if we previously had a solution running; cancel it.
        releaseHACD();

        Parameters desc = _desc;
        mTaskRunner = _desc.m_taskRunner ? _desc.m_taskRunner : this;
        desc.m_taskRunner = mTaskRunner;

        // We need to copy the input vertices and triangles into our own buffers so we can operate
        // on them safely from the background thread.
        mVertices = (double*)malloc(sizeof(double) * countPoints * 3);
        mIndices = (uint32_t*)malloc(sizeof(uint32_t) * countTriangles * 3);
        memcpy(mVertices, _points, sizeof(double) * countPoints * 3);
        memcpy(mIndices, _triangles, sizeof(uint32_t) * countTriangles * 3);
        mRunning = true;
        mTask = mTaskRunner->StartTask([this, countPoints, countTriangles, desc]() {
            // printf("Computing V-HACD in background task.\n");
            ComputeNow(mVertices, countPoints, mIndices, countTriangles, desc);
            // printf("V-HACD computation complete.\n");
            // If we have a user provided callback and the user did *not* call 'cancel' we notify him that the
            // task is completed. However..if the user selected 'cancel' we do not send a completed notification event.
            if (desc.m_callback && !mCancel)
            {
                // printf("Doing callback notification\n");
                desc.m_callback->NotifyVHACDComplete();
                // printf("Notify callback complete.\n");
            }
            mRunning = false;
            // printf("Exiting compute V-HACD task.\n");
        });
        return true;
    }

    bool ComputeNow(const double* const points,
                    const uint32_t countPoints,
                    const uint32_t* const triangles,
                    const uint32_t countTriangles,
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

        if (countPoints && mVHACD)
        {
            bool ok = mVHACD->Compute(points, countPoints, triangles, countTriangles, desc);
            if (ok)
            {
                ret = mVHACD->GetNConvexHulls();
            }
        }

        return ret ? true : false;
    }

    void releaseHull(VHACD::IVHACD::ConvexHull& h)
    {
        free((void*)h.m_triangles);
        free((void*)h.m_points);
        h.m_triangles = nullptr;
        h.m_points = nullptr;
    }

    virtual bool GetConvexHull(const uint32_t index, VHACD::IVHACD::ConvexHull& ch) const final
    {
        return mVHACD->GetConvexHull(index,ch);
    }

    void releaseHACD(void) // release memory associated with the last HACD request
    {
        free(mVertices);
        mVertices = nullptr;
        free(mIndices);
        mIndices = nullptr;
    }


    virtual void release(void) // release the HACD_API interface
    {
        delete this;
    }

    virtual uint32_t getHullCount(void)
    {
        return mVHACD->GetNConvexHulls();
    }

    virtual void Cancel() final
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

    virtual bool Compute(const float* const points,
                         const uint32_t countPoints,
                         const uint32_t* const triangles,
                         const uint32_t countTriangles,
                         const Parameters& params) final
    {

        double* vertices = (double*)malloc(sizeof(double) * countPoints * 3);
        const float* source = points;
        double* dest = vertices;
        for (uint32_t i = 0; i < countPoints; i++)
        {
            dest[0] = source[0];
            dest[1] = source[1];
            dest[2] = source[2];
            dest += 3;
            source += 3;
        }

        bool ret = Compute(vertices, countPoints, triangles, countTriangles, params);
        free(vertices);
        return ret;
    }

    virtual uint32_t GetNConvexHulls() const final
    {
        processPendingMessages();
        return mVHACD->GetNConvexHulls();
    }

    virtual void Clean(void) final // release internally allocated memory
    {
        Cancel();
        releaseHACD();
        mVHACD->Clean();
    }

    virtual void Release(void) final // release IVHACD
    {
        delete this;
    }

    virtual void Update(const double overallProgress,
                        const double stageProgress,
                        const char* const stage,const char *operation) final
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

    virtual void Log(const char* const msg) final
    {
        mMessageMutex.lock();
        LogMessage m;
        m.mOperation = std::string(msg);
        mHaveMessages = true;
        mMessages.push_back(m);
        mMessageMutex.unlock();
    }

    virtual bool IsReady(void) const final
    {
        processPendingMessages();
        return !mRunning;
    }

    // As a convenience for the calling application we only send it update and log messages from it's own main
    // thread.  This reduces the complexity burden on the caller by making sure it only has to deal with log
    // messages in it's main application thread.
    void processPendingMessages(void) const
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
                    mCallback->Update(i.mOverallProgress,i.mStageProgress,i.mStage.c_str(),i.mOperation.c_str());
                }
            }
            mMessages.clear();
            mHaveMessages = false;
            mMessageMutex.unlock();
        }
    }

    // Will compute the center of mass of the convex hull decomposition results and return it
    // in 'centerOfMass'.  Returns false if the center of mass could not be computed.
    virtual bool ComputeCenterOfMass(double centerOfMass[3]) const override
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
    virtual uint32_t findNearestConvexHull(const double pos[3],double &distanceToHull) final
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
    double* mVertices{ nullptr };
    uint32_t* mIndices{ nullptr };
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
#endif

#endif

#endif // VHACD_H
