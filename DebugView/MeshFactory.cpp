#include "MeshFactory.h"
#include "ImportDAE.h"
#include "ImportSTL.h"
#include "StringHelper.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>

#pragma warning(disable:4100 4996)


// Manages loading a collection of meshes in either COLLADA or STL format by name
namespace MESH_FACTORY
{

	static float fm_normalize(float *n) // normalize this vector
	{
		float dist = (float)sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
		if (dist > 0.0000001f)
		{
			float mag = 1.0f / dist;
			n[0] *= mag;
			n[1] *= mag;
			n[2] *= mag;
		}
		else
		{
			n[0] = 1;
			n[1] = 0;
			n[2] = 0;
		}

		return dist;
	}

	class SubMeshImpl : public SubMesh
	{
	public:
		SubMeshImpl(void)
		{

		}
		SubMeshImpl(const IMPORT_DAE::ImportDAE::SubMesh *sm,const IMPORT_DAE::ImportDAE::SceneNode *sn)
		{
			mVertexCount	= sm->mVertexCount;
			mTriangleCount	= sm->mTriangleCount;
			mMatName		= std::string(sm->mMaterialName);
			mMaterialName	= mMatName.c_str();
			if (mVertexCount)
			{
				mSourceMeshVertices = new Vertex[mVertexCount];
				memcpy(mSourceMeshVertices, sm->mMeshVertices, sizeof(Vertex)*mVertexCount);
				// If we have a scene node associated with this submesh, then apply the 
				// matrix transform to it
				if (sn)
				{
					for (uint32_t i = 0; i < mVertexCount; i++)
					{
						Vertex &v = mSourceMeshVertices[i];
						sn->transform(v.mPosition, v.mPosition);
						sn->rotate(v.mNormal, v.mNormal);
						// Since scale could have been applied (in addition to rotation) we need to
						// re-normalize the normal vector
						fm_normalize(v.mNormal);
					}
				}
				mMeshVertices = mSourceMeshVertices;
			}
			if (mTriangleCount)
			{
				mSourceMeshIndices = new uint32_t[mTriangleCount * 3];
				memcpy(mSourceMeshIndices, sm->mMeshIndices, sizeof(uint32_t)*mTriangleCount * 3);
				mMeshIndices = mSourceMeshIndices;
			}
		}

		SubMeshImpl(uint32_t tcount,const IMPORT_STL::StlTriangle *triangles)
		{
			mVertexCount = tcount * 3;
			mTriangleCount = tcount;
			mSourceMeshVertices = new Vertex[mVertexCount];
			mSourceMeshIndices = new uint32_t[tcount * 3];
			mMeshVertices = mSourceMeshVertices;
			mMeshIndices = mSourceMeshIndices;
			for (uint32_t i = 0; i < tcount; i++)
			{
				const IMPORT_STL::StlTriangle &t = triangles[i];

				Vertex &v1 = mSourceMeshVertices[i * 3 + 0];
				Vertex &v2 = mSourceMeshVertices[i * 3 + 1];
				Vertex &v3 = mSourceMeshVertices[i * 3 + 2];

				auto addVert = [this](Vertex &v, const float *p, const float *n)
				{
					v.mPosition[0] = p[0];
					v.mPosition[1] = p[1];
					v.mPosition[2] = p[2];
					v.mNormal[0] = n[0];
					v.mNormal[1] = n[1];
					v.mNormal[2] = n[2];
					v.mTexel[0] = p[0];
					v.mTexel[1] = p[2];
				};

				addVert(v1, t.mP1, t.mNormal);
				addVert(v2, t.mP2, t.mNormal);
				addVert(v3, t.mP3, t.mNormal);

				mSourceMeshIndices[i * 3 + 0] = i * 3 + 0;
				mSourceMeshIndices[i * 3 + 1] = i * 3 + 1;
				mSourceMeshIndices[i * 3 + 2] = i * 3 + 2;
			}
		}

		virtual ~SubMeshImpl(void)
		{
			// Free up memory for mesh data
			delete[]mSourceMeshVertices;
			delete[]mSourceMeshIndices;
		}
		std::string		mMatName; // name of material
		Vertex			*mSourceMeshVertices{ nullptr };
		uint32_t		*mSourceMeshIndices{ nullptr };
	};

	class	MeshImpl : public Mesh
	{
	public:
		MeshImpl(void)
		{

		}
		MeshImpl(const char *meshName,uint32_t triangleCount,const IMPORT_STL::StlTriangle *triangles)
		{
			mName = std::string(meshName);
			mMeshName = mName.c_str();
			mSubMeshCount = 1;
			mSubMeshes = new SubMeshImpl *[mSubMeshCount];
			mSubMeshes[0] = new SubMeshImpl(triangleCount, triangles);
		}

