#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <math.h>
#include <string>
#include <assert.h>
#include <unordered_map>

#include "wavefront.h"
#include "NvRenderDebug.h"
#include "NvPhysicsDOM.h"
#include "FloatMath.h"
#include "TestHACD.h"
#include "TestRaycast.h"
#include "VHACD.h"
#include "FastXml.h"
#include "InspectXml.h"
#include "ImportURDF.h"
#include "MeshFactory.h"
#include "DisplayURDF.h"
#include "VoxelizeMesh.h"

#pragma warning(disable:4456 4457)

#define SAFE_RELEASE(x) if ( x ) { x->release(); x = nullptr; }

DISPLAY_URDF::DisplayURDF *gDisplayURDF = nullptr;
PHYSICS_DOM::NvPhysicsDOM	*gNvPhysicsDOM = nullptr;
RENDER_DEBUG::RenderDebugTyped *gRenderDebugTyped=nullptr;
RENDER_DEBUG::RenderDebug *gRenderDebug=nullptr;

static bool			gCenterMesh = false;
static bool			gShowSourceMesh = true;
static bool			gShowConvexDecomposition = true;
static bool			gUseHACD = true;
static float		gScaleInputMesh = 1;
static uint32_t		gTessellateInputMesh = 1;
static float		gExplodeViewScale = 1;
static float		gCenter[3] { 0, 0, 0 };
static uint32_t		gVertexCount = 0;
static uint32_t		gTriangleCount = 0;
static double		*gVertices = nullptr;
static uint32_t		*gIndices = nullptr;
static std::string	 gSourceMeshName;
static VHACD::IVHACD::Parameters gDesc;
static float		gDragForce = 2;

static bool gShowVoxelSurface = true;
static bool gShowVoxelInside = false;
static bool gShowVoxelOutside = false;
static bool gShowVoxelUndefined = false;

