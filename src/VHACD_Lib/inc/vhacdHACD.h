/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#ifndef VHACD_VHACD_H
#define VHACD_VHACD_H
#include <VHACDVersion.h>
#include <VHACDVector.h>
#include <VHACDGraph.h>
#include <VHACDMesh.h>
#include <set>
#include <vector>
#include <queue>

namespace VHACD
{
	static const Real EPSILON = 0.0000000001;
	static const Real PI      = 3.14159265359;
	class VHACD;


    struct Sphere  
    {
		Real                                                      m_radius;
		Vec3< Real >                                              m_center;
    };

	class Plane
	{
	public:
		Real										m_a;
		Real										m_b;
		Real										m_c;
		Real										m_d;
													Plane(Real a = 1.0, Real b = 0.0, Real c = 0.0, Real d = 0.0)
													{
														m_a = a;
														m_b = b;
														m_c = c;
														m_d = d;
													}
			//! Destructor
													~Plane(void){}

        friend bool		                            operator<(const Plane & lhs, const Plane & rhs);
        friend bool	                                operator>(const Plane & lhs, const Plane & rhs);
    };
    inline bool										operator<(const Plane & lhs, const Plane & rhs)
													{
														if (lhs.m_a == rhs.m_a)
														{
															if (lhs.m_b == rhs.m_b)
															{
																if (lhs.m_c == rhs.m_c)
																{
																	return (lhs.m_d<rhs.m_d);
																}													
																return (lhs.m_c<rhs.m_c);
															}													
															return (lhs.m_b<rhs.m_b);
														}													
														return (lhs.m_a<rhs.m_a);
													}
    inline bool										operator>(const Plane & lhs, const Plane & rhs)
													{
														if (lhs.m_a == rhs.m_a)
														{
															if (lhs.m_b == rhs.m_b)
															{
																if (lhs.m_c == rhs.m_c)
																{
																	return (lhs.m_d>rhs.m_d);
																}													
																return (lhs.m_c>rhs.m_c);
															}													
															return (lhs.m_b>rhs.m_b);
														}													
														return (lhs.m_a>rhs.m_a);
													}


	// just to be able to set the capcity of the container	
	template<class _Ty, class _Container = std::vector<_Ty>, class _Pr = std::less<typename _Container::value_type> >
	class reservable_priority_queue: public std::priority_queue<_Ty, _Container, _Pr> 
	{
        typedef typename std::priority_queue<_Ty, _Container, _Pr>::size_type size_type;
	public:
                                                    reservable_priority_queue(size_type capacity = 0) { reserve(capacity); };
		void										reserve(size_type capacity) { this->c.reserve(capacity); } 
        size_type									capacity() const { return this->c.capacity(); } 
	};
	
	//! priority queque element
	class VHACD;

    class ElementPriorityQueue
    {
		public:
			long								   GetName() const { return m_name; }
			//! Constructor
			//! @param name edge's id
			//! @param priority edge's priority
													ElementPriorityQueue(long name, Real priority)
													{
														m_name = name;
														m_priority = priority;
													}
			//! Destructor
													~ElementPriorityQueue(void){}
		private:
			long									m_name;						//!< edge name
			Real                                    m_priority;					//!< priority
		//! Operator < for GraphEdgePQ
        friend bool                                 operator<(const ElementPriorityQueue & lhs, const ElementPriorityQueue & rhs);
		//! Operator > for GraphEdgePQ
        friend bool                                 operator>(const ElementPriorityQueue & lhs, const ElementPriorityQueue & rhs);
		friend class VHACD;
    };
    inline bool										operator<(const ElementPriorityQueue & lhs, const ElementPriorityQueue & rhs)
													{
														return (lhs.m_priority<rhs.m_priority);
													}
    inline bool										operator>(const ElementPriorityQueue & lhs, const ElementPriorityQueue & rhs)
													{
														return lhs.m_priority>rhs.m_priority;
													}
    typedef void (*CallBackFunction)(const char *);

	//! Provides an implementation of the Volumetric Hierarchical Approximate Convex Decomposition (VHACD) technique
    class VHACD
	{            
    public:

		const std::vector< long > &					GetFinalConvexHullsIndices() const  { return m_finalConvexHullsIndices; }
		void										SetInitialConvexHulls(std::vector< Mesh * > * convexHulls) { m_convexHulls = convexHulls;}
		const std::vector< Mesh * > *               GetInitialConvexHulls() const { return m_convexHulls;}
		const SArray<long, SARRAY_DEFAULT_MIN_SIZE>& GetVertexAncestors(long v) { return m_graph.m_vertices[v].m_ancestors;}



		//! Sets the call-back function
		//! @param callBack pointer to the call-back function
		void										SetCallBack(CallBackFunction  callBack) { m_callBack = callBack;}
		//! Gives the call-back function
		//! @return pointer to the call-back function
		const CallBackFunction                      GetCallBack() const { return m_callBack;}
        
