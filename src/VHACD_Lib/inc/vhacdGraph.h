/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#ifndef VHACD_GRAPH_H
#define VHACD_GRAPH_H
#include <VHACDVersion.h>
#include <VHACDVector.h>
#include <map>
#include <vector>
#include <VHACDSArray.h>
#include <VHACDMesh.h>

namespace VHACD
{
    class GraphVertex;
    class GraphEdge;
    class Graph;
	class VHACD;
       
    class GraphVertex  
    {
    public:
        bool                                                      AddEdge(long name) 
                                                                  { 
                                                                    m_edges.Insert(name); 
                                                                    return true; 
                                                                  }
        bool                                                      DeleteEdge(long name);        
                                                                  GraphVertex();
                                                                  ~GraphVertex(){};      
    private:
        long                                                      m_name;
        SArray<long, SARRAY_DEFAULT_MIN_SIZE>                     m_edges;
        bool                                                      m_deleted;
		SArray<long, SARRAY_DEFAULT_MIN_SIZE>                     m_ancestors;

		Real                                                      m_concavity;
        double                                                    m_volume;
      

        friend class GraphEdge;
        friend class Graph;
		friend class VHACD;
    };
    
	class GraphEdge 
    {
    public:
                                                                 GraphEdge();
                                                                 ~GraphEdge()
                                                                 {
                                                                    delete m_convexHull;
                                                                 };
    private:
        long                                                     m_name;
        long                                                     m_v1;
        long                                                     m_v2;
        double                                                   m_concavity;
        Real                                                     m_volume;
        Mesh *                                                   m_convexHull;
        bool                                                     m_deleted;
        
        friend class GraphVertex;
        friend class Graph;
		friend class VHACD;
    };
    
    class Graph  
    {
    public:
		size_t									                 GetNEdges() const { return m_nE;}
		size_t									                 GetNVertices() const { return m_nV;}
        bool                                                     EdgeCollapse(long v1, long v2);
        long                                                     AddVertex();
        long                                                     AddEdge(long v1, long v2);
        bool                                                     DeleteEdge(long name);	
        bool                                                     DeleteVertex(long name);
        long                                                     GetEdgeID(long v1, long v2) const;
		void									                 Clear();
        void                                                     Print() const;
        
                                                                 Graph();
        virtual                                                  ~Graph();      
		void									                 Allocate(size_t nV, size_t nE);

    private:
        size_t                                                   m_nCCs;
        size_t                                                   m_nV;
        size_t                                                   m_nE;
        std::vector<GraphEdge>                                   m_edges;
        std::vector<GraphVertex>                                 m_vertices;

		friend class VHACD;
    };
}
#endif