void createMenus(void)
{
	gRenderDebug->sendRemoteCommand("BeginTab \"Convex Decomposition - V-HACD\"");	// Mark the beginning of a new tab display in the DebugView application

	gRenderDebug->sendRemoteCommand("BeginGroup \"Controls\"");	// Mark the beginning of a group of controls.
	gRenderDebug->sendRemoteCommand("FileTransferButton \" Select Mesh File\" MeshFile \"Choose a Wavefront OBJ file to transfer\" *.obj;*.off;*.stl;*.dae;*.urdf");
	gRenderDebug->sendRemoteCommand("Button SaveConvexDecomposition \"save\"");
    gRenderDebug->sendRemoteCommand("Button TestRaycastMesh \"raycast\"");
	gRenderDebug->sendRemoteCommand("Button CenterMesh CenterMesh");
	gRenderDebug->sendRemoteCommand("Button SaveObj SaveObj");
    gRenderDebug->sendRemoteCommand("Button SavePython SavePython");
	gRenderDebug->sendRemoteCommand("EndGroup"); // End the group called 'controls'

	gRenderDebug->sendRemoteCommand("BeginGroup \"View\"");	// Mark the beginning of a group of controls.
	gRenderDebug->sendRemoteCommand("CheckBox ShowSourceMesh true ShowSourceMesh");
	gRenderDebug->sendRemoteCommand("CheckBox ShowConvexDecomposition true ShowConvexDecomposition");
	gRenderDebug->sendRemoteCommand("CheckBox WireframeSourceMesh false WireframeSourceMesh");
    gRenderDebug->sendRemoteCommand("CheckBox WireframeConvex false WireframeConvex");
	gRenderDebug->sendRemoteCommand("CheckBox ShowPhysics true ShowPhysics");
	gRenderDebug->sendRemoteCommand("CheckBox ShowSelectedVerts false ShowSelectedVerts");
	gRenderDebug->sendRemoteCommand("Slider ScaleInputMesh 1 0.01 100 ScaleInputMesh");
	gRenderDebug->sendRemoteCommand("SliderInt TessellateInputMesh 1 1 100 TessellateInputMesh");
	gRenderDebug->sendRemoteCommand("Slider ExplodeViewScale 1 1 4 ExplodeViewScale");
	gRenderDebug->sendRemoteCommand("EndGroup"); // End the group called 'controls'


	gRenderDebug->sendRemoteCommand("BeginGroup \"V-HACD Settings1\"");	// Mark the beginning of a group of controls.
    gRenderDebug->sendRemoteCommand("Combo VoxelFillMode VoxelFillMode FLOOD_FILL SURFACE_ONLY RAYCAST_FILL");
	gRenderDebug->sendRemoteCommand("SliderInt MaxHullVertices 32 5 512 MaxHullVertices");
	gRenderDebug->sendRemoteCommand("SliderInt MaxConvexHulls 32 1 2048 MaxConvexHulls");
	gRenderDebug->sendRemoteCommand("Slider Concavity 0.001 0 0.1 Concavity");
	gRenderDebug->sendRemoteCommand("Button PerformConvexDecomposition decomp");
	gRenderDebug->sendRemoteCommand("Button Cancel \"cancel\"");
	gRenderDebug->sendRemoteCommand("EndGroup"); // End the group called 'HACD settings'

	gRenderDebug->sendRemoteCommand("BeginGroup \"V-HACD Settings2\"");	// Mark the beginning of a group of controls.
    gRenderDebug->sendRemoteCommand("CheckBox AsyncACD true AsyncACD");
	gRenderDebug->sendRemoteCommand("Slider Alpha 0.0005 0 0.1 Alpha");
	gRenderDebug->sendRemoteCommand("Slider Beta 0.05 0 0.1 Beta");
    gRenderDebug->sendRemoteCommand("CheckBox ProjectHullVertices true ProjectHullVertices");
	gRenderDebug->sendRemoteCommand("SliderInt Resolution 100000 10000 100000000 Resolution");

	gRenderDebug->sendRemoteCommand("EndGroup"); // End the group called 'HACD settings'

	gRenderDebug->sendRemoteCommand("BeginGroup \"Simulation\"");	// Mark the beginning of a group of controls.
	gRenderDebug->sendRemoteCommand("Combo ForwardAxis ForwardAxis X-AXIS Y-AXIS Z-AXIS");
	gRenderDebug->sendRemoteCommand("Combo ConstraintType ConstraintType HINGE FIXED SPHERICAL BALLSOCKET");
	gRenderDebug->sendRemoteCommand("SliderInt LimitRangeDegrees 45 1 90 LimitRangeDegrees");
	gRenderDebug->sendRemoteCommand("CheckBox ShowConstraints true ShowConstraints");
	gRenderDebug->sendRemoteCommand("CheckBox ShowSkeleton true ShowSkeleton");
	gRenderDebug->sendRemoteCommand("CheckBox ShowCollisionPairs false ShowCollisionPairs");
	gRenderDebug->sendRemoteCommand("CheckBox SimulateAsRagdoll false SimulateAsRagdoll");
	gRenderDebug->sendRemoteCommand("Button ToggleSimulation ToggleSimulation");
	gRenderDebug->sendRemoteCommand("Slider DragForce 2 0.02 100 DragForce");
	gRenderDebug->sendRemoteCommand("EndGroup"); // End the group called 'controls'

	gRenderDebug->sendRemoteCommand("BeginGroup \"Bone Weighting\"");	// Mark the beginning of a group of controls.
	gRenderDebug->sendRemoteCommand("Slider BoneWeightPercentage 5 1 100 BoneWeightPercentage");
	gRenderDebug->sendRemoteCommand("Slider BoneWeightFalloff 1 1 4 BoneWeightFalloff");
	gRenderDebug->sendRemoteCommand("EndGroup"); // End the group called 'controls'
	gRenderDebug->sendRemoteCommand("EndTab"); // End the tab called 'Test RenderDebug'


        // Debug the V-HACD code
    gRenderDebug->sendRemoteCommand("BeginTab \"V-HACD Debugging\"");	// Mark the beginning of a new tab display in the DebugView application

    gRenderDebug->sendRemoteCommand("BeginGroup \"Voxelization\"");
    gRenderDebug->sendRemoteCommand("Button Voxelize Voxelize");
    gRenderDebug->sendRemoteCommand("Button ComputeInsideVoxels ComputeInsideVoxels");
    gRenderDebug->sendRemoteCommand("Button SnapVoxels SnapVoxels");
    gRenderDebug->sendRemoteCommand("CheckBox ShowVoxelSurface true ShowVoxelSurface");
    gRenderDebug->sendRemoteCommand("CheckBox ShowVoxelInside false ShowVoxelInside");
    gRenderDebug->sendRemoteCommand("CheckBox ShowVoxelOutside false ShowVoxelOutside");
    gRenderDebug->sendRemoteCommand("CheckBox ShowVoxelUndefined false ShowVoxelUndefined");
    gRenderDebug->sendRemoteCommand("EndGroup"); //

	gRenderDebug->sendRemoteCommand("BeginGroup \"LegionFu\"");	// Mark the beginning of a group of controls.
	gRenderDebug->sendRemoteCommand("Button MeshVertsToHulls MeshVertsToHulls");
	gRenderDebug->sendRemoteCommand("EndGroup"); // End the group called 'controls'


    gRenderDebug->sendRemoteCommand("EndTab"); // End the tab called 'Test RenderDebug'
}

