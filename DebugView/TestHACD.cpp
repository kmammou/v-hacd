#include "TestHACD.h"
#include "NvRenderDebug.h"
#include "PhysicsDOM.h"
#include "FloatMath.h"
#include "StringHelper.h"
#include "PhysicsDOMDef.h"
#include "PhysicsDOMHelper.h"
#include "PhysicsDOMInterface.h"
#include "NvPhysicsDOM.h"
#include "SkeletonRig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#pragma warning(disable:4100)

#define SAFE_RELEASE(x) if ( x ) { x->release(); x = nullptr; }

#define DEFAULT_MASS 10
#define TSCALE1 (1.0f/4.0f)
#define USE_DOM 1

class Mat44
{
public:
	float	mXform[16];
};

class WeightedVertex
{
public:
	void applyBoneWeighting(const RENDER_DEBUG::RenderDebugMeshVertex &source,
		RENDER_DEBUG::RenderDebugMeshVertex &dest,
		const Mat44 *compositeTransforms) const
	{
		// Initialize position and rotation to zero
		dest.mPosition[0] = 0;
		dest.mPosition[1] = 0;
		dest.mPosition[2] = 0;
		dest.mNormal[0] = 0;
		dest.mNormal[1] = 0;
		dest.mNormal[2] = 0;

		// Compute the weighted sum of the transformed position and rotated normal vectors
		for (uint32_t i = 0; i < 4; i++)
		{
			if (mBoneWeighting[i] == 0)
			{
				break;
			}
			applyPositionWeighting(source.mPosition, dest.mPosition, compositeTransforms[mBoneIndices[i]], mBoneWeighting[i]);
			applyRotationWeighting(source.mNormal, dest.mNormal, compositeTransforms[mBoneIndices[i]], mBoneWeighting[i]);
		}
	}

	void applyPositionWeighting(const float source[3], float dest[3], const Mat44 &xform,float weighting) const
	{
		float xpos[3];
		FLOAT_MATH::fm_transform(xform.mXform, source, xpos);
		dest[0] += xpos[0] * weighting;
		dest[1] += xpos[1] * weighting;
		dest[2] += xpos[2] * weighting;
	}

	void applyRotationWeighting(const float source[3], float dest[3], const Mat44 &xform, float weighting) const
	{
		float xpos[3];
		FLOAT_MATH::fm_rotate(xform.mXform, source, xpos);
		dest[0] += xpos[0] * weighting;
		dest[1] += xpos[1] * weighting;
		dest[2] += xpos[2] * weighting;
	}


	uint32_t	mBoneIndices[4];
	float		mBoneWeighting[4];
};

