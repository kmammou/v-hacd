/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _CRT_SECURE_NO_WARNINGS
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <VHACDHACD.h>

#include <VHACDMeshDecimator.h>
#include <VHACDMesh.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#ifdef WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif


void CallBack(const char * msg)
{
	std::cout << msg;
}
bool LoadOFF(const std::string & fileName, std::vector< VHACD::Vec3<VHACD::Real> > & points, std::vector< VHACD::Vec3<long> > & triangles, bool invert);
bool SaveOFF(const std::string & fileName, const std::vector< VHACD::Vec3<VHACD::Real> > & points, const std::vector< VHACD::Vec3<long> > & triangles);
bool SaveOFF(const std::string & fileName, size_t nV, size_t nT, const VHACD::Vec3<VHACD::Real> * const points, const VHACD::Vec3<long> * const triangles);

int main(int argc, char * argv[])
{
	/*
    VHACD::Mesh meshpoly;
    VHACD::Polyline2D poly;
    poly.InsertPoint(VHACD::Vec2< VHACD::Real > (0.0,0.0));
    poly.InsertPoint(VHACD::Vec2< VHACD::Real > (3.0,0.0));
    poly.InsertPoint(VHACD::Vec2< VHACD::Real > (3.0,3.0));
    poly.InsertPoint(VHACD::Vec2< VHACD::Real > (2.0,3.0));
    poly.InsertPoint(VHACD::Vec2< VHACD::Real > (2.0,2.0));
    poly.InsertPoint(VHACD::Vec2< VHACD::Real > (0.0,2.0));

    poly.InsertEdge(VHACD::Vec2< long > (0, 1));
    poly.InsertEdge(VHACD::Vec2< long > (1, 2));
    poly.InsertEdge(VHACD::Vec2< long > (2, 3));
    poly.InsertEdge(VHACD::Vec2< long > (3, 4));
    poly.InsertEdge(VHACD::Vec2< long > (4, 5));
    poly.InsertEdge(VHACD::Vec2< long > (5, 0));

    poly.ComputeBB();
    poly.Triangulate(meshpoly);
    meshpoly.SaveVRML2("C:\\work\\git\\v-hacd\\data\\test\\mesh.wrl");
    return 0;
*/
	// C:\work\git\v-hacd\data\test\block.off 30 0.01 0 16 32 8 64 0.01 2000

    if (argc != 11)
    { 
        std::cout << "Usage: ./testVHACD fileName.off depth maxConcavity invertInputFaces posSampling angleSampling posRefine angleRefine alpha targetNTrianglesDecimatedMesh"<< std::endl;
		std::cout << "Recommended parameters: ./testVHACD fileName.off 10 0.01 0 10 10 5 5 0.01 1000"<< std::endl;
        return -1;
    }

	const std::string fileName(argv[1]);
    int depth            = atoi(argv[2]);
    double concavity     = atof(argv[3]);
	bool invert          = (atoi(argv[4]) == 0)?false:true;
	int posSampling      = atoi(argv[5]);
    int angleSampling    = atoi(argv[6]);
	int posRefine        = atoi(argv[7]);
    int angleRefine      = atoi(argv[8]);
    double alpha         = atof(argv[9]);
	size_t targetNTrianglesDecimatedMesh = atoi(argv[10]);
    
    
	std::string folder;
	int found = fileName.find_last_of(PATH_SEP);
	if (found != -1)
	{
		folder = fileName.substr(0,found);
	}
	if (folder == "")
    {
        folder = ".";
    }

	std::string file(fileName.substr(found+1));
	std::string outOFFFileName = folder + PATH_SEP + file.substr(0, file.find_last_of(".")) + ".off";
    std::string outOFFFileNameDecimated = folder + PATH_SEP + file.substr(0, file.find_last_of(".")) + "_decimated.off";
	std::vector< VHACD::Vec3<VHACD::Real> > points;
	std::vector< VHACD::Vec3<long> > triangles;
    LoadOFF(fileName, points, triangles, invert);
	SaveOFF(outOFFFileName.c_str(), points, triangles);


	bool decimatedMeshComputed = false;
	size_t nTriangles = triangles.size();
	size_t nPoints    = points.size();
	VHACD::Vec3<VHACD::Real> * pPoints    = &points[0];
	VHACD::Vec3<long> *        pTriangles = &triangles[0];

	if (targetNTrianglesDecimatedMesh > 0 && targetNTrianglesDecimatedMesh < nTriangles)
	{
		decimatedMeshComputed = true;

		VHACD::MeshDecimator myMDecimator;
		myMDecimator.SetCallBack(&CallBack);
		myMDecimator.Initialize(nPoints, nTriangles, &points[0], &triangles[0]);
		myMDecimator.Decimate(0, targetNTrianglesDecimatedMesh);

		nTriangles = myMDecimator.GetNTriangles();
		nPoints    = myMDecimator.GetNVertices();
		pPoints     = new VHACD::Vec3<VHACD::Real>[nPoints];
		pTriangles  = new VHACD::Vec3<long>[nTriangles];
		myMDecimator.GetMeshData(pPoints, pTriangles);
	}

	VHACD::Mesh mesh;
	mesh.ResizePoints(nPoints);
	mesh.ResizeTriangles(nTriangles);
	for(size_t p = 0; p < nPoints;    ++p) mesh.SetPoint(p, pPoints[p]);
	for(size_t t = 0; t < nTriangles; ++t) mesh.SetTriangle(t, pTriangles[t]);
	if (decimatedMeshComputed)
	{
		delete [] pPoints;
		delete [] pTriangles;
	}

	mesh.SaveOFF(outOFFFileNameDecimated);

    std::vector< VHACD::Mesh * > parts;
	VHACD::ApproximateConvexDecomposition(mesh, depth, posSampling, angleSampling, posRefine, angleRefine, alpha, concavity, parts, &CallBack);

    const size_t nParts = parts.size();

	std::string outFileName = folder + PATH_SEP + file.substr(0, file.find_last_of(".")) + "_VHACD.wrl";
	std::string outFileNameCH = folder + PATH_SEP + file.substr(0, file.find_last_of(".")) + "_VHACD_CHs.wrl";
    std::ofstream fout(outFileName.c_str());
    std::ofstream foutCH(outFileNameCH.c_str());
	if (fout.is_open() && foutCH.is_open()) 
	{
		VHACD::Material mat;
        for(size_t p = 0; p < nParts; ++p)
        {
            mat.m_diffuseColor.X() = mat.m_diffuseColor.Y() = mat.m_diffuseColor.Z() = 0.0;
            while (mat.m_diffuseColor.X() == mat.m_diffuseColor.Y() ||
                   mat.m_diffuseColor.Z() == mat.m_diffuseColor.Y() ||
                   mat.m_diffuseColor.Z() == mat.m_diffuseColor.X()  )
            {
                mat.m_diffuseColor.X() = (rand()%100) / 100.0;
                mat.m_diffuseColor.Y() = (rand()%100) / 100.0;
                mat.m_diffuseColor.Z() = (rand()%100) / 100.0;
            }
            parts[p]->SaveVRML2(fout, mat);
            VHACD::Mesh ch;
            parts[p]->ComputeConvexHull(ch);
            ch.SaveVRML2(foutCH, mat);
        }	        
		fout.close();
        foutCH.close();
    }
    for(size_t p = 0; p < nParts; ++p)
    {
        delete parts[p];
    }
    
_CrtDumpMemoryLeaks();

    return 0;
}

