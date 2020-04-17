#include "VoxelizeMesh.h"
#include "vhacdVolume.h"
#include "vhacdMesh.h"
#include "NvRenderDebug.h"
#include "FloatMath.h"
#include "vhacdRaycastMesh.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vector>

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

namespace voxelizemesh
{

	struct Vec3
	{
		float x;
		float y;
		float z;
	};

	struct SnapPoint
	{
		Vec3 from;
		Vec3 to;
	};

	typedef std::vector< SnapPoint > SnapPointVector;

	#define TSCALE1 (1.0f/4.0f)

	float fm_computePlane(const float *A, const float *B, const float *C, float *n) // returns D
	{
		float vx = (B[0] - C[0]);
		float vy = (B[1] - C[1]);
		float vz = (B[2] - C[2]);

		float wx = (A[0] - B[0]);
		float wy = (A[1] - B[1]);
		float wz = (A[2] - B[2]);

		float vw_x = vy * wz - vz * wy;
		float vw_y = vz * wx - vx * wz;
		float vw_z = vx * wy - vy * wx;

		float mag = ::sqrtf((vw_x * vw_x) + (vw_y * vw_y) + (vw_z * vw_z));

		if (mag < 0.000001f)
		{
			mag = 0;
		}
		else
		{
			mag = 1.0f / mag;
		}

		float x = vw_x * mag;
		float y = vw_y * mag;
		float z = vw_z * mag;


		float D = 0.0f - ((x*A[0]) + (y*A[1]) + (z*A[2]));

		n[0] = x;
		n[1] = y;
		n[2] = z;

		return D;
	}

	class MeshBuilder
	{
	public:
		MeshBuilder(void)
		{
		}

		MeshBuilder(uint32_t maxVertices)
		{
			mVertices.reserve(maxVertices);
		}

		void getVertex(const float *p, const float *n, uint32_t i1, uint32_t i2)
		{
			RENDER_DEBUG::RenderDebugMeshVertex v;

			v.mPosition[0] = p[0];
			v.mPosition[1] = p[1];
			v.mPosition[2] = p[2];

			v.mNormal[0] = n[0];
			v.mNormal[1] = n[1];
			v.mNormal[2] = n[2];

			v.mTexel[0] = p[i1] * TSCALE1;
			v.mTexel[1] = p[i2] * TSCALE1;

			mVertices.push_back(v);
		}

		void addTriangle(const float *p1, const float *p2, const float *p3)
		{
			float normal[3];
			fm_computePlane(p3, p2, p1, normal);

			double nx = fabs(normal[0]);
			double ny = fabs(normal[1]);
			double nz = fabs(normal[2]);

			uint32_t i1 = 0;
			uint32_t i2 = 0;

			if (nx <= ny && nx <= nz)
				i1 = 0;
			if (ny <= nx && ny <= nz)
				i1 = 1;
			if (nz <= nx && nz <= ny)
				i1 = 2;

			switch (i1)
			{
			case 0:
				if (ny < nz)
					i2 = 1;
				else
					i2 = 2;
				break;
			case 1:
				if (nx < nz)
					i2 = 0;
				else
					i2 = 2;
				break;
			case 2:
				if (nx < ny)
					i2 = 0;
				else
					i2 = 1;
				break;
			}

			getVertex(p1, normal, i1, i2);
			getVertex(p2, normal, i1, i2);
			getVertex(p3, normal, i1, i2);
		}

		void clear(void)
		{
			mVertices.clear();
		}


		std::vector< RENDER_DEBUG::RenderDebugMeshVertex > mVertices;
	};


class DebugMesh
{
public:
	DebugMesh(VHACD::Mesh &m,RENDER_DEBUG::RenderDebug *r) : mRenderDebug(r)
	{
		if ( m.GetNTriangles() )
		{
			const int32_t *indices = m.GetTriangles();
			uint32_t triangleCount = uint32_t(m.GetNTriangles());
			MeshBuilder mb(uint32_t(m.GetNPoints()));
			uint32_t tcount = 0;
#define MAX_VERT_MESH 200000
			for (uint32_t i=0; i<triangleCount; i++)
			{
				uint32_t i1 = uint32_t(indices[i*3+0]);
				uint32_t i2 = uint32_t(indices[i*3+1]);
				uint32_t i3 = uint32_t(indices[i*3+2]);
				VHACD::Vec3<float> p1 = m.GetPointFloat(i1);
				VHACD::Vec3<float> p2 = m.GetPointFloat(i2);
				VHACD::Vec3<float> p3 = m.GetPointFloat(i3);
				mb.addTriangle(&p1[0],&p2[0],&p3[0]);
				tcount++;
				if ( tcount == MAX_VERT_MESH )
				{
					uint32_t meshId = mRenderDebug->getMeshId();
					mRenderDebug->createTriangleMesh(meshId, (uint32_t)mb.mVertices.size(), &mb.mVertices[0], 0, nullptr);
					mMeshIds.push_back(meshId);
					mb.clear();
					tcount = 0;
				}
			}
			if ( mb.mVertices.size() )
			{
				uint32_t meshId = mRenderDebug->getMeshId();
				mRenderDebug->createTriangleMesh(meshId, (uint32_t)mb.mVertices.size(), &mb.mVertices[0], 0, nullptr);
				mMeshIds.push_back(meshId);
			}
		}
	}

