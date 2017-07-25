#include "vhacdNearestPoint.h"
#include <string.h>
#include <float.h>

namespace VHACD
{


class NearestPointImpl : public NearestPoint
{
public:
	NearestPointImpl(uint32_t pointCount, const double *inputVertices,uint32_t stridePoints)
	{
		mInputVertices = new double[pointCount * 3];
		mQueryID = new uint32_t[pointCount];
		memset(mQueryID, 0xFF, sizeof(uint32_t)*pointCount);
		const double *p1 = inputVertices;
		for (uint32_t i = 0; i < pointCount; i++)
		{
			bool found = false;
			for (uint32_t j = 0; j < mPointCount; j++)
			{
				const double *p2 = &mInputVertices[j * 3];
				if (p1[0] == p2[0] &&
					p1[1] == p2[1] &&
					p1[2] == p2[2])
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				double *dest = &mInputVertices[mPointCount * 3];
				dest[0] = p1[0];
				dest[1] = p1[1];
				dest[2] = p1[2];
				mPointCount++;
			}
			p1 += stridePoints;
		}
	}

	NearestPointImpl(uint32_t pointCount, const float *inputVertices, uint32_t stridePoints)
	{
		mInputVertices = new double[pointCount * 3];
		mQueryID = new uint32_t[pointCount];
		memset(mQueryID, 0xFF, sizeof(uint32_t)*pointCount);
		const float *p1 = inputVertices;
		for (uint32_t i = 0; i < pointCount; i++)
		{
			bool found = false;
			for (uint32_t j = 0; j < mPointCount; j++)
			{
				const double *p2 = &mInputVertices[j * 3];
				if (p1[0] == p2[0] &&
					p1[1] == p2[1] &&
					p1[2] == p2[2])
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				double *dest = &mInputVertices[mPointCount * 3];
				dest[0] = p1[0];
				dest[1] = p1[1];
				dest[2] = p1[2];
				mPointCount++;
			}
			p1 += stridePoints;
		}
	}


	virtual ~NearestPointImpl(void)
	{
		delete[]mInputVertices;
		delete[]mQueryID;
	}

	// resets an internal counter to identify when we he unique or duplicated vertices
	virtual void startFreshQuery(void)
	{
		mQueryCount++;
	}

	// Finds the nearest vertex to the input Vertex.  Returns true if this is the first time
	// we have returned this vertex for this query, false if it is not.
	// This is to prevent adding duplicate vertices as input to the convex hull generator
	virtual bool getNearestVert(const double *inputVertex, double *outputVertex)
	{
		bool ret = true;

		uint32_t foundIndex = 0xFFFFFFFF;

		double nearest = FLT_MAX;

		for (uint32_t i = 0; i < mPointCount; i++)
		{
			const double *pos = &mInputVertices[i * 3];
			double dx = pos[0] - inputVertex[0];
			double dy = pos[1] - inputVertex[1];
			double dz = pos[2] - inputVertex[2];
			double distSquared = dx*dx + dy*dy + dz*dz;
			if (distSquared < nearest)
			{
				nearest = distSquared;
				foundIndex = i;
			}
		}

		if (foundIndex != 0xFFFFFFFF)
		{
			const double *pos = &mInputVertices[foundIndex * 3];
			outputVertex[0] = pos[0];
			outputVertex[1] = pos[1];
			outputVertex[2] = pos[2];
			if (mQueryCount == mQueryID[foundIndex])
			{
				ret = false;
			}
			else
			{
				mQueryID[foundIndex] = mQueryCount;
			}
		}

		return ret;
	}

	virtual void release(void)
	{
		delete this;
	}

	uint32_t	mPointCount{ 0 };
	double		*mInputVertices{ nullptr };
	uint32_t	mQueryCount{ 0 };
	uint32_t	*mQueryID{ nullptr };
};

NearestPoint *NearestPoint::createNearestPoint(uint32_t pointCount, const double *inputVertices, uint32_t stridePoints)
{
	NearestPointImpl *ret = new NearestPointImpl(pointCount, inputVertices,stridePoints);
	return static_cast<NearestPoint *>(ret);
}

NearestPoint *NearestPoint::createNearestPoint(uint32_t pointCount, const float *inputVertices, uint32_t stridePoints)
{
	NearestPointImpl *ret = new NearestPointImpl(pointCount, inputVertices, stridePoints);
	return static_cast<NearestPoint *>(ret);
}



} // end of VHACD namespace
