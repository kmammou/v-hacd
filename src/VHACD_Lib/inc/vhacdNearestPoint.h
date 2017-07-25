#ifndef VHACD_NEAREST_POINT
#define VHACD_NEAREST_POINT

#include <stdint.h>

// This is a helper class which can return the nearest point given a set of 
// input vertices.  To avoid doing a brute force search, this implementation
// builds a Kd-Tree for the input point data set.  Once this has been built
// if can be rapidly queried to retrieve the 'nearest point' given an arbitrary
// input.

namespace VHACD
{

class NearestPoint
{
public:

	static NearestPoint *createNearestPoint(uint32_t pointCount,const double *inputVertices,uint32_t stridePoints);
	static NearestPoint *createNearestPoint(uint32_t pointCount, const float *inputVertices,uint32_t stridePoints);

	// resets an internal counter to identify when we he unique or duplicated vertices
	virtual void startFreshQuery(void) = 0;

	// Finds the nearest vertex to the input Vertex.  Returns true if this is the first time
	// we have returned this vertex for this query, false if it is not.
	// This is to prevent adding duplicate vertices as input to the convex hull generator
	virtual bool getNearestVert(const double *inputVertex,double *outputVertex) = 0;

	virtual void release(void) = 0;
};

} // end of VHACD namespace

#endif