	~DebugMesh(void)
	{
		for (auto &i:mMeshIds)
		{
			mRenderDebug->releaseTriangleMesh(i);
		}
	}

	void visualize(RENDER_DEBUG::DebugTextures::Enum t)
	{
		if ( !mMeshIds.empty() )
		{
			mRenderDebug->pushRenderState();
			mRenderDebug->setCurrentTexture(t,1,RENDER_DEBUG::DebugTextures::WHITE,4);
			RENDER_DEBUG::RenderDebugInstance instance;
			float xform[16];
			FLOAT_MATH::fm_identity(xform);
			for (auto &i:mMeshIds)
			{
				mRenderDebug->renderTriangleMeshInstances(i, 1, &instance);
			}
			mRenderDebug->popRenderState();
		}
	}

	std::vector<uint32_t>		mMeshIds;
	RENDER_DEBUG::RenderDebug	*mRenderDebug{nullptr};
};

class VoxelizeMeshImpl : public VoxelizeMesh
{
public:
	VoxelizeMeshImpl(uint32_t vertexCount,
					 uint32_t triangleCount,
					 const double *vertices,
					 const uint32_t *indices,
					 uint32_t resolution,
					 RENDER_DEBUG::RenderDebug *renderDebug,
                     VHACD::IVHACD::Parameters &params) : mRenderDebug(renderDebug)
	{

		{
			mRaycastMesh = VHACD::RaycastMesh::createRaycastMesh(vertexCount,vertices,triangleCount,indices);
		}

		double a = pow((double)(resolution), 0.33);
		mDimensions = (uint32_t)(a*1.5);
		// Minimum voxel resolution is 32x32x32
		if (mDimensions < 32)
		{
			mDimensions = 32;
		}
		mVolume = new VHACD::Volume(params);

		printf("Voxelizing input mesh of %d triangles at dimensions:%d\n", triangleCount, mDimensions);

		mVolume->Voxelize(vertices,vertexCount,(const int32_t *)indices,triangleCount,mDimensions,VHACD::FillMode::SURFACE_ONLY,nullptr);
		printf("Building OnSurface mesh\n");
		mVolume->Convert(mOnSurface,VHACD::PRIMITIVE_ON_SURFACE);
		printf("Building OutsideSurface mesh\n");
		mVolume->Convert(mOutsideSurface,VHACD::PRIMITIVE_OUTSIDE_SURFACE);
		printf("Building InsideSurface mesh\n");
		mVolume->Convert(mInsideSurface,VHACD::PRIMITIVE_INSIDE_SURFACE);
		printf("Building UndefinedSurface mesh\n");
		mVolume->Convert(mUndefined,VHACD::PRIMITIVE_UNDEFINED);

		printf("Building OnSurface Debug Visualization mesh\n");
		mOnSurfaceMesh = new DebugMesh(mOnSurface, mRenderDebug);

		printf("Building OutsideSurface Debug Visualization mesh\n");
		mOutsideSurfaceMesh = new DebugMesh(mOutsideSurface,mRenderDebug);

		printf("Building InsideSurface Debug Visualization mesh\n");
		mInsideSurfaceMesh = new DebugMesh(mInsideSurface,mRenderDebug);

		printf("Building UndefinedSurface Debug Visualization mesh\n");
		mUndefinedMesh = new DebugMesh(mUndefined,mRenderDebug);

		printf("Voxelization complete.\n");
	}

	virtual ~VoxelizeMeshImpl(void)
	{
		mRaycastMesh->release();
		delete mVolume;
		delete mOutsideSurfaceMesh;
		delete mInsideSurfaceMesh;
		delete mOnSurfaceMesh;
		delete mUndefinedMesh;
	}

