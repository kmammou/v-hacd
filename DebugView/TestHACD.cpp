#include "TestHACD.h"
#include "NvRenderDebug.h"
#include "NvPhysXFramework.h"
#include "FloatMath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#pragma warning(disable:4100)

#define DEFAULT_MASS 10

class TestHACDImpl : public TestHACD, public VHACD::IVHACD::IUserCallback, public VHACD::IVHACD::IUserLogger
{
public:
	TestHACDImpl(RENDER_DEBUG::RenderDebug *renderDebug,NV_PHYSX_FRAMEWORK::PhysXFramework *pf) : mRenderDebug(renderDebug), mPhysXFramework(pf)
	{
		mHACD = VHACD::CreateVHACD_ASYNC();
		mCenterOfMass[0] = 0;
		mCenterOfMass[1] = 0;
		mCenterOfMass[2] = 0;
	}

	virtual ~TestHACDImpl(void)
	{
		mHACD->Release();
		releaseSimulationObjects();
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

	virtual void render(float explodeViewScale,
						const float center[3],
						bool wireframe,
						bool showConstraints,
						bool showSkeleton,
						bool showCollisionPairs) final
	{
		uint32_t hullCount = mHACD->GetNConvexHulls();
		if (hullCount)
		{
			if (!mHaveConstraints)
			{
				mHaveConstraints = true;
				mHACD->ComputeConstraints();
			}
			mRenderDebug->pushRenderState();
			float xform[16];
			getTransform(xform);
			mRenderDebug->setPose(xform);

			// Render constraints here...
			uint32_t constraintCount;
			const VHACD::IVHACD::Constraint *constraints = mHACD->GetConstraints(constraintCount);

			for (uint32_t j = 0; j < hullCount; j++)
			{
				VHACD::IVHACD::ConvexHull h;
				mHACD->GetConvexHull(j, h);
				{

					if (wireframe)
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
						for (uint32_t i = 0; i < constraintCount; i++)
						{
							const VHACD::IVHACD::Constraint &c = constraints[i];
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
				for (uint32_t i = 0; i < constraintCount; i++)
				{
					const VHACD::IVHACD::Constraint &c = constraints[i];
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
				const uint32_t *pairs = mHACD->GetCollisionFilterPairs(collisionCount);
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
		else
		{
			if (!mHACD->IsReady()) // if we are still computing the convex decomposition in a background thread, display the current status
			{
				mRenderDebug->debugText2D(0, 0.2f, 0.5f, 2.0f, false, 0xFF8080, "%s : %s : %0.2f : %0.2f : %0.2f\n", mStage.c_str(), mOperation.c_str(), mOverallProgress, mStageProgress, mOperationProgress);
			}
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
		mHACD->Compute(points, countPoints, triangles, countTriangles, desc);
	}

	virtual void release(void)
	{
		delete this;
	}

	virtual void Update(const double overallProgress,
		const double stageProgress,
		const double operationProgress,
		const char* const stage,
		const char* const operation) final
	{
		mOverallProgress = overallProgress;
		mStageProgress = stageProgress;
		mOperationProgress = operationProgress;
		mStage = std::string(stage);
		mOperation = std::string(operation);
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

		if (mHACD)
		{
			mHACD->GetConstraints(ret);
		}

		return ret;
	}

	virtual uint32_t getCollisionFilterCount(void) const final
	{
		uint32_t ret = 0;

		if (mHACD)
		{
			mHACD->GetCollisionFilterPairs(ret);
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

	virtual void toggleSimulation(void)
	{
		if (mCompoundActor)
		{
			releaseSimulationObjects();
		}
		else if (mHACD && mHACD->IsReady() )
		{
			releaseSimulationObjects();
			mConvexMeshCount = mHACD->GetNConvexHulls();
			if (mConvexMeshCount)
			{
				mConvexMeshes = new NV_PHYSX_FRAMEWORK::PhysXFramework::ConvexMesh *[mConvexMeshCount];
				mCompoundActor = mPhysXFramework->createCompoundActor();
				for (uint32_t i = 0; i < mConvexMeshCount; i++)
				{
					VHACD::IVHACD::ConvexHull ch;
					mHACD->GetConvexHull(i, ch);
					float *vertices = new float[ch.m_nPoints * 3];
					for (uint32_t j = 0; j < ch.m_nPoints; j++)
					{
						vertices[j * 3 + 0] = float(ch.m_points[j * 3 + 0]-ch.m_center[0]);
						vertices[j * 3 + 1] = float(ch.m_points[j * 3 + 1]-ch.m_center[1]);
						vertices[j * 3 + 2] = float(ch.m_points[j * 3 + 2]-ch.m_center[2]);
					}
					mConvexMeshes[i] = mPhysXFramework->createConvexMesh(ch.m_nPoints, vertices, ch.m_nTriangles, (const uint32_t *)ch.m_triangles);
					delete[]vertices;
					if (mCompoundActor)
					{
						float center[3];
						center[0] = float(ch.m_center[0]);
						center[1] = float(ch.m_center[1]);
						center[2] = float(ch.m_center[2]);
						float scale[3] = { 1,1,1 };
						mCompoundActor->addConvexMesh(mConvexMeshes[i], center, scale);
					}
				}
				if (mCompoundActor)
				{
					double centerOfMass[3];
					mHACD->ComputeCenterOfMass(centerOfMass);
					mCenterOfMass[0] = float(centerOfMass[0]);
					mCenterOfMass[1] = float(centerOfMass[1]);
					mCenterOfMass[2] = float(centerOfMass[2]);
					mCompoundActor->createActor(mCenterOfMass,DEFAULT_MASS);
				}
			}
		}
	}

	void releaseSimulationObjects(void)
	{
		if (mCompoundActor)
		{
			mCompoundActor->release();
			mCompoundActor = nullptr;
		}
		for (uint32_t i = 0; i < mConvexMeshCount; i++)
		{
			NV_PHYSX_FRAMEWORK::PhysXFramework::ConvexMesh *cm = mConvexMeshes[i];
			if (cm)
			{
				cm->release();
			}
		}
		mConvexMeshCount = 0;
		delete[]mConvexMeshes;
		mConvexMeshes = nullptr;
	}

	virtual void getTransform(float xform[16]) final
	{
		FLOAT_MATH::fm_identity(xform);
		if (mCompoundActor)
		{
			mCompoundActor->getXform(xform);
		}
	}

	void computeConstraints(void)
	{
		if (mHACD)
		{
			mHACD->ComputeConstraints();
		}
	}

	uint32_t							mConvexMeshCount{ 0 };
	NV_PHYSX_FRAMEWORK::PhysXFramework::ConvexMesh		**mConvexMeshes{ nullptr };
	NV_PHYSX_FRAMEWORK::PhysXFramework::CompoundActor	*mCompoundActor{ nullptr };
	RENDER_DEBUG::RenderDebug			*mRenderDebug;
	NV_PHYSX_FRAMEWORK::PhysXFramework	*mPhysXFramework;
	VHACD::IVHACD						*mHACD;
	double								mOverallProgress{ 0 };
	double								mStageProgress{ 0 };
	double								mOperationProgress{ 0 };
	std::string							mStage;
	std::string							mOperation;
	float								mCenterOfMass[3];
	bool								mHaveConstraints{ false };
};

TestHACD *TestHACD::create(RENDER_DEBUG::RenderDebug *renderDebug,NV_PHYSX_FRAMEWORK::PhysXFramework *pf)
{
	TestHACDImpl *t = new TestHACDImpl(renderDebug,pf);
	return static_cast<TestHACD *>(t);
}


