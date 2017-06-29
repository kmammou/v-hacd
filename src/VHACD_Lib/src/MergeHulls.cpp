#include "../inc/MergeHulls.h"
#include "../inc/vhacdICHull.h"
#include <string.h>
#include <math.h>
#include <unordered_map>
#include <assert.h>
#include <atomic>

#define HACD_ALLOC(x) malloc(x)
#define HACD_FREE(x) free(x)
#define HACD_ASSERT(x) assert(x)

namespace VHACD
{

typedef std::unordered_map< uint32_t, double > TestedMap;

static double fm_computeBestFitAABB(uint32_t vcount,const double *points,uint32_t pstride,double *bmin,double *bmax) // returns the diagonal distance
{

	const uint8_t *source = (const uint8_t *) points;

	bmin[0] = points[0];
	bmin[1] = points[1];
	bmin[2] = points[2];

	bmax[0] = points[0];
	bmax[1] = points[1];
	bmax[2] = points[2];


	for (uint32_t i=1; i<vcount; i++)
	{
		source+=pstride;
		const double *p = (const double *) source;

		if ( p[0] < bmin[0] ) bmin[0] = p[0];
		if ( p[1] < bmin[1] ) bmin[1] = p[1];
		if ( p[2] < bmin[2] ) bmin[2] = p[2];

		if ( p[0] > bmax[0] ) bmax[0] = p[0];
		if ( p[1] > bmax[1] ) bmax[1] = p[1];
		if ( p[2] > bmax[2] ) bmax[2] = p[2];

	}

	double dx = bmax[0] - bmin[0];
	double dy = bmax[1] - bmin[1];
	double dz = bmax[2] - bmin[2];

	return sqrt( dx*dx + dy*dy + dz*dz );

}




static bool fm_intersectAABB(const double *bmin1,const double *bmax1,const double *bmin2,const double *bmax2)
{
	if ((bmin1[0] > bmax2[0]) || (bmin2[0] > bmax1[0])) return false;
	if ((bmin1[1] > bmax2[1]) || (bmin2[1] > bmax1[1])) return false;
	if ((bmin1[2] > bmax2[2]) || (bmin2[2] > bmax1[2])) return false;
	return true;
}


static inline double det(const double *p1,const double *p2,const double *p3)
{
	return  p1[0]*p2[1]*p3[2] + p2[0]*p3[1]*p1[2] + p3[0]*p1[1]*p2[2] -p1[0]*p3[1]*p2[2] - p2[0]*p1[1]*p3[2] - p3[0]*p2[1]*p1[2];
}


static double  fm_computeMeshVolume(const double *vertices,uint32_t tcount,const uint32_t *indices)
{
	double volume = 0;
	for (uint32_t i=0; i<tcount; i++,indices+=3)
	{
		const double *p1 = &vertices[ indices[0]*3 ];
		const double *p2 = &vertices[ indices[1]*3 ];
		const double *p3 = &vertices[ indices[2]*3 ];
		volume+=det(p1,p2,p3); // compute the volume of the tetrahedran relative to the origin.
	}

	volume*=(1.0f/6.0f);
	if ( volume < 0 )
		volume*=-1;
	return volume;
}



class CHull
	{
	public:
		CHull(uint32_t vcount,const double *vertices,uint32_t tcount,const uint32_t *indices,uint32_t guid)
		{
			mGuid = guid;
			mVertexCount = vcount;
			mTriangleCount = tcount;
			mVertices = (double *)HACD_ALLOC(sizeof(double)*3*vcount);
			memcpy(mVertices,vertices,sizeof(double)*3*vcount);
			mIndices = (uint32_t *)HACD_ALLOC(sizeof(uint32_t)*3*tcount);
			memcpy(mIndices,indices,sizeof(uint32_t)*3*tcount);
			mVolume = fm_computeMeshVolume( mVertices, mTriangleCount, mIndices);
			mDiagonal = fm_computeBestFitAABB( mVertexCount, mVertices, sizeof(double)*3, mMin, mMax );
			double dx = mMax[0] - mMin[0];
			double dy = mMax[1] - mMin[1];
			double dz = mMax[2] - mMin[2];

			dx*=0.1f; // inflate 1/10th on each edge
			dy*=0.1f; // inflate 1/10th on each edge
			dz*=0.1f; // inflate 1/10th on each edge

			mMin[0]-=dx;
			mMin[1]-=dy;
			mMin[2]-=dz;

			mMax[0]+=dx;
			mMax[1]+=dy;
			mMax[2]+=dz;
		}

