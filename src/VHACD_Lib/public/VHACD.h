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
#define VHACD_H

#define VHACD_VERSION_MAJOR 2
#define VHACD_VERSION_MINOR 3

// Changes for version 2.3
//
// m_gamma : Has been removed.  This used to control the error metric to merge convex hulls.  Now it uses the 'm_maxConvexHulls' value instead.
// m_maxConvexHulls : This is the maximum number of convex hulls to produce from the merge operation; replaces 'm_gamma'.
//
// Note that the decomposition depth controls the maximum number of hulls which can be produced through 
// recursive binary tree subdivision.
// For example, if you only want no more than 4 convex hulls, you should set the convex decomposition depth
// to 2.  For a max of 8 hulls, set it to 3, etc. etc.
//
// As a convenience to the user, each convex hull produced now includes the volume of the hull as well as it's center.
//
// This version supports a convenience method to automatically make V-HACD run asynchronously in a background thread.
// To get a fully asynchronous version, call 'CreateVHACD_ASYNC' instead of 'CreateVHACD'.  You get the same interface however,
// now when computing convex hulls, it is no longer a blocking operation.  All callback messages are still returned
// in the application's thread so you don't need to worry about mutex locks or anything in that case.
// To tell if the operation is complete, the application should call 'IsReady'.  This will return true if
// the last approximation operation is complete and will dispatch any pending messages.
// If you call 'Compute' while a previous operation was still running, it will automatically cancel the last request
// and begin a new one.  To cancel a currently running approximation just call 'Cancel'.


namespace VHACD {
class IVHACD {
public:
    class IUserCallback {
    public:
        virtual ~IUserCallback(){};
        virtual void Update(const double overallProgress,
            const double stageProgress,
            const double operationProgress,
            const char* const stage,
            const char* const operation)
            = 0;
    };

    class IUserLogger {
    public:
        virtual ~IUserLogger(){};
        virtual void Log(const char* const msg) = 0;
    };

    class ConvexHull {
    public:
        double* m_points;
        int* m_triangles;
        unsigned int m_nPoints;
        unsigned int m_nTriangles;
		double		m_volume;
		double		m_center[3];
    };

    class Parameters {
    public:
        Parameters(void) { Init(); }
        void Init(void)
        {
            m_resolution = 100000;
            m_depth = 20;
            m_concavity = 0.001;
            m_planeDownsampling = 4;
            m_convexhullDownsampling = 4;
            m_alpha = 0.05;
            m_beta = 0.05;
            m_pca = 0;
            m_mode = 0; // 0: voxel-based (recommended), 1: tetrahedron-based
            m_maxNumVerticesPerCH = 64;
            m_minVolumePerCH = 0.0001;
            m_callback = 0;
            m_logger = 0;
            m_convexhullApproximation = true;
            m_oclAcceleration = true;
            m_maxConvexHulls = 1024;
        }
        double m_concavity;
        double m_alpha;
        double m_beta;
        double m_minVolumePerCH;
        IUserCallback* m_callback;
        IUserLogger* m_logger;
        unsigned int m_resolution;
        unsigned int m_maxNumVerticesPerCH;
        int m_depth;
        int m_planeDownsampling;
        int m_convexhullDownsampling;
        int m_pca;
        int m_mode;
        int m_convexhullApproximation;
        int m_oclAcceleration;
        unsigned int	m_maxConvexHulls;
    };

    virtual void Cancel() = 0;
    virtual bool Compute(const float* const points,
        const unsigned int stridePoints,
        const unsigned int countPoints,
        const int* const triangles,
        const unsigned int strideTriangles,
        const unsigned int countTriangles,
        const Parameters& params)
        = 0;
    virtual bool Compute(const double* const points,
        const unsigned int stridePoints,
        const unsigned int countPoints,
        const int* const triangles,
        const unsigned int strideTriangles,
        const unsigned int countTriangles,
        const Parameters& params)
        = 0;
    virtual unsigned int GetNConvexHulls() const = 0;
    virtual void GetConvexHull(const unsigned int index, ConvexHull& ch) const = 0;
    virtual void Clean(void) = 0; // release internally allocated memory
    virtual void Release(void) = 0; // release IVHACD
    virtual bool OCLInit(void* const oclDevice,
        IUserLogger* const logger = 0)
        = 0;
    virtual bool OCLRelease(IUserLogger* const logger = 0) = 0;

	// In synchronous mode (non-multi-threaded) the state is always 'ready'
	// In asynchronous mode, this returns true if the background thread is not still actively computing
	// a new solution.  In an asynchronous config the 'IsReady' call will report any update or log
	// messages in the caller's current thread.
	virtual bool IsReady(void) const
	{
		return true;
	}

protected:
    virtual ~IVHACD(void) {}
};
IVHACD* CreateVHACD(void);
IVHACD* CreateVHACD_ASYNC(void);
}
#endif // VHACD_H