#define HOST_NAME "localhost"
//#define HOST_NAME "192.168.1.2"  // IP address of my local machine

class ConvexDecomposition : public PHYSICS_DOM::NvPhysicsDOM::CommandCallback
{
public:
	ConvexDecomposition(void)
	{
		mTestHACD = TestHACD::create(gRenderDebug,gNvPhysicsDOM);
		mTestHACD->setDragForce(gDragForce);
		gRenderDebug->addToCurrentState(RENDER_DEBUG::DebugRenderState::CenterText);
		gNvPhysicsDOM->setCommandCallback(this);
	}

	~ConvexDecomposition(void)
	{
        SAFE_RELEASE(mVoxelizeMesh);
        SAFE_RELEASE(mTestHACD);
		delete[]mMeshVertices;
	}

	/**
	*\brief Optional callback to the application to process an arbitrary console command.

	This allows the application to process an incoming command from the server.  If the application consumes the command, then it will not be passed on
	to the rest of the default processing.  Return true to indicate that you consumed the command, false if you did not.

	\return Return true if your application consumed the command, return false if it did not.
	*/
	virtual bool processDebugCommand(uint32_t argc, const char **argv)
	{
		bool ret = false;

		if (argc )
		{
			const char *cmd = argv[0];
			if (strcmp(cmd, "client_stop") == 0)
			{
				mExit = true;
			}
			else if (strcmp(cmd, "decomp") == 0 && mTestHACD)
			{
				printf("Performing Convex Decomposition\n");
				mTestHACD->decompose(gVertices, gVertexCount, gIndices, gTriangleCount, gDesc);
			}
			else if (strcmp(cmd, "ShowPhysics") == 0 && argc == 2)
			{
				const char *value = argv[1];
				mShowPhysics = strcmp(value, "true") == 0;
			}
			else if (strcmp(cmd, "ShowConstraints") == 0 && argc == 2)
			{
				const char *value = argv[1];
				mShowConstraints = strcmp(value, "true") == 0;
			}
			else if (strcmp(cmd, "ShowSkeleton") == 0 && argc == 2)
			{
				const char *value = argv[1];
				mShowSkeleton = strcmp(value, "true") == 0;
			}
			else if (strcmp(cmd, "ShowCollisionPairs") == 0 && argc == 2)
			{
				const char *value = argv[1];
				mShowCollisionPairs = strcmp(value, "true") == 0;
			}
			else if (strcmp(cmd, "SimulateAsRagdoll") == 0 && argc == 2)
			{
				const char *value = argv[1];
				mSimulateAsRagdoll = strcmp(value, "true") == 0;
			}
			else if (strcmp(cmd, "SaveObj") == 0)
			{
				mWavefront.saveObj("wavefront.obj");
				printf("Saving mesh at current scale to 'wavefront.obj'\n");
			}
            else if (strcmp(cmd, "SavePython") == 0)
            {
                mWavefront.savePython("python.py");
                printf("Saving mesh at current scale to 'python.py'\n");
            }
			else if (strcmp(cmd, "CenterMesh") == 0)
			{
                releaseMesh();
				gCenterMesh = true;
				printf("Centering mesh.\n");
			}
			else if (strcmp(cmd, "DragForce") == 0)
			{
				if (argc == 2 )
				{
					gDragForce = (float)atof(argv[1]);
					if (mTestHACD)
					{
						mTestHACD->setDragForce(gDragForce);
					}
				}
			}
			else if (strcmp(cmd, "ToggleSimulation") == 0 && mTestHACD )
			{
				mTestHACD->toggleSimulation(mSimulateAsRagdoll,mBoneWeightFalloff,mBoneWeightPercentage, mConstraintType, 0, mLimitRangeDegrees, mLimitRangeDegrees, mLimitRangeDegrees, mForwardAxis);
			}
			else if (strcmp(cmd, "MeshVertsToHulls") == 0 && mTestHACD )
			{
				mTestHACD->meshVertsToHulls(gVertexCount,gVertices);
			}
			else if (strcmp(cmd, "raycast") == 0 && mTestHACD)
			{
				printf("Testing RaycastMesh\n");
				TestRaycast *r = TestRaycast::create();
				r->testRaycast(gVertexCount, gTriangleCount, gVertices, gIndices, gRenderDebug);
				r->release();
			}
			else if (strcmp(cmd, "cancel") == 0 && mTestHACD)
			{
				printf("Canceling Convex Decomposition\n");
				mTestHACD->cancel();
			}
			else if (strcmp(cmd, "LimitRangeDegrees") == 0 && argc == 2)
			{
				mLimitRangeDegrees = uint32_t( atoi(argv[1]));
				printf("LimitRangeDegrees=%d\n", mLimitRangeDegrees);
			}
			else if (strcmp(cmd, "BoneWeightFalloff") == 0 && argc == 2)
			{
				mBoneWeightFalloff = float(atof(argv[1]));
				printf("BoneWeightFalloff=%0.2f\n", mBoneWeightFalloff);
			}
			else if (strcmp(cmd, "BoneWeightPercentage") == 0 && argc == 2)
			{
				mBoneWeightPercentage = float(atof(argv[1]));
				printf("BoneWeightPercentage=%0.2f\n", mBoneWeightPercentage);
			}
			else if (strcmp(cmd, "ForwardAxis") == 0 && argc == 2)
			{
				const char *ct = argv[1];
				if (strcmp(ct, "X-AXIS") == 0)
				{
					mForwardAxis = 0;
				}
				else if (strcmp(ct, "Y-AXIS") == 0)
				{
					mForwardAxis = 1;
				}
				else if (strcmp(ct, "Z-AXIS") == 0)
				{
					mForwardAxis = 2;
				}
				printf("ForwardAxis=%s\n", ct);
			}
			else if (strcmp(cmd, "ConstraintType") == 0 && argc == 2)
			{
				const char *ct = argv[1];
				if (strcmp(ct, "HINGE") == 0)
				{
					mConstraintType = CT_HINGE;
				}
				else if (strcmp(ct, "FIXED") == 0)
				{
					mConstraintType = CT_FIXED;
				}
				else if (strcmp(ct, "SPHERICAL") == 0)
				{
					mConstraintType = CT_SPHERICAL;
				}
				else if (strcmp(ct, "BALLSOCKET") == 0)
				{
					mConstraintType = CT_BALL_AND_SOCKET;
				}
				printf("ConstraintType=%s\n", ct);
			}
			else if (strcmp(cmd, "MaxHullVertices") == 0 && argc == 2)
			{
				gDesc.m_maxNumVerticesPerCH = atoi(argv[1]);
				printf("MaxHullVertices=%d\n", gDesc.m_maxNumVerticesPerCH);
			}
			else if (strcmp(cmd, "MaxConvexHulls") == 0 && argc == 2)
			{
				gDesc.m_maxConvexHulls = atoi(argv[1]);
				printf("MaxConvexHulls=%d\n", gDesc.m_maxConvexHulls);
			}
			else if (strcmp(cmd, "ShowSourceMesh") == 0 && argc == 2)
			{
				const char *value = argv[1];
				gShowSourceMesh = strcmp(value, "true") == 0;
				printf("ShowSourceMesh=%s\n", value);
			}
			else if (strcmp(cmd, "ShowConvexDecomposition") == 0 && argc == 2)
			{
				const char *value = argv[1];
				gShowConvexDecomposition = strcmp(value, "true") == 0;
				printf("ShowConvexDecomposition=%s\n", value);
			}
			else if (strcmp(cmd, "Concavity") == 0 && argc == 2)
			{
				const char *value = argv[1];
				gDesc.m_concavity = (float)atof(value);
				printf("Concavity=%0.5f\n", gDesc.m_concavity);
			}
			else if (strcmp(cmd, "Alpha") == 0 && argc == 2)
			{
				const char *value = argv[1];
				gDesc.m_alpha = (float)atof(value);
				printf("Alpha=%0.5f\n", gDesc.m_alpha);
			}
			else if (strcmp(cmd, "Beta") == 0 && argc == 2)
			{
				const char *value = argv[1];
				gDesc.m_beta = (float)atof(value);
				printf("Beta=%0.5f\n", gDesc.m_beta);
			}
			else if (strcmp(cmd, "ProjectHullVertices") == 0 && argc == 2)
			{
				const char *value = argv[1];
				gDesc.m_projectHullVertices = strcmp(value, "true") == 0;
				printf("ProjectHullVertices=%s\n", gDesc.m_projectHullVertices ? "true" : "false");
			}
            else if (strcmp(cmd, "AsyncACD") == 0 && argc == 2)
            {
                const char *value = argv[1];
                gDesc.m_asyncACD = strcmp(value, "true") == 0;
                printf("AsyncACD=%s\n", gDesc.m_asyncACD ? "true" : "false");
            }
			else if (strcmp(cmd, "WireframeConvex") == 0 && argc == 2)
			{
				const char *value = argv[1];
				mWireframeConvex = strcmp(value, "true") == 0;
			}
			else if (strcmp(cmd, "ShowSelectedVerts") == 0 && argc == 2)
			{
				const char *value = argv[1];
				mShowSelectedVerts = strcmp(value, "true") == 0;
			}
			else if (strcmp(cmd, "WireframeSourceMesh") == 0 && argc == 2)
			{
				const char *value = argv[1];
				mWireframeSourceMesh = strcmp(value, "true") == 0;
				if (gDisplayURDF)
				{
					gDisplayURDF->debugLinks();
				}
			}
			else if (strcmp(cmd, "Resolution") == 0 && argc == 2)
			{
				const char *value = argv[1];
				gDesc.m_resolution = atoi(value);
				printf("Resolution=%d\n", gDesc.m_resolution);
			}
			else if (strcmp(cmd, "ExplodeViewScale") == 0 && argc == 2)
			{
				const char *value = argv[1];
				gExplodeViewScale = (float)atof(value);
				printf("ExplodeViewScale=%0.5f\n", gExplodeViewScale);
			}
			else if (strcmp(cmd, "ScaleInputMesh") == 0 && argc == 2)
			{
				const char *value = argv[1];
				gScaleInputMesh = (float)atof(value);
				printf("ScaleInputMesh=%0.5f\n", gScaleInputMesh);
                releaseMesh();
			}
			else if (strcmp(cmd, "TessellateInputMesh") == 0 && argc == 2)
			{
				const char *value = argv[1];
				gTessellateInputMesh = uint32_t(atoi(value));
				printf("TessellateInputMesh=%d\n", gTessellateInputMesh);
                releaseMesh();
			}
			else if (strcmp(cmd, "save") == 0)
			{
				if (mTestHACD)
				{
					mTestHACD->saveConvexDecomposition("ConvexDecomposition.obj", gSourceMeshName.c_str());
				}
			}
            else if (strcmp(cmd, "Voxelize") == 0)
            {
            SAFE_RELEASE(mVoxelizeMesh);
            mVoxelizeMesh = voxelizemesh::VoxelizeMesh::create(gVertexCount, gTriangleCount, gVertices, gIndices, gDesc.m_resolution, gRenderDebug, gDesc);
            }
            else if (strcmp(cmd, "ComputeInsideVoxels") == 0)
            {
            if (mVoxelizeMesh)
            {
                mVoxelizeMesh->computeInsideVoxels();
            }
            }
            else if (strcmp(cmd, "SnapVoxels") == 0)
            {
                if (mVoxelizeMesh)
                {
                    mVoxelizeMesh->snapVoxelToMesh();
                }
            }
            else if (strcmp(cmd, "ShowVoxelSurface") == 0 && argc == 2)
            {
                const char *value = argv[1];
                gShowVoxelSurface = strcmp(value, "true") == 0;
                printf("ShowVoxelSurface=%s\n", value);
            }
            else if (strcmp(cmd, "ShowVoxelInside") == 0 && argc == 2)
            {
                const char *value = argv[1];
                gShowVoxelInside = strcmp(value, "true") == 0;
                printf("ShowVoxelInside=%s\n", value);
            }
            else if (strcmp(cmd, "ShowVoxelOutside") == 0 && argc == 2)
            {
                const char *value = argv[1];
                gShowVoxelOutside = strcmp(value, "true") == 0;
                printf("ShowVoxelOutside=%s\n", value);
            }
            else if (strcmp(cmd, "ShowVoxelUndefined") == 0 && argc == 2)
            {
                const char *value = argv[1];
                gShowVoxelUndefined = strcmp(value, "true") == 0;
                printf("ShowVoxelUndefined=%s\n", value);
            }
            else if (strcmp(cmd, "VoxelFillMode") == 0 && argc == 2)
            {
                const char *fmode = argv[1];
                if (strcmp(fmode, "FLOOD_FILL") == 0)
                {
                    gDesc.m_fillMode = VHACD::FillMode::FLOOD_FILL;
                }
                else if (strcmp(fmode, "SURFACE_ONLY") == 0)
                {
                    gDesc.m_fillMode = VHACD::FillMode::SURFACE_ONLY;
                }
                else if (strcmp(fmode, "RAYCAST_FILL") == 0)
                {
                    gDesc.m_fillMode = VHACD::FillMode::RAYCAST_FILL;
                }
                else
                {
                    assert(0);
                }
                printf("Voxel FillMode=%s\n", fmode);
            }

		}

		return ret;
	}

