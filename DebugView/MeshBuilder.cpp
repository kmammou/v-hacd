#include "MeshBuilder.h"
#include <vector>

#define TSCALE1 (1.0f/4.0f)
#define EPSILON 0.000000001f

namespace MESH_BUILDER
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

static	float fm_computePlane(const float *A, const float *B, const float *C, float *n) // returns D
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

class MeshBuilderImpl : public MeshBuilder
{
public:
	MeshBuilderImpl(void)
	{
	}

	virtual ~MeshBuilderImpl(void)
	{

	}

	virtual void release(void) final
	{
		delete this;
	}

	void getVertex(const float *p, const float *n, uint32_t i1, uint32_t i2)
	{
		Vertex v;

		v.mPosition[0] = p[0];
		v.mPosition[1] = p[1];
		v.mPosition[2] = p[2];

		v.mNormal[0] = n[0];
		v.mNormal[1] = n[1];
		v.mNormal[2] = n[2];

		v.mTexel[0] = p[i1] * TSCALE1;
		v.mTexel[1] = p[i2] * TSCALE1;

		mTriangleVertices.push_back(v);

		bool found = false;
		for (size_t i = 0; i < mVertices.size(); i++)
		{
			Vertex &fv = mVertices[i];
			float dx = v.mPosition[0] - fv.mPosition[0];
			float dy = v.mPosition[1] - fv.mPosition[1];
			float dz = v.mPosition[2] - fv.mPosition[2];
			float squaredDistance = dx*dx + dy*dy + dz*dz;
			if (squaredDistance < (EPSILON*EPSILON))
			{
				mDirtyNormals = true;
				found = true;
				fv.mNormal[0] += v.mNormal[0];
				fv.mNormal[1] += v.mNormal[1];
				fv.mNormal[2] += v.mNormal[2];
				mIndices.push_back(uint32_t(i));
				break;
			}
		}
		if (!found)
		{
			uint32_t index = uint32_t(mVertices.size());
			mIndices.push_back(index);
			mVertices.push_back(v);
		}
	}

	virtual void addTriangle(const float *p1, const float *p2, const float *p3) final
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

	// Returns the triangles as faceted array; 3 verts per triangle
	virtual const Vertex *getTriangleVertices(uint32_t &triCount) const final
	{
		const Vertex *ret = nullptr;
		triCount = uint32_t(mTriangleVertices.size()) / 3;
		if (triCount)
		{
			ret = &mTriangleVertices[0];
		}
		return ret;
	}

	// Returns vertices with smoothed normals.
	virtual const Vertex *getVertices(uint32_t &vertexCount) final
	{
		const Vertex *ret = nullptr;

		vertexCount = uint32_t(mVertices.size());
		if (vertexCount)
		{
			if (mDirtyNormals)
			{
				mDirtyNormals = false;
				for (uint32_t i = 0; i < vertexCount; i++)
				{
					Vertex &v = mVertices[i];
					fm_normalize(v.mNormal);
				}
			}
			ret = &mVertices[0];
		}

		return ret;
	}

	// Returns the indices for the triangle mesh
	virtual const uint32_t *getIndices(uint32_t &tcount) const final
	{
		const uint32_t *ret = nullptr;

		tcount = uint32_t(mIndices.size()) / 3;
		if (tcount)
		{
			ret = &mIndices[0];
		}

		return ret;
	}

	bool					mDirtyNormals{ false };
	std::vector< Vertex >	mTriangleVertices;
	std::vector< Vertex >	mVertices;
	std::vector<uint32_t>	mIndices;
};

MeshBuilder *MeshBuilder::create(void)
{
	MeshBuilderImpl *m = new MeshBuilderImpl;
	return static_cast<MeshBuilder *>(m);
}

}
