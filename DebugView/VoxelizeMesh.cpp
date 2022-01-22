#include "VoxelizeMesh.h"
#include "NvRenderDebug.h"
#include "FloatMath.h"

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

	}

	virtual ~VoxelizeMeshImpl(void)
	{
	}

	virtual void visualize(bool showSurface,bool showInside,bool showOutside,bool showUndefined) final
	{
	}

	virtual void release(void) final
	{
		delete this;
	}

	void visualizeVolume(bool showSurface,bool showInside,bool showOutside,bool showUndefined)
	{
	}

	void traceRay(const double *start,const double *dir,uint32_t &insideCount,uint32_t &outsideCount)
	{
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
	}

	virtual void snapVoxelToMesh(void) final
	{
	}

	uint32_t		mDimensions{64};
	RENDER_DEBUG::RenderDebug *mRenderDebug{nullptr};

	SnapPointVector		mSnapPoints;
};

VoxelizeMesh *VoxelizeMesh::create(uint32_t vertexCount,uint32_t triangleCount,const double *vertices,const uint32_t *indices,uint32_t resolution,RENDER_DEBUG::RenderDebug *renderDebug,VHACD::IVHACD::Parameters &params)
{
	auto ret = new VoxelizeMeshImpl(vertexCount,triangleCount,vertices,indices,resolution,renderDebug,params);
	return static_cast< VoxelizeMesh *>(ret);
}


}