	bool process(void)
	{
		if (mTestHACD == nullptr)
		{
			mTestHACD = TestHACD::create(gRenderDebug, gNvPhysicsDOM);
			mTestHACD->setDragForce(gDragForce);
		}
		if (mTestHACD->getMeshID() == 0 && mSourceMesh.mVertexCount)
		{
			mSourceMesh.deepCopyScale(mWavefront, gScaleInputMesh,gCenterMesh,gTessellateInputMesh);
			gCenterMesh = false; // clear the center mesh semaphore
			gVertexCount = mWavefront.mVertexCount;
			gTriangleCount = mWavefront.mTriCount;
			delete[]mMeshVertices;
			mMeshVertices = new double[mWavefront.mVertexCount * 3];
			gVertices = mMeshVertices;
			gIndices = mWavefront.mIndices;
			for (uint32_t i = 0; i < mWavefront.mVertexCount; i++)
			{
				mMeshVertices[i * 3 + 0] = mWavefront.mVertices[i * 3 + 0];
				mMeshVertices[i * 3 + 1] = mWavefront.mVertices[i * 3 + 1];
				mMeshVertices[i * 3 + 2] = mWavefront.mVertices[i * 3 + 2];
			}
			FLOAT_MATH::fm_computeCentroid(mWavefront.mVertexCount, mWavefront.mVertices, mWavefront.mTriCount,mWavefront.mIndices, gCenter);
			mTestHACD->setRenderMesh(mWavefront.mVertexCount, mWavefront.mVertices, mWavefront.mTriCount, mWavefront.mIndices);
		}
		gRenderDebug->debugText2D(0, 0.04f, 0.5f, 2.0f, false, 0xFFFF00, "%s", mMeshName.c_str());
		gRenderDebug->debugText2D(0, 0.08f, 0.5f, 2.0f, false, 0xFFFF00, "VertexCount: %d TriangleCount: %d", mWavefront.mVertexCount, mWavefront.mTriCount);
		gRenderDebug->debugText2D(0, 0.12f, 0.5f, 2.0f, false, 0xFFFF00, "HullCount: %d ConstraintCount: %d CollisionFilterCount: %d", mTestHACD->getHullCount(), mTestHACD->getConstraintCount(), mTestHACD->getCollisionFilterCount());

		if (gDisplayURDF) // if we have imported a URDF, then render it instead
		{
			gDisplayURDF->render(gRenderDebug, gScaleInputMesh,gShowSourceMesh,gShowConvexDecomposition);
		}
		else
		{
			// render it
			mTestHACD->render(gExplodeViewScale, gCenter,
				gShowSourceMesh,
				mWireframeSourceMesh,
				gShowConvexDecomposition,
				mWireframeConvex,
				mShowConstraints,
				mShowSkeleton,
				mShowCollisionPairs,
				mShowSelectedVerts,
				mForwardAxis);
		}

        if (mVoxelizeMesh)
        {
            mVoxelizeMesh->visualize(gShowVoxelSurface, gShowVoxelInside, gShowVoxelOutside, gShowVoxelUndefined);
        }

		gNvPhysicsDOM->simulate(mShowPhysics);

		const char *nameSpace;
		const char *resourceName;
		bool isBigEndianRemote;
		uint32_t dlen;
		const void *data = gRenderDebug->getRemoteResource(nameSpace, resourceName, dlen, isBigEndianRemote);
		while (data)
		{
			gSourceMeshName = std::string(resourceName);
			printf("Received remote resource %s:%s %d bytes long and remote machine is %sbig endian\r\n",
				nameSpace,
				resourceName,
				dlen,
				isBigEndianRemote ? "" : "not ");

			if (strcmp(nameSpace, "MeshFile") == 0)
			{
                releaseMesh();
				mMeshName = std::string(resourceName);

				if (gDisplayURDF)
				{
					gDisplayURDF->release();
					gDisplayURDF = nullptr;
				}
				char _resourceName[512];
				strncpy(_resourceName, resourceName, 512);
				_strlwr(_resourceName);
				const char *isObj = strstr(_resourceName, ".obj");
				const char *isOFF = strstr(_resourceName, ".off");
				const char *isSTL = strstr(_resourceName, ".stl");
				const char *isDAE = strstr(_resourceName, ".dae");
				const char *isURDF = strstr(_resourceName, ".urdf");
				if (isURDF)
				{
					gDisplayURDF = DISPLAY_URDF::DisplayURDF::create();
					if (gDisplayURDF)
					{
						gDisplayURDF->importURDF(resourceName);
					}
				}
				else
				{
					if (isObj)
					{
						mSourceMesh.loadObj((const uint8_t *)data, dlen);
						//mSourceMesh.saveCPP("mesh.cpp");
					}
					if (isOFF)
					{
						mSourceMesh.loadOFF((const uint8_t *)data, dlen);
					}
					if (isSTL)
					{
						mSourceMesh.loadSTL((const uint8_t *)data, dlen);
					}
					if (isDAE)
					{
						mSourceMesh.loadDAE((const uint8_t *)data, dlen);
					}
					printf("Loaded Mesh file %s with %d triangles and %d vertices.\r\n", resourceName, mSourceMesh.mTriCount, mSourceMesh.mVertexCount);
				}
			}
			data = gRenderDebug->getRemoteResource(nameSpace, resourceName, dlen, isBigEndianRemote);
		}

		static bool first = true;
		if (first)
		{
			first = false;
			createMenus();
		}
		return !mExit;
	}

