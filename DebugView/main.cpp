#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <math.h>
#include <string>

#include "wavefront.h"
#include "NvRenderDebug.h"
#include "TestHACD.h"
#include "TestRaycast.h"
#include "VHACD.h"

#pragma warning(disable:4456 4457)

#define TSCALE1 (1.0f/4.0f)

RENDER_DEBUG::RenderDebugTyped *gRenderDebugTyped=NULL;
RENDER_DEBUG::RenderDebug *gRenderDebug=NULL;

static bool			gShowSourceMesh = true;
static bool			gShowConvexDecomposition = true;
static bool			gUseHACD = true;
static float		gScaleInputMesh = 1;
static float		gExplodeViewScale = 1;
static float		gCenter[3] { 0, 0, 0 };
static uint32_t		gVertexCount = 0;
static uint32_t		gTriangleCount = 0;
static double		*gVertices = nullptr;
static uint32_t		*gIndices = nullptr;
static std::string	 gSourceMeshName;

static VHACD::IVHACD::Parameters gDesc;


float fm_computePlane(const float *A,const float *B,const float *C,float *n) // returns D
{
	float vx = (B[0] - C[0]);
	float vy = (B[1] - C[1]);
	float vz = (B[2] - C[2]);

	float wx = (A[0] - B[0]);
	float wy = (A[1] - B[1]);
	float wz = (A[2] - B[2]);

	float vw_x = vy * wz - vz * wy;
	float vw_y = vz * wx - vx * wz;
	float vw_z = vx * wy - vy * wx;

	float mag = ::sqrtf((vw_x * vw_x) + (vw_y * vw_y) + (vw_z * vw_z));

	if ( mag < 0.000001f )
	{
		mag = 0;
	}
	else
	{
		mag = 1.0f/mag;
	}

	float x = vw_x * mag;
	float y = vw_y * mag;
	float z = vw_z * mag;


	float D = 0.0f - ((x*A[0])+(y*A[1])+(z*A[2]));

	n[0] = x;
	n[1] = y;
	n[2] = z;

	return D;
}

class MeshBuilder
{
public:	
	MeshBuilder(uint32_t maxVertices)
	{
		mVertices.reserve(maxVertices);
	}

	void getVertex(const float *p,const float *n,uint32_t i1,uint32_t i2)
	{
		RENDER_DEBUG::RenderDebugMeshVertex v;

		v.mPosition[0] = p[0];
		v.mPosition[1] = p[1];
		v.mPosition[2] = p[2];

		v.mNormal[0] = n[0];
		v.mNormal[1] = n[1];
		v.mNormal[2] = n[2];

		v.mTexel[0] = p[i1]*TSCALE1;
		v.mTexel[1] = p[i2]*TSCALE1;

		mVertices.push_back(v);
	}

	void addTriangle(const float *p1,const float *p2,const float *p3)
	{
		float normal[3];
		fm_computePlane(p3,p2,p1,normal);

		double nx = fabs(normal[0]);
		double ny = fabs(normal[1]);
		double nz = fabs(normal[2]);

		uint32_t i1 = 0;
		uint32_t i2 = 0;

		if ( nx <= ny && nx <= nz ) 
			i1 = 0;
		if ( ny <= nx && ny <= nz ) 
			i1 = 1;
		if ( nz <= nx && nz <= ny ) 
			i1 = 2;

		switch ( i1 )
		{
		case 0:
			if ( ny < nz )
				i2 = 1;
			else
				i2 = 2;
			break;
		case 1:
			if ( nx < nz )
				i2 = 0;
			else
				i2 = 2;
			break;
		case 2:
			if ( nx < ny )
				i2 = 0;
			else
				i2 = 1;
			break;
		}

		getVertex(p1,normal,i1,i2);
		getVertex(p2,normal,i1,i2);
		getVertex(p3,normal,i1,i2);
	}

	std::vector< RENDER_DEBUG::RenderDebugMeshVertex > mVertices;
};

