#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ENABLE_VHACD_IMPLEMENTATION 1
#include "VHACD.h"
#include "wavefront.h"
#include "ScopedTime.h"

#include <thread>
#include <string>
#include <vector>

#ifdef _MSC_VER
#pragma warning(disable:4100 4996)
#include <conio.h>
#endif

// Evaluates if this is true or false, returns true if it 
// could be evaluated. Stores the result into 'value'
bool getTrueFalse(const char *option,bool &value)
{
	bool ret = false;

	if ( strcmp(option,"t") == 0 ||
		 strcmp(option,"true") == 0 ||
		 strcmp(option,"1") == 0 )
	{
		ret = true;
		value = true;
	}
	else if ( strcmp(option,"f") == 0 ||
		 strcmp(option,"false") == 0 ||
		 strcmp(option,"0") == 0 )
	{
		ret = true;
		value = false;
	}
	else
	{
		printf("Valid values are 'true' or 'false', 't' or 'f', or '1' or '0', only.\n");
	}

	return ret;
}

class Logging : public VHACD::IVHACD::IUserCallback, public VHACD::IVHACD::IUserLogger
{
public:
	Logging(void)
	{
	}

	~Logging(void)
	{
		flushMessages();
	}

        // Be aware that if you are running V-HACD asynchronously (in a background thread) this callback will come from
        // a different thread. So if your print/logging code isn't thread safe, take that into account.
        virtual void Update(const double overallProgress,
                            const double stageProgress,
                            const char* const stage,const char *operation) final
		{
			char scratch[512];
			snprintf(scratch,sizeof(scratch),"[%-40s] : %0.0f%% : %0.0f%% : %s",stage,overallProgress,stageProgress,operation);

			if ( strcmp(stage,mCurrentStage.c_str()) == 0 )
			{
				for (uint32_t i=0; i<mLastLen; i++)
				{
					printf("%c", 8);
				}
			}
			else
			{
				printf("\n");
				mCurrentStage = std::string(stage);
			}
			mLastLen = (uint32_t)strlen(scratch);
			printf("%s", scratch);
		}

        // This is an optional user callback which is only called when running V-HACD asynchronously.
        // This is a callback performed to notify the user that the
        // convex decomposition background process is completed. This call back will occur from
        // a different thread so the user should take that into account.
        virtual void NotifyVHACDComplete(void)
        {
			Log("VHACD::Complete");
        }

		virtual void Log(const char* const msg) final
		{
			mLogMessages.push_back(std::string(msg));
		}

		void flushMessages(void)
		{
			if ( !mLogMessages.empty() )
			{
				printf("\n");
				for (auto &i:mLogMessages)
				{
					printf("%s\n", i.c_str());
				}
				mLogMessages.clear();
			}
		}

		uint32_t	mLastLen{0};
		std::string mCurrentStage;
		std::vector< std::string > mLogMessages;

};

