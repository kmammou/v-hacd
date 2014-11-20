/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#ifndef VHACD_MESH_H
#define VHACD_MESH_H
#include <vhacdVersion.h>
#include <vhacdSArray.h>
#include <vhacdVector.h>

namespace VHACD
{
    struct Material
    {

        Vec3<double>                                            m_diffuseColor;
        double                                                  m_ambientIntensity;
        Vec3<double>                                            m_specularColor;
        Vec3<double>                                            m_emissiveColor;
        double                                                  m_shininess;
        double                                                  m_transparency;
                                                                Material(void)
                                                                {
                                                                    m_diffuseColor.X()  = 0.5;
                                                                    m_diffuseColor.Y()  = 0.5;
                                                                    m_diffuseColor.Z()  = 0.5;
                                                                    m_specularColor.X() = 0.5;
                                                                    m_specularColor.Y() = 0.5;
                                                                    m_specularColor.Z() = 0.5;
                                                                    m_ambientIntensity  = 0.4;
                                                                    m_emissiveColor.X() = 0.0;
                                                                    m_emissiveColor.Y() = 0.0;
                                                                    m_emissiveColor.Z() = 0.0;
                                                                    m_shininess         = 0.4;
                                                                    m_transparency      = 0.0;
                                                                };
    };

    //! Triangular mesh data structure
    class Mesh
    {            
    public:    
        void                                                    AddPoint(const Vec3<Real> & pt)                 { m_points.PushBack(pt);};
        void                                                    SetPoint(long index, const Vec3<Real> & pt)     { m_points[index] = pt; };
        const Vec3<Real> &                                      GetPoint(long index) const                      { return m_points[index]; };
        Vec3<Real> &                                            GetPoint(long index)                            { return m_points[index]; };
        size_t                                                  GetNPoints() const                              { return m_points.Size();};
        const Real *                                            GetPoints() const                               { return (Real *) m_points.Data();} // ugly

        void                                                    AddTriangle(const Vec3<long> & tri)             { m_triangles.PushBack(tri);};
        void                                                    SetTriangle(long index, const Vec3<long> & tri) { m_triangles[index] = tri; };
        const Vec3<long> &                                      GetTriangle(long index) const                   { return m_triangles[index];};
        Vec3<long> &                                            GetTriangle(long index)                         { return m_triangles[index];};
        size_t                                                  GetNTriangles() const                           { return m_triangles.Size();};
        const long *                                            GetTriangles() const                            { return (long *) m_triangles.Data();} // ugly

        const Vec3<Real> &                                      GetCenter() const                               { return m_center;}
        const Vec3<Real> &                                      GetMinBB() const                                { return m_minBB;}
        const Vec3<Real> &                                      GetMaxBB() const                                { return m_maxBB;}

        void                                                    ClearPoints()                                   { m_points.Clear();}
        void                                                    ClearTriangles()                                { m_triangles.Clear();}
        void                                                    Clear()                                         { ClearPoints(); ClearTriangles();}
        void                                                    ResizePoints(size_t nPts)                       { m_points.Resize(nPts);}
        void                                                    ResizeTriangles(size_t nTri)                    { m_triangles.Resize(nTri);}
        bool                                                    LoadOFF(const std::string & fileName, bool invert);
        bool                                                    SaveVRML2(const std::string & fileName, const Vec3<Real> * colors= 0) const ;
        bool                                                    SaveVRML2(std::ofstream & fout, const Material & material, const Vec3<Real> * colors = 0) const;
        bool                                                    SaveOFF(const std::string & fileName) const;
        Real                                                    ComputeVolume() const;

        //! Constructor.
                                                                Mesh();
        //! Destructor.
                                                                ~Mesh(void);

        private:
            SArray< Vec3<Real>, 256 >                           m_points;
            SArray< Vec3<long>, 256 >                           m_triangles;
            Vec3<Real>                                          m_minBB;
            Vec3<Real>                                          m_maxBB;
            Vec3<Real>                                          m_center;
            Real                                                m_diag;
    };
}
#endif