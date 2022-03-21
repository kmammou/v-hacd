#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "VHACD.h"
#include "wavefront.h"
#include "ScopedTime.h"

#include <thread>


#ifdef _MSC_VER
#pragma warning(disable:4100 4996)
#include <conio.h>
#endif

int main(int argc,const char **argv)
{
	if ( argc < 2 )
	{
		printf("Usage: TestVHACD <wavefront.obj> (options)\n");
		printf("\n");
		printf("-h <n>                  : Maximum number of output convex hulls. Default is 32\n");
		printf("-r <voxelresolution>    : Total number of voxels to use. Default is 100,000\n");
		printf("-e <volumeErrorPercent> : Volume error allowed as a percentage. Default is 8%%\n");
		printf("-d <maxRecursionDepth>  : Maximum recursion depth. Default value is 12.\n");
		printf("-s <true/false>         : Whether or not to shrinkwrap output to source mesh. Default is true.\n");
		printf("-f <fillMode>           : Fill mode. Default is 'flood', also 'surface' and 'raycast' are valid.\n");
		printf("-v <maxHullVertCount>   : Maximum number of vertices in the output convex hull. Default value is 64\n");
		printf("-a <true/false>         : Whether or not to run asynchronously. Default is 'true'\n");
		printf("-l <minEdgeLength>      : Minimum size of a voxel edge. Default value is 4 voxels.\n");
		printf("-q <quantizeCount>      : Maximum number of voxel points to consider when generating a convex hull. Default is 100000\n");
	}
	else
	{
		VHACD::IVHACD::Parameters p;
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
				}
				else if ( strcmp(option,"-s") == 0 )
				{
					if ( strcmp(value,"true") == 0 )
					{
						p.m_shrinkWrap = true;
						printf("Shrinkwrap enabled.\n");
					}
					if ( strcmp(value,"false") == 0 )
					{
						p.m_shrinkWrap = false;
						printf("Shrinkwrap disabled.\n");
					}
					else
					{
						printf("Valid values are 'true' or 'false' only.\n");
					}
				}
				else if ( strcmp(option,"-f") == 0 )
				{
				}
				else if ( strcmp(option,"-v") == 0 )
				{
				}
				else if ( strcmp(option,"-a") == 0 )
				{
				}
				else if ( strcmp(option,"-l") == 0 )
				{
				}
				else if ( strcmp(option,"-q") == 0 )
				{
				}
				else
				{
					printf("Unknown option (%s)\n", option );
				}
			}

			VHACD::IVHACD *iface = VHACD::CreateVHACD_ASYNC();

#ifdef _MSC_VER
			printf("Press any key to cancel convex decomposition before it has completed.\n");
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
						printf("Canceling convex decomposition.\n");
						iface->Cancel();
						getch();
						canceled = true;
					}
#endif
				}
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