		~CHull(void)
		{
			HACD_FREE(mVertices);
			HACD_FREE(mIndices);
		}

		bool overlap(const CHull &h) const
		{
			return fm_intersectAABB(mMin,mMax, h.mMin, h.mMax );
		}

		uint32_t			mGuid;
		double		mMin[3];
		double		mMax[3];
		double		mVolume;
		double		mDiagonal; // long edge..
		uint32_t			mVertexCount;
		uint32_t			mTriangleCount;
		double			*mVertices;
		uint32_t			*mIndices;
	};

	// Usage: std::sort( list.begin(), list.end(), StringSortRef() );
	class CHullSort
	{
	public:

		bool operator()(const CHull *a,const CHull *b) const
		{
			return a->mVolume < b->mVolume;
		}
	};



typedef std::vector< CHull * > CHullVector;

class MyMergeHullsInterface : public MergeHullsInterface
{
public:
	MyMergeHullsInterface(void)
	{
	}

	virtual ~MyMergeHullsInterface(void)
	{

	}

	// Merge these input hulls.
	virtual uint32_t mergeHulls(const MergeHullVector &inputHulls,
								MergeHullVector &outputHulls,
								uint32_t mergeHullCount,
								double smallClusterThreshold,
								uint32_t maxHullVertices,
								VHACD::IVHACD::IUserCallback *callback) final
	{
		mGuid = 0;

		uint32_t count = (uint32_t)inputHulls.size();
		mSmallClusterThreshold = smallClusterThreshold;
		mMaxHullVertices = maxHullVertices;
		mMergeNumHulls = mergeHullCount;

		mTotalVolume = 0;
		for (uint32_t i = 0; i < inputHulls.size(); i++)
		{
			const MergeHull &h = inputHulls[i];
			CHull *ch = new CHull(h.mVertexCount, h.mVertices, h.mTriangleCount, h.mIndices, mGuid++);
			mChulls.push_back(ch);
			mTotalVolume += ch->mVolume;
			if (callback)
			{
				double fraction = (double)i / (double)inputHulls.size();
				callback->Update(1, 1, fraction, "MergeHulls", "Gathering Hulls To Merge");
			}
			if (mCancel)
			{
				return 0;
			}
		}

		//
		uint32_t mergeCount = count - mergeHullCount;
		uint32_t mergeIndex = 0;

		for (;;)
		{
			if (callback)
			{
				double fraction = (double)mergeIndex / (double)mergeCount;
				callback->Update(1, 1, fraction, "MergeHulls", "Merging");
			}
			bool combined = combineHulls(); // mege smallest hulls first, up to the max merge count.
			if (!combined) break;
			mergeIndex++;
			if (mCancel)
			{
				return 0;
			}

		}

		// return results..
		for (uint32_t i = 0; i < mChulls.size(); i++)
		{
			CHull *ch = mChulls[i];
			MergeHull mh;
			mh.mVertexCount = ch->mVertexCount;
			mh.mTriangleCount = ch->mTriangleCount;
			mh.mIndices = ch->mIndices;
			mh.mVertices = ch->mVertices;
			outputHulls.push_back(mh);
			if (callback)
			{
				double fraction = (double)i / (double)mChulls.size();
				callback->Update(1, 1, fraction, "MergeHulls", "Gathering Merged Hulls Output");
			}
			if (mCancel)
			{
				return 0;
			}

		}
		return (uint32_t)outputHulls.size();
	}

	virtual void ConvexDecompResult(uint32_t hvcount, const double *hvertices, uint32_t htcount, const uint32_t *hindices)
	{
		CHull *ch = new CHull(hvcount, hvertices, htcount, hindices, mGuid++);
		if (ch->mVolume > 0.00001f)
		{
			mChulls.push_back(ch);
		}
		else
		{
			delete ch;
		}
	}


