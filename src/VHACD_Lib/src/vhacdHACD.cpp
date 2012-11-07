/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define _CRT_SECURE_NO_WARNINGS
#include <sstream>
#include <VHACDGraph.h>
#include <VHACDHACD.h>
#include <string.h>
#include <algorithm>
#include <iterator>
#include <limits>
#include <VHACDMeshDecimator.h>
#include <btConvexHullComputer.h>
#include <limits>
#include <fstream>

//#define VHACD_DEBUG
namespace VHACD
{ 

	void VHACD::AddEdge(long v1, long v2)
    {
		if (m_graph.GetEdgeID(v1, v2) == -1)
		{
			m_graph.AddEdge(v1, v2);
		}
	}

	void VHACD::CreateGraph()
    {
		std::vector< Sphere > spheres;
		const size_t nCH = m_convexHulls->size();
		spheres.resize(nCH);		
		for(size_t p = 0; p < nCH; ++p)
		{
			(*m_convexHulls)[p]->ComputeBB();
			spheres[p].m_center = 0.5 * ((*m_convexHulls)[p]->GetMinBB() + (*m_convexHulls)[p]->GetMaxBB());
			spheres[p].m_radius = 0.5 * ((*m_convexHulls)[p]->GetMaxBB() - (*m_convexHulls)[p]->GetMinBB()).GetNorm();
		}
		m_graph.Clear();
		m_graph.Allocate(nCH, 5 * nCH);

		for(size_t p1 = 0; p1 < nCH; ++p1)
		{
			m_graph.m_vertices[p1].m_volume     = fabs((*m_convexHulls)[p1]->ComputeVolume());
			
			for(size_t p2 = p1+1; p2 < nCH; ++p2)
			{
				if ( (spheres[p2].m_center-spheres[p1].m_center).GetNorm() < spheres[p1].m_radius+spheres[p2].m_radius )
				{
					m_graph.AddEdge(p1, p2);
				}
			}
		}

    }

	VHACD::VHACD()
	{
        m_nClusters = 0;
        m_concavity = 0.0;
        m_diag = 1.0;
        m_nVerticesPerCH = 30;
		m_callBack = 0;
		m_nMinClusters = 3;
		m_convexHulls = 0;
//		m_finalConvexHulls = 0;
	}																
	VHACD::~VHACD(void)
	{
	}

