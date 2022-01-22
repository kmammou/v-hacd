#include "TestRaycast.h"
#include "NvRenderDebug.h"
#include <float.h>
#include <math.h>

const double FM_PI = 3.1415926535897932384626433832795028841971693993751f;
const double FM_DEG_TO_RAD = ((2.0f * FM_PI) / 360.0f);
const double FM_RAD_TO_DEG = (360.0f / (2.0f * FM_PI));

#pragma warning(disable:4100 4505)

static void fm_getAABB(uint32_t vcount, const double *p,double *bmin, double *bmax)
{
	bmin[0] = p[0];
	bmin[1] = p[1];
	bmin[2] = p[2];

	bmax[0] = p[0];
	bmax[1] = p[1];
	bmax[2] = p[2];

	p += 3;

	for (uint32_t i = 1; i < vcount; i++)
	{
		if (p[0] < bmin[0]) bmin[0] = p[0];
		if (p[1] < bmin[1]) bmin[1] = p[1];
		if (p[2] < bmin[2]) bmin[2] = p[2];

		if (p[0] > bmax[0]) bmax[0] = p[0];
		if (p[1] > bmax[1]) bmax[1] = p[1];
		if (p[2] > bmax[2]) bmax[2] = p[2];
		p += 3;

	}
}

// Little helper class to test the raycast mesh code and display the results
class TestRaycastImpl : public TestRaycast
{
public:
	TestRaycastImpl(void)
	{

	}
	virtual ~TestRaycastImpl(void)
	{

	}

	void doubleToFloatVert(const double *source, float *dest)
	{
		dest[0] = float(source[0]);
		dest[1] = float(source[1]);
		dest[2] = float(source[2]);
	}

	void floatToDoubleVert(const float *source, double *dest)
	{
		dest[0] = double(source[0]);
		dest[1] = double(source[1]);
		dest[2] = double(source[2]);
	}


	virtual void testRaycast(uint32_t vcount,	// number of vertices
		uint32_t tcount,	// number of triangles
		const double *vertices,		// Vertices in the mesh
		const uint32_t *indices,	// triangle indices
		RENDER_DEBUG::RenderDebug *renderDebug)  // rendering interface 
	{
	}

	virtual void release(void)
	{
		delete this;
	}

protected:
};

TestRaycast *TestRaycast::create(void)
{
	TestRaycastImpl *ret = new TestRaycastImpl;
	return static_cast<TestRaycast *>(ret);
}