	virtual void visualize(bool showSurface,bool showInside,bool showOutside,bool showUndefined) final
	{
#if 0
		if ( mOnSurfaceMesh && showSurface )
		{
			mOnSurfaceMesh->visualize(RENDER_DEBUG::DebugTextures::GOLD);
		}
		if (mOutsideSurfaceMesh && showOutside)
		{
			mOutsideSurfaceMesh->visualize(RENDER_DEBUG::DebugTextures::LIGHT_TORQUISE);
		}
		if (mInsideSurfaceMesh && showInside)
		{
			mInsideSurfaceMesh->visualize(RENDER_DEBUG::DebugTextures::BLUE_GRAY);
		}
		if (mUndefinedMesh && showUndefined)
		{
			mUndefinedMesh->visualize(RENDER_DEBUG::DebugTextures::GRAY);
		}
#else
		visualizeVolume(showSurface,showInside,showOutside,showUndefined);
#endif
		if ( !mSnapPoints.empty() )
		{
			mRenderDebug->pushRenderState();
			for (auto &i:mSnapPoints)
			{
				mRenderDebug->setCurrentColor(0xFF0000);
				mRenderDebug->debugLine(&i.from.x,&i.to.x);
				mRenderDebug->debugPoint(&i.from.x,0.05f);
				mRenderDebug->setCurrentColor(0x00FF00);
				mRenderDebug->debugPoint(&i.to.x,0.05f);
			}
			mRenderDebug->popRenderState();
		}
	}

	virtual void release(void) final
	{
		delete this;
	}

	void visualizeVolume(bool showSurface,bool showInside,bool showOutside,bool showUndefined)
	{
		float scale = float(mVolume->m_scale);
		float bmin[3];

		bmin[0] = float(mVolume->m_minBB[0]);
		bmin[1] = float(mVolume->m_minBB[1]);
		bmin[2] = float(mVolume->m_minBB[2]);

		const size_t i0 = mVolume->m_dim[0];
		const size_t j0 = mVolume->m_dim[1];
		const size_t k0 = mVolume->m_dim[2];

		uint32_t lastColor = 0;

		for (size_t i = 0; i < i0; ++i)
		{
			for (size_t j = 0; j < j0; ++j)
			{
				for (size_t k = 0; k < k0; ++k)
				{
					const unsigned char& voxel = mVolume->GetVoxel(i, j, k);
					float pos[3];
					pos[0] = float(i)*scale + bmin[0];
					pos[1] = float(j)*scale + bmin[1];
					pos[2] = float(k)*scale + bmin[2];
					uint32_t color = 0;
					switch ( voxel )
					{
						case VHACD::PRIMITIVE_ON_SURFACE:
							if ( showSurface )
							{
								color = 0xFFFF00;
							}
							break;
						case VHACD::PRIMITIVE_OUTSIDE_SURFACE:
							if (showOutside)
							{
								color = 0xFF0000;
							}
							break;
						case VHACD::PRIMITIVE_INSIDE_SURFACE:
							if (showInside)
							{
								color = 0x00FF00;
							}
							break;
						case VHACD::PRIMITIVE_UNDEFINED:
							if (showUndefined)
							{
								color = 0x202020;
							}
							break;
						default:
							assert(0);
							break;
					}
					if (color)
					{
						if (color != lastColor)
						{
							lastColor = color;
						}
						mRenderDebug->setCurrentColor(color);
						mRenderDebug->debugPoint(pos, scale*0.5f);
					}
				}
			}
		}
	}

	void traceRay(const double *start,const double *dir,uint32_t &insideCount,uint32_t &outsideCount)
	{
		double outT,u,v,w,faceSign;
		uint32_t faceIndex;
		bool hit = mRaycastMesh->raycast(start,dir,outT,u,v,w,faceSign,faceIndex);
		if ( hit )
		{
			if ( faceSign >= 0 )
			{
				insideCount++;
			}
			else
			{
				outsideCount++;
			}
		}

	}

	void initVec3(double *dest, uint32_t vindex, double x, double y, double z)
	{
		dest[vindex * 3 + 0] = x;
		dest[vindex * 3 + 1] = y;
		dest[vindex * 3 + 2] = z;
	}

