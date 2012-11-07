/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <VHACDMesh.h>

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <iosfwd>
#include <kdtree.h>
#include <triangle.h>
#include <btConvexHullComputer.h>


// #define DEBUG_VHACD

namespace VHACD
{    
	static const Real EPSILON=0.0000000001;
    Mesh::Mesh()
    {
		m_diag = 1.0;
    }
    Mesh::~Mesh()
    {
    }
	void Mesh::ComputeBB()
	{
		const size_t nPoints = GetNPoints();
		if (nPoints == 0) return;
		m_minBB = m_points[0];
		m_maxBB = m_points[0];
		Real x, y, z;
        for (size_t v = 1; v < nPoints ; v++) 
        {
	        x = m_points[v][0];
            y = m_points[v][1];
            z = m_points[v][2];
            if      ( x < m_minBB[0]) m_minBB[0] = x;
			else if ( x > m_maxBB[0]) m_maxBB[0] = x;
            if      ( y < m_minBB[1]) m_minBB[1] = y;
			else if ( y > m_maxBB[1]) m_maxBB[1] = y;
            if      ( z < m_minBB[2]) m_minBB[2] = z;
			else if ( z > m_maxBB[2]) m_maxBB[2] = z;
        }
		m_center  = 0.5 * m_minBB + 0.5 * m_maxBB;
	}
	bool Mesh::SaveVRML2(const std::string & fileName, const Vec3<Real> * colors) const
	{
		std::cout << "Saving " <<  fileName << std::endl;
		std::ofstream fout(fileName.c_str());
		if (fout.is_open()) 
		{
			const Material material;
	        
			if (SaveVRML2(fout, material, colors))
			{
				fout.close();
				return true;
			}
			return false;
		}
		return false;
	}

	bool Mesh::SaveVRML2(std::ofstream & fout, const Material & material, const Vec3<Real> * colors) const
	{		
		if (fout.is_open()) 
		{
			fout.setf(std::ios::fixed, std::ios::floatfield);
			fout.setf(std::ios::showpoint);
			fout.precision(6);
			size_t nV = m_points.size();
			size_t nT = m_triangles.size();            
			fout <<"#VRML V2.0 utf8" << std::endl;	    	
			fout <<"" << std::endl;
			fout <<"# Vertices: " << nV << std::endl;		
			fout <<"# Triangles: " << nT << std::endl;		
			fout <<"" << std::endl;
			fout <<"Group {" << std::endl;
			fout <<"	children [" << std::endl;
			fout <<"		Shape {" << std::endl;
			fout <<"			appearance Appearance {" << std::endl;
			fout <<"				material Material {" << std::endl;
			fout <<"					diffuseColor "      << material.m_diffuseColor[0]      << " " 
															<< material.m_diffuseColor[1]      << " "
															<< material.m_diffuseColor[2]      << std::endl;  
			fout <<"					ambientIntensity "  << material.m_ambientIntensity      << std::endl;
			fout <<"					specularColor "     << material.m_specularColor[0]     << " " 
															<< material.m_specularColor[1]     << " "
															<< material.m_specularColor[2]     << std::endl; 
			fout <<"					emissiveColor "     << material.m_emissiveColor[0]     << " " 
															<< material.m_emissiveColor[1]     << " "
															<< material.m_emissiveColor[2]     << std::endl; 
			fout <<"					shininess "         << material.m_shininess             << std::endl;
			fout <<"					transparency "      << material.m_transparency          << std::endl;
			fout <<"				}" << std::endl;
			fout <<"			}" << std::endl;
			fout <<"			geometry IndexedFaceSet {" << std::endl;
			fout <<"				ccw TRUE" << std::endl;
			fout <<"				solid TRUE" << std::endl;
			fout <<"				convex TRUE" << std::endl;
			if (colors && nT>0)
			{
				fout <<"				colorPerVertex FALSE" << std::endl;
				fout <<"				color Color {" << std::endl;
				fout <<"					color [" << std::endl;
				for(size_t c = 0; c < nT; c++)
				{
					fout <<"						" << colors[c][0] << " " 
													  << colors[c][1] << " " 
													  << colors[c][2] << "," << std::endl;
				}
				fout <<"					]" << std::endl;
				fout <<"				}" << std::endl;
						}
			if (nV > 0) 
			{
				fout <<"				coord DEF co Coordinate {" << std::endl;
				fout <<"					point [" << std::endl;
				for(size_t v = 0; v < nV; v++)
				{
					fout <<"						" << m_points[v][0] << " " 
													  << m_points[v][1] << " " 
													  << m_points[v][2] << "," << std::endl;
				}
				fout <<"					]" << std::endl;
				fout <<"				}" << std::endl;
			}
			if (nT > 0) 
			{
				fout <<"				coordIndex [ " << std::endl;
				for(size_t f = 0; f < nT; f++)
				{
					fout <<"						" << m_triangles[f][0] << ", " 
													  << m_triangles[f][1] << ", "                                                  
													  << m_triangles[f][2] << ", -1," << std::endl;
				}
				fout <<"				]" << std::endl;
			}
			fout <<"			}" << std::endl;
			fout <<"		}" << std::endl;
			fout <<"	]" << std::endl;
			fout <<"}" << std::endl;	
			return true;
		}
		return false;
	}
	bool Mesh::SaveOFF(const std::string & fileName) const
	{
		std::cout << "Saving " <<  fileName << std::endl;
		std::ofstream fout(fileName.c_str());
		if (fout.is_open()) 
		{           
			size_t nV = m_points.size();
			size_t nT = m_triangles.size();  
			fout <<"OFF" << std::endl;	    	
			fout << nV << " " << nT << " " << 0<< std::endl;		
			for(size_t v = 0; v < nV; v++)
			{
				fout << m_points[v][0] << " " 
					 << m_points[v][1] << " " 
					 << m_points[v][2] << std::endl;
			}
			for(size_t f = 0; f < nT; f++)
			{
				fout <<"3 " << m_triangles[f][0] << " " 
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
				printf( "Loading error: format not recognized \n");
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
				m_points.reserve(nv);
				m_triangles.reserve(nf);
				Vec3<Real> coord;
				float x,y,z;
				for (long p = 0; p < nv ; p++) 
				{
					fscanf(fid, "%f", &x);
					fscanf(fid, "%f", &y);
					fscanf(fid, "%f", &z);
					m_points.push_back(Vec3<Real>(x, y, z));
				}        
				long i, j, k, s;
				for (long t = 0; t < nf ; ++t) {
					fscanf(fid, "%i", &s);
					if (s == 3)
					{
						fscanf(fid, "%i", &i);
						fscanf(fid, "%i", &j);
						fscanf(fid, "%i", &k);
						if (invert)
						{
							m_triangles.push_back(Vec3<long>(i, k, j));
						}
						else
						{
							m_triangles.push_back(Vec3<long>(i, j, k));
						}
					}
					else			// Fix me: support only triangular meshes
					{
						for(long h = 0; h < s; ++h) fscanf(fid, "%i", &s);
					}
				}
				fclose(fid);
			}
		}
		else 
		{
			printf( "Loading error: file not found \n");
			return false;
		}
		return true;
	}