		//! Sets the minimum number of clusters to be generated.
		//! @param nClusters minimum number of clusters
		void										SetNClusters(size_t nClusters) { m_nMinClusters = nClusters;}
		//! Gives the number of generated clusters.
		//! @return number of generated clusters
		const size_t								GetNClusters() const { return m_nClusters;}
		//! Sets the maximum allowed concavity.
		//! @param concavity maximum concavity
		void										SetConcavity(double concavity) { m_concavity = concavity;}
		//! Gives the maximum allowed concavity.
		//! @return maximum concavity
		double                                      GetConcavity() const { return m_concavity;}

		//! Sets the maximum number of vertices for each generated convex-hull.
		//! @param nVerticesPerCH maximum # vertices per CH
        void										SetNVerticesPerCH(size_t nVerticesPerCH) { m_nVerticesPerCH = nVerticesPerCH;}
		
		//! Gives the maximum number of vertices for each generated convex-hull.
		//! @return maximum # vertices per CH
		const size_t								GetNVerticesPerCH() const { return m_nVerticesPerCH;}
		
		//! Gives the number of vertices for the cluster number numCH.
		//! @return number of vertices
//		size_t                                      GetNPointsCH(size_t numCH) const;

		//! Gives the number of triangles for the cluster number numCH.
		//! @param numCH cluster's number
		//! @return number of triangles
//        size_t                                      GetNTrianglesCH(size_t numCH) const;

		//! Gives the vertices and the triangles of the cluster number numCH.
		//! @param numCH cluster's number
		//! @param points pointer to the vector of points to be filled
		//! @param triangles pointer to the vector of triangles to be filled
		//! @return true if sucess
//        bool                                        GetCH(size_t numCH, Vec3<Real> * const points, Vec3<long> * const triangles);     

		//! Computes the VHACD decomposition.
		//! @param convexHulls vector of initial convex-hulls each defined as a seperate mesh
		//! @return true if sucess
        bool										Compute();

		//! Saves the generated convex-hulls in a VRML 2.0 file.
		//! @param fileName the output file name
		//! @param uniColor specifies whether the different convex-hulls should have the same color or not
        //! @param numCluster specifies the cluster to be saved, if numCluster < 0 export all clusters
        //! @return true if sucess
//		bool										Save(const char * fileName, bool uniColor, long numCluster=-1) const;
		
		//!
		bool										Init();

		//!
		void										AddEdge(long v1, long v2);

		//! Destructor.
													~VHACD(void);

        //! Constructor.
													VHACD();
	private:

		//! Creates the Graph by associating to each mesh triangle a vertex in the graph and to each couple of adjacent triangles an edge in the graph.
        void										CreateGraph();

		//! Computes the cost of an edge
		//! @param e edge's id
        void                                        ComputeEdgeCost(size_t e);

		//! Initializes the priority queue
		//! @param fast specifies whether fast mode is used
		//! @return true if success
        bool                                        InitializePriorityQueue();

        //! Computes final convex-hulls
        //! @param fullCH specifies whether to generate convex-hulls with a full or limited (i.e. < m_nVerticesPerCH) number of vertices
		void										ComputeConvexHulls(bool fullCH);


		void										Simplify();

	private:

		CallBackFunction							m_callBack;					//>! call-back function
        size_t										m_nClusters;				//>! number of clusters
        size_t										m_nMinClusters;				//>! minimum number of clusters
        double										m_concavity;				//>! maximum concavity
        double										m_diag;						//>! length of the BB diagonal
//        ICHull *                                    m_finalConvexHulls;  		//>! convex-hulls associated with the final VHACD clusters
		Graph										m_graph;					//>! simplification graph
        size_t                                      m_nVerticesPerCH;			//>! maximum number of vertices per convex-hull
		std::vector< long >							m_finalConvexHullsIndices;  //>! indices of the final clusters
		std::vector< Mesh * > *						m_convexHulls;			    //>! all convex-hulls
		
		reservable_priority_queue<ElementPriorityQueue, 
            std::vector<ElementPriorityQueue>,
			std::greater<std::vector<ElementPriorityQueue>::value_type> > m_pqueue;		//!> priority queue
													VHACD(const VHACD & rhs);
	};
	bool InitialConvexDecomposion(const Mesh & inputMesh, const size_t nSub[3], std::vector< Mesh * > & convexHulls, CallBackFunction callBack = 0, bool bbCenter=true);
	bool SaveConvexHulls(const std::string & fileName, const std::vector< Mesh * > & convexHulls);
	bool ApproximateConvexDecomposition(const Mesh & inputMesh, const std::vector< Mesh * > & convexHulls, int depth, int posSampling, int angleSampling, CallBackFunction callBack);
	bool ApproximateConvexDecomposition(const Mesh & inputMesh, int depth, 
										int posSampling, int angleSampling,
										int posRefine, int angleRefine, 
                                        Real alpha, Real concavityThreshold,
                                        std::vector< Mesh * > & parts, CallBackFunction callBack);
}
#endif