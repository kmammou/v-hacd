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
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <VHACDHACD.h>
#include <VHACDMesh.h>
#include <VHACDVolume.h>
#include <VHACDTimer.h>
#include <vector>

/*
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
*/

using namespace VHACD;
using namespace std;

void CallBack(const char * msg)
{
    cout << msg;
}
bool LoadOFF(const string         & fileName, 
             vector< Vec3<Real> > & points, 
             vector< Vec3<long> > & triangles);
bool LoadOBJ(const string         & fileName, 
             vector< Vec3<Real> > & points,
             vector< Vec3<long> > & triangles);
bool SaveConvexHulls(const string & fileName, 
                     const SArray< Mesh * > & convexHulls);
void GetFileExtension(const string & fileName, string & fileExtension);

int main(int argc, char * argv[])
{
    if (argc != 12)
    { 
        cout << "Usage: ./testVHACD fileName.off resolution maxNumVoxels maxDepth maxConcavity planeDownsampling convexhullDownsampling alpha beta mode outFileName.wrl"<< endl;
        cout << "Recommended parameters: ./testVHACD fileName.off 100000 10 0.01 1 8 0.05 0.05 0 0 VHACD_CHs.wrl"<< endl;
        return -1;
    }

    const string  fileName(argv[1]);                        // table.obj
    size_t        resolution              = atoi(argv[2]);  // 1000000 voxels
    int           depth                   = atoi(argv[3]);  // 20
    double        concavity               = atof(argv[4]);  // 0.0025
    int           planeDownsampling       = atoi(argv[5]);  // 4
    int           convexhullDownsampling  = atoi(argv[6]);  // 4
    double        alpha                   = atof(argv[7]);  // 0.05
    double        beta                    = atof(argv[8]);  // 0.05
    int           pca                     = atoi(argv[9]);  // 1
    int           mode                    = atoi(argv[10]); // 0: voxel-based (recommended), 1: tethedron-based
    const string  fileNameOut(argv[11]);                    // VHACD_CHs.wrl

    resolution             = (resolution   < 0)? 0 : resolution;
    planeDownsampling      = (planeDownsampling < 1 )? 1  : planeDownsampling;
    convexhullDownsampling = (convexhullDownsampling < 1 )? 1  : convexhullDownsampling;

    cout << "+ Parameters" << std::endl;
    cout << "\t input                      " << fileName               << std::endl;
    cout << "\t resolution                 " << resolution             << std::endl;
    cout << "\t max depth                  " << depth                  << std::endl;
    cout << "\t max concavity              " << concavity              << std::endl;
    cout << "\t plane downsampling         " << planeDownsampling      << std::endl;
    cout << "\t convexhull downsampling    " << convexhullDownsampling << std::endl;
    cout << "\t alpha                      " << alpha                  << std::endl;
    cout << "\t beta                       " << beta                   << std::endl;
    cout << "\t pca                        " << pca                    << std::endl;
    cout << "\t mode                       " << mode                   << std::endl;
    cout << "\t output                     " << fileNameOut            << std::endl;

    Timer timer;

    cout << "+ Load mesh " << endl;
    timer.Tic();
    vector< Vec3<Real> > points;
    vector< Vec3<long> > triangles;
    string fileExtension;
    GetFileExtension(fileName, fileExtension);
    if (fileExtension == ".OFF")
    {
        if (!LoadOFF(fileName, points, triangles))
        {
            return -1;
        }
    }
    else if (fileExtension == ".OBJ")
    {
        if (!LoadOBJ(fileName, points, triangles))
        {
            return -1;
        }
    }
    else
    {
        cout << "Format not supported!" << endl;
        return -1;
    }
    timer.Toc();
    cout << "\t time " << timer.GetElapsedTime() / 1000.0 << "s" << endl;


    size_t dim = (size_t)(pow((Real)resolution, 1.0/3.0) + 0.5);
    
    Real rot[3][3];
    Vec3<Real> barycenter(0.0);
    if (pca)
    {
        cout << "+ Align mesh" << endl;
        timer.Tic();

        cout << "\t dim = " << dim << "\t-> ";
        Volume * volume = new Volume;
        volume->Voxelize(points.size(), 
                         triangles.size(), 
                         &points[0], 
                         &triangles[0], 
                         dim);
        size_t n = volume->GetNumOnSurfVoxels() + volume->GetNumInsideSurfVoxels();
        cout << n << " voxels" << endl;
        volume->AlignToPrincipalAxes(rot);
        delete volume;

        size_t nPoints = points.size();
        for(size_t i = 0; i < nPoints; ++i)
        {
            barycenter += points[i];
        }
        barycenter /= (Real) nPoints;
        Real x, y, z;
        for(size_t i = 0; i < nPoints; ++i)
        {
            x            = points[i][0] - barycenter[0];
            y            = points[i][1] - barycenter[1];
            z            = points[i][2] - barycenter[2];
            points[i][0] = rot[0][0] * x + rot[1][0] * y + rot[2][0] * z;
            points[i][1] = rot[0][1] * x + rot[1][1] * y + rot[2][1] * z;
            points[i][2] = rot[0][2] * x + rot[1][2] * y + rot[2][2] * z;
        }
        timer.Toc();
        cout << "\t time " << timer.GetElapsedTime() / 1000.0 << "s" << endl;
    }
    cout << "+ Generate volume" << endl;
    timer.Tic();
    Volume * volume = 0;
    int iteration   = 0;
    while(iteration++ < 5)
    {
        cout << "\t [" << iteration << "] dim = " << dim << "\t-> ";
        volume = new Volume;
        volume->Voxelize(points.size(), 
                         triangles.size(), 
                         &points[0], 
                         &triangles[0], 
                         dim);
        size_t n = volume->GetNumOnSurfVoxels() + volume->GetNumInsideSurfVoxels();
        cout << n << " voxels" << endl;
         double a = pow((double)(resolution) / n, 0.33);
        size_t dim_next = (size_t)(dim * a + 0.5);
        if (n < resolution && iteration < 5 && volume->GetNumOnSurfVoxels() < resolution/8 && dim != dim_next)
        {
            delete volume;
            volume   = 0;
            dim      = dim_next;
        }
        else
        {
            break;
        }
    }
    timer.Toc();
    cout << "\t time " << timer.GetElapsedTime() / 1000.0 << "s" << endl;

    if (mode == 0)
    {
        cout << "+ Convert volume to vset " << endl;
        timer.Tic();
        VoxelSet * vset = new VoxelSet;
        volume->Convert(*vset);
        delete volume;
        volume = 0;
        timer.Toc();
        cout << "\t # voxels                   " << vset->GetNVoxels() << std::endl;
        cout << "\t # inside surface           " << vset->GetNumInsideSurfVoxels() << std::endl;
        cout << "\t # on surface               " << vset->GetNumOnSurfVoxels() << std::endl;
        cout << "\t time " << timer.GetElapsedTime() / 1000.0 << "s" << endl;

        cout << "+ Approximate convex decomposition " << endl;
        timer.Tic();
        SArray< VoxelSet * >       partsVoxelSet;
        ApproximateConvexDecomposition(vset, 
                                       depth, 
                                       planeDownsampling,
                                       convexhullDownsampling, 
                                       alpha, 
                                       beta, 
                                       concavity, 
                                       partsVoxelSet, 
                                       &CallBack);
        timer.Toc();
        cout << "\t time " << timer.GetElapsedTime() / 1000.0 << "s" << endl;

        const size_t nParts = partsVoxelSet.Size();
        cout << "+ Generate output: " << nParts << " convex-hulls " << endl;
        timer.Tic();
        ofstream foutCH(fileNameOut.c_str());
        if (foutCH.is_open()) 
        {
            Material mat;
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
                Mesh ch;
                partsVoxelSet[p]->ComputeConvexHull(ch, 1);
                if (pca)
                {
                    size_t nv = ch.GetNPoints();
                    Real x, y, z;
                    for(long i = 0; i < (long) nv; ++i)
                    {
                        Vec3<Real> & pt = ch.GetPoint(i);
                        x     = pt[0];
                        y     = pt[1];
                        z     = pt[2];
                        pt[0] = rot[0][0] * x + rot[0][1] * y + rot[0][2] * z + barycenter[0];
                        pt[1] = rot[1][0] * x + rot[1][1] * y + rot[1][2] * z + barycenter[1];
                        pt[2] = rot[2][0] * x + rot[2][1] * y + rot[2][2] * z + barycenter[2];
                    }
                }
                ch.SaveVRML2(foutCH, mat);
                cout << "\t CH["<< setfill('0') << setw(5) <<  p <<"] " << ch.GetNPoints() << " V, "<< ch.GetNTriangles() << " T" << endl;
            }
            foutCH.close();
        }
        for(size_t p = 0; p < nParts; ++p)
        {
            delete partsVoxelSet[p];
        }
        timer.Toc();
        cout << "\t time " << timer.GetElapsedTime() / 1000.0 << "s" << endl;
    }
    else
    {
        cout << "+ Convert volume to vset " << endl;
        timer.Tic();
        TetrahedronSet * tset = new TetrahedronSet;
        volume->Convert(*tset);
        delete volume;
        volume = 0;
        timer.Toc();
        cout << "\t # tetrahedra               " << tset->GetNTetrahedra() << std::endl;
        cout << "\t # inside surface           " << tset->GetNumInsideSurfTetrahedra() << std::endl;
        cout << "\t # on surface               " << tset->GetNumOnSurfTetrahedra() << std::endl;
        cout << "\t time " << timer.GetElapsedTime() / 1000.0 << "s" << endl;
        cout << "+ Approximate convex decomposition " << endl;
        timer.Tic();
        SArray< TetrahedronSet * > partsTetrahedronSet;
        ApproximateConvexDecomposition(tset, 
                                       depth, 
                                       planeDownsampling,
                                       convexhullDownsampling, 
                                       alpha, 
                                       beta, 
                                       concavity, 
                                       (pca!=0),
                                       partsTetrahedronSet, 
                                       &CallBack);
        timer.Toc();
        cout << "\t time " << timer.GetElapsedTime() / 1000.0 << "s" << endl;

        const size_t nParts = partsTetrahedronSet.Size();
        cout << "+ Generate output: " << nParts << " convex-hulls " << endl;
        timer.Tic();
        ofstream foutCH(fileNameOut.c_str());
        if (foutCH.is_open()) 
        {
            Material mat;
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
                Mesh ch;
                partsTetrahedronSet[p]->ComputeConvexHull(ch, 1);
                if (pca)
                {
                    size_t nv = ch.GetNPoints();
                    Real x, y, z;
                    for(long i = 0; i < (long) nv; ++i)
                    {
                        Vec3<Real> & pt = ch.GetPoint(i);
                        x     = pt[0];
                        y     = pt[1];
                        z     = pt[2];
                        pt[0] = rot[0][0] * x + rot[0][1] * y + rot[0][2] * z + barycenter[0];
                        pt[1] = rot[1][0] * x + rot[1][1] * y + rot[1][2] * z + barycenter[1];
                        pt[2] = rot[2][0] * x + rot[2][1] * y + rot[2][2] * z + barycenter[2];
                    }
                }
                ch.SaveVRML2(foutCH, mat);
                cout << "\t CH["<< setfill('0') << setw(5) <<  p <<"] " << ch.GetNPoints() << " V, "<< ch.GetNTriangles() << " T" << endl;
            }
            foutCH.close();
        }
        for(size_t p = 0; p < nParts; ++p)
        {
            delete partsTetrahedronSet[p];
        }
        timer.Toc();
        cout << "\t time " << timer.GetElapsedTime() / 1000.0 << "s" << endl;
    }