	// We are going to compute which voxels are 'inside' vs. 'outside' using raycasting
	virtual void computeInsideVoxels(void) final
	{
		float scale = float(mVolume->m_scale);
		float bmin[3];

		bmin[0] = float(mVolume->m_minBB[0]);
		bmin[1] = float(mVolume->m_minBB[1]);
		bmin[2] = float(mVolume->m_minBB[2]);

		const size_t i0 = mVolume->m_dim[0];
		const size_t j0 = mVolume->m_dim[1];
		const size_t k0 = mVolume->m_dim[2];

		for (size_t i = 0; i < i0; ++i)
		{
			for (size_t j = 0; j < j0; ++j)
			{
				for (size_t k = 0; k < k0; ++k)
				{
					const unsigned char& voxel = mVolume->GetVoxel(i, j, k);
					if (voxel != VHACD::PRIMITIVE_ON_SURFACE)
					{
						double start[3];
						start[0] = double(i)*scale + bmin[0];
						start[1] = double(j)*scale + bmin[1];
						start[2] = double(k)*scale + bmin[2];
						uint32_t insideCount = 0;
						uint32_t outsideCount = 0;

						double directions[6*3];
						initVec3(directions, 0, 1, 0, 0);
						initVec3(directions, 1, 1, 0, 0);
						initVec3(directions, 2, 0, 1, 0);
						initVec3(directions, 3, 0, -1, 0);
						initVec3(directions, 4, 0, 0, 1);
						initVec3(directions, 5, 0, 0, -1);
						// Ray trace out in 6 directions.
						// We need to hit an inside facing triangle at least 3 times
						// for it to be fully considered 'inside'
						for (uint32_t r = 0; r < 6; r++)
						{
							traceRay(start, &directions[r * 3], insideCount, outsideCount);
							// Early out if we hit the outside of the mesh
							if (outsideCount)
							{
								break;
							}
							// Early out if we accumulated 3 inside hits
							if (insideCount >= 3)
							{
								break;
							}
						}

						if ( outsideCount == 0 && insideCount >= 3 )
						{
							mVolume->SetVoxel(i, j, k, VHACD::PRIMITIVE_INSIDE_SURFACE);
						}
						else
						{
							mVolume->SetVoxel(i, j, k, VHACD::PRIMITIVE_OUTSIDE_SURFACE);
						}
					}
				}
			}
		}
	}

	virtual void snapVoxelToMesh(void) final
	{
		mSnapPoints.clear();
		float scale = float(mVolume->m_scale);
		float bmin[3];

		bmin[0] = float(mVolume->m_minBB[0]);
		bmin[1] = float(mVolume->m_minBB[1]);
		bmin[2] = float(mVolume->m_minBB[2]);

		const size_t i0 = mVolume->m_dim[0];
		const size_t j0 = mVolume->m_dim[1];
		const size_t k0 = mVolume->m_dim[2];

		double center[3];

		center[0] = (mVolume->m_maxBB[0]-mVolume->m_minBB[0])*0.5 + mVolume->m_minBB[0];
		center[1] = (mVolume->m_maxBB[1]-mVolume->m_minBB[1])*0.5 + mVolume->m_minBB[1];
		center[2] = (mVolume->m_maxBB[2]-mVolume->m_minBB[2])*0.5 + mVolume->m_minBB[2];

		for (size_t i = 0; i < i0; ++i)
		{
			for (size_t j = 0; j < j0; ++j)
			{
				for (size_t k = 0; k < k0; ++k)
				{
					const unsigned char& voxel = mVolume->GetVoxel(i, j, k);

					if (voxel == VHACD::PRIMITIVE_ON_SURFACE)
					{
						double pos[3];
						double closest[3];

						pos[0] = double(i)*scale + bmin[0];
						pos[1] = double(j)*scale + bmin[1];
						pos[2] = double(k)*scale + bmin[2];

						bool found = mRaycastMesh->getClosestPointWithinDistance(pos,scale*2,closest);

						if ( found )
						{
							SnapPoint p;

							p.from.x = float(pos[0]);
							p.from.y = float(pos[1]);
							p.from.z = float(pos[2]);

							p.to.x = float(closest[0]);
							p.to.y = float(closest[1]);
							p.to.z = float(closest[2]);

							mSnapPoints.push_back(p);
						}
					}
				}
			}
		}
	}

	uint32_t		mDimensions{64};
	VHACD::Volume	*mVolume{nullptr};
	VHACD::Mesh		mOutsideSurface;
	VHACD::Mesh		mInsideSurface;
	VHACD::Mesh		mOnSurface;
	VHACD::Mesh		mUndefined;

	DebugMesh		*mOutsideSurfaceMesh{nullptr};
	DebugMesh		*mInsideSurfaceMesh{nullptr};
	DebugMesh		*mOnSurfaceMesh{nullptr};
	DebugMesh		*mUndefinedMesh{nullptr};

	RENDER_DEBUG::RenderDebug *mRenderDebug{nullptr};

	VHACD::RaycastMesh	*mRaycastMesh{nullptr};

	SnapPointVector		mSnapPoints;
};

VoxelizeMesh *VoxelizeMesh::create(uint32_t vertexCount,uint32_t triangleCount,const double *vertices,const uint32_t *indices,uint32_t resolution,RENDER_DEBUG::RenderDebug *renderDebug,VHACD::IVHACD::Parameters &params)
{
	auto ret = new VoxelizeMeshImpl(vertexCount,triangleCount,vertices,indices,resolution,renderDebug,params);
	return static_cast< VoxelizeMesh *>(ret);
}


}


