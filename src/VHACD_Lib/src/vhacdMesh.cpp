/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <iosfwd>
#include "btConvexHullComputer.h"
#include "vhacdMesh.h"

namespace VHACD
{
    Mesh::Mesh()
    {
        m_diag = 1.0;
    }
    Mesh::~Mesh()
    {
    }
    double Mesh::ComputeVolume() const
    {
        const size_t nV = GetNPoints();
        const size_t nT = GetNTriangles();
        if (nV == 0 || nT == 0)
        {
            return 0.0;
        }

        Vec3<double> bary(0.0, 0.0, 0.0);
        for (size_t v = 0; v < nV; v++)
        {
            bary +=  GetPoint(v);
        }
        bary /= static_cast<double>(nV);

        Vec3<double> ver0, ver1, ver2;
        double totalVolume = 0.0;
        for(int t = 0; t < nT; t++)
        {
            const Vec3<int> & tri = GetTriangle(t);
            ver0 = GetPoint(tri[0]);
            ver1 = GetPoint(tri[1]);
            ver2 = GetPoint(tri[2]);
            totalVolume += ComputeVolume4(ver0, ver1, ver2, bary);
        }
        return totalVolume/6.0;
    }

    void Mesh::ComputeConvexHull(const double * const pts, 
                                 const size_t         nPts)
    {
        ResizePoints(0);
        ResizeTriangles(0);
        btConvexHullComputer ch;
        ch.compute(pts, 3 * sizeof(double), (int) nPts, -1.0, -1.0);
        for (int v = 0; v < ch.vertices.size(); v++)
        {
            AddPoint(Vec3<double>(ch.vertices[v].getX(), ch.vertices[v].getY(), ch.vertices[v].getZ()));
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
                AddTriangle(Vec3<int>(a, b, c));
                edge = edge->getNextEdgeOfFace();
                b = c;
                c = edge->getTargetVertex();
            }
        }
    }
    void Mesh::Clip(const Plane            & plane, 
                    SArray< Vec3<double> > & positivePart, 
                    SArray< Vec3<double> > & negativePart) const
    {
        const size_t nV = GetNPoints();
        if (nV == 0)
        {
            return;
        }       
        double d;
        for (size_t v = 0; v < nV; v++)
        {
            const Vec3<double> & pt =  GetPoint(v);
            d     = plane.m_a * pt[0] + plane.m_b * pt[1] + plane.m_c * pt[2] + plane.m_d;
            if (d > 0.0)
            {
                positivePart.PushBack(pt);
            }
            else if (d < 0.0)
            {
                negativePart.PushBack(pt);
            }
            else
            {
                positivePart.PushBack(pt);
                negativePart.PushBack(pt);
            }
        }
    }
    bool Mesh::IsInside(const Vec3<double> & pt) const
    {
        const size_t nV = GetNPoints();
        const size_t nT = GetNTriangles();
        if (nV == 0 || nT == 0)
        {
            return false;
        }
        Vec3<double> ver0, ver1, ver2;
        double volume;
        for (int t = 0; t < nT; t++)
        {
            const Vec3<int> & tri = GetTriangle(t);
            ver0 = GetPoint(tri[0]);
            ver1 = GetPoint(tri[1]);
            ver2 = GetPoint(tri[2]);
            volume = ComputeVolume4(ver0, ver1, ver2, pt);
            if (volume < 0.0)
            {
                return false;
            }
        }
        return true;
    }
	int Mesh::GetSmallestSideForTracing(Vec3<int>* range_min, Vec3<int>* range_max, const VoxelBase& basis) const
	{
		Vec3<short> voxel_extent = basis.m_maxVoxel - basis.m_minVoxel;
		if(voxel_extent[0] >= voxel_extent[1] && voxel_extent[0] >= voxel_extent[2])
		{
			// axis x is the longest axis
			(*range_min)[0] = basis.m_minVoxel[1]; // y
			(*range_max)[0] = basis.m_maxVoxel[1]; 
			(*range_min)[1] = basis.m_minVoxel[2]; // z
			(*range_max)[1] = basis.m_maxVoxel[2]; 
			(*range_min)[2] = basis.m_minVoxel[0] - 1; // x
			(*range_max)[2] = basis.m_maxVoxel[0] + 1; 
			return 0;
		}
		else if(voxel_extent[1] >= voxel_extent[0] && voxel_extent[1] >= voxel_extent[2])
		{
			// axis y is the longest axis
			(*range_min)[0] = basis.m_minVoxel[0]; // x
			(*range_max)[0] = basis.m_maxVoxel[0]; 
			(*range_min)[1] = basis.m_minVoxel[2]; // z
			(*range_max)[1] = basis.m_maxVoxel[2]; 
			(*range_min)[2] = basis.m_minVoxel[1] - 1; // y
			(*range_max)[2] = basis.m_maxVoxel[1] + 1; 
			return 1; 
		}
		else 
		{
			// axis z is the longest axis
			(*range_min)[0] = basis.m_minVoxel[0]; // x
			(*range_max)[0] = basis.m_maxVoxel[0]; 
			(*range_min)[1] = basis.m_minVoxel[1]; // y
			(*range_max)[1] = basis.m_maxVoxel[1]; 
			(*range_min)[2] = basis.m_minVoxel[2] - 1; // z
			(*range_max)[2] = basis.m_maxVoxel[2] + 1; 
			return 2; // axis z
		}
	}
	void PrepareRay(Vec3<double>* from, Vec3<double>* to, int x, int y, int zmin, int zmax, int side, const VoxelBase& voxel)
	{
		Vec3<int> vfrom, vto;
		switch(side)
		{
		case 0: // y[x], z[y], x[zmin-zmax]
			vfrom = Vec3<int>(zmin, x, y);
			vto = Vec3<int>(zmax, x, y);
			break;
		case 1: // x[x], z[y], y[zmin-zmax]
			vfrom = Vec3<int>(x, zmin, y);
			vto = Vec3<int>(x, zmax, y);
			break;
		case 2: // x[x], y[y], z[zmin-zmax]
			vfrom = Vec3<int>(x, y, zmin);
			vto = Vec3<int>(x, y, zmax);
			break;
		}
		*from = voxel.Point(vfrom);
		*to = voxel.Point(vto);
	}
	const double EPSILON = 1e-7;
	// from Tomas Moller's "Fast Minimum Storage Ray-Triangle Intersection".
	bool IntersectTriangle(const Vec3<double>& orig, const Vec3<double>& dir, const Vec3<double>& v0, const Vec3<double>& v1, const Vec3<double>& v2, double* t)
	{
		// Find vectors for two edges sharing vert0
		Vec3<double> edge1 = v1 - v0;
		Vec3<double> edge2 = v2 - v0;

		// Begin calculating determinant - also used to calculate U parameter
		Vec3<double> pvec = dir ^ edge2;

		// If determinant is near zero, ray lies in plane of triangle
		double det = edge1 * pvec;

		// the non-culling branch
		if(det > -EPSILON && det < EPSILON)
			return false;
		double inv_det = 1.0 / det;

		// Calculate distance from vert0 to ray origin
		Vec3<double> tvec = orig - v0;

		// calculate U parameter and test bounds
		double u = tvec * pvec * inv_det;
		if(u < 0.0 || u > 1.0)
			return false;

		// prepare to test V parameter
		Vec3<double> qvec = tvec ^ edge1;

		// calculate V parameter and test bounds 
		double v = dir * qvec * inv_det;
		if(v < 0.0 || u + v > 1.0)
			return false;

		// calculate t, ray intersects triangle
		*t = edge2 * qvec * inv_det;
		return true;
	}
	bool Mesh::TraceRayToHull(Vec3<double>* hit0, Vec3<double>* hit1, const Vec3<double>& ray0, const Vec3<double>& ray1) const
	{
		size_t nT = GetNTriangles();
		double t0, t1;
		bool hit = false;
		for(int t = 0; t < nT; t++)
		{
			const Vec3<int> & tri = GetTriangle(t);
			Vec3<double> v0, v1, v2;
			v0 = GetPoint(tri[0]);
			v1 = GetPoint(tri[1]);
			v2 = GetPoint(tri[2]);
			double tt;
			if(IntersectTriangle(ray0, ray1 - ray0, v0, v1, v2, &tt))
			{
				if(!hit)
				{
					t0 = t1 = tt;
					hit = true;
				}
				else
				{
					if(t0 > tt) t0 = tt;
					if(t1 < tt) t1 = tt;
				}
			}
		}
		if(hit)
		{
			*hit0 = ray0 + (ray1 - ray0) * t0;
			*hit1 = ray0 + (ray1 - ray0) * t1;
		}
		return hit;
	}
	double DistancePlaneToCube(const Vec3<double>& plane_normal, double plane_c, const Vec3<double>& cube_center, double cube_extent)
	{
		double dist = cube_center * plane_normal - plane_c;
		dist -= abs(cube_extent * plane_normal[0]);
		dist -= abs(cube_extent * plane_normal[1]);
		dist -= abs(cube_extent * plane_normal[2]);
		return dist;
	}
	bool Mesh::CubeInsideHull(const Vec3<double>& cube_center, double cube_size) const
	{
		size_t nT = GetNTriangles();
		for(int t = 0; t < nT; t++)
		{
			const Vec3<int> & tri = GetTriangle(t);
			Vec3<double> v0, v1, v2;
			v0 = GetPoint(tri[0]);
			v1 = GetPoint(tri[1]);
			v2 = GetPoint(tri[2]);
			Vec3<double> plane_normal = (v2 - v0) ^ (v1 - v0); plane_normal.Normalize();
			double plane_c = v0 * plane_normal;
			double dist = DistancePlaneToCube(plane_normal, plane_c, cube_center, cube_size * 0.5);
			if(dist < -cube_size * 0.025) // box is out of plane
				return false;
		}
		return true;
	}
	int Mesh::GetVoxelLineInsideHull(const Vec3<double>& hit0, const Vec3<double>& hit1, int side, const VoxelBase& voxel, int* rz0, int* rz1) const
	{
		Vec3<int> vpos0 = voxel.Position(hit0);
		Vec3<int> vpos1 = voxel.Position(hit1);
		int z0 = vpos0[side];
		int z1 = vpos1[side];
		if(z0 > z1) // z0 must be <= z1
			std::swap(z0, z1);
		bool bad_z0 = true;
		bool bad_z1 = true;

		Vec3<int> vp = vpos0;
		Vec3<double> rpos;
		for( ;; )
		{
			if(bad_z0)
			{
				vp[side] = z0;
				rpos = voxel.Point(vp);
				if(CubeInsideHull(rpos, voxel.m_size))
					bad_z0 = false;
				else 
					++z0;
			}
			if(bad_z1)
			{
				vp[side] = z1;
				rpos = voxel.Point(vp);
				if(CubeInsideHull(rpos, voxel.m_size))
					bad_z1 = false;
				else 
					--z1;

			}
			if(!bad_z0 && !bad_z1)
			{
				*rz0 = z0;
				*rz1 = z1;
				return z1 - z0 + 1;
			}
			if(z0 > z1)
				return 0;
		}
		return 0;
	}
	size_t Mesh::ComputeVoxelsInsideHull(const VoxelBase& basis) const
	{
		if(!GetNPoints())
			return 0;

		//std::ofstream fout("tracer.txt", std::ios_base::app);
		Vec3<int> rangeMin, rangeMax;
		int side = GetSmallestSideForTracing(&rangeMin, &rangeMax, basis);

		size_t voxelsInside = 0;
		for(int x = rangeMin[0]; x <= rangeMax[0]; ++x)
		{
			for(int y = rangeMin[1]; y <= rangeMax[1]; ++y)
			{
				Vec3<double> from, to;
				//fout << "x " << x << " y " << y;
				PrepareRay(&from, &to, x, y, rangeMin[2], rangeMax[2], side, basis);

				Vec3<double> hit0, hit1;
				if(TraceRayToHull(&hit0, &hit1, from, to))
				{
					// ray hit
					int z0, z1;
					int count = GetVoxelLineInsideHull(hit0, hit1, side, basis, &z0, &z1);
					//if(count > 0)
					//	fout << " z0 " << z0 << " z1 " << z1 << " = " << count;
					voxelsInside += count;
				}
				//fout << std::endl;
			}
		}
		//fout << "voxels in hull " << ComputeVolume() / Cube(voxelSize) << " traced " << voxelsInside << std::endl;
		return voxelsInside;
	}
	double	Mesh::ComputeVoxelizationError(const VoxelBase& basis) const
	{
		if(!GetNPoints())
			return 0.0;

		double volumeUnit = Cube(basis.m_size);
		double volumeVoxel = ComputeVoxelsInsideHull(basis) * volumeUnit;
		double volumeHull = ComputeVolume();
		double volumeError = volumeHull - volumeVoxel;
		double volumeErrorThreshold = 0.25 * volumeUnit;
		return std::max(volumeError, volumeErrorThreshold);
	}