	// source http://softsurfer.com/Archive/algorithm_0103/algorithm_0103.htm
	// cn_PnPoly(): crossing number test for a point in a polygon
	//      Input:   P = a point,
	//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
	//      Return:  0 = outside, 1 = inside
	// This code is patterned after [Franklin, 2000]
	int cnPointInPoly(const Vector2dVector & V, const Vec2< Real > & P)
	{
		int n = V.size() - 1;
		int cn = 0;    // the crossing number counter

		// loop through all edges of the polygon
		for (int i=0; i<n; i++) {    // edge from V[i] to V[i+1]
		   if (((V[i].Y() <= P.Y()) && (V[i+1].Y() > P.Y()))    // an upward crossing
			|| ((V[i].Y() > P.Y()) && (V[i+1].Y() <= P.Y()))) { // a downward crossing
				// compute the actual edge-ray intersect x-coordinate
				Real vt = (Real)(P.Y() - V[i].Y()) / (V[i+1].Y() - V[i].Y());
				if (P.X() < V[i].X() + vt * (V[i+1].X() - V[i].X())) // P.x < intersect
					++cn;   // a valid crossing of y=P.y right of P.x
			}
		}
		return (cn&1);    // 0 if even (out), and 1 if odd (in)
	}
	int PointInPolygon(const Vector2dVector & poly, const Vec2< Real > & point)
	{
		int nvert = poly.size();
		int i, j, c = 0;			
		for (i = 0, j = nvert-1; i < nvert; j = i++) 
		{
			if ( ( (poly[i].Y()>point.Y()) != (poly[j].Y()>point.Y()) ) &&
				   (point.X() < (poly[j].X()-poly[i].Y()) * (point.Y()-poly[i].Y()) / (poly[j].Y()-poly[i].Y()) + poly[i].X()) )
			{
				c = !c;
			}
	  }
	  return c;
	}