static void  fm_computCenter(uint32_t vcount, const float *vertices, float center[3])
{
	float bmin[3];
	float bmax[3];

	bmin[0] = vertices[0];
	bmin[1] = vertices[1];
	bmin[2] = vertices[2];

	bmax[0] = vertices[0];
	bmax[1] = vertices[1];
	bmax[2] = vertices[2];

	for (uint32_t i = 1; i < vcount; i++)
	{
		const float *v = &vertices[i * 3];

		if (v[0] < bmin[0]) bmin[0] = v[0];
		if (v[1] < bmin[1]) bmin[1] = v[1];
		if (v[2] < bmin[2]) bmin[2] = v[2];

		if (v[0] > bmax[0]) bmax[0] = v[0];
		if (v[1] > bmax[1]) bmax[1] = v[1];
		if (v[2] > bmax[2]) bmax[2] = v[2];
	}

	center[0] = (bmax[0] - bmin[0])*0.5f + bmin[0];
	center[1] = (bmax[1] - bmin[1])*0.5f + bmin[1];
	center[2] = (bmax[2] - bmin[2])*0.5f + bmin[2];

}



void createMenus(void)
{
	gRenderDebug->sendRemoteCommand("BeginTab \"Convex Decomposition - V-HACD\"");	// Mark the beginning of a new tab display in the DebugView application

	gRenderDebug->sendRemoteCommand("BeginGroup \"Controls\"");	// Mark the beginning of a group of controls.
	gRenderDebug->sendRemoteCommand("FileTransferButton \" Select Wavefront File\" WavefrontFile \"Choose a Wavefront OBJ file to transfer\" *.obj");
	gRenderDebug->sendRemoteCommand("FileTransferButton \" Select OFF File\" OFFFile \"Choose an OFF file to transfer\" *.off");
	gRenderDebug->sendRemoteCommand("Button SaveConvexDecomposition \"save\"");
	gRenderDebug->sendRemoteCommand("EndGroup"); // End the group called 'controls'

	gRenderDebug->sendRemoteCommand("BeginGroup \"View\"");	// Mark the beginning of a group of controls.
	gRenderDebug->sendRemoteCommand("CheckBox ShowSourceMesh true ShowSourceMesh");
	gRenderDebug->sendRemoteCommand("CheckBox ShowConvexDecomposition true ShowConvexDecomposition");
	gRenderDebug->sendRemoteCommand("Slider ScaleInputMesh 1 0.01 100 ScaleInputMesh");
	gRenderDebug->sendRemoteCommand("Slider ExplodeViewScale 1 1 4 ExplodeViewScale");
	gRenderDebug->sendRemoteCommand("Button PerformConvexDecomposition \"decomp\"");
	gRenderDebug->sendRemoteCommand("Button TestRaycastMesh \"raycast\"");
	gRenderDebug->sendRemoteCommand("Button Cancel \"cancel\"");
	gRenderDebug->sendRemoteCommand("EndGroup"); // End the group called 'controls'


	gRenderDebug->sendRemoteCommand("BeginGroup \"V-HACD Settings1\"");	// Mark the beginning of a group of controls.
	gRenderDebug->sendRemoteCommand("SliderInt DecompositionDepth 10 1 20 DecompositionDepth");
	gRenderDebug->sendRemoteCommand("SliderInt MaxHullVertices 32 8 512 MaxHullVertices");
	gRenderDebug->sendRemoteCommand("SliderInt MaxConvexHulls 32 1 512 MaxConvexHulls");
	gRenderDebug->sendRemoteCommand("Slider Concavity 0.001 0 0.1 Concavity");
	gRenderDebug->sendRemoteCommand("EndGroup"); // End the group called 'HACD settings'

	gRenderDebug->sendRemoteCommand("BeginGroup \"V-HACD Settings2\"");	// Mark the beginning of a group of controls.
	gRenderDebug->sendRemoteCommand("Slider Alpha 0.0005 0 0.1 Alpha");
	gRenderDebug->sendRemoteCommand("Slider Beta 0.05 0 0.1 Beta");
	//gRenderDebug->sendRemoteCommand("SliderInt Resolution 100000 10000 1000000 Resolution");
	gRenderDebug->sendRemoteCommand("SliderInt Resolution 100000 10000 64000000 Resolution");
	gRenderDebug->sendRemoteCommand("EndGroup"); // End the group called 'HACD settings'


	gRenderDebug->sendRemoteCommand("EndTab"); // End the tab called 'Test RenderDebug'
}

