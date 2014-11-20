/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#ifndef VHACD_VOLUME_H
#define VHACD_VOLUME_H
#include <VHACDVersion.h>
#include <VHACDVector.h>
#include <VHACDMesh.h>
#include <assert.h>

#ifdef LINUX
    #define int64 long 
#elif WIN32
    #define int64 long long
#endif


namespace VHACD
{

    enum VOXEL_VALUE
    {
        VOXEL_UNDEFINED                 = 0,
        VOXEL_OUTSIDE_SURFACE           = 1,
        VOXEL_INSIDE_SURFACE            = 2,
        VOXEL_ON_SURFACE                = 3,
        VOXEL_ON_CLIP_PLANE             = 4
    };

    enum AXIS
    {
        AXIS_X = 0,
        AXIS_Y = 1,
        AXIS_Z = 2
    };

    struct Voxel
    {
    public:
        short                                       m_coord[3];
        unsigned char                               m_data;
    };

    struct Plane
    {
        Real                                        m_a;
        Real                                        m_b;
        Real                                        m_c;
        Real                                        m_d;
        AXIS                                        m_axis;
        short                                       m_index;
    };

    //!
    class VoxelSet
    {            
        friend class Volume;
    public:
        //! Destructor.
                                                    ~VoxelSet(void);
        //! Constructor.
                                                    VoxelSet();

        size_t                                      GetNVoxels() const { return m_voxels.Size();}
        Real                                        ComputeVolume() const { return m_unitVolume * m_voxels.Size();}
        Real                                        ComputeMaxVolumeError() const { return 0.5 * m_unitVolume * m_numVoxelsOnSurface;}
        void                                        ComputeConvexHull(Mesh & meshCH, const size_t sampling) const;
        const Vec3<short> &                         GetMinBBVoxels()  const { return m_minBBVoxels;}
        const Vec3<short> &                         GetMaxBBVoxels()  const { return m_maxBBVoxels;}
        Vec3<Real>                                  GetPoint(Vec3<short> voxel) const
                                                    {
                                                        return Vec3<Real> (voxel[0] * m_scale +  m_minBB[0], 
                                                                           voxel[1] * m_scale +  m_minBB[1], 
                                                                           voxel[2] * m_scale +  m_minBB[2]);
                                                    }
        Vec3<Real>                                  GetPoint(const Voxel & voxel) const
                                                    {
                                                        return Vec3<Real> (voxel.m_coord[0] * m_scale  +  m_minBB[0], 
                                                                           voxel.m_coord[1] * m_scale  +  m_minBB[1], 
                                                                           voxel.m_coord[2] * m_scale  +  m_minBB[2]);
                                                    }
        Vec3<Real>                                  GetPoint(Vec3<Real> voxel) const
                                                    {
                                                        return Vec3<Real> (voxel[0] * m_scale +  m_minBB[0], 
                                                                           voxel[1] * m_scale +  m_minBB[1], 
                                                                           voxel[2] * m_scale +  m_minBB[2]);
                                                    }
        const size_t                                GetNumOnSurfVoxels()      const { return m_numVoxelsOnSurface    ;}
        const size_t                                GetNumInsideSurfVoxels()  const { return m_numVoxelsInsideSurface;}
        const size_t                                GetNumOnClipPlaneVoxels() const { return m_numVoxelsOnClipPlane  ;}
        const Real                                  GetEigenValue(AXIS axis)      const { return m_D[axis][axis];}

        void                                        Clip(const Plane & plane, VoxelSet * const positivePart, VoxelSet * const negativePart) const;
        void                                        ComputeBB();
        void                                        Convert(Mesh & mesh, const VOXEL_VALUE value) const;
        void                                        ComputePrincipalAxes();

    private:
        size_t                                      m_numVoxelsOnSurface    ;
        size_t                                      m_numVoxelsInsideSurface;
        size_t                                      m_numVoxelsOnClipPlane;
        Vec3<Real>                                  m_minBB;
        Real                                        m_scale;
        SArray< Voxel, 8 >                          m_voxels;
        Real                                        m_unitVolume;
        Vec3<Real>                                  m_minBBPts;
        Vec3<Real>                                  m_maxBBPts;
        Vec3<short>                                 m_minBBVoxels;
        Vec3<short>                                 m_maxBBVoxels;
        Vec3<short>                                 m_barycenter;
        Real                                        m_Q[3][3];
        Real                                        m_D[3][3];
        Vec3<Real>                                  m_barycenterPCA;
    };


    struct Tetrahedron
    {
    public:
        Vec3<Real>                                  m_pts[4];
        unsigned char                               m_data;
    };

    //!
    class TetrahedronSet
    {            
        friend class Volume;
    public:
        //! Destructor.
                                                    ~TetrahedronSet(void);
        //! Constructor.
                                                    TetrahedronSet();

