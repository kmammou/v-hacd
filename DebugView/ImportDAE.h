#ifndef IMPORT_DAE_H
#define IMPORT_DAE_H

// A simple code snippet to import mesh data from a COLLADA/DAE file
#include <stdint.h>

namespace IMPORT_DAE
{

class ImportDAE
{
public:
	// A DAE file could have a wide variety of vertex interpolants.
	// However, for this first simple implementation it will only
	// provide position, normal, and one texture coordinate.
	// Should the need arise, support for additional interpolants would
	// be added later.
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

	class SubMesh
	{
	public:
		uint32_t		mVertexCount{ 0 };				// Number of vertices in this submesh
		uint32_t		mTriangleCount{ 0 };			// Number of triangles in this submesh
		const Vertex	*mMeshVertices{ nullptr };			// Pointer to the array of vertices
		const uint32_t	*mMeshIndices{ nullptr };			// Pointer to the array of indices
		const char		*mMaterialName{ nullptr };		// Name of the material associated with this submesh
	};

	class MeshImport
	{
	public:
		// Return the total number of submeshes
		virtual uint32_t		getSubMeshCount(void) const = 0;

		// Returns a pointer to a specific submesh
		virtual const SubMesh	*getSubMesh(uint32_t index) const = 0;

		const char				*mMeshName;
	};

	// A node in the scene graph
	class SceneNode
	{
	public:
		SceneNode(void)
		{
			mMatrix[0]  = 1; mMatrix[1]  = 0; mMatrix[2]  = 0; mMatrix[3]  = 0;
			mMatrix[4]  = 0; mMatrix[5]  = 1; mMatrix[6]  = 0; mMatrix[7]  = 0;
			mMatrix[8]  = 0; mMatrix[9]  = 0; mMatrix[10] = 1; mMatrix[11] = 0;
			mMatrix[12] = 0; mMatrix[13] = 0; mMatrix[14] = 0; mMatrix[15] = 1;
		}

		inline void  transform(const float v[3],float t[3]) const // rotate and translate this point
		{
			float tx = (mMatrix[0 * 4 + 0] * v[0]) + (mMatrix[1 * 4 + 0] * v[1]) + (mMatrix[2 * 4 + 0] * v[2]) + mMatrix[3 * 4 + 0];
			float ty = (mMatrix[0 * 4 + 1] * v[0]) + (mMatrix[1 * 4 + 1] * v[1]) + (mMatrix[2 * 4 + 1] * v[2]) + mMatrix[3 * 4 + 1];
			float tz = (mMatrix[0 * 4 + 2] * v[0]) + (mMatrix[1 * 4 + 2] * v[1]) + (mMatrix[2 * 4 + 2] * v[2]) + mMatrix[3 * 4 + 2];
			// In cast the source and dest are the same, we assign the translation afterwards
			t[0] = tx;
			t[1] = ty;
			t[2] = tz;
		}

		inline void  rotate(const float v[3], float t[3]) const // rotate but don't translate
		{
			float tx = (mMatrix[0 * 4 + 0] * v[0]) + (mMatrix[1 * 4 + 0] * v[1]) + (mMatrix[2 * 4 + 0] * v[2]);
			float ty = (mMatrix[0 * 4 + 1] * v[0]) + (mMatrix[1 * 4 + 1] * v[1]) + (mMatrix[2 * 4 + 1] * v[2]);
			float tz = (mMatrix[0 * 4 + 2] * v[0]) + (mMatrix[1 * 4 + 2] * v[1]) + (mMatrix[2 * 4 + 2] * v[2]);
			// In cast the source and dest are the same, we assign the translation afterwards
			t[0] = tx;
			t[1] = ty;
			t[2] = tz;
		}

		const char		*mId{ nullptr };
		const char		*mName{ nullptr };
		float			mMatrix[16];
		uint32_t		mMeshCount{ 0 };
		MeshImport		**mMeshes{ nullptr }; // array of meshes
	};

	static ImportDAE *create(void);

	// Import as a single mesh the contents of this DAE file
	// This snippet only reads the basic mesh data, not nodes,
	// animation data, skinning, etc. etc.  Just simple meshes
	// suitable for importing URDF files
	virtual void importDAE(const void *data,uint32_t dlen) = 0;
	virtual void importDAE(const char *fname) = 0;

	// Export the mesh as a Wavefront OBJ file
	virtual void exportOBJ(const char *fname,const char *materialName) = 0;

	// Return the total number of meshes imported
	virtual uint32_t getMeshCount(void) const = 0;

	// Return the mesh interface
	virtual const MeshImport *getMeshImport(uint32_t index) const = 0;

	virtual uint32_t getVisualSceneCount(void) const = 0;
	virtual uint32_t getVisualSceneNodeCount(uint32_t index, const char *&sceneId, const char *&sceneName) const = 0;
	virtual const SceneNode * getVisualSceneNode(uint32_t sceneIndex, uint32_t nodeIndex) = 0;


	virtual void release(void) = 0;
protected:
	virtual ~ImportDAE(void)
	{
	}
};

} // end of IMPORT_DAE namespace


#endif
