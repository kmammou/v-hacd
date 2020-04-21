#ifndef MESH_BUILDER_H
#define MESH_BUILDER_H

#include <stdint.h>

// Simple code snippet to accept raw triangles positions and compute
// vertex numbers and texture-co-ordinates for them.
namespace MESH_BUILDER
{

class MeshBuilder
{
public:
	class Vertex
	{
	public:
		/**
		\brief  The world-space position
		*/
		float mPosition[3];
		/**
		\brief The normal vector to use for lighting
		*/
		float mNormal[3];
		/**
		\brief  Texture co-ordinates
		*/
		float mTexel[2];
	};

	static MeshBuilder *create(void);

	virtual void release(void) = 0;

	virtual void addTriangle(const float *p1, const float *p2, const float *p3) = 0;

	// Returns the triangles as faceted array; 3 verts per triangle
	virtual const Vertex *getTriangleVertices(uint32_t &triCount) const = 0;

	// Returns vertices with smoothed normals. First call may compute mean normal result
	virtual const Vertex *getVertices(uint32_t &vertexCount) = 0;

	// Returns the indices for the triangle mesh
	virtual const uint32_t *getIndices(uint32_t &tcount) const = 0;
protected:
	virtual ~MeshBuilder(void)
	{

	}
};

}

#endif