	void Triangulate(const std::vector< Vector2dVector > & contourSet, const std::vector< long > & numAncestors , std::vector< Vec2< Real > > & triangles)
	{
		size_t contoursCount = contourSet.size();
		size_t segmentsCount = 0;
		size_t holesCount  = 0;
		for (size_t i = 0; i < contoursCount; ++i)
		{
			segmentsCount += contourSet[i].size();
			if (numAncestors[i] % 2 != 0)
			{
				holesCount++;
			}
		}
		size_t pointsCount = segmentsCount;

		REAL* pointList  = new REAL[pointsCount   * 2];
		int*  segmentList = new int[segmentsCount * 2];
		for (size_t i = 0, pointIndex = 0, startIndex = 0; i < contoursCount; ++i)
		{
			size_t contourSize = contourSet[i].size();
			startIndex         = pointIndex;
			for (size_t j = 0; j < contourSize - 1; ++j)
			{
				pointList[pointIndex * 2]           = contourSet[i][j].X();
				pointList[pointIndex * 2 + 1]       = contourSet[i][j].Y();
				segmentList[pointIndex * 2]         = pointIndex;
				segmentList[pointIndex * 2 + 1]     = pointIndex + 1;
				++pointIndex;
			}
			pointList[pointIndex * 2]         = contourSet[i][contourSize - 1].X();
			pointList[pointIndex * 2 + 1]     = contourSet[i][contourSize - 1].Y();
			segmentList[pointIndex * 2]       = pointIndex;
			segmentList[pointIndex * 2 + 1]   = startIndex;
			++pointIndex;
		}

		/// Create holes
		
		REAL* holeList    = new REAL[holesCount * 2];
		for (size_t i = 0; i < contoursCount; ++i)
		{
			if (numAncestors[i] % 2 != 0)
			{
				Vec2< Real > holePoint = 1.0/3.0 * (contourSet[i][0] + contourSet[i][1] + contourSet[i][2]);
				size_t nPts = contourSet[i].size();
				Vec2< Real > U, V;
				for(size_t j = 0; j < nPts-2; ++j)
				{
					U = contourSet[i][j] - contourSet[i][j+1];
					V = contourSet[i][j] - contourSet[i][j+2];
					if (fabs(V.X() * U.Y() - V.Y() * U.X()) > EPSILON)
					{
						holePoint = EPSILON/3.0 * (contourSet[i][j] + contourSet[i][j+1] + contourSet[i][j+2]) + (1-EPSILON) * contourSet[i][0];
						break;
					}
				}
				holeList[2*(i-1)]     = holePoint.X();
				holeList[2*(i-1) + 1] = holePoint.Y();
			}
		}
	
		/// Triangle input and output structures
		struct triangulateio in, out;
		memset(&in, 0, sizeof(triangulateio));
		memset(&out, 0, sizeof(triangulateio));

		/// Define input points
		in.numberofpoints = pointsCount;
		in.pointlist      = pointList;

		/// Define segments
		in.numberofsegments = segmentsCount;
		in.segmentlist      = segmentList;

		/// Define holes
		in.numberofholes    = holesCount;
		in.holelist         = holeList;

		triangulate("pzQ", &in, &out, 0);


		int nIndices = out.numberoftriangles * 3;
		triangles.resize(nIndices);

		for (int i = 0; i < nIndices;)
		{
			for (int k = 0; k < 3; ++k)
			{
				triangles[i] = Vec2< Real > ( pointList[ 2 * out.trianglelist[i] ], 
					                          pointList[ 2 * out.trianglelist[i] +1 ]);
				i++;
			}
		}


		delete[] pointList;
		delete[] segmentList;
		delete[] holeList;
		free(out.pointlist);
		free(out.pointmarkerlist);
		free(out.trianglelist);
		free(out.neighborlist);
		free(out.segmentlist);
		free(out.segmentmarkerlist);
	}
	void PolyToMesh( Mesh * const triPoly, const Polyline2D * const poly,  REAL* const pointList,  int* const segmentList)
	{
		struct triangulateio in, out;
		memset(&in, 0, sizeof(triangulateio));
		memset(&out, 0, sizeof(triangulateio));

		in.numberofpoints = poly->GetNPoints();
		in.pointlist      = pointList;

		/// Define segments
		in.numberofsegments = poly->GetNEdges();
		in.segmentlist      = segmentList;
					
		for (long i = 0, p = 0; i < in.numberofpoints; ++i)
		{
			const Vec2< Real > & pt = poly->GetPoint(i);
			pointList[p++]  = pt.X();
			pointList[p++]  = pt.Y();
		}
		for (long i = 0, p = 0; i < in.numberofsegments; ++i)
		{
			const Vec2< long > & edge = poly->GetEdge(i);
			segmentList[p++]  = edge.X();
			segmentList[p++]  = edge.Y();
		}

		triangulate("pzQ", &in, &out, 0);

		triPoly->ResizePoints(out.numberofpoints);
		for (int i = 0; i < out.numberofpoints; ++i)
		{
			triPoly->SetPoint(i, Vec3< Real > (out.pointlist[2*i], out.pointlist[2*i+1], 0.0));
		}

		int nTriangles = out.numberoftriangles;
		triPoly->ResizeTriangles(nTriangles);

		Vec3< long > tri;
		Vec3< Real > pts[3];
		for (int i = 0; i < nTriangles; ++i)
		{					 
			for(int k = 0; k < 3; ++k)
			{
				tri[k] = out.trianglelist[3 * i + k];
			}
			triPoly->SetTriangle(i, tri);
		}
		free(out.pointlist);
		free(out.pointmarkerlist);
		free(out.trianglelist);
		free(out.neighborlist);
		free(out.segmentlist);
		free(out.segmentmarkerlist);
	}
	Real Sign(const Vec3< Real > & p1, const Vec3< Real > & p2, const Vec3< Real > & p3)
	{
	  return (p1.X() - p3.X()) * (p2.Y() - p3.Y()) - (p2.X() - p3.X()) * (p1.Y() - p3.Y());
	}
	bool PointIn2DTriangle(const Vec3< Real > & pt, 
						   const Vec3< Real > & v1, const Vec3< Real > & v2, const Vec3< Real > & v3)
	{
	  bool b1, b2, b3;
	  Real s1 = Sign(pt, v1, v2);
	  Real s2 = Sign(pt, v2, v3);
	  Real s3 = Sign(pt, v3, v1);

	  b1 = s1 < 0.0;
	  b2 = s2 < 0.0;
	  b3 = s3 < 0.0;
	  return ((b1 == b2) && (b2 == b3));
	}
#ifdef DEBUG_VHACD
	int g_it  =0;
	char g_fileName[1024];
	char g_root[] = "C:\\Work\\HACD\\HACD\\data\\test\\temp\\";
#endif
	void Mesh::Clip(const Real a, const Real b, const Real c, const Real d,  // Plane: ax + by + cz + d = 0
					Mesh * positivePart, Mesh * negativePart, const bool fillClippingHoles) const
	{
		if (!positivePart || !negativePart) return;
		positivePart->Clear();
		negativePart->Clear();
		positivePart->m_points.reserve(GetNPoints());
		negativePart->m_points.reserve(GetNPoints());
		Vec3<long> tri;
		Vec3<Real> pts[3];
		Vec3<Real> pt1, pt2;
		Real dot[3];
		long sign[3];
		int p0, p1, p2;
		Polyline2D contours;

		Vec3<Real> U_0, V_0;
		Vec3<Real> Orig(0.0);
		
		if (fillClippingHoles)
		{
			Vec3<Real> N(a, b, c);
			
			Vec3<Real> e1(1.0, 0.0, 0.0);
			Vec3<Real> e2(0.0, 1.0, 0.0);
			Vec3<Real> e3(0.0, 0.0, 1.0);
			N.Normalize();

			Real aa = fabs(a);
			Real ab = fabs(b);
			Real ac = fabs(c);
			if (aa > ab && aa > ac && aa > EPSILON)
			{
				Orig[0] = -d/a;
				if ( ab > ac ) U_0 = e3 - (e3 * N) * N;
				else           U_0 = e2 - (e2 * N) * N;
			}
			else if (ab > ac && ab > EPSILON)
			{
				Orig[1] = -d/b;
				U_0 = e1 - (e1 * N) * N;
				if ( aa > ac ) U_0 = e3 - (e3 * N) * N;
				else           U_0 = e1 - (e1 * N) * N;
			}
			else if (ac > EPSILON)
			{
				Orig[2] = -d/c;
				U_0 = e3 - (e3 * N) * N;
				if ( aa > ab ) U_0 = e2 - (e2 * N) * N;
				else           U_0 = e1 - (e1 * N) * N;
			}
			U_0.Normalize();
			V_0 = N ^ U_0;
		}


		for(size_t t = 0; t < GetNTriangles(); t++)
		{
			tri = GetTriangle(t);
			int nPositive = 0;
			int nNegative = 0;
			for(int k = 0; k < 3; k++)
			{
				pts[k] = GetPoint(tri[k]);
				dot[k] =  a * pts[k][0] + b * pts[k][1] + c * pts[k][2] + d;
				if (dot[k] >= 0.0)
				{					
					nPositive++;
					sign[k] = 1;
				}
				else
				{
					nNegative++;
					sign[k] = -1;
				}
			}

			if (nPositive == 3)  
			{
				for(int k = 0; k < 3; k++)
				{
					positivePart->AddPoint(pts[k]);
				}
			}
			else if (nNegative == 3) 
			{
				for(int k = 0; k < 3; k++)
				{
					negativePart->AddPoint(pts[k]);
				}
			}
			else if (nPositive == 2)
			{
				int k = 0;
				for(; sign[k] != -1; k++);
				p0 = k;			// point on the negative side
				p1 = (k+1)%3;	// point on the positive side
				p2 = (k+2)%3;	// point on the positive side

				// intersection points
				pt1 = pts[p0] + (dot[p0] / (dot[p0] - dot[p1])) * (pts[p1] - pts[p0]);
				pt2 = pts[p0] + (dot[p0] / (dot[p0] - dot[p2])) * (pts[p2] - pts[p0]);
				if (fillClippingHoles)
				{
					long indexPt1 = contours.InsertPoint(Vec2< Real >((pt1 - Orig) * U_0, (pt1 - Orig) * V_0));
					long indexPt2 = contours.InsertPoint(Vec2< Real >((pt2 - Orig) * U_0, (pt2 - Orig) * V_0));
					contours.InsertEdge(Vec2<long>(indexPt1, indexPt2));
				}

				// first triangle (positive side)
				positivePart->AddPoint(pts[p2]);
				positivePart->AddPoint(pt2);
				positivePart->AddPoint(pts[p1]);
				// second triangle (positive side)
				positivePart->AddPoint(pts[p1]);
				positivePart->AddPoint(pt2);
				positivePart->AddPoint(pt1);

				// one triangle (negative side)
				negativePart->AddPoint(pts[p0]);
				negativePart->AddPoint(pt1);
				negativePart->AddPoint(pt2);
			}
			else
			{
				int k = 0;
				for(; sign[k] != 1; k++);
				p0 = k;			// point on the negative side
				p1 = (k+1)%3;	// point on the positive side
				p2 = (k+2)%3;	// point on the positive side

				// intersection points
				pt1 = pts[p0] + (dot[p0] / (dot[p0] - dot[p1])) * (pts[p1] - pts[p0]);
				pt2 = pts[p0] + (dot[p0] / (dot[p0] - dot[p2])) * (pts[p2] - pts[p0]);
				if (fillClippingHoles)
				{
					long indexPt1 = contours.InsertPoint(Vec2< Real >((pt1 - Orig) * U_0, (pt1 - Orig) * V_0));
					long indexPt2 = contours.InsertPoint(Vec2< Real >((pt2 - Orig) * U_0, (pt2 - Orig) * V_0));
					contours.InsertEdge(Vec2<long>(indexPt1, indexPt2));
				}

				// first triangle (negative side)
				negativePart->AddPoint(pts[p2]);
				negativePart->AddPoint(pt2);
				negativePart->AddPoint(pts[p1]);
				// second triangle (negative side)
				negativePart->AddPoint(pts[p1]);
				negativePart->AddPoint(pt2);
				negativePart->AddPoint(pt1);

				// one triangle (positive side)
				positivePart->AddPoint(pts[p0]);
				positivePart->AddPoint(pt1);
				positivePart->AddPoint(pt2);
			}
		}

		if (fillClippingHoles && contours.GetNPoints() > 3)
		{
		
			std::vector< Mesh * > triPolys;

#ifdef DEBUG_VHACD
			Mesh mesh0;
			for(size_t pt = 0; pt < contours.GetNPoints(); ++pt)
			{
				mesh0.AddPoint(Vec3< Real >(contours.GetPoint(pt).X(), contours.GetPoint(pt).Y(), 0.0));
			}
			for(size_t ed = 0; ed < contours.GetNEdges(); ++ed)
			{
				mesh0.AddTriangle(Vec3<long>(contours.GetEdge(ed).X(), contours.GetEdge(ed).Y(), contours.GetEdge(ed).X()));
			}
			sprintf(g_fileName, "%scontours_d_%i.wrl", g_root, g_it);			
			mesh0.SaveVRML2(g_fileName);
#endif


			long * v2CC       = new long [contours.GetNPoints()];
			long * mapping    = new long [contours.GetNPoints()];
			REAL* pointList   = new REAL [contours.GetNPoints() * 2];
			int*  segmentList = new int  [contours.GetNEdges() * 2];

			Mesh * triContours = new Mesh;
			PolyToMesh(triContours, &contours,  pointList, segmentList);
			triPolys.push_back(triContours);
#ifdef DEBUG_VHACD
			char fileName[1024];
			sprintf(fileName, "%striPolys_%i.wrl", g_root, g_it); 
			triContours->SaveVRML2(fileName);
#endif
			size_t nCC = contours.ComputeConnectedComponents(v2CC);
			for(size_t p = 0; p < nCC; ++p)
			{				
				Polyline2D poly;
				contours.ExtractConnectedComponent(p, v2CC, mapping, poly);
				if (poly.GetNPoints() > 3)
				{
					Mesh * triPoly = new Mesh;
					PolyToMesh(triPoly, &poly,  pointList, segmentList);
					triPolys.push_back(triPoly);
#ifdef DEBUG_VHACD
					char fileName[1024];
					sprintf(fileName, "%striPoly_%i_%i.wrl", g_root, g_it, p); 
					triPoly->SaveVRML2(fileName);
#endif
				}
			}

			const size_t nTriPolys = triPolys.size();
			std::vector<long> nAncestors;
			nAncestors.resize(nTriPolys);
			for(size_t p1 = 1; p1 < nTriPolys; ++p1)			
			{
				const Mesh * const m1 = triPolys[p1];
				size_t nT = m1->GetNTriangles();
				for(size_t p2 = 1; p2 < nTriPolys; ++p2)
				{
					if (p1 != p2)
					{
						const Mesh * const m2 = triPolys[p2];
						size_t nV = m2->GetNPoints();
						bool inside = true;
						for(size_t v = 0; (v < nV) && (inside); ++v)
						{
							const Vec3< Real > & pt = m2->GetPoint(v);
							inside = false;
							for(size_t tt = 0; tt < nT; ++tt)
							{
								const Vec3< long > & tri = m1->GetTriangle(tt);
								if (PointIn2DTriangle(pt, m1->GetPoint(tri[0]), m1->GetPoint(tri[1]), m1->GetPoint(tri[2])))
								{
									inside = true;
									break;
								}
							}
						}
						if (inside)
						{
							nAncestors[p2]++;
						}
					}
				}
			}
			
			size_t nT = triContours->GetNTriangles();
			Vec3< Real > bary, pts[3];
			for(size_t tt = 0; tt < nT; ++tt)
			{
				long nHoles  = 0;
				long nObject = 0;

				const Vec3< long > & tri = triContours->GetTriangle(tt);

				for(int k = 0; k < 3; ++k)
				{
					pts[k] = triContours->GetPoint(tri[k]);
				}

				bary = 1/3.0 * (pts[0]+pts[1]+pts[2]);
				for(size_t p1 = 1; p1 < nTriPolys; ++p1)			
				{
					const Mesh * const m1 = triPolys[p1];
					size_t nT1 = m1->GetNTriangles();
					bool inside = false;
					for(size_t t1 = 0; t1 < nT1; ++t1)
					{
						const Vec3< long > & tri1 = m1->GetTriangle(t1);
						if (PointIn2DTriangle(bary, m1->GetPoint(tri1[0]), m1->GetPoint(tri1[1]), m1->GetPoint(tri1[2])))
						{
							inside = true;
							break;
						}
					}
					if ( inside )
					{
						if (nAncestors[p1] % 2 == 0)
						{
							nObject++;
						}
						else
						{
							nHoles++;
						}
					}
				}
				if (nObject > nHoles)
				{
					for(int k = 0; k < 3; ++k)
					{
						pts[k] = Orig + pts[k].X() * U_0 + pts[k].Y() * V_0;	
					}
					positivePart->AddPoint(pts[1]);
					positivePart->AddPoint(pts[0]);
					positivePart->AddPoint(pts[2]);
					negativePart->AddPoint(pts[0]);
					negativePart->AddPoint(pts[1]);
					negativePart->AddPoint(pts[2]);
				}
			}
			

			delete [] pointList;
			delete [] segmentList;
			delete [] v2CC;
			delete [] mapping;
			
			for(size_t p=0; p < nTriPolys;  ++p)
			{
				delete triPolys[p];
			}
		}
		positivePart->ResizeTriangles(positivePart->GetNPoints() / 3);
		for(size_t t = 0, index = 0; t < positivePart->GetNTriangles(); t++)
		{
			positivePart->SetTriangle(t, Vec3<long>(index, index+1, index+2));
			index += 3;
		}
		negativePart->ResizeTriangles(negativePart->GetNPoints() / 3);
		for(size_t t = 0, index = 0; t < negativePart->GetNTriangles(); t++)
		{
			negativePart->SetTriangle(t, Vec3<long>(index, index+1, index+2));
			index += 3;
		}
#ifdef DEBUG_VHACD
		sprintf(g_fileName, "%snegative_d_%i.wrl", g_root, g_it);			
		negativePart->SaveVRML2(g_fileName);
		sprintf(g_fileName, "%spositive_d_%i.wrl", g_root, g_it);			
		positivePart->SaveVRML2(g_fileName);
		g_it++;
#endif
	}
	void Mesh::Center(bool bbCenter, Axis axis)
	{
		const size_t nPoints    = GetNPoints();
		const size_t nTriangles = GetNTriangles();
		if (bbCenter)
		{
			if (nPoints == 0) return;
			ComputeBB();
			m_center  = 0.5 * m_minBB + 0.5 * m_maxBB;
		}
		else
		{
			Vec3<long> tri;
			Vec3<Real> pt1, pt2, pt3, bary(0.0);
			Real weight, sumWeight = 0.0;
			for (size_t v = 0; v<nTriangles; ++v)
			{
				tri = GetTriangle(v);
				pt1 = m_points[tri.X()];
				pt2 = m_points[tri.Y()];
				pt3 = m_points[tri.Z()];
				weight = ((pt2-pt1)^(pt3-pt1)).GetNorm();
				sumWeight += weight;
				bary +=  weight * (pt1+pt2+pt3)/3.0;
			}
			bary /= sumWeight;
			m_center = bary;
		}
		if (axis == X_AXIS)
		{
			m_center.Y() = 0.0;
			m_center.Z() = 0.0;
		}
		else if (axis == Y_AXIS)
		{
			m_center.X() = 0.0;
			m_center.Z() = 0.0;
		}
		else if (axis == Z_AXIS)
		{
			m_center.X() = 0.0;
			m_center.Y() = 0.0;
		}
        for (size_t v = 0; v < nPoints ; v++) m_points[v] -= m_center;
	}