    void releaseMesh(void)
    {
        SAFE_RELEASE(mTestHACD);
        SAFE_RELEASE(mVoxelizeMesh);
    }

	uint32_t	mLimitRangeDegrees{ 45 };
	float		mBoneWeightFalloff{ 1 };
	float		mBoneWeightPercentage{ 5 };
	bool		mSimulateAsRagdoll{ false };
	bool		mShowConstraints{ true };
	bool		mShowCollisionPairs{ false };
	bool		mShowSkeleton{ true };
	bool		mShowPhysics{ true };
	bool		mWireframeConvex{ false };
	bool		mShowSelectedVerts{ false };
	bool		mWireframeSourceMesh{ false };
	TestHACD	*mTestHACD{ nullptr };
    voxelizemesh::VoxelizeMesh *mVoxelizeMesh{nullptr};
	bool		mExit{ false };
	WavefrontObj mSourceMesh;
	WavefrontObj mWavefront;
	double		*mMeshVertices{ nullptr };
	std::string	mMeshName;
	ConstraintType	mConstraintType{ CT_HINGE };
	uint32_t	mForwardAxis{ 0 };
};

#define USE_DEBUG 0


int32_t main(int32_t /*argc*/,const char ** /*argv*/)
{
	{

		const char *dllName = nullptr;
#if _M_X64
#if USE_DEBUG
		dllName = "PhysicsDOM64DEBUG.dll";
#else
		dllName = "PhysicsDOM64.dll";
#endif
#else
#if USE_DEBUG
		dllName = "NvPhysicsDOM32DEBUG.dll";
#else
		dllName = "NvPhysicsDOM32.dll";
#endif
#endif

		printf("Loading NvPhysicsDOM DLL\r\n");

		gNvPhysicsDOM = PHYSICS_DOM::createNvPhysicsDOM(PHYSICS_DOM_VERSION_NUMBER, dllName);
		gRenderDebug = gNvPhysicsDOM ? gNvPhysicsDOM->getRenderDebug() : nullptr;
		if ( gRenderDebug )
		{
			ConvexDecomposition cd;
			while (cd.process());
		}
		else
		{
			printf("Failed to locate DebugView.\n");
			printf("Go to: https://github.com/jratcliff63367/debugview\n");
			printf("Clone the depot and then run the provided DebugView.exe application first\n");
		}
		if (gDisplayURDF)
		{
			gDisplayURDF->release();
			gDisplayURDF = nullptr;
		}
		if (gNvPhysicsDOM)
		{
			gNvPhysicsDOM->release();
		}

	}
	return 0;
}
