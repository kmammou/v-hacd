#ifndef __PPCGEKKO__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <vector>

#include "wavefront.h"
#include "FloatMath.h"
#include "InParser.h"

/*!
**
** Copyright (c) 2014 by John W. Ratcliff mailto:jratcliffscarab@gmail.com
**
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


**
** If you find this code snippet useful; you can tip me at this bitcoin address:
**
** BITCOIN TIP JAR: "1BT66EoaGySkbY9J6MugvQRhMMXDwPxPya"
**



*/

#ifdef _WIN32
#	define strcasecmp _stricmp
#endif

#pragma warning(disable:4996)

typedef std::vector< uint32_t > IntVector;
typedef std::vector< float > FloatVector;

namespace WAVEFRONT
{

/*******************************************************************/
/******************** Obj.h  ********************************/
/*******************************************************************/


class OBJ : public IN_PARSER::InPlaceParserInterface
{
public:
	uint32_t	LoadMesh(const char *fname);
	uint32_t	LoadMesh(const uint8_t *data,uint32_t dlen);

	uint32_t	LoadOFF(const char *fname);
	uint32_t	LoadOFF(const uint8_t *data, uint32_t dlen);
	uint32_t	LoadSTL(const uint8_t *data, uint32_t dlen);
	uint32_t	LoadDAE(const uint8_t *data, uint32_t dlen);

	uint32_t	ParseLine(uint32_t lineno,uint32_t argc,const char **argv);  // return TRUE to continue parsing, return FALSE to abort parsing process
	IntVector		mTriIndices;
	FloatVector		mVerts;
	bool			mIsValidOFF{ false };
	uint32_t		mVertexCountOFF{ 0 };
	uint32_t		mFaceCountOFF{ 0 };
	uint32_t		mEdgeCountOFF{ 0 };
};


/*******************************************************************/
/******************** Obj.cpp  ********************************/
/*******************************************************************/

uint32_t OBJ::LoadMesh(const char *fname)
{
	uint32_t ret = 0;

	mVerts.clear();
	mTriIndices.clear();

	IN_PARSER::InPlaceParser ipp(fname);

	ipp.Parse(this);


	return ret;
}

uint32_t OBJ::LoadMesh(const uint8_t *data,uint32_t dlen)
{
	uint32_t ret = 0;

	mVerts.clear();
	mTriIndices.clear();

	uint8_t *tdata = new uint8_t[dlen+1];
	tdata[dlen] = 0;
	memcpy(tdata,data,dlen);
	IN_PARSER::InPlaceParser ipp((char *)tdata,dlen);
	ipp.Parse(this);
	delete []tdata;


	return ret;
}


uint32_t OBJ::ParseLine(uint32_t /*lineno*/,uint32_t argc,const char **argv)  // return TRUE to continue parsing, return FALSE to abort parsing process
{
    uint32_t ret = 0;

  if ( argc >= 1 )
  {
	const char *foo = argv[0];
	if ( *foo != '#' )
	{
	  if ( strcasecmp(argv[0],"v") == 0 && argc == 4 )
	  {
		float vx = (float) atof( argv[1] );
		float vy = (float) atof( argv[2] );
		float vz = (float) atof( argv[3] );
		mVerts.push_back(vx);
		mVerts.push_back(vy);
		mVerts.push_back(vz);
	  }
	  else if ( strcasecmp(argv[0],"f") == 0 && argc >= 4 )
	  {
		uint32_t vcount = argc-1;

		uint32_t i1 = (uint32_t)atoi(argv[1])-1;
		uint32_t i2 = (uint32_t)atoi(argv[2])-1;
		uint32_t i3 = (uint32_t)atoi(argv[3])-1;

		mTriIndices.push_back(i3);
		mTriIndices.push_back(i2);
		mTriIndices.push_back(i1);


		if ( vcount >=3 ) // do the fan
		{
		  for (uint32_t i=2; i<(vcount-1); i++)
		  {
			  i2 = i3;
			  i3 = (uint32_t)atoi(argv[i+2])-1;
			  mTriIndices.push_back(i3);
			  mTriIndices.push_back(i2);
			  mTriIndices.push_back(i1);
		  }
		}
	  }
	}
  }

  return ret;
}





};

using namespace WAVEFRONT;

WavefrontObj::WavefrontObj(void)
{
	mVertexCount = 0;
	mTriCount    = 0;
	mIndices     = 0;
	mVertices    = NULL;
}

WavefrontObj::~WavefrontObj(void)
{
	releaseMesh();
	delete mIndices;
	delete mVertices;
}