class MeshBuilder
{
public:
	MeshBuilder(void)
	{

	}
	~MeshBuilder(void)
	{
		reset();
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
		FLOAT_MATH::fm_computePlane(p3, p2, p1, normal);

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

	void reset(void)
	{
		mVertices.clear();
		delete[]mTransformedVertices;
		delete[]mWeightedVertices;
		delete[]mInverseRestPoses;
		delete[]mSimulatedPoses;
		delete[]mCompositePoses;
		mTransformedVertices	= nullptr;
		mWeightedVertices		= nullptr;
		mInverseRestPoses		= nullptr;
		mSimulatedPoses			= nullptr;
		mCompositePoses			= nullptr;
		mBodyCount				= 0;
	}

	uint32_t getTriangleCount(void) const
	{
		return uint32_t(mVertices.size()) / 3;
	}

	const RENDER_DEBUG::RenderDebugMeshVertex *getVertices(void) const
	{
		const RENDER_DEBUG::RenderDebugMeshVertex *ret = nullptr;
		if (!mVertices.empty())
		{
			ret = &mVertices[0];
		}
		return ret;
	}

	const WeightedVertex *getWeightedVertices(void) const
	{
		return mWeightedVertices;
	}

	inline uint32_t getVertexCount(void) const
	{
		return uint32_t(mVertices.size());
	}

	const RENDER_DEBUG::RenderDebugMeshVertex *getTransformedVertices(PHYSICS_DOM::NodeState **states)
	{
		const RENDER_DEBUG::RenderDebugMeshVertex *ret = mTransformedVertices;
		if ( states )
		{
			for (uint32_t i = 0; i < mBodyCount; i++)
			{
				NV_MATH::NvTransform t;
				states[i]->getPose(&t.p.x, &t.q.x);
				NV_MATH::NvMat44 xform(t);
				memcpy(mSimulatedPoses[i].mXform, &xform, sizeof(xform));
				FLOAT_MATH::fm_multiplyTransform(mInverseRestPoses[i].mXform, mSimulatedPoses[i].mXform, mCompositePoses[i].mXform);
			}
			for (uint32_t i = 0; i < getVertexCount(); i++)
			{
				const RENDER_DEBUG::RenderDebugMeshVertex &sourceVertex = mVertices[i];
				RENDER_DEBUG::RenderDebugMeshVertex &destVertex = mTransformedVertices[i];
				const WeightedVertex &wv = mWeightedVertices[i];
				wv.applyBoneWeighting(sourceVertex, destVertex, mCompositePoses);
			}
		}
		else
		{
			ret = &mVertices[0];
		}

		return ret;
	}

    void buildWeightedVertices(skeletonrig::SkeletonRig *vhacd, uint32_t bodyCount, double boneWeightFalloff, double boneWeightPercentage, PHYSICS_DOM::NodeState **states)
    {
        delete mInverseRestPoses;
        mInverseRestPoses = new Mat44[bodyCount];

        delete[]mCompositePoses;
        mCompositePoses = new Mat44[bodyCount];

        delete[]mSimulatedPoses;
        mSimulatedPoses = new Mat44[bodyCount];

        if (states)
        {
            for (uint32_t i = 0; i < bodyCount; i++)
            {
                NV_MATH::NvTransform t;
                states[i]->getPose(&t.p.x, &t.q.x);
                NV_MATH::NvMat44 xform(t);
                memcpy(mSimulatedPoses[i].mXform, &xform, sizeof(xform));
                FLOAT_MATH::fm_inverseTransform(mSimulatedPoses[i].mXform, mInverseRestPoses[i].mXform);
                FLOAT_MATH::fm_multiplyTransform(mInverseRestPoses[i].mXform, mSimulatedPoses[i].mXform, mCompositePoses[i].mXform);
            }
        }

        mBodyCount = bodyCount;
        delete[]mWeightedVertices;
        mWeightedVertices = new WeightedVertex[getVertexCount()];

        delete[]mTransformedVertices;
        mTransformedVertices = new RENDER_DEBUG::RenderDebugMeshVertex[getVertexCount()];

        for (uint32_t i = 0; i < getVertexCount(); i++)
        {
            double position[3];

            position[0] = mVertices[i].mPosition[0];
            position[1] = mVertices[i].mPosition[1];
            position[2] = mVertices[i].mPosition[2];

            double weightings[4];
            WeightedVertex &wv = mWeightedVertices[i];

            vhacd->getBoneWeightings(position, boneWeightFalloff, boneWeightPercentage, wv.mBoneIndices, weightings);

            wv.mBoneWeighting[0] = float(weightings[0]);
            wv.mBoneWeighting[1] = float(weightings[1]);
            wv.mBoneWeighting[2] = float(weightings[2]);
            wv.mBoneWeighting[3] = float(weightings[3]);
        }
    }

	bool isSelected(uint32_t index, uint32_t body) const
	{
		bool ret = false;

		if (mWeightedVertices[index * 3 + 0].mBoneIndices[0] == body ||
			mWeightedVertices[index * 3 + 1].mBoneIndices[0] == body ||
			mWeightedVertices[index * 3 + 2].mBoneIndices[0] == body)
		{
			ret = true;
		}

		return ret;
	}

	void resetSimulation(void)
	{
		delete mInverseRestPoses;
		mInverseRestPoses = nullptr;
		delete[]mCompositePoses;
		mCompositePoses = nullptr;
		delete[]mSimulatedPoses;
		mSimulatedPoses = nullptr;
		mBodyCount = 0;
		delete[]mWeightedVertices;
		mWeightedVertices = nullptr;
	}

	uint32_t											mBodyCount{ 0 };
	Mat44												*mCompositePoses{ nullptr };		// Composite transforms; current pose of the bone times the inverse rest position
	Mat44												*mSimulatedPoses{ nullptr };		// The current simulated transforms
	Mat44												*mInverseRestPoses{ nullptr };		// inverse rest poses for the constraints
	WeightedVertex										*mWeightedVertices{ nullptr };
	RENDER_DEBUG::RenderDebugMeshVertex					*mTransformedVertices{ nullptr };
	std::vector< RENDER_DEBUG::RenderDebugMeshVertex >	mVertices;
};


class TestHACDImpl : public TestHACD, public VHACD::IVHACD::IUserCallback, public VHACD::IVHACD::IUserLogger
{
public:
	TestHACDImpl(RENDER_DEBUG::RenderDebug *renderDebug,PHYSICS_DOM::NvPhysicsDOM *pf) : mRenderDebug(renderDebug), mPhysicsDOM(pf)
	{
		mHACD = VHACD::CreateVHACD_ASYNC();
		mCenterOfMass[0] = 0;
		mCenterOfMass[1] = 0;
		mCenterOfMass[2] = 0;
        mSkeletonRigFactory = skeletonrig::createSkeletonRigPlugin(SKELETON_RIG_VERSION,"SkeletonRig64.dll");
//        mSkeletonRigFactory = skeletonrig::createSkeletonRigPlugin(SKELETON_RIG_VERSION,"SkeletonRig64DEBUG.dll");
	}

	virtual ~TestHACDImpl(void)
	{
		mHACD->Release();
		releaseSimulationObjects();
		if (mMeshID)
		{
			mRenderDebug->releaseTriangleMesh(mMeshID);
		}
        SAFE_RELEASE(mSkeletonRig);
	}

	void getExplodePosition(const double source[3], float dest[3], const double diff[3],const float center[3])
	{
		dest[0] = float(source[0] + diff[0] + center[0]);
		dest[1] = float(source[1] + diff[1] + center[1]);
		dest[2] = float(source[2] + diff[2] + center[2]);
	}

	void getExplodePosition(const float source[3], float dest[3], const double diff[3], const float center[3])
	{
		dest[0] = float(source[0] + diff[0] + center[0]);
		dest[1] = float(source[1] + diff[1] + center[1]);
		dest[2] = float(source[2] + diff[2] + center[2]);
	}

	void getExplodeViewPosition(const float center[3], const VHACD::IVHACD::ConvexHull &h, float explodeViewScale, const double sourcePos[3], float destPos[3])
	{
		double diff[3];

		diff[0] = h.m_center[0] - center[0];
		diff[1] = h.m_center[1] - center[1];
		diff[2] = h.m_center[2] - center[2];

		diff[0] *= explodeViewScale;
		diff[1] *= explodeViewScale;
		diff[2] *= explodeViewScale;

		diff[0] -= h.m_center[0];
		diff[1] -= h.m_center[1];
		diff[2] -= h.m_center[2];

		getExplodePosition(sourcePos, destPos, diff, center);
	}

	void getColor(const WeightedVertex &v, uint32_t &color)
	{
		switch (v.mBoneIndices[0] & 3)
		{
			case 0:
				color = 0xFF0000;
				break;
			case 1:
				color = 0x00FF00;
				break;
			case 2:
				color = 0x0000FF;
				break;
		}
	}