    void VHACD::ComputeEdgeCost(size_t e)
    {
		GraphEdge & gE = m_graph.m_edges[e];
        long v1 = gE.m_v1;
        long v2 = gE.m_v2;

		const GraphVertex & gV1 = m_graph.m_vertices[v1];
		const GraphVertex & gV2 = m_graph.m_vertices[v2];

		Mesh * m1 = (*m_convexHulls)[v1];
		Mesh * m2 = (*m_convexHulls)[v2];

		const size_t n1 = m1->GetNPoints();
		const size_t n2 = m2->GetNPoints();
		const size_t n = n1 + n2;

		Vec3< Real > * points = new Vec3< Real > [n1+n2];
				
		for(size_t v = 0; v < n1; ++v)
		{
			points[v] = m1->GetPoint(v);
		}
		for(size_t v = n1; v < n; ++v)
		{
			points[v] = m2->GetPoint(v-n1);
		}
		
		btConvexHullComputer ch;
		ch.compute((Real *)points, 3 * sizeof(Real), n , -1.0, -1.0); 
		delete [] points;


		delete gE.m_convexHull;

		gE.m_convexHull = new Mesh;

		for(int v = 0; v < ch.vertices.size(); v++)
		{			
			gE.m_convexHull->AddPoint(Vec3<Real>(ch.vertices[v].getX(), ch.vertices[v].getY(), ch.vertices[v].getZ()));
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
				gE.m_convexHull->AddTriangle(Vec3<long>(a, b, c));				
				edge = edge->getNextEdgeOfFace();
				b = c;
				c = edge->getTargetVertex();
			}
		}
		gE.m_volume = gE.m_convexHull->ComputeVolume();
		gE.m_concavity = gE.m_volume - gV1.m_volume - gV2.m_volume; //fabs(gE.m_volume - gV1.m_volume - gV2.m_volume);		
	}

    bool VHACD::InitializePriorityQueue()
    {
		m_pqueue.reserve(m_graph.m_nE + 100);
        for (size_t e=0; e < m_graph.m_nE; ++e) 
        {
			ComputeEdgeCost(static_cast<long>(e));
			m_pqueue.push(GraphEdgePriorityQueue(static_cast<long>(e), m_graph.m_edges[e].m_concavity, std::min(m_graph.m_edges[e].m_v1, m_graph.m_edges[e].m_v2)));
        }
		return true;
    }
	void VHACD::Simplify()
	{

		long v1 = -1;
        long v2 = -1;        
        double progressOld = -1.0;
        double progress = 0.0;
        double globalConcavity  = 0.0;     
		char msg[1024];
		double ptgStep = 1.0;
		const size_t V0 = m_graph.GetNVertices();
		std::vector<GraphEdgePriorityQueue> highPriority;
		while ( (!m_pqueue.empty()) || (highPriority.size() > 0)) 
		{
            progress = 100.0-m_graph.GetNVertices() * 100.0 / V0;
            if (fabs(progress-progressOld) > ptgStep && m_callBack)
            {
				sprintf(msg, "%3.2f %% V = %lu \t C = %f \t \t \r", progress, static_cast<unsigned long>(m_graph.GetNVertices()), globalConcavity);
				(*m_callBack)(msg, progress, globalConcavity,  m_graph.GetNVertices());
                progressOld = progress;
				if (progress > 99.0)
				{
					ptgStep = 0.01;
				}
				else if (progress > 90.0)
				{
					ptgStep = 0.1;
				}
            }

			GraphEdgePriorityQueue currentEdge(0,0.0, -1);
			bool done = false;

			do
			{
				done = false;
				if (highPriority.size() == 0)
				{
					done = true;
                    break;
				}
				currentEdge = highPriority[highPriority.size()-1];
				highPriority.pop_back();
                m_pqueue.pop();
			}
			while (  m_graph.m_edges[currentEdge.m_name].m_deleted || 
					 m_graph.m_edges[currentEdge.m_name].m_concavity != currentEdge.m_priority);

			if (done)
			{
				do
				{
					done = false;
					if (m_pqueue.size() == 0)
					{
						done = true;
						break;
					}
					currentEdge = m_pqueue.top();
					m_pqueue.pop();
				}
				while (  m_graph.m_edges[currentEdge.m_name].m_deleted || 
						 m_graph.m_edges[currentEdge.m_name].m_concavity != currentEdge.m_priority);
			}
			if (!done)
			{
				if ((m_graph.m_edges[currentEdge.m_name].m_concavity < m_concavity) && 
						(globalConcavity < m_concavity)                                && 
						(m_graph.GetNVertices() > m_nMinClusters)                      && 
						(m_graph.GetNEdges() > 0))
				{
					v1 = m_graph.m_edges[currentEdge.m_name].m_v1;
					v2 = m_graph.m_edges[currentEdge.m_name].m_v2;	
					globalConcavity = std::max<double>(globalConcavity ,m_graph.m_edges[currentEdge.m_name].m_concavity);
					GraphEdge & gE = m_graph.m_edges[currentEdge.m_name];
					GraphVertex & gV1 = m_graph.m_vertices[v1];
					GraphVertex & gV2 = m_graph.m_vertices[v2];
					// update vertex info
					gV1.m_concavity     = gE.m_concavity;
					delete (*m_convexHulls)[v1];
					delete (*m_convexHulls)[v2];				
					(*m_convexHulls)[v1] = gE.m_convexHull;
					(*m_convexHulls)[v2] = 0;
					gE.m_convexHull = 0;
					gV1.m_volume = gE.m_volume;
					gV1.m_concavity = gE.m_concavity;
					m_graph.EdgeCollapse(v1, v2);
					long idEdge;
					for(size_t itE = 0; itE < m_graph.m_vertices[v1].m_edges.Size(); ++itE)
					{
						idEdge = m_graph.m_vertices[v1].m_edges[itE];
						ComputeEdgeCost(idEdge);
						m_pqueue.push(GraphEdgePriorityQueue(idEdge, m_graph.m_edges[idEdge].m_concavity, std::min(m_graph.m_edges[idEdge].m_v1, m_graph.m_edges[idEdge].m_v2)));
/*
						if (m_graph.m_edges[idEdge].m_concavity > gV1.m_concavity)
						{
							m_pqueue.push(GraphEdgePriorityQueue(idEdge, m_graph.m_edges[idEdge].m_concavity));
						}
						else
						{
							highPriority.push_back(GraphEdgePriorityQueue(idEdge, m_graph.m_edges[idEdge].m_concavity));
						}
*/
					}
				}
				else
				{
					break;
				}
			}
		}
		m_nClusters = m_graph.GetNVertices();
		m_finalConvexHullsIndices.resize(m_nClusters);
		for (size_t p=0, v = 0; v != m_graph.m_vertices.size(); ++v) 
		{
			if (!m_graph.m_vertices[v].m_deleted)
			{
				m_finalConvexHullsIndices[p] = v;
                if (m_callBack) 
                {
                    char msg[1024];
					sprintf(msg, "\t CH(%lu) \t %f \t %lu\n", v, static_cast<float>(m_graph.m_vertices[v].m_concavity), (*m_convexHulls)[v]->GetNPoints());
					(*m_callBack)(msg, 0.0, 0.0, m_nClusters);
					p++;
                }
			}
		}
        if (m_callBack)
        {
			sprintf(msg, "# clusters =  %lu \t C = %f\n", static_cast<unsigned long>(m_nClusters), globalConcavity);
			(*m_callBack)(msg, progress, globalConcavity,  m_graph.GetNVertices());
        }
	}

    bool VHACD::Init()
    {
		CreateGraph();
		return true;
	}


    bool VHACD::Compute()
    {
		if (!m_convexHulls) return false;
		if (m_callBack)
		{
			std::ostringstream msg;
			msg << "+ Mesh" << std::endl;
			msg << "\t # initial convex-hulls         \t" << m_convexHulls->size() << std::endl;
			msg << "+ Parameters" << std::endl;
			msg << "\t min # of clusters              \t" << m_nMinClusters << std::endl;
			msg << "\t max concavity                  \t" << m_concavity << std::endl;
			msg << "\t # vertices per convex-hull     \t" << m_nVerticesPerCH << std::endl;
//			msg << "\t produce full convex-hulls      \t" << fullCH << std::endl;	
			(*m_callBack)(msg.str().c_str(), 0.0, 0.0, m_convexHulls->size());
		}		
        // Compute volumes of convex-hulls
		if (m_callBack) (*m_callBack)("+ Initializing Priority Queue\n", 0.0, 0.0, m_convexHulls->size());
        InitializePriorityQueue();
        // we simplify the graph		
		if (m_callBack) (*m_callBack)("+ Simplification ...\n", 0.0, 0.0, m_convexHulls->size());
		Simplify();

/*
        delete [] m_finalConvexHulls;
        m_finalConvexHulls = new ICHull[m_nClusters];
		for (size_t p = 0; p != m_finalConvexHullsIndices.size(); ++p) 
		{
			size_t v = m_finalConvexHullsIndices[p];
            // compute the convex-hull
			const size_t nPts = (*m_convexHulls)[v]->GetNPoints();
			for(size_t itCH = 0; itCH < nPts; ++itCH) 
            {
				m_finalConvexHulls[p].AddPoint((*m_convexHulls)[v]->GetPoint(itCH));
            }
            if (fullCH)
            {
				if (m_finalConvexHulls[p].Process() == ICHullErrorInconsistent)
				{
					printf("error !");
				}
            }
            else
            {
				if (m_finalConvexHulls[p].Process(static_cast<unsigned long>(m_nVerticesPerCH)) == ICHullErrorInconsistent)
				{
					printf("error !");
				}
            }

		}
*/
        return true;
    }