#define HOST_NAME "localhost"
//#define HOST_NAME "192.168.1.2"  // IP address of my local machine

int main(int argc,const char **argv)
{
	{
		const char *dllName=NULL;
#ifdef _WIN64
		dllName = "NvRenderDebug_x64.dll";
#else
		dllName = "NvRenderDebug_x86.dll";
#endif
		printf("Loading RenderDebug DLL\r\n");
		RENDER_DEBUG::RenderDebug::Desc desc;
		desc.dllName = dllName;

		desc.hostName = HOST_NAME;

		desc.applicationName = "ConvexDecomposition";
		desc.runMode = RENDER_DEBUG::RenderDebug::RM_CLIENT;
		gRenderDebug = RENDER_DEBUG::createRenderDebug(desc);
		if ( gRenderDebug )
		{
			WavefrontObj sourceMesh;
			if (argc == 2)
			{
				printf("Loading wavefront.obj file '%s'\r\n", argv[1]);
				sourceMesh.loadObj(argv[1]);
			}
			{
				TestHACD *thacd = TestHACD::create(gRenderDebug);


				double *meshVertices = nullptr;

				printf("Found: %d triangles.\r\n", sourceMesh.mTriCount );

				gRenderDebug->addToCurrentState(RENDER_DEBUG::DebugRenderState::CenterText);

				{
					uint32_t meshId = 0;
					bool solid=true;
					const char *meshName = argv[1];

					// main pump loop...
					WavefrontObj w;

					for (;;)
					{
						if (meshId == 0 && sourceMesh.mVertexCount )
						{
							sourceMesh.deepCopyScale(w, gScaleInputMesh);
							gVertexCount = w.mVertexCount;
							gTriangleCount = w.mTriCount;
							delete[]meshVertices;
							meshVertices = new double[w.mVertexCount * 3];
							gVertices = meshVertices;
							gIndices = w.mIndices;

							for (uint32_t i = 0; i < w.mVertexCount; i++)
							{
								meshVertices[i * 3 + 0] = w.mVertices[i * 3 + 0];
								meshVertices[i * 3 + 1] = w.mVertices[i * 3 + 1];
								meshVertices[i * 3 + 2] = w.mVertices[i * 3 + 2];
							}

							meshId = gRenderDebug->getMeshId();
							{
								MeshBuilder mb(w.mTriCount * 3);
								for (uint32_t i = 0; i < w.mTriCount; i++)
								{
									uint32_t i1 = w.mIndices[i * 3 + 0];
									uint32_t i2 = w.mIndices[i * 3 + 1];
									uint32_t i3 = w.mIndices[i * 3 + 2];
									const float *p1 = &w.mVertices[i1 * 3];
									const float *p2 = &w.mVertices[i2 * 3];
									const float *p3 = &w.mVertices[i3 * 3];
									mb.addTriangle(p3, p2, p1);
								}
								gRenderDebug->createTriangleMesh(meshId, (uint32_t)mb.mVertices.size(), &mb.mVertices[0], 0, NULL);
							}
							fm_computCenter(w.mVertexCount, w.mVertices, gCenter);
						}


						gRenderDebug->debugText2D(0, 0, 0.5f, 2.0f, false, 0xFFFF00, "%s", meshName);
						if (thacd)
						{
							gRenderDebug->debugText2D(0, 0.04f, 0.5f, 2.0f, false, 0xFFFF00, "HullCount: %d", thacd->getHullCount());
						}
	

						gRenderDebug->addToCurrentState(RENDER_DEBUG::DebugRenderState::SolidWireShaded);
						gRenderDebug->addToCurrentState(RENDER_DEBUG::DebugRenderState::CameraFacing);
						gRenderDebug->setCurrentColor(0xFFFF00);

						if (gShowSourceMesh)
						{
							if (solid)
							{
								RENDER_DEBUG::RenderDebugInstance instance;
								gRenderDebug->renderTriangleMeshInstances(meshId, 1, &instance);
							}
							else
							{
								for (uint32_t i = 0; i < w.mTriCount; i++)
								{
									uint32_t i1 = w.mIndices[i * 3 + 0];
									uint32_t i2 = w.mIndices[i * 3 + 1];
									uint32_t i3 = w.mIndices[i * 3 + 2];

									const float *p1 = &w.mVertices[i1 * 3];
									const float *p2 = &w.mVertices[i2 * 3];
									const float *p3 = &w.mVertices[i3 * 3];

									gRenderDebug->debugTri(p3, p2, p1);
								}
							}
						}
						if (thacd == nullptr)
						{
							thacd = TestHACD::create(gRenderDebug);
						}
						if (thacd && gShowConvexDecomposition )
						{
							thacd->render(gExplodeViewScale,gCenter);
						}

						gRenderDebug->render(1.0f/60.0f,NULL);

						uint32_t argc;
						const char **argv = gRenderDebug->getRemoteCommand(argc);
						if ( argv )
						{
							const char *cmd = argv[0];
							if ( strcmp(cmd,"client_stop") == 0 )
							{
								break;
							}
							else if (strcmp(cmd,"toggle") == 0  )
							{
								solid = solid ? false : true;
							}
							else if (strcmp(cmd, "decomp") == 0 && thacd)
							{
								printf("Performing Convex Decomposition\n");
								thacd->decompose(gVertices, gVertexCount, (const int *)gIndices, gTriangleCount, gDesc);
							}
							else if (strcmp(cmd, "raycast") == 0 && thacd)
							{
								printf("Testing RaycastMesh\n");
								TestRaycast *r = TestRaycast::create();
								r->testRaycast(gVertexCount, gTriangleCount, gVertices, gIndices, gRenderDebug);
								r->release();
							}
							else if (strcmp(cmd, "cancel") == 0 && thacd)
							{
								printf("Canceling Convex Decomposition\n");
								thacd->cancel();
							}
							else if (strcmp(cmd, "DecompositionDepth") == 0 && argc == 2)
							{
								gDesc.m_depth = atoi(argv[1]);
								printf("DecompositionDepth=%d\n", gDesc.m_depth);
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
								if (thacd)
								{
									thacd->release();
									thacd = nullptr;
								}
								gRenderDebug->releaseTriangleMesh(meshId);
								meshId = 0;
							}
							else if (strcmp(cmd, "save") == 0 )
							{
								if (thacd)
								{
									thacd->saveConvexDecomposition("ConvexDecomposition.obj", gSourceMeshName.c_str() );
								}
							}
						}

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

							if (strcmp(nameSpace, "WavefrontFile") == 0)
							{
								thacd->release();
								thacd = nullptr;
								meshName = resourceName;
								sourceMesh.loadObj((const uint8_t *)data, dlen);
								printf("Loaded Wavefront file %s with %d triangles and %d vertices.\r\n", resourceName, sourceMesh.mTriCount, sourceMesh.mVertexCount);
								gRenderDebug->releaseTriangleMesh(meshId);
								meshId = 0;
							}
							if (strcmp(nameSpace, "OFFFile") == 0)
							{
								thacd->release();
								thacd = nullptr;
								meshName = resourceName;
								sourceMesh.loadOFF((const uint8_t *)data, dlen);
								printf("Loaded OFF file %s with %d triangles and %d vertices.\r\n", resourceName, sourceMesh.mTriCount, sourceMesh.mVertexCount);
								gRenderDebug->releaseTriangleMesh(meshId);
								meshId = 0;
							}

							data = gRenderDebug->getRemoteResource(nameSpace, resourceName, dlen, isBigEndianRemote);
						}

						static bool first = true;
						if (first)
						{
							first = false;
							createMenus();
						}
					}
				}
				if (thacd)
				{
					thacd->release();
				}
				delete[]meshVertices;
			}
			gRenderDebug->release();
		}
		else
		{
			printf("Failed to locate DebugView.\n");
			printf("Go to: https://github.com/jratcliff63367/debugview\n");
			printf("Clone the depot and then run the provided DebugView.exe application first\n");
		}
	}
	return 0;
}