	virtual void render(float explodeViewScale,
						const float center[3],
						bool	showSourceMesh,
						bool	sourceMeshWireframe,
						bool	showCollision,
						bool	collisionWireframe,
						bool	showConstraints,
						bool	showSkeleton,
						bool	showCollisionPairs,
						bool	showSelectedVerts,
						uint32_t forwardAxis) final
	{
		if (!mHaveConstraints && mHACD->IsReady() )
		{
			mHaveConstraints = true;
            createSkeletonRig(forwardAxis);
            mSkeletonRig->computeConstraints(forwardAxis);
		}
		if (!mHACD->IsReady()) // if we are still computing the convex decomposition in a background thread, display the current status
		{
			mRenderDebug->debugText2D(0, 0.2f, 0.5f, 2.0f, false, 0xFF8080, "%s : %s : %0.2f : %0.2f : %0.2f\n", mStage.c_str(), mOperation.c_str(), mOverallProgress, mStageProgress, mOperationProgress);
		}
		// We can't currently do the visualization if we are doing a full ragdoll simulation
		// while simulating; so don't try
		if ( mSimulateAsRagdoll && isSimulating())
		{
			if (showSourceMesh && mMeshID )
			{
				const RENDER_DEBUG::RenderDebugMeshVertex *vertices = mMeshBuilder.getTransformedVertices(mRigidBodies);
				if (!sourceMeshWireframe)
				{

					mRenderDebug->refreshTriangleMeshVertices(mMeshID, mMeshBuilder.getVertexCount(), vertices, nullptr);

					RENDER_DEBUG::RenderDebugInstance instance;
					float xform[16];
					FLOAT_MATH::fm_identity(xform);
					instance.mTransform[0] = xform[12];
					instance.mTransform[1] = xform[13];
					instance.mTransform[2] = xform[14];

					instance.mTransform[3] = xform[0];
					instance.mTransform[4] = xform[1];
					instance.mTransform[5] = xform[2];

					instance.mTransform[6] = xform[4];
					instance.mTransform[7] = xform[5];
					instance.mTransform[8] = xform[6];

					instance.mTransform[9] = xform[8];
					instance.mTransform[10] = xform[9];
					instance.mTransform[11] = xform[10];
//					mRenderDebug->setCurrentTexture(RENDER_DEBUG::DebugTextures::IDETAIL01, 1, RENDER_DEBUG::DebugTextures::IDETAIL02,4);
					mRenderDebug->renderTriangleMeshInstances(getMeshID(), 1, &instance);
				}
				else
				{
					uint32_t tcount = mMeshBuilder.getTriangleCount();
					if (vertices)
					{
						mRenderDebug->pushRenderState();
						mRenderDebug->addToCurrentState(RENDER_DEBUG::DebugRenderState::SolidWireShaded);
						mRenderDebug->setCurrentColor(0xFFFF00);
						float xform[16];
						FLOAT_MATH::fm_identity(xform);
						mRenderDebug->setPose(xform);
						uint32_t selectedBody=0;
						bool selected = false; // TODO TODO mCompoundActor->getSelectedBody(selectedBody);
						if (showSelectedVerts && selected )
						{
							for (uint32_t i = 0; i < tcount; i++)
							{
								if (mMeshBuilder.isSelected(i, selectedBody))
								{
									const RENDER_DEBUG::RenderDebugMeshVertex *v1 = &vertices[i * 3 + 0];
									const RENDER_DEBUG::RenderDebugMeshVertex *v2 = &vertices[i * 3 + 1];
									const RENDER_DEBUG::RenderDebugMeshVertex *v3 = &vertices[i * 3 + 2];
									mRenderDebug->debugTri(v1->mPosition, v2->mPosition, v3->mPosition);
								}
							}
						}
						else
						{
							for (uint32_t i = 0; i < tcount; i++)
							{
								const RENDER_DEBUG::RenderDebugMeshVertex *v1 = &vertices[i * 3 + 0];
								const RENDER_DEBUG::RenderDebugMeshVertex *v2 = &vertices[i * 3 + 1];
								const RENDER_DEBUG::RenderDebugMeshVertex *v3 = &vertices[i * 3 + 2];
								mRenderDebug->debugTri(v1->mPosition, v2->mPosition, v3->mPosition);
							}
						}
						mRenderDebug->popRenderState();
					}
				}
			}
			if (mConstraintCount && showConstraints )
			{
				for (uint32_t i = 0; i < mConstraintCount; i++)
				{
					NV_MATH::NvTransform t;
					mConstraints[i]->getPose(&t.p.x, &t.q.x);
					NV_MATH::NvMat44 xform(t);
					mRenderDebug->debugAxes((const float *)&xform,1.0f);
				}
			}
			return;
		}
		if (showSourceMesh)
		{
			if (!sourceMeshWireframe)
			{
				RENDER_DEBUG::RenderDebugInstance instance;
				float xform[16];
				FLOAT_MATH::fm_identity(xform);
				getTransform(xform);
				instance.mTransform[0] = xform[12];
				instance.mTransform[1] = xform[13];
				instance.mTransform[2] = xform[14];

				instance.mTransform[3] = xform[0];
				instance.mTransform[4] = xform[1];
				instance.mTransform[5] = xform[2];

				instance.mTransform[6] = xform[4];
				instance.mTransform[7] = xform[5];
				instance.mTransform[8] = xform[6];

				instance.mTransform[9] = xform[8];
				instance.mTransform[10] = xform[9];
				instance.mTransform[11] = xform[10];
				mRenderDebug->renderTriangleMeshInstances(getMeshID(), 1, &instance);
			}
			else
			{
				uint32_t tcount = mMeshBuilder.getTriangleCount();
				const RENDER_DEBUG::RenderDebugMeshVertex *vertices = mMeshBuilder.getVertices();
				if (vertices)
				{
					mRenderDebug->pushRenderState();
					mRenderDebug->addToCurrentState(RENDER_DEBUG::DebugRenderState::SolidWireShaded);
					mRenderDebug->setCurrentColor(0xFFFF00,0xFF0000);
					float xform[16];
					FLOAT_MATH::fm_identity(xform);
					getTransform(xform);
					mRenderDebug->setPose(xform);
					for (uint32_t i = 0; i < tcount; i++)
					{
						const RENDER_DEBUG::RenderDebugMeshVertex *v1 = &vertices[i * 3 + 0];
						const RENDER_DEBUG::RenderDebugMeshVertex *v2 = &vertices[i * 3 + 1];
						const RENDER_DEBUG::RenderDebugMeshVertex *v3 = &vertices[i * 3 + 2];
						mRenderDebug->debugTri(v1->mPosition, v2->mPosition, v3->mPosition);
					}
					mRenderDebug->popRenderState();
				}
			}
		}
		if (!showCollision)
		{
			return;
		}
		uint32_t hullCount = mHACD->GetNConvexHulls();
		if (hullCount)
		{
			mRenderDebug->pushRenderState();
			float xform[16];
			getTransform(xform);
			mRenderDebug->setPose(xform);
            createSkeletonRig(forwardAxis);
			// Render constraints here...
            const skeletonrig::Constraint *constraints = mSkeletonRig->getConstraints(mConstraintCount);
			for (uint32_t j = 0; j < hullCount; j++)
			{
				VHACD::IVHACD::ConvexHull h;
				mHACD->GetConvexHull(j, h);
				{

					if (collisionWireframe)
					{
						mRenderDebug->removeFromCurrentState(RENDER_DEBUG::DebugRenderState::SolidShaded);
						mRenderDebug->removeFromCurrentState(RENDER_DEBUG::DebugRenderState::SolidWireShaded);
					}
					else
					{
						mRenderDebug->addToCurrentState(RENDER_DEBUG::DebugRenderState::SolidWireShaded);
					}

					uint32_t cindex = (j % 20) + RENDER_DEBUG::DebugColors::Red;

					uint32_t color = mRenderDebug->getDebugColor((RENDER_DEBUG::DebugColors::Enum)cindex);
					mRenderDebug->setCurrentColor(color,0xFFFFFF);

					double diff[3];

					diff[0] = h.m_center[0] - center[0];
					diff[1] = h.m_center[1] - center[1];
					diff[2] = h.m_center[2] - center[2];

					diff[0] *= explodeViewScale;
					diff[1] *= explodeViewScale;
					diff[2] *= explodeViewScale;

					diff[0] -= h.m_center[0];
					diff[1] -= h.m_center[1];
					diff[2] -= h.m_center[2];

					for (uint32_t i = 0; i < h.m_nTriangles; i++)
					{
						uint32_t i1 = h.m_triangles[i * 3 + 0];
						uint32_t i2 = h.m_triangles[i * 3 + 1];
						uint32_t i3 = h.m_triangles[i * 3 + 2];

						const double *p1 = &h.m_points[i1 * 3];
						const double *p2 = &h.m_points[i2 * 3];
						const double *p3 = &h.m_points[i3 * 3];

						float v1[3];
						float v2[3];
						float v3[3];

						getExplodePosition(p1, v1, diff, center);
						getExplodePosition(p2, v2, diff, center);
						getExplodePosition(p3, v3, diff, center);

						mRenderDebug->debugTri(v1, v2, v3);
					}

					if (constraints && showConstraints)
					{
						mRenderDebug->pushRenderState();
						for (uint32_t i = 0; i < mConstraintCount; i++)
						{
                            const skeletonrig::Constraint &c = constraints[i];
							if (c.mHullA == j)
							{
								float p1[3];
								FLOAT_MATH::fm_doubleToFloat3(c.mConstraintPoint, p1);
								float quat[4];
								quat[0] = float(c.mConstraintOrientation[0]);
								quat[1] = float(c.mConstraintOrientation[1]);
								quat[2] = float(c.mConstraintOrientation[2]);
								quat[3] = float(c.mConstraintOrientation[3]);
								float transform[16];
								FLOAT_MATH::fm_quatToMatrix(quat, transform);
								transform[12] = p1[0];
								transform[13] = p1[1];
								transform[14] = p1[2];
								float p2[3];
								float basis[3] = { 1.5f, 0, 0, };
								FLOAT_MATH::fm_transform(transform, basis, p2);
								float v1[3];
								float v2[3];
								getExplodePosition(p1, v1, diff, center);
								getExplodePosition(p2, v2, diff, center);
								mRenderDebug->debugThickRay(v1, v2, 0.01f);
								mRenderDebug->setCurrentColor(0xFF00FF);
								mRenderDebug->addToCurrentState(RENDER_DEBUG::DebugRenderState::SolidShaded);
								mRenderDebug->debugSphere(v1, 0.02f);
							}
						}
						mRenderDebug->popRenderState();
					}
				}
			}
			if (showSkeleton)
			{
				for (uint32_t i = 0; i < mConstraintCount; i++)
				{
                    const skeletonrig::Constraint &c = constraints[i];
					VHACD::IVHACD::ConvexHull h1, h2;

					mHACD->GetConvexHull(c.mHullA, h1);
					mHACD->GetConvexHull(c.mHullB, h2);

					float p1[3];
					float p2[3];

					getExplodeViewPosition(center, h1, explodeViewScale, h1.m_center, p1);
					getExplodeViewPosition(center, h2, explodeViewScale, h2.m_center, p2);

					mRenderDebug->pushRenderState();
					mRenderDebug->setCurrentColor(0xFFFF00);
					mRenderDebug->addToCurrentState(RENDER_DEBUG::DebugRenderState::SolidShaded);
					mRenderDebug->debugSphere(p1, 0.1f);
					mRenderDebug->debugSphere(p2, 0.1f);
					mRenderDebug->debugThickRay(p1, p2, 0.02f);
					mRenderDebug->popRenderState();
				}
			}
			if (showCollisionPairs)
			{
				uint32_t collisionCount;
                const uint32_t *pairs = mSkeletonRig->getCollisionFilterPairs(collisionCount);
				for (uint32_t i = 0; i < collisionCount; i++)
				{
					uint32_t hulla = pairs[i * 2];
					uint32_t hullb = pairs[i * 2 + 1];

					VHACD::IVHACD::ConvexHull h1, h2;

					mHACD->GetConvexHull(hulla, h1);
					mHACD->GetConvexHull(hullb, h2);

					float p1[3];
					float p2[3];

					getExplodeViewPosition(center, h1, explodeViewScale, h1.m_center, p1);
					getExplodeViewPosition(center, h2, explodeViewScale, h2.m_center, p2);

					mRenderDebug->pushRenderState();
					mRenderDebug->setCurrentColor(0xFF0000);
					mRenderDebug->addToCurrentState(RENDER_DEBUG::DebugRenderState::SolidShaded);
					mRenderDebug->debugSphere(p1, 0.1f);
					mRenderDebug->debugSphere(p2, 0.1f);
					mRenderDebug->debugThickRay(p1, p2, 0.02f);
					mRenderDebug->popRenderState();

				}
			}

			mRenderDebug->popRenderState();
		}
	}

