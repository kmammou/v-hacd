#ifndef MESH_FACTORY_H
#define MESH_FACTORY_H

#include <stdint.h>

// Manages loading a collection of meshes in either COLLADA or STL format by name
namespace MESH_FACTORY
{

// Define a mesh vertex with a position, normal, and just one texture coordinate
class Vertex
{
public:
	Vertex(void)
	{
		mPosition[0] = 0;
		mPosition[1] = 0;
		mPosition[2] = 0;
		mNormal[0] = 0;
		mNormal[1] = 1;
		mNormal[2] = 0;
		mTexel[0] = 0;
		mTexel[1] = 0;
	}
	float		mPosition[3];		// Position of this vertex
	float		mNormal[3];			// Normal vector for this vertex
	float		mTexel[2];			// Texture co-ordinates
};

// Define a submesh
class SubMesh
{
public:
	uint32_t		mVertexCount{ 0 };				// Number of vertices in this submesh
	uint32_t		mTriangleCount{ 0 };			// Number of triangles in this submesh
	const Vertex	*mMeshVertices{ nullptr };			// Pointer to the array of vertices
	const uint32_t	*mMeshIndices{ nullptr };			// Pointer to the array of indices
	const char		*mMaterialName{ nullptr };		// Name of the material associated with this submesh
};

// A mesh is a collection of submeshes
class Mesh
{
public:
	// Return the total number of submeshes
	virtual uint32_t		getSubMeshCount(void) const = 0;

	// Returns a pointer to a specific submesh
	virtual const SubMesh	*getSubMesh(uint32_t index) const = 0;

	const char				*mMeshName{ nullptr };
};


class MeshFactory
{
public:


	static MeshFactory *create(void);

	virtual Mesh *importMesh(const char *meshName,					// Unique name of the mesh
							const char *filePathName) = 0;           // Fully qualified path name for the mesh

	virtual void release(void) = 0;


protected:
	virtual ~MeshFactory(void)
	{
	}
};

} // end of MESH_FACTORY namespace

#endif