int main(int argc,const char **argv)
{
	if ( argc < 2 )
	{
		printf("Usage: TestVHACD <wavefront.obj> (options)\n");
		printf("\n");
		printf("-h <n>                  : Maximum number of output convex hulls. Default is 32\n");
		printf("-r <voxelresolution>    : Total number of voxels to use. Default is 100,000\n");
		printf("-e <volumeErrorPercent> : Volume error allowed as a percentage. Default is 4%%\n");
		printf("-d <maxRecursionDepth>  : Maximum recursion depth. Default value is 12.\n");
		printf("-s <true/false>         : Whether or not to shrinkwrap output to source mesh. Default is true.\n");
		printf("-f <fillMode>           : Fill mode. Default is 'flood', also 'surface' and 'raycast' are valid.\n");
		printf("-v <maxHullVertCount>   : Maximum number of vertices in the output convex hull. Default value is 64\n");
		printf("-a <true/false>         : Whether or not to run asynchronously. Default is 'true'\n");
		printf("-l <minEdgeLength>      : Minimum size of a voxel edge. Default value is 4 voxels.\n");
	}
	else
	{
		Logging logging;
		VHACD::IVHACD::Parameters p;
		p.m_callback = &logging;
		p.m_logger = &logging;
		const char *inputFile = argv[1];

		WavefrontObj w;
		uint32_t tcount = w.loadObj(inputFile);
		if ( tcount == 0 )
		{
			printf("Failed to load valid mesh from wavefront OBJ file:%s\n", inputFile);
		}
		else
		{
			for (int i=2; i<argc; i+=2)
			{
				const char *option = argv[i];
				const char *value = argv[i+1];
				if ( strcmp(option,"-h") == 0 )
				{
					int32_t v = atoi(value);
					if ( v < 1 || v > 100000 )
					{
						printf("Invalid maximum number of convex hulls. Valid range is 1-100000\n");
					}
					else
					{
						p.m_maxConvexHulls = uint32_t(v);
						printf("Maximum convex hulls set to:%d\n", p.m_maxConvexHulls);
					}
				}
				else if ( strcmp(option,"-r") == 0 )
				{
					int32_t r = atoi(value);
					if ( r >= 10000 && r <= 10000000 )
					{
						printf("Voxel Resolution set to:%d\n", r);
						p.m_resolution = uint32_t(r);
					}
					else
					{
						printf("Invalid voxel resolution must be between 10,000 and 10,000,000\n");
					}
				}
				else if ( strcmp(option,"-e") == 0 )
				{
					double e = atof(value);
					if ( e < 0 || e > 50 )
					{
						printf("Invalid error percentage. Valid values are 0 to 50\n");
					}
					else
					{
						p.m_minimumVolumePercentErrorAllowed = e;
						printf("Minimum volume error allowed set to:%0.2f%%\n", p.m_minimumVolumePercentErrorAllowed);
					}
				}
				else if ( strcmp(option,"-d") == 0 )
				{
					int32_t r = atoi(value);
					if ( r >= 2 && r <= 64 )
					{
						printf("Maximum recursion depth set to:%d\n", r);
						p.m_maxRecursionDepth = uint32_t(r);
					}
					else
					{
						printf("Invalid maximum recursion depth, must be between 2 and 64\n");
					}
				}
				else if ( strcmp(option,"-s") == 0 )
				{
					if ( getTrueFalse(value,p.m_shrinkWrap) )
					{
						if ( p.m_shrinkWrap )
						{
							printf("Shrinkwrap enabled.\n");
						}
						else
						{
							printf("Shrinkwrap disabled.\n");
						}
					}
				}
				else if ( strcmp(option,"-f") == 0 )
				{
					if ( strcmp(value,"flood") == 0 )
					{
						p.m_fillMode = VHACD::FillMode::FLOOD_FILL;
						printf("FillMode set to FLOOD\n");
					}
					else if ( strcmp(value,"raycast") == 0 )
					{
						p.m_fillMode = VHACD::FillMode::RAYCAST_FILL;
						printf("FillMode set to RAYCAST\n");
					}
					else if ( strcmp(value,"surface") == 0 )
					{
						p.m_fillMode = VHACD::FillMode::SURFACE_ONLY;
						printf("FillMode set to SURFACE\n");
					}
					else
					{
						printf("Invalid fill mode, only valid options are 'flood', 'raycast', and 'surface'\n");
					}
				}
				else if ( strcmp(option,"-v") == 0 )
				{
					int32_t r = atoi(value);
					if ( r >= 8 && r <= 2048 )
					{
						printf("Maximum hull vertices set to:%d\n", r);
						p.m_maxNumVerticesPerCH = uint32_t(r);
					}
					else
					{
						printf("Invalid maximum hull vertices, must be between 8 and 20484\n");
					}
				}
				else if ( strcmp(option,"-a") == 0 )
				{
					if ( getTrueFalse(value,p.m_asyncACD) )
					{
						if ( p.m_asyncACD )
						{
							printf("Asynchronous mode enabled\n");
						}
						else
						{
							printf("Synchronous mode disabled\n");
						}
					}
				}
				else if ( strcmp(option,"-l") == 0 )
				{
					int32_t r = atoi(value);
					if ( r >= 1 && r <= 32 )
					{
						printf("Minimum voxel edge length set to:%d\n", r);
						p.m_minEdgeLength = uint32_t(r);
					}
					else
					{
						printf("Invalid minimum voxel edge length, must be between 1 and 32\n");
					}
				}
			}

			VHACD::IVHACD *iface = VHACD::CreateVHACD_ASYNC();
#ifdef _MSC_VER
			printf("Press the SPACEBAR to cancel convex decomposition before it has completed.\n");
#endif
			double *points = new double[w.mVertexCount*3];
			for (uint32_t i=0; i<w.mVertexCount*3; i++)
			{
				points[i] = w.mVertices[i];
			}
			bool canceled = false;
			{
				ScopedTime st("Computing Convex Decomposition");
				iface->Compute(points,w.mVertexCount,w.mIndices,w.mTriCount,p);
				while ( !iface->IsReady() )
				{
					std::this_thread::sleep_for(std::chrono::nanoseconds(10000)); // s
#ifdef _MSC_VER
					if ( kbhit() )
					{
						char c = getch();
						if ( c == 32 )
						{
							printf("Canceling convex decomposition.\n");
							iface->Cancel();
							canceled = true;
						}
					}
#endif
				}
				logging.flushMessages();
			}
			if ( !canceled && iface->GetNConvexHulls() )
			{
				FILE *fph = fopen("decomp.obj", "wb");
				if ( fph )
				{
					printf("Saving Convex Decomposition results of %d convex hulls to 'decomp.obj'\n", iface->GetNConvexHulls());
					uint32_t baseIndex = 1;
					for (uint32_t i=0; i<iface->GetNConvexHulls(); i++)
					{
						VHACD::IVHACD::ConvexHull ch;
						iface->GetConvexHull(i,ch);
						for (uint32_t j=0; j<ch.m_nPoints; j++)
						{
							const double *pos = &ch.m_points[j*3];
							fprintf(fph,"v %0.9f %0.9f %0.9f\n", pos[0], pos[1], pos[2]);
						}
						for (uint32_t j=0; j<ch.m_nTriangles; j++)
						{
							uint32_t i1 = ch.m_triangles[j*3+0]+baseIndex;
							uint32_t i2 = ch.m_triangles[j*3+1]+baseIndex;
							uint32_t i3 = ch.m_triangles[j*3+2]+baseIndex;
							fprintf(fph,"f %d %d %d\n", i1, i2, i3);
						}
						baseIndex+=ch.m_nPoints;
					}
					fclose(fph);
				}
			}

			delete []points;
			iface->Release();


		}
	}
	return 0;
}