	void Mesh::Normalize()
	{
		const size_t nPoints    = GetNPoints();
		if (nPoints == 0)
		{
			return;
		}
		Center(true, N_AXIS);
        Real m_diag = (m_maxBB-m_minBB).GetNorm();
        const Real invDiag = static_cast<Real>(1000.0/ m_diag);
		if (m_diag != 0.0)
		{
			for (size_t v = 0; v < nPoints ; v++) 
			{
				m_points[v] = m_points[v]* invDiag;
			}
		}
    }

	Axis Mesh::GetMaxDim(Real & d) const
	{
		return Z_AXIS;
		Real dx = GetMaxBB()[0] - GetMinBB()[0];
		Real dy = GetMaxBB()[1] - GetMinBB()[1];
		Real dz = GetMaxBB()[2] - GetMinBB()[2];
		if (dx > dy && dx > dz)
		{
			d = dx;
			return X_AXIS;
		}	
		else if (dy > dz)
		{
			d = dy;
			return Y_AXIS;
		}
		d = dz;
		return Z_AXIS;
	}

	void Mesh::CleanDuplicatedVectices()
	{
		const size_t nPoints = GetNPoints();
		if (nPoints == 0)
		{
			return;
		}
	
		long * map = new long [nPoints];
		kdres * presults;
		kdtree * kd = kd_create(3);
		Vec3< Real > pt;
		for(size_t v = 0; v < nPoints; v++)
		{
			pt = GetPoint(v);
			map[v] = v;
			kd_insert3(kd, pt[0], pt[1], pt[2], (void *) (&map[v]));
		}

		Mesh old = *this;
		this->Clear();
		
		long nV = 0;
		long * ptIndex;
		Real pos[3];
		for(size_t v1 = 0; v1 < nPoints; v1++)
		{
			if (map[v1] >= 0)
			{
				pt  = old.GetPoint(v1);
				this->AddPoint(pt);
				presults = kd_nearest_range3(kd, pt[0], pt[1], pt[2], EPSILON);
				while( !kd_res_end( presults ) ) 
				{
					ptIndex = (long *)(kd_res_item( presults, pos));
					*ptIndex = - (nV + 1);
					kd_res_next( presults );
				}
				kd_res_free(presults);
				nV++;
			}
		}
		kd_free(kd);
		Vec3<long> triangle;
		const size_t nTriangles = old.GetNTriangles();
		for(size_t t = 0; t < nTriangles; ++t)
		{
			triangle = old.GetTriangle(t);

			for(int k = 0; k < 3; k++)
			{
				triangle[k] =  - map[triangle[k]] - 1;
			}
			if ( triangle[0] != triangle[1] &&
				 triangle[1] != triangle[2] &&
				 triangle[2] != triangle[0] )
			{
				AddTriangle(triangle);
			}
		}
		delete [] map;
	}


