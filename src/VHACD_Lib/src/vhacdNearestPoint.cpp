#include "vhacdNearestPoint.h"
#include <string.h>
#include <float.h>
#include <vector>
#include <assert.h>

namespace VHACD
{


class KdTreeNode;

typedef std::vector< KdTreeNode * > KdTreeNodeVector;

enum Axes
{
	X_AXIS = 0,
	Y_AXIS = 1,
	Z_AXIS = 2
};

class KdTreeFindNode
{
public:
	KdTreeNode  *mNode{ nullptr };
	double        mRadiusSquared{ FLT_MAX };
	double			mRadius{ FLT_MAX };
};

		class KdTreeInterface
		{
		public:
			virtual const double * getPositionDouble(uint32_t index) const = 0;
		};

		class KdTreeNode
		{
		public:
			KdTreeNode(void)
			{
				mIndex = 0;
				mLeft = 0;
				mRight = 0;
			}

			KdTreeNode(uint32_t index)
			{
				mIndex = index;
				mLeft = 0;
				mRight = 0;
			};

			~KdTreeNode(void)
			{
			}


			void addDouble(KdTreeNode *node, Axes dim, const KdTreeInterface *iface)
			{
				const double *nodePosition = iface->getPositionDouble(node->mIndex);
				const double *position = iface->getPositionDouble(mIndex);
				switch (dim)
				{
				case X_AXIS:
					if (nodePosition[0] <= position[0])
					{
						if (mLeft)
							mLeft->addDouble(node, Y_AXIS, iface);
						else
							mLeft = node;
					}
					else
					{
						if (mRight)
							mRight->addDouble(node, Y_AXIS, iface);
						else
							mRight = node;
					}
					break;
				case Y_AXIS:
					if (nodePosition[1] <= position[1])
					{
						if (mLeft)
							mLeft->addDouble(node, Z_AXIS, iface);
						else
							mLeft = node;
					}
					else
					{
						if (mRight)
							mRight->addDouble(node, Z_AXIS, iface);
						else
							mRight = node;
					}
					break;
				case Z_AXIS:
					if (nodePosition[2] <= position[2])
					{
						if (mLeft)
							mLeft->addDouble(node, X_AXIS, iface);
						else
							mLeft = node;
					}
					else
					{
						if (mRight)
							mRight->addDouble(node, X_AXIS, iface);
						else
							mRight = node;
					}
					break;
				}

			}


			uint32_t getIndex(void) const { return mIndex; };

			void search(Axes axis,
					   const double *pos,
					   KdTreeFindNode &found,
					const KdTreeInterface *iface)
			{

				const double *position = iface->getPositionDouble(mIndex);

				double dx = pos[0] - position[0];
				double dy = pos[1] - position[1];
				double dz = pos[2] - position[2];

				KdTreeNode *search1 = 0;
				KdTreeNode *search2 = 0;

				switch (axis)
				{
				case X_AXIS:
					if (dx <= 0)     // JWR  if we are to the left
					{
						search1 = mLeft; // JWR  then search to the left
						if (-dx < found.mRadius)  // JWR  if distance to the right is less than our search radius, continue on the right as well.
							search2 = mRight;
					}
					else
					{
						search1 = mRight; // JWR  ok, we go down the left tree
						if (dx < found.mRadius) // JWR  if the distance from the right is less than our search radius
							search2 = mLeft;
					}
					axis = Y_AXIS;
					break;
				case Y_AXIS:
					if (dy <= 0)
					{
						search1 = mLeft;
						if (-dy < found.mRadius)
							search2 = mRight;
					}
					else
					{
						search1 = mRight;
						if (dy < found.mRadius)
							search2 = mLeft;
					}
					axis = Z_AXIS;
					break;
				case Z_AXIS:
					if (dz <= 0)
					{
						search1 = mLeft;
						if (-dz < found.mRadius)
							search2 = mRight;
					}
					else
					{
						search1 = mRight;
						if (dz < found.mRadius)
							search2 = mLeft;
					}
					axis = X_AXIS;
					break;
				}

				double m = dx*dx + dy*dy + dz*dz;

				if (m < found.mRadiusSquared )
				{
					found.mNode = this;
					found.mRadiusSquared = m;
					found.mRadius = sqrt(m);
				}


				if (search1)
					search1->search(axis, pos, found, iface);

				if (search2)
					search2->search(axis, pos, found, iface);

			}

			const double *getPosition(const KdTreeInterface *iface)
			{
				return iface->getPositionDouble(mIndex);
			}

		private:

			void setLeft(KdTreeNode *left) { mLeft = left; };
			void setRight(KdTreeNode *right) { mRight = right; };

			KdTreeNode *getLeft(void) { return mLeft; }
			KdTreeNode *getRight(void) { return mRight; }

			uint32_t          mIndex;
			KdTreeNode     *mLeft;
			KdTreeNode     *mRight;
		};


#define MAX_BUNDLE_SIZE 1024  // 1024 nodes at a time, to minimize memory allocation and guarantee that pointers are persistent.