/*    
    size_t VHACD::GetNTrianglesCH(size_t numCH) const
    {
        if (numCH >= m_nClusters)
        {
            return 0;
        }
        return m_finalConvexHulls[numCH].GetMesh().GetNTriangles();
    }
    size_t VHACD::GetNPointsCH(size_t numCH) const
    {
        if (numCH >= m_nClusters)
        {
            return 0;
        }
        return m_finalConvexHulls[numCH].GetMesh().GetNVertices();
    }
    bool VHACD::GetCH(size_t numCH, Vec3<Real> * const points, Vec3<long> * const triangles)
    {
        if (numCH >= m_nClusters)
        {
            return false;
        }
        m_finalConvexHulls[numCH].GetMesh().GetIFS(points, triangles);
        return true;
    }

    bool VHACD::Save(const char * fileName, bool uniColor, long numCluster) const
    {
        std::ofstream fout(fileName);
        if (fout.is_open())
        {
            if (m_callBack)
            {
                char msg[1024];
                sprintf(msg, "Saving %s\n", fileName);
                (*m_callBack)(msg, 0.0, 0.0, m_graph.GetNVertices());
            }
            Material mat;
            if (numCluster < 0)
            {
                for (size_t p = 0; p != m_nClusters; ++p) 
                {
                    if (!uniColor)
                    {
                        mat.m_diffuseColor.X() = mat.m_diffuseColor.Y() = mat.m_diffuseColor.Z() = 0.0;
                        while (mat.m_diffuseColor.X() == mat.m_diffuseColor.Y() ||
                               mat.m_diffuseColor.Z() == mat.m_diffuseColor.Y() ||
                               mat.m_diffuseColor.Z() == mat.m_diffuseColor.X()  )
                        {
                            mat.m_diffuseColor.X() = (rand()%100) / 100.0;
                            mat.m_diffuseColor.Y() = (rand()%100) / 100.0;
                            mat.m_diffuseColor.Z() = (rand()%100) / 100.0;
                        }
                    }
                    m_finalConvexHulls[p].GetMesh().SaveVRML2(fout, mat);
                }
            }
            else if (numCluster < static_cast<long>(m_finalConvexHullsIndices.size()))
            {
                m_finalConvexHulls[numCluster].GetMesh().SaveVRML2(fout, mat);
            }
            fout.close();
            return true;
        }
        else
        {
            if (m_callBack)
            {
                char msg[1024];
                sprintf(msg, "Error saving %s\n", fileName);
                (*m_callBack)(msg, 0.0, 0.0, m_graph.GetNVertices());
            }
            return false;
        }
    }
*/
	bool InitialConvexDecomposion(const Mesh & inputMesh, const size_t nSub[3], std::vector< Mesh * > & convexHulls, CallBackFunction callBack, bool bbCenter)
	{
		if (callBack)
		{
			std::ostringstream msg;
			msg << "+ Mesh" << std::endl;
			msg << "\t # vertices                     \t" << inputMesh.GetNPoints() << std::endl;
			msg << "\t # triangles                    \t" << inputMesh.GetNTriangles() << std::endl;
			msg << "+ Parameters" << std::endl;
			msg << "\t sub				              \t" << nSub[0] << ", " << nSub[1] << ", " << nSub[2] << ", " << std::endl;
			(*callBack)(msg.str().c_str(), 0.0, 0.0, 0);
			(*callBack)("+ Subdividing the mesh \n", 0.0, 0.0, 0);
		}

/*
		{
			btConvexHullComputer ch;
			ch.compute(inputMesh.GetPoints(), 3 * sizeof(Real), inputMesh.GetNPoints(), -1.0, -1.0); 
			Vec3<Real> bary(0.0,0.0,0.0);
			Vec3<Real> pt;
			for(int v = 0; v < ch.vertices.size(); v++)
			{			
				pt.X() = ch.vertices[v].getX();
				pt.Y() = ch.vertices[v].getY();
				pt.Z() = ch.vertices[v].getZ();
				bary += pt;
			}
			bary /= ch.vertices.size();
			long shift = inputMesh.GetNPoints();
			for(int v = 0; v < ch.vertices.size(); v++)
			{			
				pt.X() = ch.vertices[v].getX();
				pt.Y() = ch.vertices[v].getY();
				pt.Z() = ch.vertices[v].getZ();
				inputMesh.AddPoint(bary + (pt-bary) * 1.01);
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
					inputMesh.AddTriangle(Vec3<long>(a+shift, b+shift, c+shift));				
					edge = edge->getNextEdgeOfFace();
					b = c;
					c = edge->getTargetVertex();
				}
			}
			inputMesh.SaveVRML2("C:\\Work\\HACD\\HACD\\data\\test\\inputCH.wrl");
		}
*/


		std::vector< Mesh * > parts;
		parts.resize(1);
		parts[0] = new Mesh;
		(*parts[0]) = inputMesh;
		parts[0]->Center(bbCenter, N_AXIS);
		parts[0]->ComputeBB();

		Vec3<Real> p0 = parts[0]->GetMinBB();
		Vec3<Real> p1 = parts[0]->GetMaxBB();

		for(int i = 0; i < 3; i++)
		{
			Real d = (p1[i] - p0[i]) / nSub[i];		
			std::vector< Mesh * > input;
			std::swap(input, parts);
			
			for(size_t p = 0; p < input.size(); ++p)
			{
				Mesh * mesh = input[p];
				Real r = p0[i] + d;
				int j = 0;
				int it = 0;
				Mesh * left = 0;
				Mesh * right = 0;				
				while ( r < p1[i])
				{
					left  = new Mesh;
					right = new Mesh;				
					if ( i == 0)
					{
						mesh->Clip(1.0, 0.0, 0.0, -r, right, left, true);
					}
					else if ( i == 1)
					{
						mesh->Clip(0.0, 1.0, 0.0, -r, right, left, true);
					}
					else
					{
						mesh->Clip(0.0, 0.0, 1.0, -r, right, left, true);
					}
					left->CleanDuplicatedVectices();
					right->CleanDuplicatedVectices();
#ifdef DEBUG_VHACD
					left->SaveVRML2("C:\\Work\\HACD\\HACD\\data\\test\\left.wrl");
					right->SaveVRML2("C:\\Work\\HACD\\HACD\\data\\test\\right.wrl");
#endif
					it++;
					if (left->GetNPoints() > 0 )
					{
						if (i == 3)
						{
							if (parts.size() == 0)
							{
								parts.push_back(right);
							}
						}
						else
						{
							parts.push_back(left);
						}
					}
					else
					{
						delete left;
					}
					delete mesh;
					mesh = right;
					if (!mesh)
					{
						break;
					}
					r += d;
				}
				if (right && right->GetNPoints() > 0 )
				{
					if (i == 3)
					{
						if (parts.size() == 0)
						{
							parts.push_back(right);
						}
					}
					else
					{
						parts.push_back(right);
					}
					
				}
				j++;
			}

			char fileNameOut[1024];
			sprintf(fileNameOut, "C:\\Work\\HACD\\HACD\\data\\test\\parts_%i.wrl", i);
			std::ofstream fout(fileNameOut);
			if (fout.is_open())
			{

				for(size_t p = 0; p < parts.size(); ++p)
				{
/*
			char fileNameOut[1024];
			sprintf(fileNameOut, "C:\\Work\\HACD\\HACD\\data\\test\\parts_%i_%i.wrl", i, p);
			std::ofstream fout(fileNameOut);
			if (fout.is_open())
			{
*/
					Material mat;
					mat.m_diffuseColor.X() = mat.m_diffuseColor.Y() = mat.m_diffuseColor.Z() = 0.0;
					while (mat.m_diffuseColor.X() == mat.m_diffuseColor.Y() ||
						   mat.m_diffuseColor.Z() == mat.m_diffuseColor.Y() ||
						   mat.m_diffuseColor.Z() == mat.m_diffuseColor.X()  )
					{
						mat.m_diffuseColor.X() = (rand()%100) / 100.0;
						mat.m_diffuseColor.Y() = (rand()%100) / 100.0;
						mat.m_diffuseColor.Z() = (rand()%100) / 100.0;
					}
					parts[p]->SaveVRML2(fout, mat);

//			fout.close();

				}

			}			
			fout.close();
		}
/*
		char fileNameOut[1024] = "C:\\Work\\HACD\\HACD\\data\\test\\parts.wrl";
		std::ofstream fout(fileNameOut);
		if (fout.is_open())
		{
			for(size_t p = 0; p < parts.size(); ++p)
			{
				Material mat;
				mat.m_diffuseColor.X() = mat.m_diffuseColor.Y() = mat.m_diffuseColor.Z() = 0.0;
				while (mat.m_diffuseColor.X() == mat.m_diffuseColor.Y() ||
					   mat.m_diffuseColor.Z() == mat.m_diffuseColor.Y() ||
					   mat.m_diffuseColor.Z() == mat.m_diffuseColor.X()  )
				{
					mat.m_diffuseColor.X() = (rand()%100) / 100.0;
					mat.m_diffuseColor.Y() = (rand()%100) / 100.0;
					mat.m_diffuseColor.Z() = (rand()%100) / 100.0;
				}
				parts[p]->SaveVRML2(fout, mat);
			}
		}
		fout.close();
*/


		if (callBack) (*callBack)("+ Computing convex-hulls \n", 0.0, 0.0, 0);

		long * v2CC = 0;
		long * mapping = 0;
		size_t nV = 0;
		for (size_t p = 0; p != parts.size(); ++p) 
		{
			if (nV < parts[p]->GetNPoints())
			{
				nV = parts[p]->GetNPoints();
				delete [] v2CC;
				delete [] mapping;
				v2CC = new long[nV];
				mapping = new long[nV];
			}
			size_t nCCs = parts[p]->ComputeConnectedComponents(v2CC);
			if (nCCs > 1)
			{
				for(size_t n = 0; n < nCCs; n++)
				{
					Mesh CC;
					parts[p]->ExtractConnectedComponent(n, v2CC, mapping, CC);
					btConvexHullComputer ch;
					ch.compute(CC.GetPoints(), 3 * sizeof(Real), CC.GetNPoints(), -1.0, -1.0); 

					Mesh * mesh = new Mesh;
					convexHulls.push_back(mesh);

					for(int v = 0; v < ch.vertices.size(); v++)
					{			
						mesh->AddPoint(Vec3<Real>(ch.vertices[v].getX(), ch.vertices[v].getY(), ch.vertices[v].getZ()));
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
							mesh->AddTriangle(Vec3<long>(a, b, c));				
							edge = edge->getNextEdgeOfFace();
							b = c;
							c = edge->getTargetVertex();
						}
					}
				}
			}
			else
			{
				btConvexHullComputer ch;
				ch.compute(parts[p]->GetPoints(), 3 * sizeof(Real), parts[p]->GetNPoints(), -1.0, -1.0); 
				Mesh * mesh = new Mesh;
				convexHulls.push_back(mesh);
				for(int v = 0; v < ch.vertices.size(); v++)
				{			
					mesh->AddPoint(Vec3<Real>(ch.vertices[v].getX(), ch.vertices[v].getY(), ch.vertices[v].getZ()));
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
						mesh->AddTriangle(Vec3<long>(a, b, c));				
						edge = edge->getNextEdgeOfFace();
						b = c;
						c = edge->getTargetVertex();
					}
				}
			}
			delete parts[p];
		}
		delete [] v2CC;
		delete [] mapping;
		return true;
	}

	bool SaveConvexHulls(const std::string & fileName, const std::vector< Mesh * > & convexHulls)
	{
		std::ofstream foutCH(fileName.c_str());
		if (foutCH.is_open())
		{
			for(size_t p = 0; p < convexHulls.size(); ++p)
			{
				Material mat;
				mat.m_diffuseColor.X() = mat.m_diffuseColor.Y() = mat.m_diffuseColor.Z() = 0.0;
				while (mat.m_diffuseColor.X() == mat.m_diffuseColor.Y() ||
					   mat.m_diffuseColor.Z() == mat.m_diffuseColor.Y() ||
					   mat.m_diffuseColor.Z() == mat.m_diffuseColor.X()  )
				{
					mat.m_diffuseColor.X() = (rand()%100) / 100.0;
					mat.m_diffuseColor.Y() = (rand()%100) / 100.0;
					mat.m_diffuseColor.Z() = (rand()%100) / 100.0;
				}
				convexHulls[p]->SaveVRML2(foutCH, mat);
			}
			foutCH.close();
			return true;
		}
		else
		{
			return false;
		}	
	}


	bool ApproximateConvexDecomposition(const Mesh & inputMesh, const std::vector< Mesh * > & convexHulls, int depth, int posSampling, int angleSampling, CallBackFunction callBack)
	{
		if (callBack)
		{
			std::ostringstream msg;
			msg << "+ Mesh" << std::endl;
			msg << "\t # vertices                     \t" << inputMesh.GetNPoints() << std::endl;
			msg << "\t # triangles                    \t" << inputMesh.GetNTriangles() << std::endl;
			msg << "+ Initial Decompositon" << std::endl;
			msg << "\t # convex-hulls                     \t" << convexHulls.size() << std::endl;
			msg << "+ Parameters" << std::endl;
			msg << "\t depth			              \t" << depth << std::endl;
			msg << "\t # of position samples          \t" << posSampling << std::endl;
			msg << "\t # of angle samples	          \t" << depth << std::endl;
			(*callBack)(msg.str().c_str(), 0.0, 0.0, 0);
			(*callBack)("+ Subdividing the mesh \n", 0.0, 0.0, 0);
		}
		Vec3< Real > p0 = inputMesh.GetMinBB();
		Vec3< Real > p1 = inputMesh.GetMaxBB();
		Vec3< Real > c0(0.0,0.0,0.0);//= inputMesh.GetCenter();
		Real s = (p1-p0).GetNorm();
		
		Real deltaPos = 1.0 / posSampling;

		Vec3< Real > p = c0;
		Real a, b, c, d;
		std::vector< Mesh * > cluster0;
		std::vector< Mesh * > cluster1;
		const size_t nCH = convexHulls.size();
		cluster0.reserve(nCH);
		cluster1.reserve(nCH);
		for(size_t p = 0; p < nCH; ++p)
		{
			convexHulls[p]->ComputeBB();
		}
		printf("p0(%f, %f, %f)\t p1(%f, %f, %f)\n", (float)p0.X(), (float)p0.Y(), (float)p0.Z(), (float)p1.X(), (float)p1.Y(), (float)p1.Z());

		Real e;
		p = c0;

		Vec3<Real> b0(1.0, 0.0, 0.0);
		Vec3<Real> b1(0.0, 1.0, 0.0);
		Vec3<Real> b2(0.0, 0.0, 1.0);
		
		const Real inc = PI * (3.0 - sqrt(5.0));
		const Real off = 2.0 / angleSampling;
		Real phi, y, r;
		Real t = 0.0;

		
		inputMesh.SaveVRML2("C:\\Work\\HACD\\HACD\\data\\test\\dec\\input.wrl");


		for(int i=0; i <= posSampling; ++i)
		{
			
			p.Z() = p0.Z() * (1.0-t) + t * p1.Z();
			for(int j=0; j < angleSampling; ++j)
			{
				y = j * off - 1.0 + (off / 2.0);
				r = sqrt(1.0 - y*y);
				phi = angleSampling * inc;
				a = cos(phi)*r;
				b = y;
				c = sin(phi)*r;
				d = -(a * p.X() + b * p.Y() + c * p.Z());
				e = a*p.X() + b*p.Y() + c*p.Z() + d;

				Vec3<Real> n(a, b, c);
				Vec3<Real> u = b0 - n * (n*b0);
				if (u.GetNorm() < EPSILON)
				{
					u = b1 - n * (n*b1);
					if (u.GetNorm() < EPSILON)
					{
						u = b2 - n * (n*b2);
					}
				}
				Vec3<Real> v = u ^ n;

				Mesh plane;
				plane.AddPoint(p + s * u);
				plane.AddPoint(p - s * u);
				plane.AddPoint(p + s * v);
				plane.AddPoint(p - s * v);
				plane.AddPoint(p);
				plane.AddTriangle(Vec3<long>(1,4,2));
				plane.AddTriangle(Vec3<long>(2,4,0));
				plane.AddTriangle(Vec3<long>(4,0,3));
				plane.AddTriangle(Vec3<long>(4,3,1));
				plane.AddTriangle(Vec3<long>(4,1,2));
				plane.AddTriangle(Vec3<long>(4,2,0));
				plane.AddTriangle(Vec3<long>(0,4,3));
				plane.AddTriangle(Vec3<long>(3,4,1));
				


				printf("{%i, %i} \t p(%f, %f, %f)\t (%f, %f, %f, %f)\t e=%f\n", i, j, (float)p.X(), (float)p.Y(), (float)p.Z(), (float)a, (float)b, (float)c, (float)d, (float)e);

				cluster0.clear();
				cluster1.clear();

				for(size_t p = 0; p < nCH; ++p)
				{
					const Vec3< Real > & center = convexHulls[p]->GetCenter();
					e = a*center.X() + b*center.Y() + c*center.Z() + d;

					if (e > 0.0)
					{
						cluster0.push_back(convexHulls[p]);
					}
					else
					{
						cluster1.push_back(convexHulls[p]);
					}
				}
				cluster1.push_back(&plane);
				cluster0.push_back(&plane);
				char fileName[1024];
				sprintf(fileName, "C:\\Work\\HACD\\HACD\\data\\test\\dec\\plane_%03i_%03i.wrl", i, j);
				plane.SaveVRML2(fileName);
				sprintf(fileName, "C:\\Work\\HACD\\HACD\\data\\test\\dec\\c0_%03i_%03i.wrl", i, j);
				SaveConvexHulls(fileName, cluster0);
				sprintf(fileName, "C:\\Work\\HACD\\HACD\\data\\test\\dec\\c1_%03i_%03i.wrl", i, j);
				SaveConvexHulls(fileName, cluster1);
			}
			t += deltaPos;
		}
		return true;
	}

	bool ComputeClipPlanes(Real minD, Real maxD, int posSampling, int angleSampling, std::set< Plane > & planes)
	{
		Real a, b, c, d;
		const Real inc = PI * (3.0 - sqrt(5.0));
		const Real off = 1.0 / angleSampling;
		Real phi, y, r;
		Real deltaPos = 1.0 / posSampling;
		Real t = 0.0;
		for(int i=0; i <= posSampling; ++i)
		{	
			d = (1.0 - t) * minD + t * maxD;
			t += deltaPos;

			planes.insert(Plane(1.0, 0.0, 0.0, d));
			planes.insert(Plane(0.0, 1.0, 0.0, d));
			planes.insert(Plane(0.0, 0.0, 1.0, d));
			
			for(int j=0; j < angleSampling; ++j)
			{
				y = j * off - 1.0 + (off / 2.0);
				r = sqrt(1.0 - y*y);
				phi = j * inc;
				a = cos(phi)*r;
				b = y;
				c = sin(phi)*r;
				planes.insert(Plane(a, b, c, d));																					
			}			
		}
		return true;
	}

	bool RefineClipPlanes(Plane p0, Real minD, Real maxD, int posSampling, int angleSampling, std::set< Plane > & planes)
	{
		Real a, b, c, d;
		const Real inc = PI * (3.0 - sqrt(5.0));
		const Real off = 1.0 / angleSampling;
		Real phi, y, r;
		Real deltaPos = (maxD-minD)  / posSampling;
		for(int i=-posSampling; i <= posSampling; ++i)
		{	
			d = p0.m_d + i * deltaPos;
			planes.insert(Plane(1.0, 0.0, 0.0, d));
			planes.insert(Plane(0.0, 1.0, 0.0, d));
			planes.insert(Plane(0.0, 0.0, 1.0, d));

			for(int j=0; j < angleSampling; ++j)
			{
				y = j * off - 1.0 + (off / 2.0);
				r = sqrt(1.0 - y*y);
				phi = j * inc;
				a = cos(phi)*r;
				b = y;
				c = sin(phi)*r;
				planes.insert(Plane(a, b, c, d));
			}			
		}
		return true;
	}

	void ComputeBestClippingPlane(const Mesh & inputMesh, std::set< Plane > & planes, 
		Plane & bestPlane, 
		Mesh & bestLeft, 
		Mesh & bestRight,
		Real & minConcavity,
		Real & minBalance,
		Real alpha,
		CallBackFunction callBack, bool debug)
	{
		std::set< Plane >::const_iterator itEnd = planes.end();
		std::set< Plane >::const_iterator itBegin = planes.begin();
		int i = 0;
		int iBest;
		char fileName[1024];
		Plane plane;
		Real balance, concavity;

		Real volume = inputMesh.ComputeVolume();
		printf("Nunber of clipping planes %i\n", planes.size());
		size_t nV0 = inputMesh.GetNPoints();
		long * v2CCLeft = new long[nV0];
		long * v2CCRight = new long[nV0];
		long * mapping = new long[nV0];
		size_t nV;


		for (std::set< Plane >::const_iterator it = itBegin; it != itEnd; ++it)
		{
			plane  = (*it);
			Mesh left;
			Mesh right;				
			inputMesh.Clip(plane.m_a, plane.m_b, plane.m_c, plane.m_d, &right, &left, true);	

			nV = right.GetNPoints() + left.GetNPoints();
			if ( nV > nV0)
			{
				nV0 = nV;
				delete [] v2CCLeft;
				delete [] v2CCRight;
				delete [] mapping;
				v2CCLeft  = new long[nV0];
				v2CCRight = new long[nV0];
				mapping   = new long[nV0];
			}

			right.CleanDuplicatedVectices();
			left.CleanDuplicatedVectices();

			Mesh leftCH;
			Mesh rightCH;

			// Extract CCs of left
			size_t nCCRight = right.ComputeConnectedComponents(v2CCRight);
			size_t nCCLeft  = left.ComputeConnectedComponents(v2CCLeft);
			size_t nCC      = nCCRight + nCCLeft;
			
			if ( nCCLeft > 1 || nCCRight > 1)
			{
				std::vector< Mesh * > parts;
				std::vector< Mesh > CCs;
				parts.resize(nCC);
				CCs.resize(nCC);

				for(size_t n = 0; n < nCC; n++)
				{
					parts[n] = new Mesh;
				}

				for(size_t n = 0; n < nCCLeft; n++)
				{
					left.ExtractConnectedComponent(n, v2CCLeft, mapping, CCs[n+nCCRight]);
					CCs[n+nCCRight].ComputeConvexHull(*(parts[n+nCCRight]));
					if (debug)
					{
//						sprintf(fileName, "C:\\Work\\HACD\\HACD\\data\\test\\dec\\left_%06i_CC_%i.wrl", i, n);
//						parts[n+nCCRight]->SaveVRML2(fileName);
					}
				}

				for(size_t n = 0; n < nCCRight; n++)
				{
					right.ExtractConnectedComponent(n, v2CCRight, mapping, CCs[n]);
					CCs[n].ComputeConvexHull(*(parts[n]));
					if (debug)
					{
//						sprintf(fileName, "C:\\Work\\HACD\\HACD\\data\\test\\dec\\right_%06i_CC_%i.wrl", i, n);
//						parts[n]->SaveVRML2(fileName);
					}
				}

				VHACD vhacd;

				vhacd.SetNClusters(2);										// minimum number of clusters
				vhacd.SetConcavity(std::numeric_limits<double>::max());     // maximum concavity
				vhacd.SetCallBack(callBack);
				vhacd.SetInitialConvexHulls(& parts);

				vhacd.Init();

				for(size_t n1 = 0; n1 < nCCRight; n1++)
				{
					for(size_t n2 = n1+1; n2 < nCCRight; n2++)
					{
						vhacd.AddEdge(n1, n2);
					}
				}
				for(size_t n1 = nCCRight; n1 < nCC; n1++)
				{
					for(size_t n2 = n1+1; n2 < nCC; n2++)
					{
						vhacd.AddEdge(n1, n2);
					}
				}
				vhacd.Compute();

				const std::vector< long > &	 res = vhacd.GetFinalConvexHullsIndices();
				const SArray<long, SARRAY_DEFAULT_MIN_SIZE> &  ancestors0 = vhacd.GetVertexAncestors(res[0]);
				const SArray<long, SARRAY_DEFAULT_MIN_SIZE> &  ancestors1 = vhacd.GetVertexAncestors(res[1]);
				
				left  = CCs[res[0]];
				right = CCs[res[1]];
				Vec3< long > tri;
				for(size_t n = 0; n < ancestors0.Size(); ++n)
				{
					size_t shiftPts = left.GetNPoints();
					size_t shiftTris = left.GetNTriangles();
					const Mesh & mesh = CCs[ancestors0[n]];
					size_t nPts  = mesh.GetNPoints();
					size_t nTris = mesh.GetNTriangles();
					left.ResizePoints(shiftPts   + nPts);
					left.ResizeTriangles(shiftTris + nTris);
					for(size_t v = 0; v < nPts; ++v)
					{
						left.SetPoint(v + shiftPts, mesh.GetPoint(v)); 
					}
					for(size_t t = 0; t < nTris; ++t)
					{
						tri = mesh.GetTriangle(t);
						tri += shiftPts;
						left.SetTriangle(t + shiftTris, tri); 
					}
				}
				for(size_t n = 0; n < ancestors1.Size(); ++n)
				{
					size_t shiftPts = right.GetNPoints();
					size_t shiftTris = right.GetNTriangles();
					const Mesh & mesh = CCs[ancestors1[n]];
					size_t nPts  = mesh.GetNPoints();
					size_t nTris = mesh.GetNTriangles();
					right.ResizePoints(shiftPts   + nPts);
					right.ResizeTriangles(shiftTris + nTris);
					for(size_t v = 0; v < nPts; ++v)
					{
						right.SetPoint(v + shiftPts, mesh.GetPoint(v)); 
					}
					for(size_t t = 0; t < nTris; ++t)
					{
						tri = mesh.GetTriangle(t);
						tri += shiftPts;
						right.SetTriangle(t + shiftTris, tri); 
					}
				}
				for(size_t n = 0; n < nCC; n++)
				{
					delete parts[n];
				}
				if (ancestors1.Size() > 0)
				{
					right.CleanDuplicatedVectices();
				}
				if (ancestors0.Size() > 0)
				{
					left.CleanDuplicatedVectices();
				}
			}

			if (left.GetNPoints() > 0)
			{
				left.ComputeConvexHull(leftCH);
			}
			if (right.GetNPoints() > 0)
			{
				right.ComputeConvexHull(rightCH);
			}

			Real volumeLeft    = left.ComputeVolume();
			Real volumeRight   = right.ComputeVolume();
			
			Real volumeLeftCH  = 0.0;
			Real volumeRightCH = 0.0;

			if (leftCH.GetNPoints() > 0)
			{
				volumeLeftCH  = leftCH.ComputeVolume();
			}
			if (rightCH.GetNPoints() > 0)
			{
				volumeRightCH = rightCH.ComputeVolume();
			}
				
			concavity = volumeLeftCH + volumeRightCH - volume;
			balance = fabs(volumeLeft-volumeRight);
			if (debug)
			{
//				printf("-> V=%f \t VL=%f \t VR=%f (%f) \n \t VL_CH=%f \t VR_CH=%f C = %f\n", volume, volumeLeft, volumeRight, volume - volumeLeft - volumeRight, volumeLeftCH, volumeRightCH, concavity);

				printf("-> C=%f \t B=%f \t E=%f\n", concavity, balance, volume - volumeLeft - volumeRight);

				sprintf(fileName, "C:\\Work\\HACD\\HACD\\data\\test\\dec\\left_%06i.wrl", i);
				left.SaveVRML2(fileName);

				sprintf(fileName, "C:\\Work\\HACD\\HACD\\data\\test\\dec\\right_%06i.wrl", i);
				right.SaveVRML2(fileName);		

				sprintf(fileName, "C:\\Work\\HACD\\HACD\\data\\test\\dec\\leftCH_%06i.wrl", i);
				leftCH.SaveVRML2(fileName);

				sprintf(fileName, "C:\\Work\\HACD\\HACD\\data\\test\\dec\\rightCH_%06i.wrl", i);
				rightCH.SaveVRML2(fileName);		
			}
			if (concavity + alpha * balance <  minConcavity + alpha * minBalance)
			{
				bestPlane    = plane;
				minConcavity = concavity;
				iBest	     = i;
				bestLeft	 = left;
				bestRight	 = right;
				minBalance   = balance;
			}
			i++;
		}
		delete [] v2CCRight;
		delete [] v2CCLeft;
		delete [] mapping;
		printf("Best %i \t C=%f \t B=%f \t (%f, %f, %f, %f)\n", iBest,  minConcavity, minBalance, bestPlane.m_a, bestPlane.m_b, bestPlane.m_c, bestPlane.m_d);
	}

	bool ApproximateConvexDecomposition(const Mesh & inputMesh, int depth, 
										int posSampling, int angleSampling,
										int posRefine, int angleRefine,
										Real alpha, CallBackFunction callBack)
	{
		if (callBack)
		{
			std::ostringstream msg;
			msg << "+ Mesh" << std::endl;
			msg << "\t # vertices                     \t" << inputMesh.GetNPoints() << std::endl;
			msg << "\t # triangles                    \t" << inputMesh.GetNTriangles() << std::endl;
			msg << "+ Parameters" << std::endl;
			msg << "\t depth			              \t" << depth << std::endl;
			msg << "\t # of position samples          \t" << posSampling << std::endl;
			msg << "\t # of angle samples	          \t" << depth << std::endl;
			(*callBack)(msg.str().c_str(), 0.0, 0.0, 0);
			(*callBack)("+ Subdividing the mesh \n", 0.0, 0.0, 0);
		}
		
		inputMesh.SaveVRML2("C:\\Work\\HACD\\HACD\\data\\test\\dec\\input.wrl");

		Vec3< Real > p0 = inputMesh.GetMinBB();
		Vec3< Real > p1 = inputMesh.GetMaxBB();
		Real minD = std::min(std::min(p0.X(), p0.Y()), p0.Z());
		Real maxD = std::max(std::max(p1.X(), p1.Y()), p1.Z());



		std::set< Plane > planes;
		ComputeClipPlanes(minD, maxD, posSampling, angleSampling, planes);
		Plane bestPlane;
		Mesh  bestLeft;
		Mesh  bestRight;
		Real minConcavity = std::numeric_limits<double>::max();
		Real minBalance = std::numeric_limits<double>::max();

		ComputeBestClippingPlane(inputMesh, planes, bestPlane, bestLeft, bestRight, minConcavity,  minBalance, alpha, callBack, false);

		char fileName[1024];
		sprintf(fileName, "C:\\Work\\HACD\\HACD\\data\\test\\dec\\best_left.wrl");
		bestLeft.SaveVRML2(fileName);

		sprintf(fileName, "C:\\Work\\HACD\\HACD\\data\\test\\dec\\best_right.wrl");
		bestRight.SaveVRML2(fileName);		

		Real delta   = (maxD - minD) / (posSampling);
		Real minDRef = bestPlane.m_d - delta;
		Real maxDRef = bestPlane.m_d + delta;
		std::set< Plane > planesRef;

		RefineClipPlanes(bestPlane, minDRef, maxDRef, posRefine, angleRefine * angleSampling, planesRef);
		ComputeBestClippingPlane(inputMesh, planesRef, bestPlane, bestLeft, bestRight, minConcavity,  minBalance, alpha, callBack, false);
		sprintf(fileName, "C:\\Work\\HACD\\HACD\\data\\test\\dec\\best_ref_left.wrl");
		bestLeft.SaveVRML2(fileName);

		sprintf(fileName, "C:\\Work\\HACD\\HACD\\data\\test\\dec\\best_ref_right.wrl");
		bestRight.SaveVRML2(fileName);		


		return true;
	}
}