//    _CrtDumpMemoryLeaks();
    return 0;
}

bool SaveConvexHulls(const std::string & fileName, 
                     const SArray< Mesh * > & convexHulls)
{
    std::ofstream foutCH(fileName.c_str());
    if (foutCH.is_open())
    {
        for(size_t p = 0; p < convexHulls.Size(); ++p)
        {
            Material mat;
            mat.m_diffuseColor.X() = mat.m_diffuseColor.Y() = mat.m_diffuseColor.Z() = 0.0;
            while (mat.m_diffuseColor.X() == mat.m_diffuseColor.Y() ||
                    mat.m_diffuseColor.Z() == mat.m_diffuseColor.Y() ||
                    mat.m_diffuseColor.Z() == mat.m_diffuseColor.X()  )
            {
                mat.m_diffuseColor.X() = (rand()%100) / 100.0;
                mat.m_diffuseColor.Y() = (rand()%100) / 100.0;
                mat.m_diffuseColor.Z() = (rand()%100) / 100.0;
            }
            convexHulls[p]->SaveVRML2(foutCH, mat);
        }
        foutCH.close();
        return true;
    }
    else
    {
        return false;
    }
}
void GetFileExtension(const string & fileName, string & fileExtension) 
{
    size_t lastDotPosition = fileName.find_last_of(".");
    if (lastDotPosition == string::npos)
    {
        fileExtension = "";
    }
    else
    {
        fileExtension = fileName.substr(lastDotPosition, fileName.size());
        transform(fileExtension.begin(), fileExtension.end(),fileExtension.begin(), ::toupper);
    }
}


