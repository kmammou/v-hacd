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
#include <limits>
#if _OPENMP
#include <omp.h>
#endif // _OPENMP

#include "vhacdHACD.h"
#include "btConvexHullComputer.h"


#define USE_THREAD 1
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#define ABS(a) (((a)<0) ? -(a) : (a))
#define ZSGN(a) (((a)<0) ? -1 : (a)>0 ? 1 : 0)

namespace VHACD
{
    Real ComputePreferredCuttingDirection(const VoxelSet & vset, Vec3<Real> & dir)
    {
        Real ex = vset.GetEigenValue(AXIS_X);
        Real ey = vset.GetEigenValue(AXIS_Y);
        Real ez = vset.GetEigenValue(AXIS_Z);
        Real vx = (ey-ez)*(ey-ez);
        Real vy = (ex-ez)*(ex-ez);
        Real vz = (ex-ey)*(ex-ey);
        if (vx < vy && vx < vz)
        {
            Real e = ey*ey + ez*ez;
            dir[0] = 1.0;
            dir[1] = 0.0;
            dir[2] = 0.0;
            return (e == 0.0) ? 0.0 : 1.0 - vx / e;
        }
        else if (vy < vx && vy < vz)
        {
            Real e = ex*ex + ez*ez;
            dir[0] = 0.0;
            dir[1] = 1.0;
            dir[2] = 0.0;
            return (e == 0.0) ? 0.0 : 1.0 - vy / e;
        }
        else
        {
            Real e = ex*ex + ey*ey;
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
        Vec3<Real> pt;
        Plane      plane;
        const short i0 = minV[0];
        const short i1 = maxV[0];
        plane.m_a    = 1.0;
        plane.m_b    = 0.0;
        plane.m_c    = 0.0;
        plane.m_axis = AXIS_X;
        for(short i = i0; i <= i1; i += downsampling)
        {
            pt            = vset.GetPoint(Vec3<Real>(i + 0.5, 0.0, 0.0));
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
            pt            = vset.GetPoint(Vec3<Real>(0.0, j + 0.5, 0.0));
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
            pt            = vset.GetPoint(Vec3<Real>(0.0, 0.0, k + 0.5));
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
        Vec3<Real> pt;
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
                pt            = vset.GetPoint(Vec3<Real>(i + 0.5, 0.0, 0.0));
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
                pt            = vset.GetPoint(Vec3<Real>(0.0, j+0.5, 0.0));
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
                pt            = vset.GetPoint(Vec3<Real>(0.0, 0.0, k + 0.5));
                plane.m_d     = -pt[2];
                plane.m_index = k;
                planes.PushBack(plane);
            }
        }
    }
    void ComputeBestClippingPlane(const VoxelSet        & inputVSet, 
                                  const Real              volume0, 
                                  const Real              volume, 
                                  const SArray< Plane > & planes, 
                                  const Vec3<Real>      & preferredCuttingDirection, 
                                  const Real              w,
                                  const Real              alpha,
                                  const Real              beta,
                                  const int               convexhullDownsampling,
                                  Plane                 & bestPlane, 
                                  Real                  & minConcavity,
                                  Real                  & minBalance,
                                  Real                  & minSymmetry,
                                  const CallBackFunction  callBack)
    {
        int  iBest    = -1;
        int  nPlanes  = (int) static_cast<int>(planes.Size());
        Real minTotal = minConcavity + minBalance + minSymmetry;
#if USE_THREAD == 1 && _OPENMP
        #pragma omp parallel for
#endif
        for(int x = 0; x < nPlanes; ++x)
        {
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
                Real volumeLeft    = fabs(left.ComputeVolume());
                Real volumeRight   = fabs(right.ComputeVolume());
                Real volumeLeftCH  = leftCH.ComputeVolume();
                Real volumeRightCH = rightCH.ComputeVolume();
                Real concavity     = fabs(volumeLeftCH + volumeRightCH - volume) / volume0;
                Real balance       = alpha * pow( pow(volumeLeft - volumeRight, 2.0), 0.5)/ volume0;
                Real d             = w * (preferredCuttingDirection[0] * plane.m_a + preferredCuttingDirection[1] * plane.m_b + preferredCuttingDirection[2] * plane.m_c);
                Real symmetry      = beta * d;
                Real total         = concavity +  balance +  symmetry;

#if USE_THREAD == 1 && _OPENMP
                #pragma omp critical
#endif
                {
                    if (total <  minTotal)
                    {    
                        if (callBack)
                        {
                            char msg[1024];
                            sprintf(msg, "\t\t\t Plane %04i T=%2.3f C=%2.3f B=%2.3f S=%2.3f D=%1.6f W=%1.6f [%1.1f, %1.1f, %1.1f](%1.1f, %1.1f, %1.1f, %3.3f) \n", 
                                        x, total, concavity, balance, symmetry, d, w, 
                                        preferredCuttingDirection[0], preferredCuttingDirection[1], preferredCuttingDirection[2],
                                        plane.m_a, plane.m_b, plane.m_c, plane.m_d);
                            (*callBack)(msg);
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
        if (callBack)
        {
            char msg[1024];
            sprintf(msg, "\n\t\t\t Best  %04i T=%2.3f C=%2.3f B=%2.3f S=%2.3f (%1.1f, %1.1f, %1.1f, %3.3f)\n\n", iBest, minTotal, minConcavity, minBalance, minSymmetry, bestPlane.m_a, bestPlane.m_b, bestPlane.m_c, bestPlane.m_d);
            (*callBack)(msg);
        }
    }
    bool ApproximateConvexDecomposition(VoxelSet * const       inputVSet, 
                                        const int              depth, 
                                        const int              planeDownsampling,
                                        const int              convexhullDownsampling, 
                                        const Real             alpha, 
                                        const Real             beta, 
                                        const Real             concavityThreshold,
                                        Real  &                volume0,
                                        SArray< VoxelSet * > & parts, 
                                        const CallBackFunction callBack)
    {
        SArray< VoxelSet * > inputParts;
        inputParts.PushBack(inputVSet);
        SArray< Plane > planes;
        SArray< Plane > planesRef;
        int sub = 0;
        bool firstIteration = true;
        volume0 = 1.0;
        while( sub++ < depth && inputParts.Size() > 0)
        {
            if (callBack)
            {
                std::ostringstream msg;
                msg << "\t Subdivision level " << sub << std::endl;
                (*callBack)(msg.str().c_str());
            }
            const size_t nInputParts = inputParts.Size();
            SArray< VoxelSet * > temp;
            temp.Allocate(2*nInputParts);
            for(size_t p = 0; p < nInputParts; ++p)
            {
                VoxelSet * vset = inputParts[p];
                Real volume     = vset->ComputeVolume();
                vset->ComputeBB();
                vset->ComputePrincipalAxes();

                Mesh ch;
                vset->ComputeConvexHull(ch, 1);
                Real volumeCH = fabs(ch.ComputeVolume());

                if (firstIteration)
                {
                    volume0        = volumeCH;
                    firstIteration = false;
                }

                Real error     = 1.01 * vset->ComputeMaxVolumeError() / volume0;
                Real concavity = fabs(volumeCH - volume) / volume0;

                if (callBack)
                {
                    std::ostringstream msg;
                    msg << "\t -> Part[" << p 
                        << "] C = " << concavity 
                        << ", E = " << error 
                        << ", VS = " << vset->GetNumOnSurfVoxels() 
                        << ", VC = " << vset->GetNumOnClipPlaneVoxels()
                        << ", VI = " << vset->GetNumInsideSurfVoxels()
                        << std::endl;
                    (*callBack)(msg.str().c_str());
                }


                if (concavity > concavityThreshold && concavity > error)
                {
                    Vec3<Real> preferredCuttingDirection;
                    Real w = ComputePreferredCuttingDirection(*vset, preferredCuttingDirection);

                    
                    planes.Resize(0);
                    ComputeAxesAlignedClippingPlanes(*vset, planeDownsampling, planes);

                    if (callBack)
                    {
                        std::ostringstream msg;
                        msg << "\t\t [Regular sampling] Nunber of clipping planes " << planes.Size() << std::endl;
                        (*callBack)(msg.str().c_str());
                    }

                    Plane bestPlane;
                    VoxelSet  * bestLeft  = new VoxelSet;
                    VoxelSet  * bestRight = new VoxelSet;
                    temp.PushBack(bestLeft);
                    temp.PushBack(bestRight);

                    Real minConcavity = std::numeric_limits<Real>::max();
                    Real minBalance   = std::numeric_limits<Real>::max();
                    Real minSymmetry  = std::numeric_limits<Real>::max();

                    ComputeBestClippingPlane(*vset, 
                                             volume0, 
                                             volume, 
                                             planes, 
                                             preferredCuttingDirection, 
                                             w,
                                             concavity * alpha,
                                             concavity * beta,
                                             convexhullDownsampling,
                                             bestPlane, 
                                             minConcavity,
                                             minBalance,
                                             minSymmetry,
                                             callBack);

                    if (planeDownsampling > 1 || convexhullDownsampling > 1)
                    {
                        planesRef.Resize(0);
                        RefineAxesAlignedClippingPlanes(*vset, bestPlane, planeDownsampling, planesRef);
                        if (callBack)
                        {
                            std::ostringstream msg;
                            msg << "\t\t [Refining] Nunber of clipping planes " << planesRef.Size() << std::endl;
                            (*callBack)(msg.str().c_str());
                        }
                        minConcavity = std::numeric_limits<Real>::max();
                        minBalance   = std::numeric_limits<Real>::max();
                        minSymmetry  = std::numeric_limits<Real>::max();
                        ComputeBestClippingPlane(*vset, 
                                                 volume0, 
                                                 volume, 
                                                 planesRef, 
                                                 preferredCuttingDirection, 
                                                 w,
                                                 concavity * alpha,
                                                 concavity * beta,
                                                 1,                 // convexhullDownsampling = 1
                                                 bestPlane, 
                                                 minConcavity,
                                                 minBalance,
                                                 minSymmetry,
                                                 callBack);
                    }
                    vset->Clip(bestPlane, bestRight, bestLeft);
                    delete vset;
                }
                else
                {
                    parts.PushBack(vset);
                }
            }
            inputParts.Resize(0);
            const size_t nTempParts = temp.Size();
            for(size_t p = 0; p < nTempParts; ++p)
            {
                inputParts.PushBack(temp[p]);
            }
        }
        const size_t nInputParts = inputParts.Size();
        for(size_t p = 0; p < nInputParts; ++p)
        {
            parts.PushBack(inputParts[p]);
        }
        return true;
    }
    Real ComputePreferredCuttingDirection(const TetrahedronSet & tset, Vec3<Real> & dir)
    {
        Real ex = tset.GetEigenValue(AXIS_X);
        Real ey = tset.GetEigenValue(AXIS_Y);
        Real ez = tset.GetEigenValue(AXIS_Z);
        Real vx = (ey - ez)*(ey - ez);
        Real vy = (ex - ez)*(ex - ez);
        Real vz = (ex - ey)*(ex - ey);
        if (vx < vy && vx < vz)
        {
            Real e = ey*ey + ez*ez;
            dir[0] = 1.0;
            dir[1] = 0.0;
            dir[2] = 0.0;
            return (e == 0.0) ? 0.0 : 1.0 - vx / e;
        }
        else if (vy < vx && vy < vz)
        {
            Real e = ex*ex + ez*ez;
            dir[0] = 0.0;
            dir[1] = 1.0;
            dir[2] = 0.0;
            return (e == 0.0) ? 0.0 : 1.0 - vy / e;
        }
        else
        {
            Real e = ex*ex + ey*ey;
            dir[0] = 0.0;
            dir[1] = 0.0;
            dir[2] = 1.0;
            return (e == 0.0) ? 0.0 : 1.0 - vz / e;
        }
    }
    void ComputeAxesAlignedClippingPlanes(const TetrahedronSet & tset, 
                                          const short            downsampling, 
                                          SArray< Plane >      & planes)
    {
        const Vec3<Real> minV  = tset.GetMinBB();
        const Vec3<Real> maxV  = tset.GetMaxBB();
        const Real       scale = tset.GetSacle();
        const short      i0    = 0;
        const short      j0    = 0;
        const short      k0    = 0;
        const short      i1    = (short)((maxV[0] - minV[0]) / scale+0.5);
        const short      j1    = (short)((maxV[1] - minV[1]) / scale+0.5);
        const short      k1    = (short)((maxV[2] - minV[2]) / scale+0.5);

        Plane plane;
        plane.m_a    = 1.0;
        plane.m_b    = 0.0;
        plane.m_c    = 0.0;
        plane.m_axis = AXIS_X;
        for(short i = i0; i <= i1; i += downsampling)
        {
            Real x        = minV[0] + scale * i;
            plane.m_d     = -x;
            plane.m_index = i;
            planes.PushBack(plane);
        }
        plane.m_a    = 0.0;
        plane.m_b    = 1.0;
        plane.m_c    = 0.0;
        plane.m_axis = AXIS_Y;
        for(short j = j0; j <= j1; j += downsampling)
        {
            Real y        = minV[1] + scale * j;
            plane.m_d     = -y;
            plane.m_index = j;
            planes.PushBack(plane);
        }
        plane.m_a    = 0.0;
        plane.m_b    = 0.0;
        plane.m_c    = 1.0;
        plane.m_axis = AXIS_Z;
        for(short k = k0; k <= k1; k += downsampling)
        {
            Real z        = minV[2] + scale * k;
            plane.m_d     = -z;
            plane.m_index = k;
            planes.PushBack(plane);
        }
    }
    void RefineAxesAlignedClippingPlanes(const TetrahedronSet & tset, 
                                        const  Plane          & bestPlane, 
                                        const  short            downsampling, 
                                        SArray< Plane >       & planes)
    {
        const Vec3<Real> minV  = tset.GetMinBB();
        const Vec3<Real> maxV  = tset.GetMaxBB();
        const Real       scale = tset.GetSacle();
        Plane            plane;

        if (bestPlane.m_axis == AXIS_X)
        {
            const short i0 = MAX(0, bestPlane.m_index - downsampling);
            const short i1 = (short) MIN((maxV[0] - minV[0]) / scale + 0.5, bestPlane.m_index + downsampling);
            plane.m_a      = 1.0;
            plane.m_b      = 0.0;
            plane.m_c      = 0.0;
            plane.m_axis   = AXIS_X;
            for(short i = i0; i <= i1; ++i)
            {
                Real x        = minV[0] + scale * i;
                plane.m_d     = -x;
                plane.m_index = i;
                planes.PushBack(plane);
            }
        }
        else if (bestPlane.m_axis == AXIS_Y)
        {
            const short j0 = MAX(0, bestPlane.m_index - downsampling);
            const short j1 = (short) MIN((maxV[1] - minV[1]) / scale + 0.5, bestPlane.m_index + downsampling);
            plane.m_a      = 0.0;
            plane.m_b      = 1.0;
            plane.m_c      = 0.0;
            plane.m_axis   = AXIS_Y;
            for(short j = j0; j <= j1; ++j)
            {
                Real y        = minV[1] + scale * j;
                plane.m_d     = -y;
                plane.m_index = j;
                planes.PushBack(plane);
            }
        }
        else
        {
            const short k0 = MAX(0, bestPlane.m_index - downsampling);
            const short k1 = (short) MIN((maxV[2] - minV[2]) / scale + 0.5, bestPlane.m_index + downsampling);
            plane.m_a      = 0.0;
            plane.m_b      = 0.0;
            plane.m_c      = 1.0;
            plane.m_axis   = AXIS_Z;
            for(short k = k0; k <= k1; ++k)
            {
                Real z        = minV[2] + scale * k;
                plane.m_d     = -z;
                plane.m_index = k;
                planes.PushBack(plane);
            }
        }
    }

    void ComputeBestClippingPlane(const TetrahedronSet   & inputTSet, 
                                  const Real               volume0, 
                                  const Real               volume, 
                                  const SArray< Plane >  & planes, 
                                  const Vec3<Real>       & preferredCuttingDirection, 
                                  const Real               w,
                                  const Real               alpha,
                                  const Real               beta,
                                  const int                convexhullDownsampling,
                                  Plane                  & bestPlane, 
                                  Real                   & minConcavity,
                                  Real                   & minBalance,
                                  Real                   & minSymmetry,
                                  const CallBackFunction   callBack)
    {
        int  iBest    = -1;
        int  nPlanes  = (int) static_cast<int>(planes.Size());
        Real minTotal = minConcavity + minBalance + minSymmetry;
#if USE_THREAD == 1 && _OPENMP
        #pragma omp parallel for
#endif
        for(int x = 0; x < nPlanes; ++x)
        {
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
                Real volumeLeft    = fabs(left.ComputeVolume());
                Real volumeRight   = fabs(right.ComputeVolume());
                Real volumeLeftCH  = leftCH.ComputeVolume();
                Real volumeRightCH = rightCH.ComputeVolume();
                Real concavity     = fabs(volumeLeftCH + volumeRightCH - volume) / volume0;
                Real balance       = alpha * pow( pow(volumeLeft - volumeRight, 2.0), 0.5)/ volume0;
                Real d             = w * (preferredCuttingDirection[0] * plane.m_a + preferredCuttingDirection[1] * plane.m_b + preferredCuttingDirection[2] * plane.m_c);
                Real symmetry      = beta * d;
                Real total         = concavity + balance + symmetry;

#if USE_THREAD == 1 && _OPENMP
#pragma omp critical
#endif
                {
                    if (total <  minTotal)
                    {
                        if (callBack)
                        {
                            char msg[1024];
                            sprintf(msg, "\t\t\t Plane %04i T=%2.3f C=%2.3f B=%2.3f S=%2.3f D=%1.6f W=%1.6f [%1.1f, %1.1f, %1.1f](%1.1f, %1.1f, %1.1f, %3.3f) \n",
                                x, total, concavity, balance, symmetry, d, w,
                                preferredCuttingDirection[0], preferredCuttingDirection[1], preferredCuttingDirection[2],
                                plane.m_a, plane.m_b, plane.m_c, plane.m_d);
                            (*callBack)(msg);
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
        if (callBack)
        {
            char msg[1024];
            sprintf(msg, "\n\t\t\t Best  %04i T=%2.3f C=%2.3f B=%2.3f S=%2.3f (%1.1f, %1.1f, %1.1f, %3.3f)\n\n", iBest, minTotal, minConcavity, minBalance, minSymmetry, bestPlane.m_a, bestPlane.m_b, bestPlane.m_c, bestPlane.m_d);
            (*callBack)(msg);
        }
    }
    bool ApproximateConvexDecomposition(TetrahedronSet * const       inputTSet, 
                                        const int                    depth, 
                                        const int                    planeDownsampling,
                                        const int                    convexhullDownsampling, 
                                        const Real                   alpha, 
                                        const Real                   beta, 
                                        const Real                   concavityThreshold,
                                        const bool                   pca,
                                        Real                       & volume0,
                                        SArray< TetrahedronSet * > & parts, 
                                        const CallBackFunction       callBack)
    {
        SArray< TetrahedronSet * > inputParts;
        inputParts.PushBack(inputTSet);
        SArray< Plane > planes;
        SArray< Plane > planesRef;
        int sub = 0;
        bool firstIteration = true;
        volume0 = 1.0;
        while( sub++ < depth && inputParts.Size() > 0)
        {
            if (callBack)
            {
                std::ostringstream msg;
                msg << "\t Subdivision level " << sub << std::endl;
                (*callBack)(msg.str().c_str());
            }
            const size_t nInputParts = inputParts.Size();
            SArray< TetrahedronSet * > temp;
            temp.Allocate(nInputParts);
            for(size_t p = 0; p < nInputParts; ++p)
            {
                TetrahedronSet * tset = inputParts[p];
                Real volume     = tset->ComputeVolume();
                tset->ComputeBB();
                tset->ComputePrincipalAxes();
                if (pca)
                {
                    tset->AlignToPrincipalAxes();
                }

                Mesh ch;
                tset->ComputeConvexHull(ch, 1);
                Real volumeCH = fabs(ch.ComputeVolume());

                if (firstIteration)
                {
                    volume0        = volumeCH;
                    firstIteration = false;
                }

                Real error     = 1.01 * tset->ComputeMaxVolumeError() / volume0;
                Real concavity = fabs(volumeCH - volume) / volume0;

                if (callBack)
                {
                    std::ostringstream msg;
                    msg << "\t -> Part[" << p 
                        << "] C = " << concavity 
                        << ", E = " << error 
                        << ", VS = " << tset->GetNumOnSurfTetrahedra() 
                        << ", VC = " << tset->GetNumOnClipPlaneTetrahedra()
                        << ", VI = " << tset->GetNumInsideSurfTetrahedra()
                        << std::endl;
                    (*callBack)(msg.str().c_str());
                }


                if (concavity > concavityThreshold && concavity > error)
                {
                    Vec3<Real> preferredCuttingDirection;
                    Real w = ComputePreferredCuttingDirection(*tset, preferredCuttingDirection);
                    planes.Resize(0);
                    ComputeAxesAlignedClippingPlanes(*tset, planeDownsampling, planes);

                    if (callBack)
                    {
                        std::ostringstream msg;
                        msg << "\t\t [Regular sampling] Nunber of clipping planes " << planes.Size() << std::endl;
                        (*callBack)(msg.str().c_str());
                    }

                    Plane bestPlane;
                    TetrahedronSet  * bestLeft  = new TetrahedronSet;
                    TetrahedronSet  * bestRight = new TetrahedronSet;
                    temp.PushBack(bestLeft);
                    temp.PushBack(bestRight);

                    Real minConcavity = std::numeric_limits<double>::max();
                    Real minBalance   = std::numeric_limits<double>::max();
                    Real minSymmetry  = std::numeric_limits<double>::max();

                    ComputeBestClippingPlane(*tset, 
                                             volume0, 
                                             volume, 
                                             planes, 
                                             preferredCuttingDirection, 
                                             w,
                                             concavity * alpha,
                                             concavity * beta,
                                             convexhullDownsampling,
                                             bestPlane, 
                                             minConcavity,
                                             minBalance,
                                             minSymmetry,
                                             callBack);

                    if (planeDownsampling > 1 || convexhullDownsampling > 1)
                    {
                        planesRef.Resize(0);
                        RefineAxesAlignedClippingPlanes(*tset, bestPlane, planeDownsampling, planesRef);
                        if (callBack)
                        {
                            std::ostringstream msg;
                            msg << "\t\t [Refining] Nunber of clipping planes " << planesRef.Size() << std::endl;
                            (*callBack)(msg.str().c_str());
                        }
                        minConcavity = std::numeric_limits<double>::max();
                        minBalance   = std::numeric_limits<double>::max();
                        minSymmetry  = std::numeric_limits<double>::max();
                        ComputeBestClippingPlane(*tset, 
                                                 volume0, 
                                                 volume, 
                                                 planesRef, 
                                                 preferredCuttingDirection, 
                                                 w,
                                                 concavity * alpha,
                                                 concavity * beta,
                                                 1,                 // convexhullDownsampling = 1
                                                 bestPlane, 
                                                 minConcavity,
                                                 minBalance,
                                                 minSymmetry,
                                                 callBack);
                    }
                    tset->Clip(bestPlane, bestRight, bestLeft);
                    delete tset;
                    if (pca)
                    {
                        bestRight->RevertAlignToPrincipalAxes();
                        bestLeft->RevertAlignToPrincipalAxes();
                    }
                }
                else
                {
                    if (pca)
                    {
                        tset->RevertAlignToPrincipalAxes();
                    }
                    parts.PushBack(tset);
                }
            }
            inputParts.Resize(0);
            const size_t nTempParts = temp.Size();
            for(size_t p = 0; p < nTempParts; ++p)
            {
                inputParts.PushBack(temp[p]);
            }
        }
        const size_t nInputParts = inputParts.Size();
        for(size_t p = 0; p < nInputParts; ++p)
        {
            parts.PushBack(inputParts[p]);
        }
        return true;
    }
    void AddPoints(const Mesh * const mesh, SArray< Vec3<Real> > & pts)
    {
        const long n = (long) mesh->GetNPoints();
        for(long i = 0; i < n; ++i)
        {
            pts.PushBack(mesh->GetPoint(i));
        }
    }
    void ComputeConvexHull(const Mesh * const     ch1, 
                           const Mesh * const     ch2,
                           SArray< Vec3<Real> > & pts,
                           Mesh * const           combinedCH)
    {
        pts.Resize(0);
        AddPoints(ch1, pts);
        AddPoints(ch2, pts);

        btConvexHullComputer ch;
        ch.compute((Real *)pts.Data(), 3 * sizeof(Real), (int)pts.Size(), -1.0, -1.0);
        combinedCH->ResizePoints   (0);
        combinedCH->ResizeTriangles(0);
        for (int v = 0; v < ch.vertices.size(); v++)
        {
            combinedCH->AddPoint(Vec3<Real>(ch.vertices[v].getX(), ch.vertices[v].getY(), ch.vertices[v].getZ()));
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
                combinedCH->AddTriangle(Vec3<long>(a, b, c));
                edge = edge->getNextEdgeOfFace();
                b = c;
                c = edge->getTargetVertex();
            }
        }
    }
    bool MergeConvexHulls(      Mesh **          convexHulls, 
                                size_t         & nConvexHulls, 
                          const Real             volume0,
                          const Real             gamma,
                          const CallBackFunction callBack)
    {
        if (nConvexHulls <= 1)
        {
            return true;
        }
        const size_t nConvexHulls0 = nConvexHulls;
        const Real threshold = gamma * volume0;
        SArray< Vec3<Real> > pts;
        Mesh combinedCH;
        bool iterate = true;

        while (iterate)
        {
            size_t bestp1;
            size_t bestp2;
            Real bestCost = std::numeric_limits<Real>::max();;
            for (size_t p1 = 0; p1 < nConvexHulls0 - 1; ++p1)
            {
                if (convexHulls[p1])
                {
                    Real volume1 = convexHulls[p1]->ComputeVolume();
                    size_t p2 = p1 + 1;
                    while (p2 < nConvexHulls0)
                    {
                        if (p1 != p2 && convexHulls[p2])
                        {
                            Real volume2 = convexHulls[p2]->ComputeVolume();

                            ComputeConvexHull(convexHulls[p1], convexHulls[p2], pts, &combinedCH);

                            Real combinedVolume = combinedCH.ComputeVolume();
                            Real cost           = combinedVolume - volume1 - volume2;
                            if (cost < bestCost)
                            {
                                bestCost       = cost;
                                bestp1         = p1;
                                bestp2         = p2;
                                if (callBack)
                                {
                                    std::ostringstream msg;
                                    msg << "\t\t Cost (" << p1 << ", " << p2 << ") " << cost / volume0 << std::endl;
                                    (*callBack)(msg.str().c_str());
                                }
                            }
                        }
                        ++p2;
                    }
                }
            }

            if (bestCost < threshold)
            {
                if (callBack)
                {
                    std::ostringstream msg;
                    msg << "\t\t Merging (" << bestp1 << ", " << bestp2 << ") " << bestCost / volume0 << std::endl << std::endl;
                    (*callBack)(msg.str().c_str());
                }
                Mesh * cch = new Mesh;
                ComputeConvexHull(convexHulls[bestp1], convexHulls[bestp2], pts, cch);
                delete convexHulls[bestp1];
                delete convexHulls[bestp2];
                convexHulls[bestp2] = 0;
                convexHulls[bestp1] = cch;
                iterate             = true;
                --nConvexHulls;
            }
            else
            {
                iterate             = false;
            }
        }
        return true;
    }
}