	virtual void decompose(
		const double* const points,
		const uint32_t countPoints,
		const uint32_t* const triangles,
		const uint32_t countTriangles,
		VHACD::IVHACD::Parameters &desc)
	{
		desc.m_callback = this;
		desc.m_logger = this;
		mHaveConstraints = false; // clear have constraints flag so they will be recomputed when the decomopsition is complete
        SAFE_RELEASE(mSkeletonRig);
		mHACD->Compute(points, countPoints, triangles, countTriangles, desc);
	}

	virtual void release(void)
	{
		delete this;
	}

	virtual void Update(const double overallProgress,
                            const double stageProgress,
                            const char* const stage,
                            const char *operationn) final
	{
		mOverallProgress = overallProgress;
		mStageProgress = stageProgress;
		mStage = std::string(stage);
	}

	virtual void Log(const char* const msg) final
	{
		printf("%s", msg);
		mRenderDebug->debugMessage("VHACD:%s", msg);
	}

	virtual bool Cancelled() final
	{
		printf("HACD::Cancelled\n");
		return false;
	}

	virtual uint32_t getHullCount(void) const final
	{
		return mHACD ? mHACD->GetNConvexHulls() : 0;
	}

	virtual uint32_t getConstraintCount(void) const final
	{
		uint32_t ret = 0;

        if ( mSkeletonRig )
        {
            mSkeletonRig->getConstraints(ret);
        }

		return ret;
	}