bool SaveOFF(const std::string & fileName, const std::vector< VHACD::Vec3<VHACD::Real> > & points, const std::vector< VHACD::Vec3<long> > & triangles)
{
	return SaveOFF(fileName,points.size(), triangles.size(), &points[0], &triangles[0]);
}
bool SaveOFF(const std::string & fileName, size_t nV, size_t nT, const VHACD::Vec3<VHACD::Real> * const points, const VHACD::Vec3<long> * const triangles)
{
    std::cout << "Saving " <<  fileName << std::endl;
    std::ofstream fout(fileName.c_str());
    if (fout.is_open()) 
    {           
        fout <<"OFF" << std::endl;	    	
        fout << nV << " " << nT << " " << 0<< std::endl;		
        for(size_t v = 0; v < nV; v++)
        {
            fout << points[v].X() << " " 
                 << points[v].Y() << " " 
                 << points[v].Z() << std::endl;
		}
        for(size_t f = 0; f < nT; f++)
        {
            fout <<"3 " << triangles[f].X() << " " 
                        << triangles[f].Y() << " "                                                  
                        << triangles[f].Z() << std::endl;
        }
        fout.close();
	    return true;
    }
    return false;
}

bool LoadOFF(const std::string & fileName, std::vector< VHACD::Vec3<VHACD::Real> > & points, std::vector< VHACD::Vec3<long> > & triangles, bool invert) 
{    
	FILE * fid = fopen(fileName.c_str(), "r");
	if (fid) 
    {
		const std::string strOFF("OFF");
		char temp[1024];
		fscanf(fid, "%s", temp);
		if (std::string(temp) != strOFF)
		{
			printf( "Loading error: format not recognized \n");
            fclose(fid);

			return false;            
		}
		else
		{
			int nv = 0;
			int nf = 0;
			int ne = 0;
			fscanf(fid, "%i", &nv);
			fscanf(fid, "%i", &nf);
			fscanf(fid, "%i", &ne);
            points.resize(nv);
			triangles.resize(nf);
            VHACD::Vec3<VHACD::Real> coord;
			float x = 0;
			float y = 0;
			float z = 0;
			for (long p = 0; p < nv ; p++) 
            {
				fscanf(fid, "%f", &x);
				fscanf(fid, "%f", &y);
				fscanf(fid, "%f", &z);
				points[p].X() = x;
				points[p].Y() = y;
				points[p].Z() = z;
			}        
			int i = 0;
			int j = 0;
			int k = 0;
			int s = 0;
			for (long t = 0; t < nf ; ++t) {
				fscanf(fid, "%i", &s);
				if (s == 3)
				{
					fscanf(fid, "%i", &i);
					fscanf(fid, "%i", &j);
					fscanf(fid, "%i", &k);
					triangles[t].X() = i;
					if (invert)
					{
						triangles[t].Y() = k;
						triangles[t].Z() = j;
					}
					else
					{
						triangles[t].Y() = j;
						triangles[t].Z() = k;
					}
				}
				else			// Fix me: support only triangular meshes
				{
					for(long h = 0; h < s; ++h) fscanf(fid, "%i", &s);
				}
			}
            fclose(fid);
		}
	}
	else 
    {
		printf( "Loading error: file not found \n");
		return false;
    }
	return true;
}