	size_t Mesh::ComputeConnectedComponents(long * v2CC) const
	{
		const size_t nPoints = GetNPoints();
		std::vector< SArray<long, SARRAY_DEFAULT_MIN_SIZE> > v2V;
		v2V.resize(nPoints);
		for(size_t v1 = 0; v1 < nPoints; v1++)
		{
			v2CC[v1] = -1;
		}

		const size_t nTriangles = GetNTriangles();
		Vec3<long> triangle;
		long v1, v2, v0;
		for(size_t t = 0; t < nTriangles; ++t)
		{
			triangle = GetTriangle(t);
			v0 = triangle[0];
			v1 = triangle[1];
			v2 = triangle[2];
			if (v1 != v2 && v2 != v0 && v1 != v0)
			{
				v2V[v0].Insert(v1);
				v2V[v0].Insert(v2);
				v2V[v1].Insert(v0);
				v2V[v1].Insert(v2);
				v2V[v2].Insert(v0);
				v2V[v2].Insert(v1);
			}
		}


		size_t nCCs = 0;
		long cv, nv;
		std::queue<int> L;
		int size = 0;
		for(size_t v = 0; v < nPoints; ++v)
		{
			if (v2CC[v] == -1)
			{
				size = 1;
				L.push(v);
				v2CC[v] = nCCs;
				while (L.size() > 0)
				{
					cv = L.front();
					L.pop();

					for(size_t h = 0; h < v2V[cv].Size(); ++h)
					{
						nv = v2V[cv][h];
						if (v2CC[nv] == -1) 
						{
							v2CC[nv] = nCCs;
							L.push(nv);
							++size;
						}
					}
				}
				++nCCs;
			}
		}
		return nCCs;
	}