	virtual uint32_t getCollisionFilterCount(void) const final
	{
		uint32_t ret = 0;

        if ( mSkeletonRig )
        {
            mSkeletonRig->getCollisionFilterPairs(ret);
        }

		return ret;
	}


	virtual void cancel(void)
	{
		if (mHACD)
		{
			mHACD->Cancel();
		}
	}

	virtual void saveConvexDecomposition(const char *fname,const char *sourceMeshName)
	{
		FILE *fph = fopen(fname, "wb");
		if (fph)
		{
			printf("Saving ConvexDecomposition to '%s\n", fname);
			uint32_t hcount = mHACD->GetNConvexHulls();
			printf("Saving %d convex hulls.\n", hcount);
			fprintf(fph, "# ConvexDecomposition of %s contains %d convex hull parts.\n", sourceMeshName, hcount);
			uint32_t vertIndex = 0;
			uint32_t *offsets = new uint32_t[hcount];
			for (uint32_t i = 0; i < hcount; i++)
			{
				VHACD::IVHACD::ConvexHull ch;
				mHACD->GetConvexHull(i, ch);
				printf("Hull %d contains %d vertices and %d triangles.\n", i + 1, ch.m_nPoints, ch.m_nTriangles);
				fprintf(fph, "##########################################################################################\n");
				fprintf(fph, "# Convex Hull %d contains %d vertices.\n", i + 1, ch.m_nPoints);
				fprintf(fph, "##########################################################################################\n");
				for (uint32_t j = 0; j < ch.m_nPoints; j++)
				{
					const double *pos = &ch.m_points[j * 3];
					fprintf(fph, "v %0.9f %0.9f %0.9f\n", pos[0], pos[1], pos[2]);
				}
				offsets[i] = vertIndex;
				vertIndex += ch.m_nPoints;
			}
			for (uint32_t i = 0; i < hcount; i++)
			{
				VHACD::IVHACD::ConvexHull ch;
				mHACD->GetConvexHull(i, ch);
				fprintf(fph, "##########################################################################################\n");
				fprintf(fph, "# Convex Hull %d contains %d triangles.\n", i + 1, ch.m_nPoints);
				fprintf(fph, "##########################################################################################\n");
				vertIndex = offsets[i] + 1;
				for (uint32_t j = 0; j < ch.m_nTriangles; j++)
				{
					const uint32_t *indices = &ch.m_triangles[j * 3];
					fprintf(fph, "f %d %d %d\n", indices[0] + vertIndex, indices[1] + vertIndex, indices[2] + vertIndex);
				}
			}
			fclose(fph);
			delete[]offsets;
		}
		else
		{
			printf("Failed to open output file '%s for write access\n", fname);
		}
	}

