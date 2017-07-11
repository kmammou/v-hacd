#include "../public/VHACD.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <float.h>

#define ENABLE_ASYNC 1

#define HACD_ALLOC(x) malloc(x)
#define HACD_FREE(x) free(x)
#define HACD_ASSERT(x) assert(x)

namespace VHACD
{

class MyHACD_API : public VHACD::IVHACD, public VHACD::IVHACD::IUserCallback, VHACD::IVHACD::IUserLogger
{
public:
	MyHACD_API(void)
	{
		mVHACD = VHACD::CreateVHACD();
	}

	virtual ~MyHACD_API(void)
	{
		releaseHACD();
		Cancel();
		mVHACD->Release();
	}

	
	virtual bool Compute(const double* const _points,
		const unsigned int stridePoints,
		const unsigned int countPoints,
		const int* const _triangles,
		const unsigned int strideTriangles,
		const unsigned int countTriangles,
		const Parameters& _desc) final
	{
#if ENABLE_ASYNC
		Cancel(); // if we previously had a solution running; cancel it.
		releaseHACD();

		// We need to copy the input vertices and triangles into our own buffers so we can operate
		// on them safely from the background thread.
		mVertices = (double *)HACD_ALLOC(sizeof(double)*countPoints * 3);
		mIndices = (int *)HACD_ALLOC(sizeof(int)*countTriangles * 3);

		uint32_t index = 0;
		for (uint32_t i = 0; i < countPoints; i++)
		{
			mVertices[i * 3 + 0] = _points[index + 0];
			mVertices[i * 3 + 1] = _points[index + 1];
			mVertices[i * 3 + 2] = _points[index + 2];
			index += stridePoints;
		}
		index = 0;
		for (uint32_t i = 0; i < countTriangles; i++)
		{
			mIndices[i * 3 + 0] = _triangles[index + 0];
			mIndices[i * 3 + 1] = _triangles[index + 1];
			mIndices[i * 3 + 2] = _triangles[index + 2];
			index += strideTriangles;
		}

		mRunning = true;
		mThread = new std::thread([this, countPoints, countTriangles, _desc]()
		{
			ComputeNow(mVertices, 3, countPoints, mIndices, 3, countTriangles, _desc);
			mRunning = false;
		});
#else
		releaseHACD();
		ComputeNow(_points, stridePoints, countPoints, _triangles, strideTriangles, countTriangles, _desc);
#endif
		return true;
	}

	bool ComputeNow(const double* const points,
		const unsigned int stridePoints,
		const unsigned int countPoints,
		const int* const triangles,
		const unsigned int strideTriangles,
		const unsigned int countTriangles,
		const Parameters& _desc) 
	{
		uint32_t ret = 0;

		mHullCount	= 0;
		mCallback	= _desc.m_callback;
		mLogger		= _desc.m_logger;

		IVHACD::Parameters desc = _desc;
		// Set our intercepting callback interfaces if non-null
		desc.m_callback = desc.m_callback ? this : nullptr;
		desc.m_logger = desc.m_logger ? this : nullptr;

		if ( countPoints )
		{
			bool ok = mVHACD->Compute(points, stridePoints, countPoints, triangles, strideTriangles, countTriangles, desc);
			if (ok)
			{
				ret = mVHACD->GetNConvexHulls();
				mHulls = new IVHACD::ConvexHull[ret];
				for (unsigned int i = 0; i < ret; i++)
				{
					VHACD::IVHACD::ConvexHull vhull;
					mVHACD->GetConvexHull(i, vhull);
					VHACD::IVHACD::ConvexHull h;
					h.m_nPoints = vhull.m_nPoints;
					h.m_points = (double *)HACD_ALLOC(sizeof(double) * 3 * h.m_nPoints);
					memcpy(h.m_points, vhull.m_points, sizeof(double) * 3 * h.m_nPoints);
					h.m_nTriangles = vhull.m_nTriangles;
					h.m_triangles = (int *)HACD_ALLOC(sizeof(int) * 3 * h.m_nTriangles);
					memcpy(h.m_triangles, vhull.m_triangles, sizeof(int) * 3 * h.m_nTriangles);
					h.m_volume = vhull.m_volume;
					h.m_center[0] = vhull.m_center[0];
					h.m_center[1] = vhull.m_center[1];
					h.m_center[2] = vhull.m_center[2];
					mHulls[i] = h;
					if (mCancel)
					{
						ret = 0;
						break;
					}
				}
			}
		}

		mHullCount = ret;
		return ret ? true : false;
	}

	void releaseHull(VHACD::IVHACD::ConvexHull &h)
	{
		HACD_FREE((void *)h.m_triangles);
		HACD_FREE((void *)h.m_points);
		h.m_triangles = nullptr;
		h.m_points = nullptr;
	}

	virtual void GetConvexHull(const unsigned int index, VHACD::IVHACD::ConvexHull& ch) const final
	{
		if ( index < mHullCount )
		{
			ch = mHulls[index];
		}
	}

	void	releaseHACD(void) // release memory associated with the last HACD request
	{
		for (uint32_t i=0; i<mHullCount; i++)
		{
			releaseHull(mHulls[i]);
		}
		delete[]mHulls;
		mHulls = nullptr;
		mHullCount = 0;
		HACD_FREE(mVertices);
		mVertices = nullptr;
		HACD_FREE(mIndices);
		mIndices = nullptr;
	}