	virtual void release(void)
	{
		delete this;
	}

	static double canMerge(CHull *a, CHull *b)
	{
		if (!a->overlap(*b)) return 0; // if their AABB's (with a little slop) don't overlap, then return.

		// ok..we are going to combine both meshes into a single mesh
		// and then we are going to compute the concavity...
		double ret = 0;

		uint32_t combinedVertexCount = a->mVertexCount + b->mVertexCount;
		double *combinedVertices = (double *)HACD_ALLOC(combinedVertexCount * sizeof(double) * 3);
		double *dest = combinedVertices;
		memcpy(dest, a->mVertices, sizeof(double) * 3 * a->mVertexCount);
		dest += a->mVertexCount * 3;
		memcpy(dest, b->mVertices, sizeof(double) * 3 * b->mVertexCount);

		VHACD::ICHull hl;
		hl.AddPoints((const VHACD::Vec3<double> *)combinedVertices, combinedVertexCount);
		VHACD::ICHullError err = hl.Process();
		if (err == VHACD::ICHullErrorOK)
		{
			VHACD::TMMesh& mesh = hl.GetMesh();
			uint32_t tcount = (uint32_t)mesh.GetNTriangles();
			uint32_t *indices = (uint32_t *)HACD_ALLOC(tcount * sizeof(uint32_t) * 3);
			mesh.GetIFS((VHACD::Vec3<double> *)combinedVertices, (VHACD::Vec3<int> *)indices);
			ret = fm_computeMeshVolume(combinedVertices, tcount, indices);
			HACD_FREE(indices);
		}
		HACD_FREE(combinedVertices);
		return ret;
	}


	CHull * doMerge(CHull *a, CHull *b)
	{
		CHull *ret = 0;
		uint32_t combinedVertexCount = a->mVertexCount + b->mVertexCount;
		double *combinedVertices = (double *)HACD_ALLOC(combinedVertexCount * sizeof(double) * 3);
		double *dest = combinedVertices;
		memcpy(dest, a->mVertices, sizeof(double) * 3 * a->mVertexCount);
		dest += a->mVertexCount * 3;
		memcpy(dest, b->mVertices, sizeof(double) * 3 * b->mVertexCount);

		VHACD::ICHull hl;
		hl.AddPoints((const VHACD::Vec3<double> *)combinedVertices, combinedVertexCount);
		VHACD::ICHullError err = hl.Process();
		if (err == VHACD::ICHullErrorOK)
		{
			VHACD::TMMesh& mesh = hl.GetMesh();
			uint32_t tcount = (uint32_t)mesh.GetNTriangles();
			uint32_t vcount = (uint32_t)mesh.GetNVertices();
			uint32_t *indices = (uint32_t *)HACD_ALLOC(tcount * sizeof(uint32_t) * 3);
			mesh.GetIFS((VHACD::Vec3<double> *)combinedVertices, (VHACD::Vec3<int> *)indices);
			ret = new CHull(vcount, combinedVertices, tcount, indices, mGuid++);
			HACD_FREE(indices);
		}
		HACD_FREE(combinedVertices);
		return ret;
	}

	class CombineVolumeJob
	{
	public:
		CombineVolumeJob(CHull *hullA, CHull *hullB, uint32_t hashIndex)
		{
			mHullA = hullA;
			mHullB = hullB;
			mHashIndex = hashIndex;
			mCombinedVolume = canMerge(mHullA, mHullB);
		}

		uint32_t	mHashIndex;
		CHull		*mHullA;
		CHull		*mHullB;
		double		mCombinedVolume;
	};