	void createDOM(bool simulateAsRagdoll,
		float boneWeightFalloff,
		float boneWeightPercentage,
		ConstraintType ctype,
		float limitDistance,
		uint32_t twistLimit,			// Twist limit in degrees (if used)
		uint32_t swing1Limit,			// Swing 1 limit in degrees (if used)
		uint32_t swing2Limit,
		uint32_t forwardAxis)
	{
		if (mHACD && mHACD->IsReady())
		{
            createSkeletonRig(forwardAxis); // if not already created, create it...
			releaseSimulationObjects();
			delete mDOM;
			mDOM = new PHYSICS_DOM::PhysicsDOMDef; // create the container..

			// Declare the collection that will contain all of the rigid body
			// objects
			PHYSICS_DOM::CollectionDef *cdef = new PHYSICS_DOM::CollectionDef;
			cdef->mId = "Collection1";
			mDOM->mCollections.push_back(cdef);

			// Declare the scene which will instantiate our collection
			PHYSICS_DOM::SceneDef *sdef = new PHYSICS_DOM::SceneDef;
			sdef->mId = "Scene1";
			PHYSICS_DOM::InstanceCollectionDef *ic = new PHYSICS_DOM::InstanceCollectionDef;
			ic->mId = "InstanceCollection1";
			ic->mCollection = cdef->mId;
			sdef->mNodes.push_back(ic);
			mDOM->mScenes.push_back(sdef);

			PHYSICS_DOM::PhysicsMaterialDef *pm = new PHYSICS_DOM::PhysicsMaterialDef;
			pm->mId = "Material1";
			cdef->mNodes.push_back(pm);
#if 0
			//*******************************************************
			// Add the static plane
			//*******************************************************
			PHYSICS_DOM::PlaneGeometryDef *plane = new PHYSICS_DOM::PlaneGeometryDef;
			PHYSICS_DOM::GeometryInstanceDef *plane_instance = new PHYSICS_DOM::GeometryInstanceDef;
			plane_instance->mGeometry = plane;
			plane_instance->mMaterials.push_back("Material1");
			PHYSICS_DOM::RigidStaticDef *rs = new PHYSICS_DOM::RigidStaticDef;
			rs->mId = "RigidStatic1";
			rs->mGeometryInstances.push_back(plane_instance);
			cdef->mNodes.push_back(rs);
#endif

			PHYSICS_DOM::RigidDynamicDef *compoundActor = nullptr;
			if (!simulateAsRagdoll)
			{
				compoundActor = new PHYSICS_DOM::RigidDynamicDef;
				compoundActor->mId = "RigidDynamic0";
			}
			//
			mConvexMeshCount = mHACD->GetNConvexHulls();

			if (mConvexMeshCount)
			{

				for (uint32_t i = 0; i < mConvexMeshCount; i++)
				{
					VHACD::IVHACD::ConvexHull ch;
					mHACD->GetConvexHull(i, ch);
					float *vertices = new float[ch.m_nPoints * 3];
					for (uint32_t j = 0; j < ch.m_nPoints; j++)
					{
						vertices[j * 3 + 0] = float(ch.m_points[j * 3 + 0] - ch.m_center[0]);
						vertices[j * 3 + 1] = float(ch.m_points[j * 3 + 1] - ch.m_center[1]);
						vertices[j * 3 + 2] = float(ch.m_points[j * 3 + 2] - ch.m_center[2]);
					}

					PHYSICS_DOM::ConvexHullDef *chd = new PHYSICS_DOM::ConvexHullDef;
					for (uint32_t k = 0; k < ch.m_nPoints; k++)
					{
						const float *v = &vertices[k * 3];
						PHYSICS_DOM::Vec3 point(v[0], v[1], v[2]);
						chd->mPoints.push_back(point);
					}
					delete[]vertices;

					char scratch[512];
					STRING_HELPER::stringFormat(scratch, 512, "ConvexHull%d", i);
					chd->mId = std::string(scratch);
					cdef->mNodes.push_back(chd);
					
					float center[3];
					center[0] = float(ch.m_center[0]);
					center[1] = float(ch.m_center[1]);
					center[2] = float(ch.m_center[2]);
					float scale[3] = { 1,1,1 };

					PHYSICS_DOM::ConvexHullGeometryDef *chg = new PHYSICS_DOM::ConvexHullGeometryDef;
					chg->mConvexMesh = std::string(scratch);
					chg->mScale.scale = PHYSICS_DOM::Vec3(scale[0], scale[1], scale[2]);
					
					PHYSICS_DOM::GeometryInstanceDef *ginstance = new PHYSICS_DOM::GeometryInstanceDef;
					ginstance->mGeometry = chg;
					ginstance->mMaterials.push_back("Material1");

					if ( compoundActor )
					{
						ginstance->mLocalPose.p = PHYSICS_DOM::Vec3(center[0], center[1], center[2]);
						compoundActor->mGeometryInstances.push_back(ginstance);
					}
					else
					{
						PHYSICS_DOM::RigidDynamicDef *rd = new PHYSICS_DOM::RigidDynamicDef;
						rd->mGlobalPose.p = PHYSICS_DOM::Vec3(center[0], center[1], center[2]);
						rd->mGeometryInstances.push_back(ginstance);

						char rdname[512];
						STRING_HELPER::stringFormat(rdname, 512, "RigidDynamic%d", i);
						rd->mId = std::string(rdname);

						cdef->mNodes.push_back(rd);
					}
				}

				if (compoundActor)
				{
					double centerOfMass[3];
					mHACD->ComputeCenterOfMass(centerOfMass);

					mCenterOfMass[0] = float(centerOfMass[0]);
					mCenterOfMass[1] = float(centerOfMass[1]);
					mCenterOfMass[2] = float(centerOfMass[2]);

					compoundActor->mCenterOfMassLocalPose.p = PHYSICS_DOM::Vec3(mCenterOfMass[0], mCenterOfMass[1], mCenterOfMass[2]);
					cdef->mNodes.push_back(compoundActor); // add the compound actor to the scene
				}
					if (simulateAsRagdoll)
					{
                        const skeletonrig::Constraint *constraints = mSkeletonRig->getConstraints(mConstraintCount);
						if (constraints)
						{

							float _twistLimit = (NV_MATH::NvPi * 2) * (float(twistLimit) / 360.0f);
							float _swing1Limit = (NV_MATH::NvPi * 2) * (float(swing1Limit) / 360.0f);
							float _swing2Limit = (NV_MATH::NvPi * 2) * (float(swing2Limit) / 360.0f);

							for (uint32_t i = 0; i < mConstraintCount; i++)
							{
                                const skeletonrig::Constraint &c = constraints[i];
								// ok...we have both bodies..
								PHYSICS_DOM::Pose worldPose;
								worldPose.p = PHYSICS_DOM::Vec3(float(c.mConstraintPoint[0]), float(c.mConstraintPoint[1]), float(c.mConstraintPoint[2]));
								worldPose.q = PHYSICS_DOM::Quat(float(c.mConstraintOrientation[0]), float(c.mConstraintOrientation[1]), float(c.mConstraintOrientation[2]), float(c.mConstraintOrientation[3]));

								PHYSICS_DOM::JointDef *jdef = nullptr;
								switch (ctype)
								{
									case CT_FIXED:
										{
											PHYSICS_DOM::FixedJointDef *fj = new PHYSICS_DOM::FixedJointDef;
											jdef = static_cast<PHYSICS_DOM::JointDef *>(fj);
										}
										break;
									case CT_HINGE:
										{
											PHYSICS_DOM::HingeJointDef *fj = new PHYSICS_DOM::HingeJointDef;
											jdef = static_cast<PHYSICS_DOM::JointDef *>(fj);
											fj->mLimtLow = -_twistLimit;
											fj->mLimitHigh = _twistLimit;
										}
										break;
									case CT_SPHERICAL:
										{
											PHYSICS_DOM::SphericalJointDef *fj = new PHYSICS_DOM::SphericalJointDef;
											jdef = static_cast<PHYSICS_DOM::JointDef *>(fj);
											fj->mLimitY = _swing1Limit;
											fj->mLimitZ = _swing2Limit;
										}
										break;
								}

								char bodyA[512];
								char bodyB[512];

								STRING_HELPER::stringFormat(bodyA, 512, "RigidDynamic%d", c.mHullA);
								STRING_HELPER::stringFormat(bodyB, 512, "RigidDynamic%d", c.mHullB);

								const PHYSICS_DOM::NodeDef *b1 = PHYSICS_DOM::locateNode(*mDOM, std::string(bodyA));
								const PHYSICS_DOM::NodeDef *b2 = PHYSICS_DOM::locateNode(*mDOM, std::string(bodyB));
								if (b1 && b2 && b1->mType == PHYSICS_DOM::NT_RIGID_DYNAMIC && b2->mType == PHYSICS_DOM::NT_RIGID_DYNAMIC)
								{

									const PHYSICS_DOM::RigidDynamicDef *rb1 = static_cast< const PHYSICS_DOM::RigidDynamicDef *>(b1);
									const PHYSICS_DOM::RigidDynamicDef *rb2 = static_cast< const PHYSICS_DOM::RigidDynamicDef *>(b2);

									jdef->mLocalpose0 = PHYSICS_DOM::getInversePose(rb1->mGlobalPose, worldPose);
									jdef->mLocalpose1 = PHYSICS_DOM::getInversePose(rb2->mGlobalPose, worldPose);

									char temp[512];
									STRING_HELPER::stringFormat(temp, 512, "Joint%d", i);
									jdef->mId = std::string(temp);
									jdef->mBody0 = std::string(bodyA);
									jdef->mBody1 = std::string(bodyB);
									cdef->mNodes.push_back(jdef);

								}
								else
								{
									delete jdef;
								}
							}
						}

						uint32_t collisionPairCount=0;
                        const uint32_t *collisionPairs = mSkeletonRig->getCollisionFilterPairs(collisionPairCount);
						if (collisionPairs)
						{
							PHYSICS_DOM::BodyPairFiltersDef *bpf = new PHYSICS_DOM::BodyPairFiltersDef;
							bpf->mId = "BodyPairFilters";
							for (uint32_t i = 0; i < collisionPairCount; i++)
							{
								uint32_t bodyA = collisionPairs[i * 2 + 0];
								uint32_t bodyB = collisionPairs[i * 2 + 1];
								char filterA[512];
								char filterB[512];
								STRING_HELPER::stringFormat(filterA, 512, "RigidDynamic%d", bodyA);
								STRING_HELPER::stringFormat(filterB, 512, "RigidDynamic%d", bodyB);
								PHYSICS_DOM::BodyPairFilterDef bp;
								bp.mBodyA = std::string(filterA);
								bp.mBodyB = std::string(filterB);
								bpf->mBodyPairs.push_back(bp);
							}
							cdef->mNodes.push_back(bpf);
						}
					}

				mDOM->initDOM();
				const PHYSICS_DOM::PhysicsDOM *pdom = mDOM->getPhysicsDOM();
				mDOMInstance = mPhysicsDOM->loadPhysicsDOM(*pdom);

				if (simulateAsRagdoll)
				{
					mRigidBodies = new PHYSICS_DOM::NodeState *[mConvexMeshCount];
					for (uint32_t i = 0; i < mConvexMeshCount; i++)
					{
						char scratch[512];
						STRING_HELPER::stringFormat(scratch, 512, "RigidDynamic%d", i);
						mRigidBodies[i] = mDOMInstance->getNodeState(scratch);
					}
                    mMeshBuilder.buildWeightedVertices(mSkeletonRig, mConvexMeshCount, boneWeightFalloff, boneWeightPercentage, mRigidBodies);
					if (mConstraintCount)
					{
						mConstraints = new PHYSICS_DOM::NodeState *[mConstraintCount];
						for (uint32_t i = 0; i < mConstraintCount; i++)
						{
							char scratch[512];
							STRING_HELPER::stringFormat(scratch, 512, "Joint%d", i);
							mConstraints[i] = mDOMInstance->getNodeState(scratch);
						}
					}
				}
				else
				{
					mNodeState = mDOMInstance->getNodeState("RigidDynamic0");
				}

				mSaveCount++;
				char scratch[512];
				STRING_HELPER::stringFormat(scratch, 512, "ConvexDecomposition%d.xml", mSaveCount);
				mPhysicsDOM->serializeXML(scratch);
			}
		}
	}