#ifdef VHACD_DEBUG_MESH
    bool Mesh::SaveVRML2(const std::string & fileName) const
    {
        std::ofstream fout(fileName.c_str());
        if (fout.is_open())
        {
            const Material material;

            if (SaveVRML2(fout, material))
            {
                fout.close();
                return true;
            }
            return false;
        }
        return false;
    }
    bool Mesh::SaveVRML2(std::ofstream & fout, const Material & material) const
    {
        if (fout.is_open())
        {
            fout.setf(std::ios::fixed, std::ios::floatfield);
            fout.setf(std::ios::showpoint);
            fout.precision(6);
            size_t nV = m_points.Size();
            size_t nT = m_triangles.Size();
            fout << "#VRML V2.0 utf8" << std::endl;
            fout << "" << std::endl;
            fout << "# Vertices: " << nV << std::endl;
            fout << "# Triangles: " << nT << std::endl;
            fout << "" << std::endl;
            fout << "Group {" << std::endl;
            fout << "    children [" << std::endl;
            fout << "        Shape {" << std::endl;
            fout << "            appearance Appearance {" << std::endl;
            fout << "                material Material {" << std::endl;
            fout << "                    diffuseColor " << material.m_diffuseColor[0] << " "
                << material.m_diffuseColor[1] << " "
                << material.m_diffuseColor[2] << std::endl;
            fout << "                    ambientIntensity " << material.m_ambientIntensity << std::endl;
            fout << "                    specularColor " << material.m_specularColor[0] << " "
                << material.m_specularColor[1] << " "
                << material.m_specularColor[2] << std::endl;
            fout << "                    emissiveColor " << material.m_emissiveColor[0] << " "
                << material.m_emissiveColor[1] << " "
                << material.m_emissiveColor[2] << std::endl;
            fout << "                    shininess " << material.m_shininess << std::endl;
            fout << "                    transparency " << material.m_transparency << std::endl;
            fout << "                }" << std::endl;
            fout << "            }" << std::endl;
            fout << "            geometry IndexedFaceSet {" << std::endl;
            fout << "                ccw TRUE" << std::endl;
            fout << "                solid TRUE" << std::endl;
            fout << "                convex TRUE" << std::endl;
            if (nV > 0)
            {
                fout << "                coord DEF co Coordinate {" << std::endl;
                fout << "                    point [" << std::endl;
                for (size_t v = 0; v < nV; v++)
                {
                    fout << "                        " << m_points[v][0] << " "
                        << m_points[v][1] << " "
                        << m_points[v][2] << "," << std::endl;
                }
                fout << "                    ]" << std::endl;
                fout << "                }" << std::endl;
            }
            if (nT > 0)
            {
                fout << "                coordIndex [ " << std::endl;
                for (size_t f = 0; f < nT; f++)
                {
                    fout << "                        " << m_triangles[f][0] << ", "
                        << m_triangles[f][1] << ", "
                        << m_triangles[f][2] << ", -1," << std::endl;
                }
                fout << "                ]" << std::endl;
            }
            fout << "            }" << std::endl;
            fout << "        }" << std::endl;
            fout << "    ]" << std::endl;
            fout << "}" << std::endl;
            return true;
        }
        return false;
    }
    bool Mesh::SaveOFF(const std::string & fileName) const
    {
        std::ofstream fout(fileName.c_str());
        if (fout.is_open())
        {
            size_t nV = m_points.Size();
            size_t nT = m_triangles.Size();
            fout << "OFF" << std::endl;
            fout << nV << " " << nT << " " << 0 << std::endl;
            for (size_t v = 0; v < nV; v++)
            {
                fout << m_points[v][0] << " "
                    << m_points[v][1] << " "
                    << m_points[v][2] << std::endl;
            }
            for (size_t f = 0; f < nT; f++)
            {
                fout << "3 " << m_triangles[f][0] << " "
                    << m_triangles[f][1] << " "
                    << m_triangles[f][2] << std::endl;
            }
            fout.close();
            return true;
        }
        return false;
    }

    bool Mesh::LoadOFF(const std::string & fileName, bool invert)
    {
        FILE * fid = fopen(fileName.c_str(), "r");
        if (fid)
        {
            const std::string strOFF("OFF");
            char temp[1024];
            fscanf(fid, "%s", temp);
            if (std::string(temp) != strOFF)
            {
                fclose(fid);
                return false;
            }
            else
            {
                int nv = 0;
                int nf = 0;
                int ne = 0;
                fscanf(fid, "%i", &nv);
                fscanf(fid, "%i", &nf);
                fscanf(fid, "%i", &ne);
                m_points.Resize(nv);
                m_triangles.Resize(nf);
                Vec3<double> coord;
                float x, y, z;
                for (int p = 0; p < nv; p++)
                {
                    fscanf(fid, "%f", &x);
                    fscanf(fid, "%f", &y);
                    fscanf(fid, "%f", &z);
                    m_points[p][0] = x;
                    m_points[p][1] = y;
                    m_points[p][2] = z;
                }
                int i, j, k, s;
                for (int t = 0; t < nf; ++t) {
                    fscanf(fid, "%i", &s);
                    if (s == 3)
                    {
                        fscanf(fid, "%i", &i);
                        fscanf(fid, "%i", &j);
                        fscanf(fid, "%i", &k);
                        m_triangles[t][0] = i;
                        if (invert)
                        {
                            m_triangles[t][1] = k;
                            m_triangles[t][2] = j;
                        }
                        else
                        {
                            m_triangles[t][1] = j;
                            m_triangles[t][2] = k;
                        }
                    }
                    else            // Fix me: support only triangular meshes
                    {
                        for (int h = 0; h < s; ++h) fscanf(fid, "%i", &s);
                    }
                }
                fclose(fid);
            }
        }
        else
        {
            return false;
        }
        return true;
    }
#endif // VHACD_DEBUG_MESH

}