	bool combineHulls(void)
	{
		bool combine = false;
		// each new convex hull is given a unique guid.
		// A hash map is used to make sure that no hulls are tested twice.
		CHullVector output;
		uint32_t count = (uint32_t)mChulls.size();

		// Early out to save walking all the hulls. Hulls are combined based on 
		// a target number or on a number of generated hulls.
		bool mergeTargetMet = (uint32_t)mChulls.size() <= mMergeNumHulls;
		if (mergeTargetMet && (mSmallClusterThreshold == 0.0f))
		{
			return false;
		}

		std::vector< CombineVolumeJob > jobs;

		// First, see if there are any pairs of hulls who's combined volume we have not yet calculated.
		// If there are, then we add them to the jobs list
		{
			for (uint32_t i = 0; i < count; i++)
			{
				CHull *cr = mChulls[i];
				for (uint32_t j = i + 1; j < count; j++)
				{
					CHull *match = mChulls[j];
					uint32_t hashIndex;
					if (match->mGuid < cr->mGuid)
					{
						hashIndex = (match->mGuid << 16) | cr->mGuid;
					}
					else
					{
						hashIndex = (cr->mGuid << 16) | match->mGuid;
					}

					TestedMap::iterator found = mHasBeenTested.find(hashIndex);
					if (found == mHasBeenTested.end())
					{
						CombineVolumeJob job(cr, match, hashIndex);
						jobs.push_back(job);
						mHasBeenTested[hashIndex] = 0.0f;  // assign it to some value so we don't try to create more than one job for it.
					}
				}
			}
		}

		// once we have the answers, now put the results into the hash table.
		for (uint32_t i = 0; i < jobs.size(); i++)
		{
			CombineVolumeJob &job = jobs[i];
			mHasBeenTested[job.mHashIndex] = job.mCombinedVolume;
		}

		double bestVolume = 1e9;
		CHull *mergeA = NULL;
		CHull *mergeB = NULL;
		// now find the two hulls which merged produce the smallest combined volume.
		{
			for (uint32_t i = 0; i < count; i++)
			{
				CHull *cr = mChulls[i];
				for (uint32_t j = i + 1; j < count; j++)
				{
					CHull *match = mChulls[j];
					uint32_t hashIndex;
					if (match->mGuid < cr->mGuid)
					{
						hashIndex = (match->mGuid << 16) | cr->mGuid;
					}
					else
					{
						hashIndex = (cr->mGuid << 16) | match->mGuid;
					}

					TestedMap::iterator found = mHasBeenTested.find(hashIndex);
					HACD_ASSERT(found != mHasBeenTested.end());
					if (found != mHasBeenTested.end())
					{
						double v = (*found).second;
						if (v != 0 && v < bestVolume)
						{
							bestVolume = v;
							mergeA = cr;
							mergeB = match;
						}
					}
				}
			}
		}

		// If we found a merge pair, and we are below the merge threshold or we haven't reduced to the target
		// do the merge.
		bool thresholdBelow = ((bestVolume / mTotalVolume) * 100.0f) < mSmallClusterThreshold;
		if (mergeA && (thresholdBelow || !mergeTargetMet))
		{
			CHull *merge = doMerge(mergeA, mergeB);

			double volumeA = mergeA->mVolume;
			double volumeB = mergeB->mVolume;

			if (merge)
			{
				combine = true;
				output.push_back(merge);
				for (CHullVector::iterator j = mChulls.begin(); j != mChulls.end(); ++j)
				{
					CHull *h = (*j);
					if (h != mergeA && h != mergeB)
					{
						output.push_back(h);
					}
				}
				delete mergeA;
				delete mergeB;
				// Remove the old volumes and add the new one.
				mTotalVolume -= (volumeA + volumeB);
				mTotalVolume += merge->mVolume;
			}
			mChulls = output;
		}

		return combine;
	}

	virtual void cancel(void) final
	{
		mCancel = true;
	}

private:
	TestedMap			mHasBeenTested;
	uint32_t			mGuid;
	double				mTotalVolume;
	double				mSmallClusterThreshold;
	uint32_t			mMergeNumHulls;
	uint32_t			mMaxHullVertices;
	CHullVector			mChulls;
	std::atomic<bool>	mCancel{ false };
};

MergeHullsInterface * createMergeHullsInterface(void)
{
	MyMergeHullsInterface *m = new MyMergeHullsInterface;
	return static_cast< MergeHullsInterface *>(m);
}


};