	virtual void release(void) // release the HACD_API interface
	{
		delete this;
	}

	virtual uint32_t	getHullCount(void)
	{
		return mHullCount;
	}

	virtual void Cancel() final
	{
		if (mRunning)
		{
			mVHACD->Cancel();	// Set the cancel signal to the base VHACD
		}
		if (mThread)
		{
			mThread->join();	// Wait for the thread to fully exit before we delete the instance
			delete mThread;
			mThread = nullptr;
			Log("Convex Decomposition thread canceled\n");
		}
		mCancel = false; // clear the cancel semaphore
	}

	virtual bool Compute(const float* const points,
		const unsigned int stridePoints,
		const unsigned int countPoints,
		const int* const triangles,
		const unsigned int strideTriangles,
		const unsigned int countTriangles,
		const Parameters& params) final
	{

		double *vertices = (double *)HACD_ALLOC(sizeof(double)*countPoints * 3);
		const float *source = points;
		double *dest = vertices;
		for (uint32_t i = 0; i < countPoints; i++)
		{
			dest[0] = source[0];
			dest[1] = source[1];
			dest[2] = source[2];
			dest += 3;
			source += stridePoints;
		}

		bool ret =  Compute(vertices, 3, countPoints, triangles, strideTriangles, countTriangles, params);
		HACD_FREE(vertices);
		return ret;
	}

	virtual unsigned int GetNConvexHulls() const final
	{
		processPendingMessages();
		return mHullCount;
	}

	virtual void Clean(void) final // release internally allocated memory
	{
		Cancel();
		releaseHACD();
		mVHACD->Clean();
	}

	virtual void Release(void) final  // release IVHACD
	{
		delete this;
	}

	virtual bool OCLInit(void* const oclDevice,
		IVHACD::IUserLogger* const logger = 0) final
	{
		return mVHACD->OCLInit(oclDevice, logger);
	}
		
	virtual bool OCLRelease(IVHACD::IUserLogger* const logger = 0) final
	{
		return mVHACD->OCLRelease(logger);
	}

	virtual void Update(const double overallProgress,
		const double stageProgress,
		const double operationProgress,
		const char* const stage,
		const char* const operation) final
	{
		mMessageMutex.lock();
		mHaveUpdateMessage = true;
		mOverallProgress = overallProgress;
		mStageProgress = stageProgress;
		mOperationProgress = operationProgress;
		mStage = std::string(stage);
		mOperation = std::string(operation);
		mMessageMutex.unlock();
	}

	virtual void Log(const char* const msg) final
	{
		mMessageMutex.lock();
		mHaveLogMessage = true;
		mMessage = std::string(msg);
		mMessageMutex.unlock();
	}

	virtual bool IsReady(void) const final
	{
		processPendingMessages();
		return !mRunning; 
	}

	// As a convenience for the calling application we only send it update and log messages from it's own main
	// thread.  This reduces the complexity burden on the caller by making sure it only has to deal with log
	// messages in it's main application thread.
	void processPendingMessages(void) const
	{
		// If we have a new update message and the user has specified a callback we send the message and clear the semaphore
		if (mHaveUpdateMessage && mCallback)
		{
			mMessageMutex.lock();
			mCallback->Update(mOverallProgress, mStageProgress, mOperationProgress, mStage.c_str(), mOperation.c_str());
			mHaveUpdateMessage = false;
			mMessageMutex.unlock();
		}
		// If we have a new log message and the user has specified a callback we send the message and clear the semaphore
		if (mHaveLogMessage && mLogger)
		{
			mMessageMutex.lock();
			mLogger->Log(mMessage.c_str());
			mHaveLogMessage = false;
			mMessageMutex.unlock();
		}
	}

private:
	double							*mVertices{ nullptr };
	int32_t							*mIndices{ nullptr };
	std::atomic< uint32_t>			mHullCount{ 0 };
	VHACD::IVHACD::ConvexHull		*mHulls{ nullptr };
	VHACD::IVHACD::IUserCallback	*mCallback{ nullptr };
	VHACD::IVHACD::IUserLogger		*mLogger{ nullptr };
	VHACD::IVHACD					*mVHACD{ nullptr };
	std::thread						*mThread{ nullptr };
	std::atomic< bool >				mRunning{ false };
	std::atomic<bool>				mCancel{ false };

	// Thread safe caching mechanism for messages and update status.
	// This is so that caller always gets messages in his own thread
	// Member variables are marked as 'mutable' since the message dispatch function
	// is called from const query methods.
	mutable std::mutex						mMessageMutex;
	mutable std::atomic< bool >				mHaveUpdateMessage{ false };
	mutable std::atomic< bool >				mHaveLogMessage{ false };
	mutable double							mOverallProgress{ 0 };
	mutable double							mStageProgress{ 0 };
	mutable double							mOperationProgress{ 0 };
	mutable std::string						mStage;
	mutable std::string						mOperation;
	mutable std::string						mMessage;
};

IVHACD* CreateVHACD_ASYNC(void)
{
	MyHACD_API *m = new MyHACD_API;
	return static_cast<IVHACD *>(m);
}


}; // end of VHACD namespace