	virtual void toggleSimulation(bool simulateAsRagdoll,
		float boneWeightFalloff,
		float boneWeightPercentage,
		ConstraintType ctype,
		float limitDistance,
		uint32_t twistLimit,			// Twist limit in degrees (if used)
		uint32_t swing1Limit,			// Swing 1 limit in degrees (if used)
		uint32_t swing2Limit,
		uint32_t forwardAxis) final		// Swing 2 limit in degrees (if used)
	{
		mSimulateAsRagdoll = simulateAsRagdoll;
		if (mDOM)
		{
			releaseSimulationObjects();
		}
		else
		{
			createDOM(simulateAsRagdoll, boneWeightFalloff, boneWeightPercentage, ctype, limitDistance, twistLimit, swing1Limit, swing2Limit, forwardAxis);
		}
	}

	void releaseSimulationObjects(void)
	{
		if (mDOMInstance)
		{
			mDOMInstance->release();
			mDOMInstance = nullptr;
		}
		delete[]mRigidBodies;
		delete[]mConstraints;
		mRigidBodies = nullptr;
		mConstraints = nullptr;
		mNodeState = nullptr;
		delete mDOM;
		mDOM = nullptr;
		mConvexMeshCount = 0;
	}

	virtual void getTransform(float xform[16]) final
	{
		FLOAT_MATH::fm_identity(xform);
		if (mNodeState )
		{
			NV_MATH::NvTransform t;
			mNodeState->getPose(&t.p.x, &t.q.x);
			NV_MATH::NvMat44 _xform(t);
			memcpy(xform, &_xform, sizeof(_xform));
		}
	}

