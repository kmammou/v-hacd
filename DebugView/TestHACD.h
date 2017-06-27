#ifndef TEST_HACD_H
#define TEST_HACD_H

#include <stdint.h>
#include "VHACD.h"

namespace RENDER_DEBUG
{
	class RenderDebug;
}



class TestHACD
{
public:
	static TestHACD *create(void);

	virtual void decompose(
		const double* const points,
		const unsigned int countPoints,
		const int* const triangles,
		const unsigned int countTriangles,
		VHACD::IVHACD::Parameters &desc) = 0;

	virtual void render(RENDER_DEBUG::RenderDebug *renderDebug,float explodeViewScale,const float center[3]) = 0;

	virtual uint32_t getHullCount(void) const = 0;

	virtual void cancel(void) = 0;

	virtual void release(void) = 0;
protected:
	virtual ~TestHACD(void)
	{
	}
};

#endif