	void Mesh::ExtractConnectedComponent(size_t nCC, long const * const v2CC, long * mapping, Mesh & CC) const
	{
		const size_t nPoints    = GetNPoints();
		const size_t nTriangles = GetNTriangles();
		size_t nvCC = 0;
		for(size_t vertex = 0; vertex < nPoints; ++vertex)
		{
			if (v2CC[vertex] == nCC)
			{
				CC.AddPoint(GetPoint(vertex));
				mapping[vertex] = nvCC++;
			}
			else mapping[vertex] = -1;
		}
		Vec3<long> triangle;
		for(size_t t = 0; t < nTriangles; ++t)
		{
			triangle = GetTriangle(t);
			triangle[0] = mapping[triangle[0]];
			triangle[1] = mapping[triangle[1]];
			triangle[2] = mapping[triangle[2]];
			if ( triangle[0]>=0 && triangle[1]>=0 && triangle[2]>=0 &&
				 triangle[0] != triangle[1] &&
				 triangle[1] != triangle[2] &&
				 triangle[2] != triangle[0] )
			{
				CC.AddTriangle(triangle);
			}
		}
	}


    Real Mesh::ComputeVolume() const
    {
        const size_t nV    = GetNPoints();
		if (nV == 0)
		{
			return 0.0;
		}       
        Vec3<Real> bary(0.0, 0.0, 0.0);
        for(size_t v = 0; v < nV; v++)
        {
			bary +=  GetPoint(v);
        }
        bary /= static_cast<Real>(nV);
        
        size_t nT = GetNTriangles();
        Vec3<Real> ver0, ver1, ver2;
		
        double totalVolume = 0.0;
        for(size_t t = 0; t < nT; t++)
        {
			const Vec3<long> & tri = GetTriangle(t);
            ver0 = GetPoint(tri[0]);
			ver1 = GetPoint(tri[1]);
			ver2 = GetPoint(tri[2]);
			totalVolume += Volume(ver0, ver1, ver2, bary);
        }
        return totalVolume;
    }
	
    void Mesh::ComputeConvexHull(Mesh & meshCH) const
    {
		if (GetPoints() == 0)
			return;
		btConvexHullComputer ch;
		ch.compute(GetPoints(), 3 * sizeof(Real), GetNPoints(), -1.0, -1.0); 
		meshCH.Clear();

		for(int v = 0; v < ch.vertices.size(); v++)
		{			
			meshCH.AddPoint(Vec3<Real>(ch.vertices[v].getX(), ch.vertices[v].getY(), ch.vertices[v].getZ()));
		}
		const int nt = ch.faces.size();
		for(int t = 0; t < nt; ++t)
		{

			const btConvexHullComputer::Edge * sourceEdge = & (ch.edges[ch.faces[t]]);
			int a = sourceEdge->getSourceVertex();
			int b = sourceEdge->getTargetVertex();
			const btConvexHullComputer::Edge * edge = sourceEdge->getNextEdgeOfFace();
			int c = edge->getTargetVertex();
			while (c != a)
			{				
				meshCH.AddTriangle(Vec3<long>(a, b, c));				
				edge = edge->getNextEdgeOfFace();
				b = c;
				c = edge->getTargetVertex();
			}
		}
	}




