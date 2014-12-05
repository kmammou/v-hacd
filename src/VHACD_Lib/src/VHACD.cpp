/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <sstream>
#include <iomanip>
#if _OPENMP
#include <omp.h>
#endif // _OPENMP

#include "../public/VHACD.h"
#include "btConvexHullComputer.h"
#include "vhacdMesh.h"
#include "vhacdVolume.h"
#include "vhacdTimer.h"
#include "vhacdVector.h"
#include "vhacdSArray.h"
#include "vhacdICHull.h"

#define USE_THREAD 1
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#define ABS(a) (((a)<0) ? -(a) : (a))
#define ZSGN(a) (((a)<0) ? -1 : (a)>0 ? 1 : 0)

#define VHACD_DEBUG
namespace VHACD
{
    class VHACD : public IVHACD
    {
    public:
        //! Constructor.
                                    VHACD() { Init(); }
        //! Destructor.
                                    ~VHACD(void)
                                    {

                                    }
        unsigned int                GetNConvexHulls() const { return (unsigned int)m_convexHulls.Size(); }
        void                        Cancel() 
                                    {
                                        m_cancel = true; 
                                    }
        void                        GetConvexHull(const unsigned int index, ConvexHull & ch) const
                                    {
                                        Mesh * mesh = m_convexHulls[index];
                                        ch.m_nPoints    = (unsigned int) mesh->GetNPoints();
                                        ch.m_nTriangles = (unsigned int) mesh->GetNTriangles();
                                        ch.m_points     = mesh->GetPoints();
                                        ch.m_triangles  = mesh->GetTriangles();
                                    }
        void                        Clean(void)
                                    {
                                        delete m_volume;
                                        delete m_vset;
                                        delete m_tset;
                                        size_t nCH = m_convexHulls.Size();
                                        for (size_t p = 0; p < nCH; ++p)
                                        {
                                            delete m_convexHulls[p];
                                        }
                                        m_convexHulls.Clear();
                                        Init();
                                    }
        void                        Release(void) { delete this; }
        bool                        Compute(const float * const points,
                                            const unsigned int  stridePoints,
                                            const unsigned int  nPoints,
                                            const int   * const triangles,
                                            const unsigned int  strideTriangles,
                                            const unsigned int  nTriangles,
                                            const Parameters &  params);
    private:
        void                        AlignMesh(const float * const points,
                                              const unsigned int  stridePoints,
                                              const unsigned int  nPoints,
                                              const int   * const triangles,
                                              const unsigned int  strideTriangles,
                                              const unsigned int  nTriangles,
                                              const Parameters &  params);
        void                        VoxelizeMesh(const float * const points,
                                                 const unsigned int  stridePoints,
                                                 const unsigned int  nPoints,
                                                 const int   * const triangles,
                                                 const unsigned int  strideTriangles,
                                                 const unsigned int  nTriangles,
                                                 const Parameters &  params);
        void                        Update(const double        stageProgress,
                                           const double        operationProgress,
                                           const Parameters &  params)
                                    {
                                        m_stageProgress     = stageProgress;
                                        m_operationProgress = operationProgress;
                                        if (params.m_callback)
                                        {
                                            params.m_callback->Update(m_overallProgress,
                                                                      m_stageProgress, 
                                                                      m_operationProgress, 
                                                                      m_stage.c_str(), 
                                                                      m_operation.c_str());
                                        }
                                    }
        void                        Init()
                                    {
                                        memset(m_rot, 0, sizeof(double) * 9);
                                        m_dim               = 64;
                                        m_volume            = 0;
                                        m_volume0           = 0.0;
                                        m_vset              = 0;
                                        m_tset              = 0;
                                        m_cancel            = false;
                                        m_overallProgress   = 0.0;
                                        m_stageProgress     = 0.0;
                                        m_operationProgress = 0.0;
                                        m_barycenter[0]     = m_barycenter[1] = m_barycenter[2] = 0.0;
                                        m_rot[0][0]         = m_rot[1][1]     = m_rot[2][2]     = 1.0;
                                    }
        void                        ComputeVoxelSet      (const Parameters &  params);
        void                        ComputeVoxelACD      (const Parameters &  params);
        void                        ComputeTetrahedronSet(const Parameters &  params);
        void                        ComputeTetrahedronACD(const Parameters &  params);
        void                        MergeConvexHulls     (const Parameters &  params);
        void                        SimplifyConvexHulls  (const Parameters &  params);
        void                        ComputeBestClippingPlane(const VoxelSet        & inputVSet, 
                                                             const double            volume0, 
                                                             const double            volume, 
                                                             const SArray< Plane > & planes, 
                                                             const Vec3<double>    & preferredCuttingDirection, 
                                                             const double            w,
                                                             const double            alpha,
                                                             const double            beta,
                                                             const int               convexhullDownsampling,
                                                             Plane                 & bestPlane, 
                                                             double                & minConcavity,
                                                             double                & minBalance,
                                                             double                & minSymmetry,
                                                             const Parameters      &  params);
        void                        ComputeBestClippingPlane(const TetrahedronSet  & inputVSet,
                                                             const double            volume0,
                                                             const double            volume,
                                                             const SArray< Plane > & planes,
                                                             const Vec3<double>    & preferredCuttingDirection,
                                                             const double            w,
                                                             const double            alpha,
                                                             const double            beta,
                                                             const int               convexhullDownsampling,
                                                             Plane                 & bestPlane,
                                                             double                & minConcavity,
                                                             double                & minBalance,
                                                             double                & minSymmetry,
                                                             const Parameters      &  params);
    private:
        SArray<Mesh *>              m_convexHulls;
        std::string                 m_stage;
        std::string                 m_operation;
        double                      m_overallProgress;
        double                      m_stageProgress;
        double                      m_operationProgress;
        double                      m_rot[3][3];
        double                      m_volume0;
        Vec3<double>                m_barycenter;
        Timer                       m_timer;
        size_t                      m_dim;
        Volume *                    m_volume;
        VoxelSet *                  m_vset;
        TetrahedronSet *            m_tset;
        bool                        m_cancel; 
    };
    IVHACD * CreateVHACD(void)
    {
        return new VHACD();
    }
    void VHACD::AlignMesh(const float * const points,
                          const unsigned int  stridePoints,
                          const unsigned int  nPoints,
                          const int   * const triangles,
                          const unsigned int  strideTriangles,
                          const unsigned int  nTriangles,
                          const Parameters &  params)
    {
        if (m_cancel)
        {
            return;
        }
        m_timer.Tic();

        m_stage             = "Align mesh";
        m_operation         = "Voxelization";

        std::ostringstream msg;
        if (params.m_logger)
        {
            msg << "+ " << m_stage << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

        Update(0.0, 0.0, params);
        if (m_cancel)
        {
            return;
        }
        m_dim = (size_t)(pow((double)params.m_resolution, 1.0 / 3.0) + 0.5);
        Volume volume;
        volume.Voxelize(points, stridePoints, nPoints,
                        triangles, strideTriangles, nTriangles,
                        m_dim, m_barycenter, m_rot);
        size_t n = volume.GetNumOnSurfVoxels() + volume.GetNumInsideSurfVoxels();
        Update(50.0, 100.0, params);

        if (params.m_logger)
        {
            msg.str("");
            msg << "\t dim = " << m_dim << "\t-> " << n << " voxels" << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

#ifdef VHACD_DEBUG_ALIGN_MESH
        Mesh meshOnSurf;
        volume.Convert(meshOnSurf, VOXEL_ON_SURFACE);
        meshOnSurf.SaveOFF("meshOnSurf.off");
        Mesh meshInSurf;
        volume.Convert(meshInSurf, VOXEL_INSIDE_SURFACE);
        meshInSurf.SaveOFF("meshInSurf.off");
#endif

        if (m_cancel)
        {
            return;
        }
        m_operation = "PCA";
        Update(50.0, 0.0, params);
        volume.AlignToPrincipalAxes(m_rot);
        m_overallProgress = 1.0;
        Update(100.0, 100.0, params);

        m_timer.Toc();
        if (params.m_logger)
        {
            msg.str("");
            msg << "\t time " << m_timer.GetElapsedTime() / 1000.0 << "s" << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }
    }

    void VHACD::VoxelizeMesh(const float * const points,
                             const unsigned int  stridePoints,
                             const unsigned int  nPoints,
                             const int   * const triangles,
                             const unsigned int  strideTriangles,
                             const unsigned int  nTriangles,
                             const Parameters &  params)
    {
        if (m_cancel)
        {
            return;
        }

        m_timer.Tic();
        m_stage = "Voxelization";

        std::ostringstream msg;
        if (params.m_logger)
        {
            msg << "+ " << m_stage << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

        delete m_volume;
        m_volume = 0;
        int iteration = 0;
        const int maxIteration = 5;
        double progress = 0.0;
        while (iteration++ < maxIteration && !m_cancel)
        {
            msg.str("");
            msg << "Iteration " << iteration;
            m_operation = msg.str();

            progress = iteration * 100.0 / maxIteration;
            Update(progress, 0.0, params);

            m_volume = new Volume;
            m_volume->Voxelize(points, stridePoints, nPoints,
                               triangles, strideTriangles, nTriangles,
                               m_dim, m_barycenter, m_rot);

            Update(progress, 100.0, params);

            size_t n = m_volume->GetNumOnSurfVoxels() + m_volume->GetNumInsideSurfVoxels();
            if (params.m_logger)
            {
                msg.str("");
                msg << "\t dim = " << m_dim << "\t-> " << n << " voxels" << std::endl;
                params.m_logger->Log(msg.str().c_str());
            }

            double a = pow((double)(params.m_resolution) / n, 0.33);
            size_t dim_next = (size_t)(m_dim * a + 0.5);
            if (n < params.m_resolution && 
                iteration < maxIteration && 
                m_volume->GetNumOnSurfVoxels() < params.m_resolution / 8 && 
                m_dim != dim_next)
            {
                delete m_volume;
                m_volume = 0;
                m_dim = dim_next;
            }
            else
            {
                break;
            }
        }
        m_overallProgress = 10.0;
        Update(100.0, 100.0, params);

        m_timer.Toc();
        if (params.m_logger)
        {
            msg.str("");
            msg << "\t time " << m_timer.GetElapsedTime() / 1000.0 << "s" << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

#ifdef VHACD_DEBUG_VOXELIZATION
        Mesh meshOnSurf;
        m_volume->Convert(meshOnSurf, VOXEL_ON_SURFACE);
        meshOnSurf.SaveOFF("meshOnSurf.off");
        Mesh meshInSurf;
        m_volume->Convert(meshInSurf, VOXEL_INSIDE_SURFACE);
        meshInSurf.SaveOFF("meshInSurf.off");
#endif
    }
    void VHACD::ComputeVoxelSet(const Parameters &  params)
    {
        if (m_cancel)
        {
            return;
        }
        m_timer.Tic();

        m_stage             = "Compute voxel set";
        m_operation         = "Convert volume to vset";

        std::ostringstream msg;
        if (params.m_logger)
        {
            msg << "+ " << m_stage << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

        Update(0.0, 0.0, params);
        m_vset = new VoxelSet;
        m_volume->Convert(*m_vset);
        delete m_volume;
        m_volume = 0;

        if (params.m_logger)
        {
            msg.str("");
            msg << "\t # voxels                   " << m_vset->GetNVoxels() << std::endl;
            msg << "\t # inside surface           " << m_vset->GetNumInsideSurfVoxels() << std::endl;
            msg << "\t # on surface               " << m_vset->GetNumOnSurfVoxels() << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

#ifdef VHACD_DEBUG_VSET
#endif

        m_overallProgress = 15.0;
        Update(100.0, 100.0, params);
        m_timer.Toc();
        if (params.m_logger)
        {
            msg.str("");
            msg << "\t time " << m_timer.GetElapsedTime() / 1000.0 << "s" << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }
    }
    void VHACD::ComputeTetrahedronSet(const Parameters &  params)
    {
        if (m_cancel)
        {
            return;
        }
        m_timer.Tic();

        m_stage = "Compute tetrahedron set";
        m_operation = "Convert volume to tset";

        std::ostringstream msg;
        if (params.m_logger)
        {
            msg << "+ " << m_stage << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

        Update(0.0, 0.0, params);
        m_tset = new TetrahedronSet;
        m_volume->Convert(*m_tset);
        delete m_volume;
        m_volume = 0;

        if (params.m_logger)
        {
            msg.str("");
            msg << "\t # tetrahedra               " << m_tset->GetNTetrahedra() << std::endl;
            msg << "\t # inside surface           " << m_tset->GetNumInsideSurfTetrahedra() << std::endl;
            msg << "\t # on surface               " << m_tset->GetNumOnSurfTetrahedra() << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

#ifdef VHACD_DEBUG_VSET
#endif

        m_overallProgress = 15.0;
        Update(100.0, 100.0, params);
        m_timer.Toc();
        if (params.m_logger)
        {
            msg.str("");
            msg << "\t time " << m_timer.GetElapsedTime() / 1000.0 << "s" << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }
    }



    bool VHACD::Compute(const float * const points,
                        const unsigned int  stridePoints,
                        const unsigned int  nPoints,
                        const int   * const triangles,
                        const unsigned int  strideTriangles,
                        const unsigned int  nTriangles,
                        const Parameters &  params) 
    {
        Init();
        if (params.m_pca)
        {
            AlignMesh(points   , stridePoints   , nPoints   ,
                      triangles, strideTriangles, nTriangles, params);
        }
        VoxelizeMesh(points, stridePoints, nPoints,
                     triangles, strideTriangles, nTriangles, params);
        if (params.m_mode == 0)
        {
            ComputeVoxelSet(params);
            ComputeVoxelACD(params);
        }
        else
        {
            ComputeTetrahedronSet(params);
            ComputeTetrahedronACD(params);
        }
        MergeConvexHulls(params);
        SimplifyConvexHulls(params);
        if (m_cancel)
        {
            Clean();
            return false;
        }
        return true;
    }

    double ComputePreferredCuttingDirection(const VoxelSet & vset, Vec3<double> & dir)
    {
        double ex = vset.GetEigenValue(AXIS_X);
        double ey = vset.GetEigenValue(AXIS_Y);
        double ez = vset.GetEigenValue(AXIS_Z);
        double vx = (ey-ez)*(ey-ez);
        double vy = (ex-ez)*(ex-ez);
        double vz = (ex-ey)*(ex-ey);
        if (vx < vy && vx < vz)
        {
            double e = ey*ey + ez*ez;
            dir[0] = 1.0;
            dir[1] = 0.0;
            dir[2] = 0.0;
            return (e == 0.0) ? 0.0 : 1.0 - vx / e;
        }
        else if (vy < vx && vy < vz)
        {
            double e = ex*ex + ez*ez;
            dir[0] = 0.0;
            dir[1] = 1.0;
            dir[2] = 0.0;
            return (e == 0.0) ? 0.0 : 1.0 - vy / e;
        }
        else
        {
            double e = ex*ex + ey*ey;
            dir[0] = 0.0;
            dir[1] = 0.0;
            dir[2] = 1.0;
            return (e == 0.0) ? 0.0 : 1.0 - vz / e;
        }
    }
    double ComputePreferredCuttingDirection(const TetrahedronSet & tset, Vec3<double> & dir)
    {
        double ex = tset.GetEigenValue(AXIS_X);
        double ey = tset.GetEigenValue(AXIS_Y);
        double ez = tset.GetEigenValue(AXIS_Z);
        double vx = (ey - ez)*(ey - ez);
        double vy = (ex - ez)*(ex - ez);
        double vz = (ex - ey)*(ex - ey);
        if (vx < vy && vx < vz)
        {
            double e = ey*ey + ez*ez;
            dir[0] = 1.0;
            dir[1] = 0.0;
            dir[2] = 0.0;
            return (e == 0.0) ? 0.0 : 1.0 - vx / e;
        }
        else if (vy < vx && vy < vz)
        {
            double e = ex*ex + ez*ez;
            dir[0] = 0.0;
            dir[1] = 1.0;
            dir[2] = 0.0;
            return (e == 0.0) ? 0.0 : 1.0 - vy / e;
        }
        else
        {
            double e = ex*ex + ey*ey;
            dir[0] = 0.0;
            dir[1] = 0.0;
            dir[2] = 1.0;
            return (e == 0.0) ? 0.0 : 1.0 - vz / e;
        }
    }
    void ComputeAxesAlignedClippingPlanes(const VoxelSet & vset, 
                                          const short      downsampling, 
                                          SArray<Plane>  & planes)
    {
        const Vec3<short> minV = vset.GetMinBBVoxels();
        const Vec3<short> maxV = vset.GetMaxBBVoxels();
        Vec3<double> pt;
        Plane      plane;
        const short i0 = minV[0];
        const short i1 = maxV[0];
        plane.m_a    = 1.0;
        plane.m_b    = 0.0;
        plane.m_c    = 0.0;
        plane.m_axis = AXIS_X;
        for(short i = i0; i <= i1; i += downsampling)
        {
            pt            = vset.GetPoint(Vec3<double>(i + 0.5, 0.0, 0.0));
            plane.m_d     = -pt[0];
            plane.m_index = i;
            planes.PushBack(plane);
        }
        const short j0 = minV[1];
        const short j1 = maxV[1];
        plane.m_a    = 0.0;
        plane.m_b    = 1.0;
        plane.m_c    = 0.0;
        plane.m_axis = AXIS_Y;
        for(short j = j0; j <= j1; j += downsampling)
        {
            pt            = vset.GetPoint(Vec3<double>(0.0, j + 0.5, 0.0));
            plane.m_d     = -pt[1];
            plane.m_index = j;
            planes.PushBack(plane);
        }
        const short k0 = minV[2];
        const short k1 = maxV[2];
        plane.m_a    = 0.0;
        plane.m_b    = 0.0;
        plane.m_c    = 1.0;
        plane.m_axis = AXIS_Z;
        for(short k = k0; k <= k1; k += downsampling)
        {
            pt            = vset.GetPoint(Vec3<double>(0.0, 0.0, k + 0.5));
            plane.m_d     = -pt[2];
            plane.m_index = k;
            planes.PushBack(plane);
        }
    }
    void RefineAxesAlignedClippingPlanes(const VoxelSet & vset, 
                                         const Plane    & bestPlane, 
                                         const short      downsampling, 
                                         SArray<Plane>  & planes)
    {
        const Vec3<short> minV = vset.GetMinBBVoxels();
        const Vec3<short> maxV = vset.GetMaxBBVoxels();
        Vec3<double> pt;
        Plane      plane;

        if (bestPlane.m_axis == AXIS_X)
        {
            const short i0 = MAX(minV[0], bestPlane.m_index - downsampling);
            const short i1 = MIN(maxV[0], bestPlane.m_index + downsampling);
            plane.m_a      = 1.0;
            plane.m_b      = 0.0;
            plane.m_c      = 0.0;
            plane.m_axis   = AXIS_X;
            for(short i = i0; i <= i1; ++i)
            {
                pt            = vset.GetPoint(Vec3<double>(i + 0.5, 0.0, 0.0));
                plane.m_d     = -pt[0];
                plane.m_index = i;
                planes.PushBack(plane);
            }
        }
        else if (bestPlane.m_axis == AXIS_Y)
        {
            const short j0 = MAX(minV[1], bestPlane.m_index - downsampling);
            const short j1 = MIN(maxV[1], bestPlane.m_index + downsampling);
            plane.m_a      = 0.0;
            plane.m_b      = 1.0;
            plane.m_c      = 0.0;
            plane.m_axis   = AXIS_Y;
            for(short j = j0; j <= j1; ++j)
            {
                pt            = vset.GetPoint(Vec3<double>(0.0, j+0.5, 0.0));
                plane.m_d     = -pt[1];
                plane.m_index = j;
                planes.PushBack(plane);
            }
        }
        else
        {
            const short k0 =  MAX(minV[2], bestPlane.m_index - downsampling);
            const short k1 =  MIN(maxV[2], bestPlane.m_index + downsampling);
            plane.m_a      = 0.0;
            plane.m_b      = 0.0;
            plane.m_c      = 1.0;
            plane.m_axis   = AXIS_Z;
            for(short k = k0; k <= k1; ++k)
            {
                pt            = vset.GetPoint(Vec3<double>(0.0, 0.0, k + 0.5));
                plane.m_d     = -pt[2];
                plane.m_index = k;
                planes.PushBack(plane);
            }
        }
    }
    void VHACD::ComputeBestClippingPlane(const VoxelSet        & inputVSet, 
                                         const double            volume0, 
                                         const double            volume, 
                                         const SArray< Plane > & planes, 
                                         const Vec3<double>    & preferredCuttingDirection, 
                                         const double            w,
                                         const double            alpha,
                                         const double            beta,
                                         const int               convexhullDownsampling,
                                         Plane                 & bestPlane, 
                                         double                & minConcavity,
                                         double                & minBalance,
                                         double                & minSymmetry,
                                         const Parameters      &  params)
    {
        if (m_cancel)
        {
            return;
        }
        char msg[1024];
        int  iBest     = -1;
        int  nPlanes   = (int) static_cast<int>(planes.Size());
        double minTotal  = minConcavity + minBalance + minSymmetry;
        int  processed = 0;
        bool cancel = false;
#if USE_THREAD == 1 && _OPENMP
        #pragma omp parallel for
#endif
        for(int x = 0; x < nPlanes; ++x)
        {
#if USE_THREAD == 1 && _OPENMP
            #pragma omp flush (cancel)
#endif
            if (!cancel)
            {
                //Update progress
                if (m_cancel) {
                    cancel = true;
#if USE_THREAD == 1 && _OPENMP
                    #pragma omp flush (cancel)
#endif
                }

                Plane plane = planes[x];
                VoxelSet left;
                VoxelSet right;
                inputVSet.Clip(plane, &right, &left);

                if (right.GetNVoxels() > 0 && left.GetNVoxels() > 0)
                {
                    Mesh leftCH;
                    Mesh rightCH;
                    left.ComputeConvexHull(leftCH, convexhullDownsampling);
                    right.ComputeConvexHull(rightCH, convexhullDownsampling);
                    double volumeLeft    = fabs(left.ComputeVolume());
                    double volumeRight   = fabs(right.ComputeVolume());
                    double volumeLeftCH  = leftCH.ComputeVolume();
                    double volumeRightCH = rightCH.ComputeVolume();
                    double concavity     = fabs(volumeLeftCH + volumeRightCH - volume) / volume0;
                    double balance       = alpha * pow( pow(volumeLeft - volumeRight, 2.0), 0.5)/ volume0;
                    double d             = w * (preferredCuttingDirection[0] * plane.m_a + preferredCuttingDirection[1] * plane.m_b + preferredCuttingDirection[2] * plane.m_c);
                    double symmetry      = beta * d;
                    double total         = concavity +  balance +  symmetry;

#if USE_THREAD == 1 && _OPENMP
                    #pragma omp critical
#endif
                    {
                        if (total <  minTotal)
                        {    
                            if (params.m_logger)
                            {
                                sprintf(msg, "\t\t\t Plane %04i T=%2.3f C=%2.3f B=%2.3f S=%2.3f D=%1.6f W=%1.6f [%1.1f, %1.1f, %1.1f](%1.1f, %1.1f, %1.1f, %3.3f) \n", 
                                            x, total, concavity, balance, symmetry, d, w, 
                                            preferredCuttingDirection[0], preferredCuttingDirection[1], preferredCuttingDirection[2],
                                            plane.m_a, plane.m_b, plane.m_c, plane.m_d);
                                params.m_logger->Log(msg);
                            }
                            bestPlane     = plane;
                            minTotal      = total;
                            minConcavity  = concavity;
                            iBest         = x;
                            minBalance    = balance;
                            minSymmetry   = symmetry;
                        }
                    }
                }
            }
        }
        if (params.m_logger)
        {
            sprintf(msg, "\n\t\t\t Best  %04i T=%2.3f C=%2.3f B=%2.3f S=%2.3f (%1.1f, %1.1f, %1.1f, %3.3f)\n\n", iBest, minTotal, minConcavity, minBalance, minSymmetry, bestPlane.m_a, bestPlane.m_b, bestPlane.m_c, bestPlane.m_d);
            params.m_logger->Log(msg);
        }
    }
    void VHACD::ComputeVoxelACD(const Parameters &  params)
    {
        if (m_cancel)
        {
            return;
        }
        m_timer.Tic();

        m_stage             = "Approximate Convex Decomposition";
        m_stageProgress     = 0.0;
        std::ostringstream msg;
        if (params.m_logger)
        {
            msg << "+ " << m_stage << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

        SArray< VoxelSet * > parts;
        SArray< VoxelSet * > inputParts;
        SArray< VoxelSet * > temp;
        inputParts.PushBack(m_vset);
        m_vset = 0;
        SArray< Plane > planes;
        SArray< Plane > planesRef;
        int sub = 0;
        bool firstIteration = true;
        m_volume0 = 1.0;
        double concavity0 = 1.0;
        while (sub++ < params.m_depth && inputParts.Size() > 0 && !m_cancel)
        {
            msg.str("");
            msg << "Subdivision level " << sub;
            m_operation = msg.str();

            if (params.m_logger)
            {
                msg.str("");
                msg << "\t Subdivision level " << sub << std::endl;
                params.m_logger->Log(msg.str().c_str());
            }

            double maxConcavity = 0.0;
            const size_t nInputParts = inputParts.Size();
            Update(m_stageProgress, 0.0, params);
            for(size_t p = 0; p < nInputParts && !m_cancel; ++p)
            {
                Update(m_stageProgress, p * 100.0 / nInputParts, params);

                VoxelSet * vset = inputParts[p];
                inputParts[p]    = 0;
                double volume     = vset->ComputeVolume();
                vset->ComputeBB();
//                vset->ComputePrincipalAxes();

                Mesh ch;
                vset->ComputeConvexHull(ch, 1);
                double volumeCH = fabs(ch.ComputeVolume());

                if (firstIteration)
                {
                    m_volume0        = volumeCH;
                }

                double error     = 1.01 * vset->ComputeMaxVolumeError() / m_volume0;
                double concavity = fabs(volumeCH - volume) / m_volume0;

                if (firstIteration)
                {
                    concavity0     = concavity;
                    firstIteration = false;
                }
                

                if (params.m_logger)
                {
                    msg.str("");
                    msg << "\t -> Part[" << p 
                        << "] C = " << concavity 
                        << ", E = " << error 
                        << ", VS = " << vset->GetNumOnSurfVoxels() 
                        << ", VC = " << vset->GetNumOnClipPlaneVoxels()
                        << ", VI = " << vset->GetNumInsideSurfVoxels()
                        << std::endl;
                    params.m_logger->Log(msg.str().c_str());
                }

                if (concavity > params.m_concavity && concavity > error)
                {
                    Vec3<double> preferredCuttingDirection;
                    double w = ComputePreferredCuttingDirection(*vset, preferredCuttingDirection);

                    
                    planes.Resize(0);
                    ComputeAxesAlignedClippingPlanes(*vset, params.m_planeDownsampling, planes);

                    if (params.m_logger)
                    {
                        msg.str("");
                        msg << "\t\t [Regular sampling] Number of clipping planes " << planes.Size() << std::endl;
                        params.m_logger->Log(msg.str().c_str());
                    }

                    Plane bestPlane;
                    double minConcavity = 1.0;
                    double minBalance   = 1.0;
                    double minSymmetry  = 1.0;

                    ComputeBestClippingPlane(*vset, 
                                             m_volume0, 
                                             volume, 
                                             planes, 
                                             preferredCuttingDirection, 
                                             w,
                                             concavity * params.m_alpha,
                                             concavity * params.m_beta,
                                             params.m_convexhullDownsampling,
                                             bestPlane, 
                                             minConcavity,
                                             minBalance,
                                             minSymmetry,
                                             params);
                    if (!m_cancel && (params.m_planeDownsampling > 1 || params.m_convexhullDownsampling > 1))
                    {
                        planesRef.Resize(0);
                        RefineAxesAlignedClippingPlanes(*vset, bestPlane, params.m_planeDownsampling, planesRef);
                        if (params.m_logger)
                        {
                            msg.str("");
                            msg << "\t\t [Refining] Number of clipping planes " << planesRef.Size() << std::endl;
                            params.m_logger->Log(msg.str().c_str());
                        }
                        minConcavity = 1.0;
                        minBalance   = 1.0;
                        minSymmetry  = 1.0;
                        ComputeBestClippingPlane(*vset, 
                                                 m_volume0, 
                                                 volume, 
                                                 planesRef, 
                                                 preferredCuttingDirection, 
                                                 w,
                                                 concavity * params.m_alpha,
                                                 concavity * params.m_beta,
                                                 1,                 // convexhullDownsampling = 1
                                                 bestPlane, 
                                                 minConcavity,
                                                 minBalance,
                                                 minSymmetry,
                                                 params);
                    }
                    if (m_cancel)
                    {
                        delete vset; // clean up
                        break;
                    }
                    else
                    {
                        if (maxConcavity < minConcavity)
                        {
                            maxConcavity = minConcavity;
                        }
                        VoxelSet  * bestLeft  = new VoxelSet;
                        VoxelSet  * bestRight = new VoxelSet;
                        temp.PushBack(bestLeft);
                        temp.PushBack(bestRight);
                        vset->Clip(bestPlane, bestRight, bestLeft);
                        delete vset;
                    }
                }
                else
                {
                    parts.PushBack(vset);
                }
            }

            Update(90.0*(1.0 - maxConcavity) / (1.0 - params.m_concavity), 100.0, params);
            
            if (m_cancel)
            {
                const size_t nTempParts = temp.Size();
                for (size_t p = 0; p < nTempParts; ++p)
                {
                    delete temp[p];
                }
                temp.Resize(0);
            }
            else
            {
                inputParts = temp;
                temp.Resize(0);
            }
        }

        const size_t nInputParts = inputParts.Size();
        for (size_t p = 0; p < nInputParts; ++p)
        {
            parts.PushBack(inputParts[p]);
        }

        if (m_cancel)
        {
            const size_t nParts = parts.Size();
            for (size_t p = 0; p < nInputParts; ++p)
            {
                delete parts[p];
            }
            return;
        }

        m_overallProgress = 90.0;
        Update(m_stageProgress, 100.0, params);

        msg.str("");
        msg << "Generate convex-hulls";
        m_operation = msg.str();
        size_t nConvexHulls = parts.Size();
        if (params.m_logger)
        {
            msg.str("");
            msg << "+ Generate " << nConvexHulls << " convex-hulls " << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

        Update(m_stageProgress, 0.0, params);
        m_convexHulls.Resize(0);
        for (size_t p = 0; p < nConvexHulls && !m_cancel; ++p)
        {
            Update(m_stageProgress, p * 100.0 / nConvexHulls, params);
            m_convexHulls.PushBack(new Mesh);
            parts[p]->ComputeConvexHull(*m_convexHulls[p], 1);
            size_t nv = m_convexHulls[p]->GetNPoints();
            double x, y, z;
            for (size_t i = 0; i < nv; ++i)
            {
                Vec3<double> & pt = m_convexHulls[p]->GetPoint(i);
                x = pt[0];
                y = pt[1];
                z = pt[2];
                pt[0] = m_rot[0][0] * x + m_rot[0][1] * y + m_rot[0][2] * z + m_barycenter[0];
                pt[1] = m_rot[1][0] * x + m_rot[1][1] * y + m_rot[1][2] * z + m_barycenter[1];
                pt[2] = m_rot[2][0] * x + m_rot[2][1] * y + m_rot[2][2] * z + m_barycenter[2];
            }
        }
        const size_t nParts = parts.Size();
        for (size_t p = 0; p < nInputParts; ++p)
        {
            delete parts[p];
            parts[p] = 0;
        }
        parts.Resize(0);

        if (m_cancel)
        {
            const size_t nConvexHulls = m_convexHulls.Size();
            for (size_t p = 0; p < nConvexHulls; ++p)
            {
                delete m_convexHulls[p];
            }
            m_convexHulls.Clear();
            return;
        }

        m_overallProgress = 95.0;
        Update(100.0, 100.0, params);
        m_timer.Toc();
        if (params.m_logger)
        {
            msg.str("");
            msg << "\t time " << m_timer.GetElapsedTime() / 1000.0 << "s" << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }
    }
    void ComputeAxesAlignedClippingPlanes(const TetrahedronSet & tset,
                                          const short            downsampling,
                                          SArray< Plane >      & planes)
    {
        const Vec3<double> minV  = tset.GetMinBB();
        const Vec3<double> maxV  = tset.GetMaxBB();
        const double       scale = tset.GetSacle();
        const short        i0    = 0;
        const short        j0    = 0;
        const short        k0    = 0;
        const short        i1    = (short)((maxV[0] - minV[0]) / scale + 0.5);
        const short        j1    = (short)((maxV[1] - minV[1]) / scale + 0.5);
        const short        k1    = (short)((maxV[2] - minV[2]) / scale + 0.5);

        Plane plane;
        plane.m_a = 1.0;
        plane.m_b = 0.0;
        plane.m_c = 0.0;
        plane.m_axis = AXIS_X;
        for (short i = i0; i <= i1; i += downsampling)
        {
            double x = minV[0] + scale * i;
            plane.m_d = -x;
            plane.m_index = i;
            planes.PushBack(plane);
        }
        plane.m_a = 0.0;
        plane.m_b = 1.0;
        plane.m_c = 0.0;
        plane.m_axis = AXIS_Y;
        for (short j = j0; j <= j1; j += downsampling)
        {
            double y = minV[1] + scale * j;
            plane.m_d = -y;
            plane.m_index = j;
            planes.PushBack(plane);
        }
        plane.m_a = 0.0;
        plane.m_b = 0.0;
        plane.m_c = 1.0;
        plane.m_axis = AXIS_Z;
        for (short k = k0; k <= k1; k += downsampling)
        {
            double z = minV[2] + scale * k;
            plane.m_d = -z;
            plane.m_index = k;
            planes.PushBack(plane);
        }
    }
    void RefineAxesAlignedClippingPlanes(const TetrahedronSet & tset,
                                         const  Plane         & bestPlane,
                                         const  short           downsampling,
                                         SArray< Plane >      & planes)
    {
        const Vec3<double> minV = tset.GetMinBB();
        const Vec3<double> maxV = tset.GetMaxBB();
        const double       scale = tset.GetSacle();
        Plane              plane;

        if (bestPlane.m_axis == AXIS_X)
        {
            const short i0 = MAX(0, bestPlane.m_index - downsampling);
            const short i1 = (short)MIN((maxV[0] - minV[0]) / scale + 0.5, bestPlane.m_index + downsampling);
            plane.m_a = 1.0;
            plane.m_b = 0.0;
            plane.m_c = 0.0;
            plane.m_axis = AXIS_X;
            for (short i = i0; i <= i1; ++i)
            {
                double x = minV[0] + scale * i;
                plane.m_d = -x;
                plane.m_index = i;
                planes.PushBack(plane);
            }
        }
        else if (bestPlane.m_axis == AXIS_Y)
        {
            const short j0 = MAX(0, bestPlane.m_index - downsampling);
            const short j1 = (short)MIN((maxV[1] - minV[1]) / scale + 0.5, bestPlane.m_index + downsampling);
            plane.m_a = 0.0;
            plane.m_b = 1.0;
            plane.m_c = 0.0;
            plane.m_axis = AXIS_Y;
            for (short j = j0; j <= j1; ++j)
            {
                double y = minV[1] + scale * j;
                plane.m_d = -y;
                plane.m_index = j;
                planes.PushBack(plane);
            }
        }
        else
        {
            const short k0 = MAX(0, bestPlane.m_index - downsampling);
            const short k1 = (short)MIN((maxV[2] - minV[2]) / scale + 0.5, bestPlane.m_index + downsampling);
            plane.m_a = 0.0;
            plane.m_b = 0.0;
            plane.m_c = 1.0;
            plane.m_axis = AXIS_Z;
            for (short k = k0; k <= k1; ++k)
            {
                double z = minV[2] + scale * k;
                plane.m_d = -z;
                plane.m_index = k;
                planes.PushBack(plane);
            }
        }
    }

    void VHACD::ComputeBestClippingPlane(const TetrahedronSet  & inputTSet,
                                         const double            volume0,
                                         const double            volume,
                                         const SArray< Plane > & planes,
                                         const Vec3<double>    & preferredCuttingDirection,
                                         const double            w,
                                         const double            alpha,
                                         const double            beta,
                                         const int               convexhullDownsampling,
                                         Plane                 & bestPlane,
                                         double                & minConcavity,
                                         double                & minBalance,
                                         double                & minSymmetry,
                                         const Parameters      &  params)
    {
        if (m_cancel)
        {
            return;
        }
        char msg[1024];
        int  iBest = -1;
        int  nPlanes = (int) static_cast<int>(planes.Size());
        double minTotal = minConcavity + minBalance + minSymmetry;
        int  processed = 0;
        bool cancel = false;
#if USE_THREAD == 1 && _OPENMP
        #pragma omp parallel for
#endif
        for (int x = 0; x < nPlanes; ++x)
        {
#if USE_THREAD == 1 && _OPENMP
            #pragma omp flush (cancel)
#endif
            if (!cancel)
            {
                //Update progress
                if (m_cancel) {
                    cancel = true;
#if USE_THREAD == 1 && _OPENMP
                    #pragma omp flush (cancel)
#endif
                }
                Plane plane = planes[x];
                TetrahedronSet left;
                TetrahedronSet right;
                inputTSet.Clip(plane, &right, &left);

                if (right.GetNTetrahedra() > 0 && left.GetNTetrahedra() > 0)
                {
                    Mesh leftCH;
                    Mesh rightCH;
                    left.ComputeConvexHull(leftCH, convexhullDownsampling);
                    right.ComputeConvexHull(rightCH, convexhullDownsampling);
                    double volumeLeft = fabs(left.ComputeVolume());
                    double volumeRight = fabs(right.ComputeVolume());
                    double volumeLeftCH = leftCH.ComputeVolume();
                    double volumeRightCH = rightCH.ComputeVolume();
                    double concavity = fabs(volumeLeftCH + volumeRightCH - volume) / volume0;
                    double balance = alpha * pow(pow(volumeLeft - volumeRight, 2.0), 0.5) / volume0;
                    double d = w * (preferredCuttingDirection[0] * plane.m_a + preferredCuttingDirection[1] * plane.m_b + preferredCuttingDirection[2] * plane.m_c);
                    double symmetry = beta * d;
                    double total = concavity + balance + symmetry;

#if USE_THREAD == 1 && _OPENMP
#pragma omp critical
#endif
                    {
                        if (total <  minTotal)
                        {
                            if (params.m_logger)
                            {
                                sprintf(msg, "\t\t\t Plane %04i T=%2.3f C=%2.3f B=%2.3f S=%2.3f D=%1.6f W=%1.6f [%1.1f, %1.1f, %1.1f](%1.1f, %1.1f, %1.1f, %3.3f) \n",
                                    x, total, concavity, balance, symmetry, d, w,
                                    preferredCuttingDirection[0], preferredCuttingDirection[1], preferredCuttingDirection[2],
                                    plane.m_a, plane.m_b, plane.m_c, plane.m_d);
                                params.m_logger->Log(msg);
                            }
                            bestPlane = plane;
                            minTotal = total;
                            minConcavity = concavity;
                            iBest = x;
                            minBalance = balance;
                            minSymmetry = symmetry;
                        }
                    }
                }
            }
        }
        if (params.m_logger)
        {
            sprintf(msg, "\n\t\t\t Best  %04i T=%2.3f C=%2.3f B=%2.3f S=%2.3f (%1.1f, %1.1f, %1.1f, %3.3f)\n\n", iBest, minTotal, minConcavity, minBalance, minSymmetry, bestPlane.m_a, bestPlane.m_b, bestPlane.m_c, bestPlane.m_d);
            params.m_logger->Log(msg);
        }
    }
    void VHACD::ComputeTetrahedronACD(const Parameters &  params)
    {
        if (m_cancel)
        {
            return;
        }
        m_timer.Tic();

        m_stage = "Approximate Convex Decomposition";
        m_stageProgress = 0.0;
        std::ostringstream msg;
        if (params.m_logger)
        {
            msg << "+ " << m_stage << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

        SArray< TetrahedronSet * > parts;
        SArray< TetrahedronSet * > inputParts;
        SArray< TetrahedronSet * > temp;
        inputParts.PushBack(m_tset);
        m_tset = 0;
        SArray< Plane > planes;
        SArray< Plane > planesRef;
        int sub = 0;
        bool firstIteration = true;
        m_volume0 = 1.0;
        double concavity0 = 1.0;
        while (sub++ < params.m_depth && inputParts.Size() > 0 && !m_cancel)
        {
            msg.str("");
            msg << "Subdivision level " << sub;
            m_operation = msg.str();

            if (params.m_logger)
            {
                msg.str("");
                msg << "\t Subdivision level " << sub << std::endl;
                params.m_logger->Log(msg.str().c_str());
            }

            double maxConcavity = 0.0;
            const size_t nInputParts = inputParts.Size();
            Update(m_stageProgress, 0.0, params);
            for (size_t p = 0; p < nInputParts && !m_cancel; ++p)
            {
                Update(m_stageProgress, p * 100.0 / nInputParts, params);

                TetrahedronSet * tset = inputParts[p];
                inputParts[p] = 0;
                double volume = tset->ComputeVolume();
                tset->ComputeBB();
                tset->ComputePrincipalAxes();
                if (params.m_pca)
                {
                    tset->AlignToPrincipalAxes();
                }

                Mesh ch;
                tset->ComputeConvexHull(ch, 1);
                double volumeCH = fabs(ch.ComputeVolume());

                if (firstIteration)
                {
                    m_volume0 = volumeCH;
                }

                double error = 1.01 * tset->ComputeMaxVolumeError() / m_volume0;
                double concavity = fabs(volumeCH - volume) / m_volume0;

                if (firstIteration)
                {
                    concavity0 = concavity;
                    firstIteration = false;
                }


                if (params.m_logger)
                {
                    msg.str("");
                    msg << "\t -> Part[" << p
                        << "] C = " << concavity
                        << ", E = " << error
                        << ", VS = " << tset->GetNumOnSurfTetrahedra()
                        << ", VC = " << tset->GetNumOnClipPlaneTetrahedra()
                        << ", VI = " << tset->GetNumInsideSurfTetrahedra()
                        << std::endl;
                    params.m_logger->Log(msg.str().c_str());
                }

                if (concavity > params.m_concavity && concavity > error)
                {
                    Vec3<double> preferredCuttingDirection;
                    double w = ComputePreferredCuttingDirection(*tset, preferredCuttingDirection);


                    planes.Resize(0);
                    ComputeAxesAlignedClippingPlanes(*tset, params.m_planeDownsampling, planes);

                    if (params.m_logger)
                    {
                        msg.str("");
                        msg << "\t\t [Regular sampling] Number of clipping planes " << planes.Size() << std::endl;
                        params.m_logger->Log(msg.str().c_str());
                    }

                    Plane bestPlane;
                    double minConcavity = 1.0;
                    double minBalance = 1.0;
                    double minSymmetry = 1.0;

                    ComputeBestClippingPlane(*tset,
                                             m_volume0,
                                             volume,
                                             planes,
                                             preferredCuttingDirection,
                                             w,
                                             concavity * params.m_alpha,
                                             concavity * params.m_beta,
                                             params.m_convexhullDownsampling,
                                             bestPlane,
                                             minConcavity,
                                             minBalance,
                                             minSymmetry,
                                             params);
                    if (!m_cancel && (params.m_planeDownsampling > 1 || params.m_convexhullDownsampling > 1))
                    {
                        planesRef.Resize(0);
                        RefineAxesAlignedClippingPlanes(*tset, bestPlane, params.m_planeDownsampling, planesRef);
                        if (params.m_logger)
                        {
                            msg.str("");
                            msg << "\t\t [Refining] Number of clipping planes " << planesRef.Size() << std::endl;
                            params.m_logger->Log(msg.str().c_str());
                        }
                        minConcavity = 1.0;
                        minBalance = 1.0;
                        minSymmetry = 1.0;
                        ComputeBestClippingPlane(*tset,
                                                 m_volume0,
                                                 volume,
                                                 planesRef,
                                                 preferredCuttingDirection,
                                                 w,
                                                 concavity * params.m_alpha,
                                                 concavity * params.m_beta,
                                                 1,                 // convexhullDownsampling = 1
                                                 bestPlane,
                                                 minConcavity,
                                                 minBalance,
                                                 minSymmetry,
                                                 params);
                    }
                    if (m_cancel)
                    {
                        delete tset; // clean up
                        break;
                    }
                    else
                    {
                        if (maxConcavity < minConcavity)
                        {
                            maxConcavity = minConcavity;
                        }
                        TetrahedronSet  * bestLeft  = new TetrahedronSet;
                        TetrahedronSet  * bestRight = new TetrahedronSet;
                        temp.PushBack(bestLeft);
                        temp.PushBack(bestRight);
                        tset->Clip(bestPlane, bestRight, bestLeft);
                        if (params.m_pca)
                        {
                            bestRight->RevertAlignToPrincipalAxes();
                            bestLeft->RevertAlignToPrincipalAxes();
                        }
                        delete tset;
                    }
                }
                else
                {
                    if (params.m_pca)
                    {
                        tset->RevertAlignToPrincipalAxes();
                    }
                    parts.PushBack(tset);
                }
            }

            Update(95.0*(1.0 - maxConcavity) / (1.0 - params.m_concavity), 100.0, params);
            if (m_cancel)
            {
                const size_t nTempParts = temp.Size();
                for (size_t p = 0; p < nTempParts; ++p)
                {
                    delete temp[p];
                }
                temp.Resize(0);
            }
            else
            {
                inputParts = temp;
                temp.Resize(0);
            }
        }
        const size_t nInputParts = inputParts.Size();
        for (size_t p = 0; p < nInputParts; ++p)
        {
            parts.PushBack(inputParts[p]);
        }

        if (m_cancel)
        {
            const size_t nParts = parts.Size();
            for (size_t p = 0; p < nInputParts; ++p)
            {
                delete parts[p];
            }
            return;
        }

        m_overallProgress = 90.0;
        Update(m_stageProgress, 100.0, params);


        msg.str("");
        msg << "Generate convex-hulls";
        m_operation = msg.str();
        size_t nConvexHulls = parts.Size();
        if (params.m_logger)
        {
            msg.str("");
            msg << "+ Generate " << nConvexHulls << " convex-hulls " << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

        Update(m_stageProgress, 0.0, params);
        m_convexHulls.Resize(0);
        for (size_t p = 0; p < nConvexHulls && !m_cancel; ++p)
        {
            Update(m_stageProgress, p * 100.0 / nConvexHulls, params);
            m_convexHulls.PushBack(new Mesh);
            parts[p]->ComputeConvexHull(*m_convexHulls[p], 1);
            size_t nv = m_convexHulls[p]->GetNPoints();
            double x, y, z;
            for (size_t i = 0; i < nv; ++i)
            {
                Vec3<double> & pt = m_convexHulls[p]->GetPoint(i);
                x = pt[0];
                y = pt[1];
                z = pt[2];
                pt[0] = m_rot[0][0] * x + m_rot[0][1] * y + m_rot[0][2] * z + m_barycenter[0];
                pt[1] = m_rot[1][0] * x + m_rot[1][1] * y + m_rot[1][2] * z + m_barycenter[1];
                pt[2] = m_rot[2][0] * x + m_rot[2][1] * y + m_rot[2][2] * z + m_barycenter[2];
            }
        }

        const size_t nParts = parts.Size();
        for (size_t p = 0; p < nInputParts; ++p)
        {
            delete parts[p];
            parts[p] = 0;
        }
        parts.Resize(0);

        if (m_cancel)
        {
            const size_t nConvexHulls = m_convexHulls.Size();
            for (size_t p = 0; p < nConvexHulls; ++p)
            {
                delete m_convexHulls[p];
            }
            m_convexHulls.Clear();
            return;
        }

        m_overallProgress = 95.0;
        Update(100.0, 100.0, params);
        m_timer.Toc();
        if (params.m_logger)
        {
            msg.str("");
            msg << "\t time " << m_timer.GetElapsedTime() / 1000.0 << "s" << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }
    }

    void AddPoints(const Mesh * const mesh, SArray< Vec3<double> > & pts)
    {
        const int n = (int) mesh->GetNPoints();
        for(int i = 0; i < n; ++i)
        {
            pts.PushBack(mesh->GetPoint(i));
        }
    }
    void ComputeConvexHull(const Mesh * const     ch1, 
                           const Mesh * const     ch2,
                           SArray< Vec3<double> > & pts,
                           Mesh * const           combinedCH)
    {
        pts.Resize(0);
        AddPoints(ch1, pts);
        AddPoints(ch2, pts);

        btConvexHullComputer ch;
        ch.compute((double *)pts.Data(), 3 * sizeof(double), (int)pts.Size(), -1.0, -1.0);
        combinedCH->ResizePoints   (0);
        combinedCH->ResizeTriangles(0);
        for (int v = 0; v < ch.vertices.size(); v++)
        {
            combinedCH->AddPoint(Vec3<double>(ch.vertices[v].getX(), ch.vertices[v].getY(), ch.vertices[v].getZ()));
        }
        const int nt = ch.faces.size();
        for (int t = 0; t < nt; ++t)
        {
            const btConvexHullComputer::Edge * sourceEdge = &(ch.edges[ch.faces[t]]);
            int a = sourceEdge->getSourceVertex();
            int b = sourceEdge->getTargetVertex();
            const btConvexHullComputer::Edge * edge = sourceEdge->getNextEdgeOfFace();
            int c = edge->getTargetVertex();
            while (c != a)
            {
                combinedCH->AddTriangle(Vec3<int>(a, b, c));
                edge = edge->getNextEdgeOfFace();
                b = c;
                c = edge->getTargetVertex();
            }
        }
    }

    void VHACD::MergeConvexHulls(const Parameters &  params)
    {
        if (m_cancel)
        {
            return;
        }
        m_timer.Tic();

        m_stage             = "Merge Convex Hulls";

        std::ostringstream msg;
        if (params.m_logger)
        {
            msg << "+ " << m_stage << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

        size_t nConvexHulls = m_convexHulls.Size();
        int iteration = 0;
        if (nConvexHulls > 1 && !m_cancel)
        {
            const size_t nConvexHulls0 = nConvexHulls;
            const double threshold = params.m_gamma * m_volume0;
            SArray< Vec3<double> > pts;
            Mesh combinedCH;
            bool iterate = true;

            while (iterate && !m_cancel)
            {
                msg.str("");
                msg << "Iteration " << iteration++;
                m_operation = msg.str();

                size_t bestp1;
                size_t bestp2;
                double bestCost = m_volume0;
                for (size_t p1 = 0; (p1 < nConvexHulls0 - 1) && (!m_cancel); ++p1)
                {
                    Update(iteration * 100.0 / nConvexHulls0, p1 * 100.0 / (nConvexHulls0 - 1), params);

                    if (m_convexHulls[p1] && !m_cancel)
                    {
                        double volume1 = m_convexHulls[p1]->ComputeVolume();
                        size_t p2 = p1 + 1;
                        while (p2 < nConvexHulls0)
                        {
                            if (p1 != p2 && m_convexHulls[p2] && !m_cancel)
                            {
                                double volume2 = m_convexHulls[p2]->ComputeVolume();

                                ComputeConvexHull(m_convexHulls[p1], m_convexHulls[p2], pts, &combinedCH);

                                double combinedVolume = combinedCH.ComputeVolume();
                                double cost           = combinedVolume - volume1 - volume2;
                                if (cost < bestCost)
                                {
                                    bestCost       = cost;
                                    bestp1         = p1;
                                    bestp2         = p2;
                                    if (params.m_logger)
                                    {
                                        msg.str("");
                                        msg << "\t\t Cost (" << p1 << ", " << p2 << ") " << cost / m_volume0 << std::endl;
                                        params.m_logger->Log(msg.str().c_str());
                                    }
                                }
                            }
                            ++p2;
                        }
                    }
                }
                if (bestCost < threshold && !m_cancel)
                {
                    if (params.m_logger)
                    {
                        msg.str("");
                        msg << "\t\t Merging (" << bestp1 << ", " << bestp2 << ") " << bestCost / m_volume0 << std::endl << std::endl;
                        params.m_logger->Log(msg.str().c_str());
                    }
                    Mesh * cch = new Mesh;
                    ComputeConvexHull(m_convexHulls[bestp1], m_convexHulls[bestp2], pts, cch);
                    delete m_convexHulls[bestp1];
                    delete m_convexHulls[bestp2];
                    m_convexHulls[bestp2] = 0;
                    m_convexHulls[bestp1] = cch;
                    iterate             = true;
                    --nConvexHulls;
                }
                else
                {
                    iterate             = false;
                }
            }
            if (!m_cancel)
            {
                SArray<Mesh *> temp;
                temp.Allocate(nConvexHulls);
                for (size_t p1 = 0; p1 < nConvexHulls0; ++p1)
                {
                    if (m_convexHulls[p1])
                    {
                        temp.PushBack(m_convexHulls[p1]);
                    }
                }
                m_convexHulls = temp;
            }
        }
        m_overallProgress = 99.0;
        Update(100.0, 100.0, params);
        m_timer.Toc();
        if (params.m_logger)
        {
            msg.str("");
            msg << "\t time " << m_timer.GetElapsedTime() / 1000.0 << "s" << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }
    }
    void SimplifyConvexHull(Mesh * const ch,
                            const size_t nvertices)
    {
        if (nvertices <= 4 || ch->GetNPoints() <= nvertices)
        {
            return;
        }
        ICHull icHull;
        icHull.AddPoints(ch->GetPointsBuffer(), ch->GetNPoints());
        icHull.Process((unsigned int)nvertices);
        TMMesh & mesh = icHull.GetMesh();
        const size_t nT = mesh.GetNTriangles();
        const size_t nV = mesh.GetNVertices();
        ch->ResizePoints(nV);
        ch->ResizeTriangles(nT);
        mesh.GetIFS(ch->GetPointsBuffer(), ch->GetTrianglesBuffer());
    }

    void VHACD::SimplifyConvexHulls(const Parameters &  params)
    {
        if (m_cancel || params.m_maxNumVerticesPerCH < 4)
        {
            return;
        }
        m_timer.Tic();

        m_stage     = "Simplify convex-hulls";
        m_operation = "Simplify convex-hulls";

        std::ostringstream msg;
        const size_t nConvexHulls = m_convexHulls.Size();
        if (params.m_logger)
        {
            msg << "+ Simplify " << nConvexHulls << " convex-hulls "  << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

        Update(0.0, 0.0, params);
        for (size_t i = 0; i < nConvexHulls && !m_cancel; ++i)
        {
            if (m_convexHulls[i]->GetNPoints() > params.m_maxNumVerticesPerCH)
            {
                if (params.m_logger)
                {
                    msg.str("");
                    msg << "\t\t Simplify CH[" << std::setfill('0') << std::setw(5) << i << "] " << m_convexHulls[i]->GetNPoints() << " V, " << m_convexHulls[i]->GetNTriangles() << " T" << std::endl;
                    params.m_logger->Log(msg.str().c_str());
                }
                SimplifyConvexHull(m_convexHulls[i], params.m_maxNumVerticesPerCH);
            }
        }

        m_overallProgress = 100.0;
        Update(100.0, 100.0, params);
        m_timer.Toc();
        if (params.m_logger)
        {
            msg.str("");
            msg << "\t time " << m_timer.GetElapsedTime() / 1000.0 << "s" << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }
    }
}