	void computeConstraints(uint32_t forwardAxis)
	{
        if ( mSkeletonRig )
        {
            mSkeletonRig->computeConstraints(forwardAxis);
        }
	}

	virtual bool isSimulating(void) const
	{
		return mDOM ? true : false;
	}

	virtual void setRenderMesh(uint32_t vcount,
		const float *vertices,
		uint32_t tcount,
		const uint32_t *indices)
	{
		if (mMeshID)
		{
			mRenderDebug->releaseTriangleMesh(mMeshID);
		}
		mMeshID = mRenderDebug->getMeshId();
		{
			mMeshBuilder.reset();
			for (uint32_t i = 0; i < tcount; i++)
			{
				uint32_t i1 = indices[i * 3 + 0];
				uint32_t i2 = indices[i * 3 + 1];
				uint32_t i3 = indices[i * 3 + 2];
				const float *p1 = &vertices[i1 * 3];
				const float *p2 = &vertices[i2 * 3];
				const float *p3 = &vertices[i3 * 3];
				mMeshBuilder.addTriangle(p3, p2, p1);
			}
			mRenderDebug->createTriangleMesh(mMeshID, (uint32_t)mMeshBuilder.mVertices.size(), &mMeshBuilder.mVertices[0], 0, nullptr);
		}

	}

	virtual uint32_t getMeshID(void) const final
	{
		return mMeshID;
	}

	virtual void releaseMeshID(void) final
	{
		if (mMeshID)
		{
			mRenderDebug->releaseTriangleMesh(mMeshID);
			mMeshID = 0;
		}
	}

	virtual void setDragForce(float dragForce) final
	{
		if (mPhysicsDOM)
		{
			mPhysicsDOM->setDragForce(dragForce);
		}
	}

    void createSkeletonRig(uint32_t forwardAxis)
    {
        if ( mSkeletonRig )
        {
            return;
        }
        mConvexMeshCount = mHACD->GetNConvexHulls();
        if (mConvexMeshCount)
        {
            skeletonrig::SimpleMesh *convexHulls = new skeletonrig::SimpleMesh[mConvexMeshCount];
            for (uint32_t i = 0; i < mConvexMeshCount; i++)
            {
                skeletonrig::SimpleMesh &sm = convexHulls[i];
                VHACD::IVHACD::ConvexHull ch;
                mHACD->GetConvexHull(i, ch);
                sm.mTriangeCount = ch.m_nTriangles;
                sm.mVertexCount = ch.m_nPoints;
                sm.mIndices = ch.m_triangles;
                sm.mVertices = ch.m_points;
            }
            mSkeletonRig = mSkeletonRigFactory->create(mConvexMeshCount, convexHulls);
            delete[]convexHulls;
            double centerOfMass[3];
            mSkeletonRig->computeCenterOfMass(centerOfMass);
            mSkeletonRig->computeConstraints(forwardAxis);
        }
        else
        {
            mSkeletonRig = mSkeletonRigFactory->create(0,nullptr);
        }
    }

    // Optional user callback. This is a callback performed to notify the user that the 
    // convex decomposition background process is completed. This call back will occur from
    // a different thread so the user should take that into account.
    virtual void NotifyVHACDComplete(void)
    {
        printf("Convex Decomposition Complete.\n");
    }

	virtual void meshVertsToHulls(uint32_t vcount,const double *points) final
	{
		if ( mHACD )
		{
			mRenderDebug->pushRenderState();
			mRenderDebug->setCurrentDisplayTime(15.0);
			for (uint32_t i=0; i<vcount; i++)
			{
				const double *pos = &points[i*3];
				double distanceToHull;
				uint32_t hull = mHACD->findNearestConvexHull(pos,distanceToHull);

				uint32_t cindex = (hull % 20) + RENDER_DEBUG::DebugColors::Red;

				uint32_t color = mRenderDebug->getDebugColor((RENDER_DEBUG::DebugColors::Enum)cindex);
				mRenderDebug->setCurrentColor(color,0xFFFFFF);
				float fpos[3];
				fpos[0] = float(pos[0]);
				fpos[1] = float(pos[1]);
				fpos[2] = float(pos[2]);
				mRenderDebug->debugPoint(fpos,0.03f);
			}
			mRenderDebug->popRenderState();
		}
	}

	uint32_t							mMeshID{ 0 };
	bool								mSimulateAsRagdoll{ false };
	uint32_t							mConvexMeshCount{ 0 };
	RENDER_DEBUG::RenderDebug			*mRenderDebug;
	PHYSICS_DOM::NvPhysicsDOM	*mPhysicsDOM;
	VHACD::IVHACD						*mHACD;
	double								mOverallProgress{ 0 };
	double								mStageProgress{ 0 };
	double								mOperationProgress{ 0 };
	std::string							mStage;
	std::string							mOperation;
	float								mCenterOfMass[3];
	bool								mHaveConstraints{ false };
	uint32_t							mConstraintCount{ 0 };
	MeshBuilder							mMeshBuilder;
	uint32_t							mSaveCount{ 0 };
	PHYSICS_DOM::PhysicsDOMDef			*mDOM{ nullptr };
	PHYSICS_DOM::PhysicsDOMInstance		*mDOMInstance{ nullptr };
	// Tracking current transform states
	PHYSICS_DOM::NodeState				*mNodeState{ nullptr };
	PHYSICS_DOM::NodeState				**mRigidBodies{ nullptr };
	PHYSICS_DOM::NodeState				**mConstraints{ nullptr };
    skeletonrig::SkeletonRig            *mSkeletonRig{ nullptr };
    skeletonrig::SkeletonRigFactory     *mSkeletonRigFactory{ nullptr };
};

TestHACD *TestHACD::create(RENDER_DEBUG::RenderDebug *renderDebug,PHYSICS_DOM::NvPhysicsDOM *pf)
{
	TestHACDImpl *t = new TestHACDImpl(renderDebug,pf);
	return static_cast<TestHACD *>(t);
}