uint32_t WavefrontObj::loadObj(const uint8_t *data,uint32_t dlen)
{
	uint32_t ret = 0;


	OBJ obj;

	obj.LoadMesh(data,dlen);

	mVertexCount = (uint32_t)obj.mVerts.size()/3;
	mTriCount = (uint32_t)obj.mTriIndices.size()/3;

	if ( mVertexCount )
	{
		mVertices = new float[mVertexCount*3];
		memcpy(mVertices, &obj.mVerts[0], sizeof(float)*mVertexCount*3);
	}

	if ( mTriCount )
	{
		mIndices = new uint32_t[mTriCount*3];
		memcpy(mIndices,&obj.mTriIndices[0],mTriCount*3*sizeof(uint32_t));
	}
	ret = mTriCount;

	return ret;
}

void WavefrontObj::releaseMesh(void)
{
	delete []mVertices;
	mVertices = 0;
	delete []mIndices;
	mIndices = 0;
	mVertexCount = 0;
	mTriCount = 0;
}

uint32_t WavefrontObj::loadObj(const char *fname) // load a wavefront obj returns number of triangles that were loaded.  Data is persists until the class is destructed.
{
	uint32_t ret = 0;


	OBJ obj;

	obj.LoadMesh(fname);

	mVertexCount = (uint32_t)obj.mVerts.size()/3;
	mTriCount = (uint32_t)obj.mTriIndices.size()/3;

	if ( mVertexCount )
	{
		mVertices = new float[mVertexCount*3];
		memcpy(mVertices, &obj.mVerts[0], sizeof(float)*mVertexCount*3);
	}

	if ( mTriCount )
	{
		mIndices = new uint32_t[mTriCount*3];
		memcpy(mIndices,&obj.mTriIndices[0],mTriCount*3*sizeof(uint32_t));
	}
	ret = mTriCount;

	return ret;
}

bool WavefrontObj::saveObj(const char *fname)
{
	return saveObj(fname, mVertexCount, mVertices, mTriCount, mIndices);
}

bool WavefrontObj::saveObj(const char *fname,uint32_t vcount,const float *vertices,uint32_t tcount,const uint32_t *indices)
{
	bool ret = false;

	FILE *fph = fopen(fname,"wb");
	if ( fph )
	{
		for (uint32_t i=0; i<vcount; i++)
		{
			fprintf(fph,"v %0.9f %0.9f %0.9f\r\n", vertices[0], vertices[1], vertices[2] );
			vertices+=3;
		}
		for (uint32_t i=0; i<tcount; i++)
		{
			fprintf(fph,"f %d %d %d\r\n", indices[2]+1, indices[1]+1, indices[0]+1 );
			indices+=3;
		}
		fclose(fph);
		ret = true;
	}
	return ret;
}

// save the mesh as C++ code; just vertices and indices; really simple
void WavefrontObj::saveCPP(const char *fname)
{
	FILE *fph = fopen(fname, "wb");
	if (fph == nullptr) return;

	fprintf(fph, "// Mesh: %d vertices %d triangles\r\n", mVertexCount, mTriCount);
	fprintf(fph, "#define VERTEX_COUNT %d\r\n", mVertexCount);
	fprintf(fph, "#define TRIANGLE_COUNT %d\r\n", mTriCount);

	fprintf(fph, "float gVertices[VERTEX_COUNT*3]={\r\n");
	for (uint32_t i = 0; i < mVertexCount; i++)
	{
		const float *v = &mVertices[i * 3];
		fprintf(fph, "    %0.9f,%0.9f,%0.9f,\r\n", v[0], v[1], v[2]);
	}
	fprintf(fph,"};\r\n");

	fprintf(fph, "uint32_t gIndices[TRIANGLE_COUNT*3]={\r\n");
	for (uint32_t i = 0; i < mTriCount; i++)
	{
		const uint32_t *v = &mIndices[i * 3];
		fprintf(fph, "    %d,%d,%d,\r\n", v[2], v[1], v[0]);
	}
	fprintf(fph, "};\r\n");


	fclose(fph);
}