        size_t                                      GetNTetrahedra() const { return m_tetrahedra.Size();}
        Real                                        ComputeVolume() const;
        Real                                        ComputeMaxVolumeError() const;
        void                                        ComputeConvexHull(Mesh & meshCH, const size_t sampling) const;
        const size_t                                GetNumOnSurfTetrahedra()      const { return m_numTetrahedraOnSurface    ;}
        const size_t                                GetNumInsideSurfTetrahedra()  const { return m_numTetrahedraInsideSurface;}
        const size_t                                GetNumOnClipPlaneTetrahedra() const { return m_numTetrahedraOnClipPlane  ;}
        const Vec3<Real> &                          GetMinBB()                    const { return m_minBB;}
        const Vec3<Real> &                          GetMaxBB()                    const { return m_maxBB;}
        const Vec3<Real> &                          GetBarycenter()               const { return m_barycenter;}
        const Real                                  GetEigenValue(AXIS axis)      const { return m_D[axis][axis];}
        const Real                                  GetSacle()                    const { return m_scale;}

        void                                        ComputePrincipalAxes();
        void                                        AlignToPrincipalAxes();
        void                                        RevertAlignToPrincipalAxes();
        void                                        Clip(const Plane & plane, TetrahedronSet * const positivePart, TetrahedronSet * const negativePart) const;
        void                                        ComputeBB();
        void                                        Convert(Mesh & mesh, const VOXEL_VALUE value) const;
        inline bool                                 Add(Tetrahedron & tetrahedron);

        static const Real EPS;

    private:


        void                                        AddClippedTetrahedra(const Vec3<Real> (&pts) [10], const int nPts);

        size_t                                      m_numTetrahedraOnSurface    ;
        size_t                                      m_numTetrahedraInsideSurface;
        size_t                                      m_numTetrahedraOnClipPlane;
        Real                                        m_scale;
        Vec3<Real>                                  m_minBB;
        Vec3<Real>                                  m_maxBB;
        Vec3<Real>                                  m_barycenter;
        SArray< Tetrahedron, 8 >                    m_tetrahedra;
        Real                                        m_Q[3][3];
        Real                                        m_D[3][3];
    };


    //! 
    class Volume
    {            
    public:

        //! Destructor.
                                                    ~Volume(void);

        //! Constructor.
                                                    Volume();

        //! Voxelize
        void                                        Voxelize(const size_t nPoints, 
                                                             const size_t nTriangles,
                                                             const Vec3<Real> * points, 
                                                             const Vec3<long> * const triangles,
                                                             const size_t dim);
        unsigned char &                             GetVoxel(const size_t i, const size_t j, const size_t k)
                                                    {
                                                        assert( i < m_dim[0] || i >= 0);
                                                        assert( j < m_dim[0] || j >= 0);
                                                        assert( k < m_dim[0] || k >= 0);
                                                        return m_data[i + j * m_dim[0] + k * m_dim[0] * m_dim[1]];
                                                    }
        const unsigned char &                       GetVoxel(const size_t i, const size_t j, const size_t k) const
                                                    {
                                                        assert( i < m_dim[0] || i >= 0);
                                                        assert( j < m_dim[0] || j >= 0);
                                                        assert( k < m_dim[0] || k >= 0);
                                                        return m_data[i + j * m_dim[0] + k * m_dim[0] * m_dim[1]];
                                                    }
        const size_t                                GetNumOnSurfVoxels()     const { return m_numVoxelsOnSurface;}
        const size_t                                GetNumInsideSurfVoxels() const { return m_numVoxelsInsideSurface;}
        void                                        Convert(Mesh & mesh, const VOXEL_VALUE value) const;
        void                                        Convert(VoxelSet  & vset) const;
        void                                        Convert(TetrahedronSet & tset) const;
        void                                        AlignToPrincipalAxes(Real (&rot)[3][3]) const;

    private:
        void                                        FillOutsideSurface(const size_t i0,
                                                                       const size_t j0,
                                                                       const size_t k0,
                                                                       const size_t i1,
                                                                       const size_t j1,
                                                                       const size_t k1);
        void                                        FillInsideSurface();
        void                                        ComputeBB(const size_t nPoints,
                                                              const Vec3<Real> * points);
        void                                        Allocate();
        void                                        Free();

        Vec3<Real>                                  m_minBB;
        Vec3<Real>                                  m_maxBB;
        Real                                        m_scale;
        size_t                                      m_dim[3];                   //>! dim
        size_t                                      m_numVoxelsOnSurface     ;
        size_t                                      m_numVoxelsInsideSurface ;
        size_t                                      m_numVoxelsOutsideSurface;
        unsigned char *                             m_data;
    };
}
#endif // VHACD_VOLUME_H