bool LoadOFF(const string & fileName, 
             vector< Vec3<Real> > & points, 
             vector< Vec3<long> > & triangles) 
{    
    FILE * fid = fopen(fileName.c_str(), "r");
    if (fid) 
    {
        const string strOFF("OFF");
        char temp[1024];
        fscanf(fid, "%s", temp);
        if (string(temp) != strOFF)
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
            Vec3<Real> coord;
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
                    triangles[t].Y() = j;
                    triangles[t].Z() = k;
                }
                else            // Fix me: support only triangular meshes
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

bool LoadOBJ(const string & fileName, 
             vector< Vec3<Real> > & points,
             vector< Vec3<long> > & triangles) 
{   
    const char ObjDelimiters[]=" /";
    const unsigned long BufferSize = 1024;
    FILE * fid = fopen(fileName.c_str(), "r");
    
    if (fid) 
    {        
        char buffer[BufferSize];
        Vec3<long> ip;
        Vec3<long> in;
        Vec3<long> it;
        char * pch;
        char * str;
        size_t nn = 0;
        size_t nt = 0;
        Vec3<Real> x;
        while (!feof(fid)) 
        {
            if (!fgets(buffer, BufferSize, fid))
            {
                break;
            }
            else if (buffer[0] == 'v')
            {
                if (buffer[1] == ' ')
                {
                    str = buffer+2;
                    for(int k = 0; k < 3; ++k)
                    {
                        pch = strtok (str, " ");
                        if (pch) x[k] = (Real) atof(pch);
                        else
                        {
                            return false;
                        }
                        str = NULL;
                    }
                    points.push_back(x);
                }
                else if (buffer[1] == 'n')
                {
                    ++nn;
                }
                else if (buffer[1] == 't')
                {
                    ++nt;
                }
            }
            else if (buffer[0] == 'f')
            {

                str = buffer+2;
                for(int k = 0; k < 3; ++k)
                {
                    pch = strtok (str, ObjDelimiters);
                    if (pch) ip[k] = atoi(pch) - 1;
                        else
                        {
                            return false;
                        }
                    str = NULL;
                    if (nt > 0)
                    {
                        pch = strtok (NULL, ObjDelimiters);
                        if (pch)  it[k] = atoi(pch) - 1;
                        else
                        {
                            return false;
                        }
                    }
                    if (nn > 0)
                    {
                        pch = strtok (NULL, ObjDelimiters);
                        if (pch)  in[k] = atoi(pch) - 1;
                        else
                        {
                            return false;
                        }
                    }
                }
                triangles.push_back(ip);
            }
        }
        fclose(fid);
    }
    else 
    {
        cout << "File not found" << endl;
        return false;
    }
    return true;
}