void WavefrontObj::deepCopyScale(WavefrontObj &dest,
								float scaleFactor,
								bool centerMesh,
								uint32_t tessellateInputMesh)
{
	dest.releaseMesh();
	dest.mVertexCount = mVertexCount;
	dest.mTriCount = mTriCount;
	if (mTriCount)
	{
		dest.mIndices = new uint32_t[mTriCount * 3];
		memcpy(dest.mIndices, mIndices, sizeof(uint32_t)*mTriCount * 3);
	}
	if (mVertexCount)
	{
		float adjustX = 0;
		float adjustY = 0;
		float adjustZ = 0;

		if (centerMesh)
		{
			float bmin[3];
			float bmax[3];

			bmax[0] = bmin[0] = mVertices[0];
			bmax[1] = bmin[1] = mVertices[1];
			bmax[2] = bmin[2] = mVertices[2];

			for (uint32_t i = 1; i < mVertexCount; i++)
			{
				const float *p = &mVertices[i * 3];
				if (p[0] < bmin[0])
				{
					bmin[0] = p[0];
				}
				if (p[1] < bmin[1])
				{
					bmin[1] = p[1];
				}
				if (p[2] < bmin[2])
				{
					bmin[2] = p[2];
				}
				if (p[0] > bmax[0])
				{
					bmax[0] = p[0];
				}
				if (p[1] > bmax[1])
				{
					bmax[1] = p[1];
				}
				if (p[2] > bmax[2])
				{
					bmax[2] = p[2];
				}
			}
			adjustX = (bmin[0] + bmax[0])*0.5f;
			adjustY = bmin[1]; // (bmin[1] + bmax[1])*0.5f;
			adjustZ = (bmin[2] + bmax[2])*0.5f;
		}
		dest.mVertices = new float[mVertexCount * 3];
		for (uint32_t i = 0; i < mVertexCount; i++)
		{
			dest.mVertices[i * 3 + 0] = (mVertices[i * 3 + 0]-adjustX) * scaleFactor;
			dest.mVertices[i * 3 + 1] = (mVertices[i * 3 + 1]-adjustY) * scaleFactor;
			dest.mVertices[i * 3 + 2] = (mVertices[i * 3 + 2]-adjustZ) * scaleFactor;
		}
		// if we are going to tessellate the input mesh..

		if (tessellateInputMesh > 1)
		{
			// Compute the diagonal length of the input mesh
			float bmin[3];
			float bmax[3];
			FLOAT_MATH::fm_initMinMax(bmin, bmax);
			for (uint32_t i = 0; i < dest.mVertexCount; i++)
			{
				const float *p = &dest.mVertices[i * 3];
				FLOAT_MATH::fm_minmax(p, bmin, bmax);
			}
			float diagonalLength = FLOAT_MATH::fm_distance(bmin, bmax);
			// Populate the vertex index system with vertices and save the indices
			FLOAT_MATH::fm_VertexIndex *vindex = FLOAT_MATH::fm_createVertexIndex(0.0f, false);
			std::vector< uint32_t > indices;
			for (uint32_t i = 0; i < dest.mTriCount; i++)
			{
				uint32_t i1 = dest.mIndices[i * 3 + 0];
				uint32_t i2 = dest.mIndices[i * 3 + 1];
				uint32_t i3 = dest.mIndices[i * 3 + 2];

				const float *p1 = &dest.mVertices[i1 * 3];
				const float *p2 = &dest.mVertices[i2 * 3];
				const float *p3 = &dest.mVertices[i3 * 3];

				bool newPos;
				i1 = vindex->getIndex(p1, newPos);
				i2 = vindex->getIndex(p2, newPos);
				i3 = vindex->getIndex(p3, newPos);
				indices.push_back(i1);
				indices.push_back(i2);
				indices.push_back(i3);
			}
			// The tessellation distance is the diagonal length divided by the tesselation factor
			float tessellationDistance = diagonalLength / float(tessellateInputMesh);

			FLOAT_MATH::fm_Tesselate *tess = FLOAT_MATH::fm_createTesselate();
			uint32_t outcount;
			const uint32_t *newIndices = tess->tesselate(vindex, dest.mTriCount, &indices[0], tessellationDistance, 8, outcount);

			delete[]dest.mIndices;
			delete[]dest.mVertices;

			dest.mIndices = new uint32_t[outcount * 3];
			dest.mTriCount = outcount; // new number of triangles..
			memcpy(dest.mIndices, newIndices, outcount * 3 * sizeof(uint32_t));

			dest.mVertexCount = vindex->getVcount();
			dest.mVertices = new float[dest.mVertexCount * 3];
			memcpy(dest.mVertices, vindex->getVerticesFloat(), sizeof(float)*dest.mVertexCount * 3);

			FLOAT_MATH::fm_releaseVertexIndex(vindex);
			FLOAT_MATH::fm_releaseTesselate(tess);
		}
	}
}

#endif