    Polyline2D::Polyline2D()
    {
    }
    Polyline2D::~Polyline2D()
    {
    }

	long Polyline2D::InsertPoint(const Vec2<Real> & pt)
	{
		const size_t nPts = GetNPoints();
		Vec2<Real> point;
		for(size_t p = 0; p < nPts; p++)
		{
			point = GetPoint(p);
			if ( (point-pt).GetNorm() < EPSILON )
			{
				return static_cast<long>(p);
			}
		}
		AddPoint(pt);
		return static_cast<long>(GetNPoints() - 1);
	}

	// Copyright 2001, softSurfer (www.softsurfer.com)
	// This code may be freely used and modified for any purpose
	// providing that this copyright notice is included with it.
	// SoftSurfer makes no warranty for this code, and cannot be held
	// liable for any real or imagined damage resulting from its use.
	// Users of this code must verify correctness for their application.

	// inSegment(): determine if a point is inside a segment
	//    Input:  a point P, and a collinear segment S=[AB]
	//    Return: 1 = P is inside S
	//            0 = P is not inside S
	int PointIn2DSegment( const Vec2< Real > & P, const Vec2< Real > & A,  const Vec2< Real > & B)
	{
		if (A.X() != B.X()) {    // S is not vertical
			if (A.X() <= P.X() && P.X() <= B.X())
				return 1;
			if (A.X() >= P.X() && P.X() >= B.X())
				return 1;
		}
		else {    // S is vertical, so test y coordinate
			if (A.Y() <= P.Y() && P.Y() <= B.Y())
				return 1;
			if (A.Y() >= P.Y() && P.Y() >= B.Y())
				return 1;
		}
		return 0;
	}

	// intersect2D_2Segments(): the intersection of 2 finite 2D segments
	//    Input:  two finite segments S1=[PA, PB] and S2=[PC, PD]
	//    Output: I0 = intersect point (when it exists)
	//            I1 = endpoint of intersect segment [I0,I1] (when it exists)
	//    Return: 0=disjoint (no intersect)
	//            1=intersect in unique point I0
	//            2=overlap in segment from I0 to I1
	int
	Intersect2DSegments( const Vec2< Real > & PA, 
						 const Vec2< Real > & PB,
						 const Vec2< Real > & PC, 
						 const Vec2< Real > & PD,
						 Vec2< Real > & I0, Vec2< Real > & I1 )
	{
		Vec2< Real >    u = PB - PA;
		Vec2< Real >    v = PD - PC;
		Vec2< Real >    w = PA - PC;
		Real            D = u ^ v;

		// test if they are parallel (includes either being a point)
		if (fabs(D) < EPSILON) {          // S1 and S2 are parallel
			if ((u^w) != 0 || (v^w) != 0) {
				return 0;                   // they are NOT collinear
			}
			// they are collinear or degenerate
			// check if they are degenerate points
			Real du = u * u;
			Real dv = v * v;
			if (du==0 && dv==0) {           // both segments are points
				if (PA.X() != PC.X() && PA.Y() != PC.Y())                 // they are distinct points
					return 0;
				I0 = PA;                // they are the same point
				return 1;
			}
			if (du==0) {                    // S1 is a single point
				if (PointIn2DSegment(PA, PC, PD) == 0)  // but is not in S2
					return 0;
				I0 = PA;
				return 1;
			}
			if (dv==0) {                    // S2 a single point
				if (PointIn2DSegment(PC, PA, PB) == 0)  // but is not in S1
					return 0;
				I0 = PC;
				return 1;
			}
			// they are collinear segments - get overlap (or not)
			Real t0, t1;                   // endpoints of S1 in eqn for S2
			Vec2 < Real > w2 = PB - PC;
			if (v.X() != 0) {
					t0 = w.X() / v.X();
					t1 = w2.X() / v.X();
			}
			else {
					t0 = w.Y() / v.Y();
					t1 = w2.Y() / v.Y();
			}
			if (t0 > t1) {                  // must have t0 smaller than t1
					Real t=t0; t0=t1; t1=t;    // swap if not
			}
			if (t0 > 1 || t1 < 0) {
				return 0;     // NO overlap
			}
			t0 = t0<0? 0 : t0;              // clip to min 0
			t1 = t1>1? 1 : t1;              // clip to max 1
			if (t0 == t1) {                 // intersect is a point
				I0 = PC + t0 * v;
				return 1;
			}

			// they overlap in a valid subsegment
			I0 = PC + t0 * v;
			I1 = PC + t1 * v;
			return 2;
		}

		// the segments are skew and may intersect in a point
		// get the intersect parameter for S1
		Real     sI = (v^w) / D;
		if (sI < 0 || sI > 1)               // no intersect with S1
			return 0;

		// get the intersect parameter for S2
		Real     tI = (u^w) / D;
		if (tI < 0 || tI > 1)               // no intersect with S2
			return 0;

		I0 = PA + sI * u;               // compute S1 intersect point
		return 1;
	}


