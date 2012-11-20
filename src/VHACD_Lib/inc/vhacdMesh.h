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
#include <VHACDVersion.h>
#include <VHACDSArray.h>
#include <VHACDVector.h>
#include <VHACDMaterial.h>

#include <vector>
#include <queue>
#include <set>
namespace VHACD
{
	typedef std::vector< Vec2<Real> > Vector2dVector;
	
	class Mesh;

	enum Axis
	{
		X_AXIS,
		Y_AXIS,
		Z_AXIS,
		N_AXIS
	};
    class Mesh;

	unsigned long long											EdgeID(long a, long b);

	//! Contour data structure
    class Polyline2D
	{            
    public:	
		void													AddPoint(const Vec2<Real> & pt)					{ m_points.push_back(pt);};
		void													SetPoint(long index, const Vec2<Real> & pt)		{ m_points[index] = pt;};
		const Vec2<Real> &										GetPoint(long index) const						{ return m_points[index];};
		Vec2<Real> &											GetPoint(long index)							{ return m_points[index];};
		size_t													GetNPoints() const								{ return m_points.size();};
		long													InsertPoint(const Vec2<Real> & pt);

		void													AddEdge(const Vec2<long> & edge)				{ m_edges.push_back(edge);}
		void													SetEdge(long index, const Vec2<long> & edge)    { m_edges[index] = edge;};
		const Vec2<long> &										GetEdge(long index) const  						{ return m_edges[index];};
		Vec2<long> &											GetEdge(long index)		  					    { return m_edges[index];};
		size_t													GetNEdges() const								{ return m_edges.size();};
		void													InsertEdge(const Vec2<long> & edge);

		void													ClearPoints()									{ m_points.clear();}
		void													ClearEdges()									{ m_edges.clear();}
		void													Clear()											{ ClearPoints(); ClearEdges();}
		void													ResizePoints(size_t nPts)						{ m_points.resize(nPts);}
		void													ResizeEdges(size_t nTri)						{ m_edges.resize(nTri);}
		size_t													ComputeConnectedComponents(long * v2CC) const;
		void													ExtractConnectedComponent(size_t nCC, 
																						  long const * const v2CC, 
																						  long * mapping, 
																						  Polyline2D & CC) const;
        void                                                    Triangulate(Mesh & mesh) const ;
		void													ComputeBB();
		const Vec2<Real> &										GetCenter() const								{ return m_center;}
		const Vec2<Real> &										GetMinBB() const								{ return m_minBB;}
		const Vec2<Real> &										GetMaxBB() const								{ return m_maxBB;}

		//! Constructor.
																Polyline2D();
		//! Destructor.
																~Polyline2D(void);

		private:
			std::vector< Vec2<Real> >							m_points;
			std::vector< Vec2<long> >							m_edges;
			Vec2<Real>											m_minBB;
			Vec2<Real>											m_maxBB;
			Vec2<Real>											m_center;

			friend class										Mesh;
	};


	//! Triangular mesh data structure
    class Mesh
	{            
    public:	
		void													AddNormal(const Vec3<Real> & pt)				{ m_normals.push_back(pt);};
		void													SetNormal(long index, const Vec3<Real> & pt)	{ m_normals[index] = pt; };
		const Vec3<Real> &										GetNormal(long index) const						{ return m_normals[index]; };
		Vec3<Real> &											GetNormal(long index)							{ return m_normals[index]; };
		size_t													GetNNormals() const								{ return m_normals.size();};
		const Real *											GetNormals() const								{ return (Real *) (&(m_normals[0]));} // ugly

		void													AddPoint(const Vec3<Real> & pt)					{ m_points.push_back(pt);};
		void													SetPoint(long index, const Vec3<Real> & pt)		{ m_points[index] = pt; };
		const Vec3<Real> &										GetPoint(long index) const						{ return m_points[index]; };
		Vec3<Real> &											GetPoint(long index)							{ return m_points[index]; };
		size_t													GetNPoints() const								{ return m_points.size();};
		const Real *											GetPoints() const								{ return (Real *) (&(m_points[0]));} // ugly

		void													AddTriangle(const Vec3<long> & tri)			    { m_triangles.push_back(tri);};
		void													SetTriangle(long index, const Vec3<long> & tri) { m_triangles[index] = tri; };
		const Vec3<long> &										GetTriangle(long index) const  				    { return m_triangles[index];};
		Vec3<long> &											GetTriangle(long index)		  				    { return m_triangles[index];};
		size_t													GetNTriangles() const							{ return m_triangles.size();};
		const long *											GetTriangles() const							{ return (long *) (&(m_triangles[0]));} // ugly

		const Vec3<Real> &										GetCenter() const								{ return m_center;}
		const Vec3<Real> &										GetMinBB() const								{ return m_minBB;}
		const Vec3<Real> &										GetMaxBB() const								{ return m_maxBB;}

		void													ClearNormals()									{ m_normals.clear();}
		void													ClearPoints()									{ m_points.clear();}
		void													ClearTriangles()								{ m_triangles.clear();}
		void													Clear()											{ ClearPoints(); ClearTriangles();}
		void													ResizeNormals(size_t nNormals)					{ m_normals.resize(nNormals);}
		void													ResizePoints(size_t nPts)						{ m_points.resize(nPts);}
		void													ResizeTriangles(size_t nTri)					{ m_triangles.resize(nTri);}
		void													Clip(const Real a, const Real b, const Real c, const Real d,  // ax + by + cz + d = 0
																	 Mesh * positivePart, Mesh * negativePart, const bool fillClippingHoles) const;
		void													ComputeBB();
		Axis													GetMaxDim(double & d) const;
		void													Center(bool bbCenter, Axis axis);
		void													Normalize();
		bool													LoadOFF(const std::string & fileName, bool invert);
		bool													SaveVRML2(const std::string & fileName, const Vec3<Real> * colors= 0) const ;
		bool													SaveVRML2(std::ofstream & fout, const Material & material, const Vec3<Real> * colors = 0) const;
		bool													SaveOFF(const std::string & fileName) const;
		size_t													ComputeConnectedComponents(long * v2CC) const;
		void													ExtractConnectedComponent(size_t nCC, 
																						  long const * const v2CC, 
																						  long * mapping, 
																						  Mesh & CC) const;
		void													CleanDuplicatedVectices();
		Real													ComputeVolume() const;
		void													ComputeConvexHull(Mesh & meshCH) const;
        long                                                    FindPoint(const Vec3<Real> & pt) const;
        long                                                    InsertPoint(const Vec3<Real> & pt);
		void													Subdivide(int n);
		void													ComputeNormals();
		

		//! Constructor.
																Mesh();
		//! Destructor.
																~Mesh(void);

		private:
			std::vector< Vec3<Real> >							m_normals;
			std::vector< Vec3<Real> >							m_points;
			std::vector< Vec3<long> >							m_triangles;
			Vec3<Real>											m_minBB;
			Vec3<Real>											m_maxBB;
			Vec3<Real>											m_center;
			Real												m_diag;
	};
	bool Triangulate(const Vector2dVector &contour,Vector2dVector &result);
}
#endif