		class KdTreeNodeBundle 
		{
		public:

			KdTreeNodeBundle(void)
			{
				mNext = 0;
				mIndex = 0;
			}

			bool isFull(void) const
			{
				return (bool)(mIndex == MAX_BUNDLE_SIZE);
			}

			KdTreeNode * getNextNode(void)
			{
				assert(mIndex<MAX_BUNDLE_SIZE);
				KdTreeNode *ret = &mNodes[mIndex];
				mIndex++;
				return ret;
			}

			KdTreeNodeBundle  *mNext;
			uint32_t             mIndex;
			KdTreeNode         mNodes[MAX_BUNDLE_SIZE];
		};


		typedef std::vector< double > DoubleVector;

		class KdTree : public KdTreeInterface
		{
		public:
			KdTree(void)
			{
				mRoot = 0;
				mBundle = 0;
				mVcount = 0;
			}

			virtual ~KdTree(void)
			{
				reset();
			}

			const double * getPositionDouble(uint32_t index) const
			{
				return  &mVerticesDouble[index * 3];
			}

			void search(const double *inPosition,double *outPosition,uint32_t &foundIndex) const
			{
				if (!mRoot)	return;
				KdTreeFindNode found;
				mRoot->search(X_AXIS, inPosition, found, this);
				if (found.mNode)
				{
					const double *pos = found.mNode->getPosition(this);
					foundIndex = found.mNode->getIndex();
					outPosition[0] = pos[0];
					outPosition[1] = pos[1];
					outPosition[2] = pos[2];
				}
			}

			void reset(void)
			{
				mRoot = 0;
				mVerticesDouble.clear();
				KdTreeNodeBundle *bundle = mBundle;
				while (bundle)
				{
					KdTreeNodeBundle *next = bundle->mNext;
					delete bundle;
					bundle = next;
				}
				mBundle = 0;
				mVcount = 0;
			}

			uint32_t add(double x, double y, double z)
			{
				uint32_t ret = mVcount;
				mVerticesDouble.push_back(x);
				mVerticesDouble.push_back(y);
				mVerticesDouble.push_back(z);
				mVcount++;
				KdTreeNode *node = getNewNode(ret);
				if (mRoot)
				{
					mRoot->addDouble(node, X_AXIS, this);
				}
				else
				{
					mRoot = node;
				}
				return ret;
			}

			KdTreeNode * getNewNode(uint32_t index)
			{
				if (mBundle == 0)
				{
					mBundle = new KdTreeNodeBundle;
				}
				if (mBundle->isFull())
				{
					KdTreeNodeBundle *bundle = new KdTreeNodeBundle;
					mBundle->mNext = bundle;
					mBundle = bundle;
				}
				KdTreeNode *node = mBundle->getNextNode();
				new (node) KdTreeNode(index);
				return node;
			}

			const double * getVerticesDouble(void) const
			{
				const double *ret = 0;
				if (!mVerticesDouble.empty())
				{
					ret = &mVerticesDouble[0];
				}
				return ret;
			}

			uint32_t getVcount(void) const { return mVcount; };

		private:
			KdTreeNode             *mRoot;
			KdTreeNodeBundle       *mBundle;
			uint32_t                mVcount;
			DoubleVector            mVerticesDouble;
		};



class NearestPointImpl : public NearestPoint
{
public:
	NearestPointImpl(uint32_t pointCount, const double *inputVertices,uint32_t stridePoints)
	{
		mQueryID = new uint32_t[pointCount];
		memset(mQueryID, 0xFF, sizeof(uint32_t)*pointCount);
		const double *p1 = inputVertices;
		for (uint32_t i = 0; i < pointCount; i++)
		{
			mKdTree.add(p1[0], p1[1], p1[2]);
			p1 += stridePoints;
		}
	}

	NearestPointImpl(uint32_t pointCount, const float *inputVertices, uint32_t stridePoints)
	{
		mQueryID = new uint32_t[pointCount];
		memset(mQueryID, 0xFF, sizeof(uint32_t)*pointCount);
		const float *p1 = inputVertices;
		for (uint32_t i = 0; i < pointCount; i++)
		{
			mKdTree.add(p1[0], p1[1], p1[2]);
			p1 += stridePoints;
		}
	}


	virtual ~NearestPointImpl(void)
	{
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

		uint32_t foundIndex = 0;
		mKdTree.search(inputVertex,outputVertex,foundIndex);
		if (mQueryID[foundIndex] == mQueryCount)
		{
			ret = false; // we have already returned this result once
		}
		else
		{
			mQueryID[foundIndex] = mQueryCount; // update the query counter for this vertex
		}

		return ret;
	}

	virtual void release(void)
	{
		delete this;
	}

	uint32_t	mQueryCount{ 0 };
	uint32_t	*mQueryID{ nullptr };
	KdTree		mKdTree;
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
