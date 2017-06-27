#include "TestHACD.h"
#include "NvRenderDebug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

class TestHACDImpl : public TestHACD, public VHACD::IVHACD::IUserCallback, public VHACD::IVHACD::IUserLogger
{
public:
	TestHACDImpl(RENDER_DEBUG::RenderDebug *renderDebug) : mRenderDebug(renderDebug)
	{
		mHACD = VHACD::CreateVHACD_ASYNC();
	}

	virtual ~TestHACDImpl(void)
	{
		mHACD->Release();
	}

	void getExplodePosition(const double source[3], float dest[3], const double diff[3],const float center[3])
	{
		dest[0] = float(source[0] + diff[0] + center[0]);
		dest[1] = float(source[1] + diff[1] + center[1]);
		dest[2] = float(source[2] + diff[2] + center[2]);
	}

	virtual void render(float explodeViewScale,const float center[3]) final
	{


		uint32_t hullCount = mHACD->GetNConvexHulls();
		if (hullCount)
		{
			for (uint32_t j = 0; j < hullCount; j++)
			{
				VHACD::IVHACD::ConvexHull h;
				mHACD->GetConvexHull(j, h);
				{
					mRenderDebug->pushRenderState();

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
					mRenderDebug->popRenderState();
				}
			}
		}
		else
		{
			if ( !mHACD->IsReady()) // if we are still computing the convex decomposition in a background thread, display the current status
			{
				mRenderDebug->debugText2D(0, 0.2f, 0.5f, 2.0f, false, 0xFF8080, "%s : %s : %0.2f : %0.2f : %0.2f\n", mStage.c_str(), mOperation.c_str(), mOverallProgress, mStageProgress, mOperationProgress);
			}
		}
	}

	virtual void decompose(
		const double* const points,
		const unsigned int countPoints,
		const int* const triangles,
		const unsigned int countTriangles,
		VHACD::IVHACD::Parameters &desc)
	{
		desc.m_callback = this;
		desc.m_logger = this;
		mHACD->Compute(points, 3, countPoints, triangles, 3, countTriangles, desc);
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

	virtual void cancel(void)
	{
		if (mHACD)
		{
			mHACD->Cancel();
		}
	}

	RENDER_DEBUG::RenderDebug	*mRenderDebug;
	VHACD::IVHACD				*mHACD;

	double				mOverallProgress{ 0 };
	double				mStageProgress{ 0 };
	double				mOperationProgress{ 0 };
	std::string			mStage;
	std::string			mOperation;
};

TestHACD *TestHACD::create(RENDER_DEBUG::RenderDebug *renderDebug)
{
	TestHACDImpl *t = new TestHACDImpl(renderDebug);
	return static_cast<TestHACD *>(t);
}


