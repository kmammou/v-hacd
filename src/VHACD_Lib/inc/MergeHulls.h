#ifndef MERGE_HULLS_H

#define MERGE_HULLS_H

#include "../public/VHACD.h"
#include <stdint.h>
#include <vector>



namespace VHACD
{

class MergeHull
{
public:
	uint32_t			mTriangleCount;
	uint32_t			mVertexCount;
	const double		*mVertices;
	const uint32_t		*mIndices;
};

typedef std::vector< MergeHull > MergeHullVector;

class MergeHullsInterface
{
public:
	// Merge these input hulls.
	virtual uint32_t mergeHulls(const MergeHullVector &inputHulls,
									MergeHullVector &outputHulls,
									uint32_t	mergeHullCount,
									double smallClusterThreshold,
									uint32_t maxHullVertices,
									VHACD::IVHACD::IUserCallback *callback) = 0;

	// Cancel the operation before it is completed...
	virtual void cancel(void) = 0;


	virtual void release(void) = 0;

protected:
	virtual ~MergeHullsInterface(void)
	{

	}

};

MergeHullsInterface * createMergeHullsInterface(void);

}; // end of VHACD namespace

#endif