	void Polyline2D::InsertEdge(const Vec2<long> & edge)
	{
		// loop over all edges
		// check if current edge intersect an existing edge
		// split the current edge into two edges
		// split the old edge into two edges 
		// try to insert the two new edges
		

		long a = edge.X();
		long b = edge.Y();

		if (a == b)
		{
			return;
		}

		long c, d;
		long i0;
//		long i1;
//		long x[2];
//		long k;
		const Vec2< Real > & A = m_points[a];
		const Vec2< Real > & B = m_points[b];

		Vec2< Real > I0, I1;
		int test;
		const size_t nE = m_edges.size();
		for(size_t e = 0; e < nE; ++e)
		{
			c = m_edges[e].X();
			d = m_edges[e].Y();
			const Vec2< Real > & C = m_points[c];
			const Vec2< Real > & D = m_points[d];

			if ( (a==c && b==d) || (a==d && b==c) ) // edge already exists
			{
				return; 
			}
			test = Intersect2DSegments(A, B, C, D,  I0, I1);

			if      (test == 0) // disjoint
			{
				// nothing
			}
			else if ( test == 1 )  // intersect in unique point I0
			{
				i0 = InsertPoint(I0);
				if ((i0 == a && i0 == d) || (i0 == b && i0 == d) ||
					(i0 == a && i0 == c) || (i0 == b && i0 == c))				
				{
					// nothing
				}
				else if (i0 == a || i0 == b) // i0 != c and i0 != d
				{
					m_edges[e].Y() = i0;
					InsertEdge(Vec2<long>(i0, d));
				}
				else // i0 != (a, b, c, d)
				{
					m_edges[e].Y() = i0;
					InsertEdge(Vec2<long>(i0, d));
					InsertEdge(Vec2<long>(a, i0));
					InsertEdge(Vec2<long>(i0, b));
					return;
				}
			}
			else if ( test == 2 )  // overlap in segment from I0 to I1
			{
/*
				long indices[4] = {a, b, c, d};
				Real dist = (I0 -  m_points[indices[0]]).GetNorm();
				Real minDist = dist;
				i0 = indices[0];				
				for (int h = 1; h < 4; ++h)
				{
					dist = (I0 -  m_points[indices[h]]).GetNorm();
					if ( dist < minDist)
					{
						minDist = dist;
						i0 = indices[h];
					}
				}
				dist = (I1 -  m_points[indices[0]]).GetNorm();
				minDist = dist;
				i1 = indices[0];				
				for (int h = 1; h < 4; ++h)
				{
					dist = (I1 -  m_points[indices[h]]).GetNorm();
					if ( dist < minDist)
					{
						minDist = dist;
						i1 = indices[h];
					}
				}

//				i0 = InsertPoint(I0);
//				i1 = InsertPoint(I1);
				m_edges[e].X() = i0;
				m_edges[e].Y() = i1;
				k = 0;
				if (a != i0 && a!=i1)
				{
					x[k++] = a;
				}
				else if (b != i0 && b!=i1)
				{
					x[k++] = b;
				}

				if (c != i0 && c!=i1)
				{
					x[k++] = c;
				}
				else if (d != i0 && d!=i1)
				{
					x[k++] = d;
				}

				if (k == 2) 
				{
					if ( (m_points[x[0]] -  m_points[i0]).GetNorm() < (m_points[x[0]] -  m_points[i1]).GetNorm() )
					{
						InsertEdge(Vec2<long>(x[0], i0));
						InsertEdge(Vec2<long>(i1, x[1]));
						return;
					}
					else
					{
						InsertEdge(Vec2<long>(x[0], i1));
						InsertEdge(Vec2<long>(i0, x[1]));
						return;
					}
				}
				else if (k == 1)
				{					

					if ( (m_points[x[0]] -  m_points[i0]).GetNorm() < (m_points[x[0]] -  m_points[i1]).GetNorm() )
					{
						InsertEdge(Vec2<long>(x[0], i0));
						return;
					}
					else
					{
						InsertEdge(Vec2<long>(x[0], i1));
						return;
					}
				}
				else
				{
					printf("error 1 Polyline2D::InsertEdge %i \n", k);
					printf("(%i, %i, %i, %i) \n", a, b, c, d);
					printf("(%i, %i) \n", i0, i1);
					printf("(%i, %i) \n", x[0], x[1]);
					getchar();
				}
*/
			}
		}	
		m_edges.push_back(edge);
	};


	size_t Polyline2D::ComputeConnectedComponents(long * v2CC) const
	{
		const size_t nPoints = GetNPoints();
		std::vector< SArray<long, SARRAY_DEFAULT_MIN_SIZE> > v2V;
		v2V.resize(nPoints);

		for(size_t v1 = 0; v1 < nPoints; v1++)
		{
			v2CC[v1] = -1;
		}

		const size_t nEdges = GetNEdges();
		Vec2<long> edge;
		long v1, v0;
		for(size_t e = 0; e < nEdges; ++e)
		{
			edge = GetEdge(e);
			v0 = edge[0];
			v1 = edge[1];
			if (v1 != v0)
			{
				v2V[v0].Insert(v1);
				v2V[v1].Insert(v0);
			}
		}


		size_t nCCs = 0;
		long cv, nv;
		std::queue<int> L;
		int size = 0;
		for(size_t v = 0; v < nPoints; ++v)
		{
			if (v2CC[v] == -1)
			{
				size = 1;
				L.push(v);
				v2CC[v] = nCCs;
				while (L.size() > 0)
				{
					cv = L.front();
					L.pop();

					for(size_t h = 0; h < v2V[cv].Size(); ++h)
					{
						nv = v2V[cv][h];
						if (v2CC[nv] == -1) 
						{
							v2CC[nv] = nCCs;
							L.push(nv);
							++size;
						}
					}
				}
				++nCCs;
			}
		}
		return nCCs;
	}
	void Polyline2D::ExtractConnectedComponent(size_t nCC, long const * const v2CC, long * mapping, Polyline2D & CC) const
	{
		const size_t nPoints    = GetNPoints();
		const size_t nEdges = GetNEdges();
		size_t nvCC = 0;
		for(size_t vertex = 0; vertex < nPoints; ++vertex)
		{
			if (v2CC[vertex] == nCC)
			{
				CC.AddPoint(GetPoint(vertex));
				mapping[vertex] = nvCC++;
			}
			else mapping[vertex] = -1;
		}
		Vec2<long> edge;
		for(size_t t = 0; t < nEdges; ++t)
		{
			edge = GetEdge(t);
			edge[0] = mapping[edge[0]];
			edge[1] = mapping[edge[1]];
			if ( edge[0]>=0 && edge[1]>=0 && edge[0] != edge[1])
			{
				CC.AddEdge(edge);
			}
		}
	}
}