		MeshImpl(const char *meshName, IMPORT_DAE::ImportDAE *imp)
		{
			mName = std::string(meshName);
			mMeshName = mName.c_str();
			uint32_t totalSubMeshCount = 0;

			uint32_t scount = imp->getVisualSceneCount();
			for (uint32_t i = 0; i < scount; i++)
			{
				const char *sceneId;
				const char *sceneName;
				uint32_t nodeCount = imp->getVisualSceneNodeCount(i, sceneId, sceneName);
				for (uint32_t j = 0; j < nodeCount; j++)
				{
					const IMPORT_DAE::ImportDAE::SceneNode *sn = imp->getVisualSceneNode(i, j);
					if (sn)
					{
						for (uint32_t k = 0; k < sn->mMeshCount; k++)
						{
							const IMPORT_DAE::ImportDAE::MeshImport *m = sn->mMeshes[k];
							totalSubMeshCount += m->getSubMeshCount();
						}
					}
				}
			}

			mSubMeshCount = totalSubMeshCount;
			if (mSubMeshCount)
			{
				mSubMeshes = new SubMeshImpl*[totalSubMeshCount];
				uint32_t index = 0;
				for (uint32_t i = 0; i < scount; i++)
				{
					const char *sceneId;
					const char *sceneName;
					uint32_t nodeCount = imp->getVisualSceneNodeCount(i, sceneId, sceneName);
					for (uint32_t j = 0; j < nodeCount; j++)
					{
						const IMPORT_DAE::ImportDAE::SceneNode *sn = imp->getVisualSceneNode(i, j);
						if (sn)
						{
							for (uint32_t k = 0; k < sn->mMeshCount; k++)
							{
								const IMPORT_DAE::ImportDAE::MeshImport *m = sn->mMeshes[k];
								uint32_t subMeshCount = m->getSubMeshCount();
								for (uint32_t l = 0; l < subMeshCount; l++)
								{
									const IMPORT_DAE::ImportDAE::SubMesh *smdae = m->getSubMesh(l);
									SubMeshImpl *sm = new SubMeshImpl(smdae,sn);
									mSubMeshes[index] = sm;
									index++;
								}
							}
						}
					}
				}
			}
		}

		virtual ~MeshImpl(void)
		{
			// Release all of the submeshes
			for (uint32_t i = 0; i < mSubMeshCount; i++)
			{
				SubMeshImpl *sm = mSubMeshes[i];
				delete sm;
			}
			delete[]mSubMeshes;
		}

		// Return the total number of submeshes
		virtual uint32_t		getSubMeshCount(void) const
		{
			return mSubMeshCount;
		}

		// Returns a pointer to a specific submesh
		virtual const SubMesh	*getSubMesh(uint32_t index) const
		{
			const SubMesh *ret = nullptr;

			if (index < mSubMeshCount)
			{
				ret = static_cast<SubMesh *>(mSubMeshes[index]);
			}

			return ret;
		}

		uint32_t		mSubMeshCount{ 0 };
		SubMeshImpl		**mSubMeshes{ nullptr };
		std::string		mName;
	};

	// meshes
	typedef std::unordered_map< std::string, Mesh * > MeshMap;


	class MeshFactoryImpl : public MeshFactory
	{
	public:
		MeshFactoryImpl(void)
		{

		}
		virtual ~MeshFactoryImpl(void)
		{

		}

		virtual Mesh *importMesh(const char *meshName,					// Unique name of the mesh
			const char *filePathName) final           // Fully qualified path name for the mesh
		{
			Mesh *ret = nullptr;

			std::string str(meshName);
			MeshMap::iterator found = mMeshes.find(str);
			if (found != mMeshes.end())
			{
				ret = static_cast<Mesh *>((*found).second);
			}
			else
			{
				printf("Importing: %s\r\n", meshName);
				char fqn[512];
				STRING_HELPER::stringFormat(fqn, 512, "%s/%s", filePathName, meshName);
				STRING_HELPER::normalizePathSlashes(fqn);
				const char *isDAE = strstr(meshName, ".dae");
				const char *isSTL = strstr(meshName, ".stl");
				if (isDAE)
				{
					IMPORT_DAE::ImportDAE *imp = IMPORT_DAE::ImportDAE::create();
					if (imp)
					{
						imp->importDAE(fqn);

						MeshImpl *m = new MeshImpl(meshName,imp);
						mMeshes[str] = m;
						ret = static_cast<Mesh *>(m);

						imp->release();
					}
				}
				else if (isSTL)
				{
					FILE *fph = fopen(fqn, "rb");
					if (fph)
					{
						fseek(fph, 0L, SEEK_END);
						size_t flen = ftell(fph);
						fseek(fph, 0L, SEEK_SET);
						if (flen)
						{
							void *data = malloc(flen);
							if (data)
							{
								size_t r = fread(data, flen, 1, fph);
								if (r == 1)
								{
									IMPORT_STL::ImportSTL *imp = IMPORT_STL::ImportSTL::create();
									if (imp)
									{
										uint32_t tcount;
										const IMPORT_STL::StlTriangle *triangles = imp->importMesh(data, uint32_t(flen), tcount);

										MeshImpl *m = new MeshImpl(meshName, tcount, triangles);
										mMeshes[str] = m;
										ret = static_cast<Mesh *>(m);

										imp->release();
									}
								}
								free(data);
							}
						}
						fclose(fph);
					}

				}
			}

			return ret;
		}

		virtual void release(void) final
		{
			delete this;
		}


		MeshMap	mMeshes;
	};

MeshFactory *MeshFactory::create(void)
{
	MeshFactoryImpl *ret = new MeshFactoryImpl;
	return static_cast<MeshFactory *>(ret);
}


} // end of MESH_FACTORY namespace




