/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <string>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <sstream>
#include <string.h>

//#define _CRTDBG_MAP_ALLOC

#ifdef _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif // _CRTDBG_MAP_ALLOC

#include "VHACD.h"

using namespace VHACD;
using namespace std;

class MyCallback : public IVHACD::IUserCallback 
{
    public:
                                    MyCallback(void) {}
                                    ~MyCallback() {};
    void                            Update(const double          overallProgress,
                                           const double          stageProgress,
                                           const double          operationProgress,
                                           const char * const    stage,
                                           const char * const    operation) 
                                    {
                                        cout << setfill(' ') << setw(3) << (int)(overallProgress  +0.5) << "% " 
                                             << "[ " << stage     << " " << setfill(' ') << setw(3) << (int)(stageProgress    +0.5) << "% ] " 
                                                     << operation << " " << setfill(' ') << setw(3) << (int)(operationProgress+0.5) << "%" << endl;
                                    };
};
class MyLogger : public IVHACD::IUserLogger 
{
    public:
                                    MyLogger(void){}
                                    MyLogger(const string & fileName){ OpenFile(fileName);  }
                                    ~MyLogger() {};
        void                        Log(const char * const msg)
                                    {
                                        if (m_file.is_open())
                                        {
                                            m_file << msg;
                                            m_file.flush();
                                        }
                                    };
        void                        OpenFile(const string & fileName) { m_file.open(fileName.c_str()); }
    private:
    ofstream                        m_file;
};
struct Material
{

    float                           m_diffuseColor[3];
    float                           m_ambientIntensity;
    float                           m_specularColor[3];
    float                           m_emissiveColor[3];
    float                           m_shininess;
    float                           m_transparency;
    Material(void)
    {
        m_diffuseColor[0]  = 0.5f;
        m_diffuseColor[1]  = 0.5f;
        m_diffuseColor[2]  = 0.5f;
        m_specularColor[0] = 0.5f;
        m_specularColor[1] = 0.5f;
        m_specularColor[2] = 0.5f;
        m_ambientIntensity = 0.4f;
        m_emissiveColor[0] = 0.0f;
        m_emissiveColor[1] = 0.0f;
        m_emissiveColor[2] = 0.0f;
        m_shininess        = 0.4f;
        m_transparency     = 0.5f;
    };
};
bool LoadOFF(const string              & fileName, 
                   vector< float >     & points, 
                   vector< int >       & triangles,
                   IVHACD::IUserLogger & logger);
bool LoadOBJ(const string              & fileName, 
                   vector< float >     & points,
                   vector< int >       & triangles,
                   IVHACD::IUserLogger & logger);
bool SaveOFF(const string          & fileName,
             const vector< float > & points,
             const vector< int >   & triangles,
             IVHACD::IUserLogger & logger);
bool SaveVRML2(      ofstream & fout,
               const double * const      & points,
               const int * const         & triangles,
               const unsigned int        & nPoints,
               const unsigned int        & nTriangles,
               const Material            & material,
                     IVHACD::IUserLogger & logger);
void GetFileExtension(const string & fileName, string & fileExtension);
void ComputeRandomColor(Material & mat);
int main(int argc, char * argv[])
{
    {
        if (argc != 15)
        {
            
            cout << "Usage: ./testVHACD fileName.off resolution maxNumVoxels maxDepth maxConcavity planeDownsampling convexhullDownsampling alpha beta gamma pca mode maxCHVertices outFileName.wrl log.txt" << endl;
            cout << "Recommended parameters: ./testVHACD fileName.off 1000000 20 0.001 4 4 0.05 0.05 0.0001 0 0 256 VHACD_CHs.wrl log.txt" << endl;
            return -1;
        }
        // set parameters
        IVHACD::Parameters    params;
        const string  fileName   (argv[1 ]);               // table.obj
        const string  fileNameOut(argv[13]);               // VHACD_CHs.wrl
        const string  fileNameLog(argv[14]);               // VHACD_CHs.wrl
        params.m_resolution = atoi(argv[2]);  // 1000000 voxels
        params.m_depth                  = atoi(argv[3]);  // 20
        params.m_concavity              = atof(argv[4]);  // 0.0025
        params.m_planeDownsampling      = atoi(argv[5]);  // 4
        params.m_convexhullDownsampling = atoi(argv[6]);  // 4
        params.m_alpha                  = atof(argv[7]);  // 0.05
        params.m_beta                   = atof(argv[8]);  // 0.05
        params.m_gamma                  = atof(argv[9]);  // 0.0001
        params.m_pca                    = atoi(argv[10]); // 0
        params.m_mode                   = atoi(argv[11]); // 0: voxel-based (recommended), 1: tethedron-based
        params.m_maxNumVerticesPerCH    = atoi(argv[12]); // 100
        params.m_resolution             = (params.m_resolution             < 64) ? 0 : params.m_resolution;
        params.m_planeDownsampling      = (params.m_planeDownsampling      <  1) ? 1 : params.m_planeDownsampling;
        params.m_convexhullDownsampling = (params.m_convexhullDownsampling <  1) ? 1 : params.m_convexhullDownsampling;

        MyCallback myCallback;
        MyLogger   myLogger(fileNameLog);
        params.m_logger = &myLogger;
        params.m_callback = &myCallback;

        std::ostringstream msg;
#ifdef _OPENMP
        msg << "+ OpenMP (ON)" << std::endl;
#else
        msg << "+ OpenMP (OFF)" << std::endl;
#endif
        msg << "+ Parameters" << std::endl;
        msg << "\t input                                   " << fileName                        << std::endl;
        msg << "\t resolution                              " << params.m_resolution             << std::endl;
        msg << "\t max. depth                              " << params.m_depth                  << std::endl;
        msg << "\t max. concavity                          " << params.m_concavity              << std::endl;
        msg << "\t plane down-sampling                     " << params.m_planeDownsampling      << std::endl;
        msg << "\t convex-hull down-sampling               " << params.m_convexhullDownsampling << std::endl;
        msg << "\t alpha                                   " << params.m_alpha                  << std::endl;
        msg << "\t beta                                    " << params.m_beta                   << std::endl;
        msg << "\t gamma                                   " << params.m_gamma                  << std::endl;
        msg << "\t pca                                     " << params.m_pca                    << std::endl;
        msg << "\t mode                                    " << params.m_mode                   << std::endl;
        msg << "\t max. vertices per convex-hull           " << params.m_maxNumVerticesPerCH << std::endl;
        msg << "\t output                                  " << fileNameOut                     << std::endl;
        msg << "+ Load mesh \n" << std::endl;
        myLogger.Log(msg.str().c_str());

        // load mesh
        vector<float > points;
        vector<int > triangles;
        string fileExtension;
        GetFileExtension(fileName, fileExtension);
        if (fileExtension == ".OFF")
        {
            if (!LoadOFF(fileName, points, triangles, myLogger))
            {
                return -1;
            }
        }
        else if (fileExtension == ".OBJ")
        {
            if (!LoadOBJ(fileName, points, triangles, myLogger))
            {
                return -1;
            }
        }
        else
        {
            myLogger.Log("Format not supported!\n");
            return -1;
        }

        // run V-HACD
        IVHACD * interfaceVHACD = CreateVHACD();
        bool res = interfaceVHACD->Compute(&points[0]   , 3, (unsigned int)points.size()    / 3,
                                           &triangles[0], 3, (unsigned int)triangles.size() / 3, params);
        if (res)
        {
            // save output
            unsigned int nConvexHulls = interfaceVHACD->GetNConvexHulls();
            msg.str("");
            msg << "+ Generate output: " << nConvexHulls << " convex-hulls " << endl;
            myLogger.Log(msg.str().c_str());
            ofstream foutCH(fileNameOut.c_str());
            IVHACD::ConvexHull ch;
            if (foutCH.is_open())
            {
                Material mat;
                for (unsigned int p = 0; p < nConvexHulls; ++p)
                {
                    interfaceVHACD->GetConvexHull(p, ch);
                    ComputeRandomColor(mat);
                    SaveVRML2(foutCH, ch.m_points, ch.m_triangles, ch.m_nPoints, ch.m_nTriangles, mat, myLogger);
                    msg.str("");
                    msg << "\t CH[" << setfill('0') << setw(5) << p << "] " << ch.m_nPoints << " V, " << ch.m_nTriangles << " T" << endl;
                    myLogger.Log(msg.str().c_str());
                }
                foutCH.close();
            }
        }
        else
        {
            myLogger.Log("Decomposition cancelled by user!\n");
        }

        interfaceVHACD->Clean();
        interfaceVHACD->Release();
    }
#ifdef _CRTDBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
#endif // _CRTDBG_MAP_ALLOC
    return 0;
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
        transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::toupper);
    }
}
void ComputeRandomColor(Material & mat)
{
    mat.m_diffuseColor[0] = mat.m_diffuseColor[1] = mat.m_diffuseColor[2] = 0.0f;
    while (mat.m_diffuseColor[0] == mat.m_diffuseColor[1] ||
        mat.m_diffuseColor[2] == mat.m_diffuseColor[1] ||
        mat.m_diffuseColor[2] == mat.m_diffuseColor[0])
    {
        mat.m_diffuseColor[0] = (rand() % 100) / 100.0f;
        mat.m_diffuseColor[1] = (rand() % 100) / 100.0f;
        mat.m_diffuseColor[2] = (rand() % 100) / 100.0f;
    }
}
bool LoadOFF(const string              & fileName,
                   vector< float >     & points,
                   vector< int>        & triangles,
                   IVHACD::IUserLogger & logger)
{
    FILE * fid = fopen(fileName.c_str(), "r");
    if (fid)
    {
        const string strOFF("OFF");
        char temp[1024];
        fscanf(fid, "%s", temp);
        if (string(temp) != strOFF)
        {
            logger.Log("Loading error: format not recognized \n");
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
            points.resize(nv*3);
            triangles.resize(nf*3);
            const int np = nv * 3;
            for (int p = 0; p < np; p++)
            {
                fscanf(fid, "%f", &(points[p]));
            }
            int s;
            for (int t = 0, r = 0; t < nf; ++t) {
                fscanf(fid, "%i", &s);
                if (s == 3)
                {
                    fscanf(fid, "%i", &(triangles[r++]));
                    fscanf(fid, "%i", &(triangles[r++]));
                    fscanf(fid, "%i", &(triangles[r++]));
                }
                else            // Fix me: support only triangular meshes
                {
                    for (int h = 0; h < s; ++h) fscanf(fid, "%i", &s);
                }
            }
            fclose(fid);
        }
    }
    else
    {
        logger.Log("Loading error: file not found \n");
        return false;
    }
    return true;
}
bool LoadOBJ(const string              & fileName,
                   vector< float >     & points,
                   vector< int >       & triangles,
                   IVHACD::IUserLogger & logger)
{
    const char ObjDelimiters[] = " /";
    const unsigned int BufferSize = 1024;
    FILE * fid = fopen(fileName.c_str(), "r");

    if (fid)
    {
        char buffer[BufferSize];
        int  ip[3];
        int  in[3];
        int  it[3];
        float x[3];
        char * pch;
        char * str;
        size_t nn = 0;
        size_t nt = 0;
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
                    str = buffer + 2;
                    for (int k = 0; k < 3; ++k)
                    {
                        pch = strtok(str, " ");
                        if (pch) x[k] = (float)atof(pch);
                        else
                        {
                            return false;
                        }
                        str = NULL;
                    }
                    points.push_back(x[0]);
                    points.push_back(x[1]);
                    points.push_back(x[2]);
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

                str = buffer + 2;
                for (int k = 0; k < 3; ++k)
                {
                    pch = strtok(str, ObjDelimiters);
                    if (pch) ip[k] = atoi(pch) - 1;
                    else
                    {
                        return false;
                    }
                    str = NULL;
                    if (nt > 0)
                    {
                        pch = strtok(NULL, ObjDelimiters);
                        if (pch)  it[k] = atoi(pch) - 1;
                        else
                        {
                            return false;
                        }
                    }
                    if (nn > 0)
                    {
                        pch = strtok(NULL, ObjDelimiters);
                        if (pch)  in[k] = atoi(pch) - 1;
                        else
                        {
                            return false;
                        }
                    }
                }
                triangles.push_back(ip[0]);
                triangles.push_back(ip[1]);
                triangles.push_back(ip[2]);
            }
        }
        fclose(fid);
    }
    else
    {
        logger.Log("File not found\n");
        return false;
    }
    return true;
}
bool SaveOFF(const string              & fileName,
             const float * const       & points,
             const int * const         & triangles,
             const unsigned int        & nPoints,
             const unsigned int        & nTriangles,
                   IVHACD::IUserLogger & logger) 
{
    ofstream fout(fileName.c_str());
    if (fout.is_open())
    {
        size_t nV = nPoints * 3;
        size_t nT = nTriangles * 3;
        fout << "OFF" << std::endl;
        fout << nPoints << " " << nTriangles << " " << 0 << std::endl;
        for (size_t v = 0; v < nV; v+=3)
        {
            fout << points[v+0] << " "
                 << points[v+1] << " "
                 << points[v+2] << std::endl;
        }
        for (size_t f = 0; f < nT; f+=3)
        {
            fout << "3 " << triangles[f+0] << " "
                         << triangles[f+1] << " "
                         << triangles[f+2] << std::endl;
        }
        fout.close();
        return true;
    }
    else
    {
        logger.Log("Can't open file\n");
        return false;
    }
}
bool SaveVRML2(      ofstream            & fout,
               const double * const& points,
               const int * const   & triangles,
               const unsigned int  & nPoints,
               const unsigned int  & nTriangles,
               const Material      & material,
               IVHACD::IUserLogger & logger)
{
    if (fout.is_open())
    {
        fout.setf(std::ios::fixed, std::ios::floatfield);
        fout.setf(std::ios::showpoint);
        fout.precision(6);
        size_t nV = nPoints*3;
        size_t nT = nTriangles*3;
        fout << "#VRML V2.0 utf8" << std::endl;
        fout << "" << std::endl;
        fout << "# Vertices: " << nPoints << std::endl;
        fout << "# Triangles: " << nTriangles << std::endl;
        fout << "" << std::endl;
        fout << "Group {" << std::endl;
        fout << "    children [" << std::endl;
        fout << "        Shape {" << std::endl;
        fout << "            appearance Appearance {" << std::endl;
        fout << "                material Material {" << std::endl;
        fout << "                    diffuseColor " << material.m_diffuseColor[0] << " "
                                                    << material.m_diffuseColor[1] << " "
                                                    << material.m_diffuseColor[2] << std::endl;
        fout << "                    ambientIntensity " << material.m_ambientIntensity << std::endl;
        fout << "                    specularColor " << material.m_specularColor[0] << " "
                                                     << material.m_specularColor[1] << " "
                                                     << material.m_specularColor[2] << std::endl;
        fout << "                    emissiveColor " << material.m_emissiveColor[0] << " "
                                                     << material.m_emissiveColor[1] << " "
                                                     << material.m_emissiveColor[2] << std::endl;
        fout << "                    shininess " << material.m_shininess << std::endl;
        fout << "                    transparency " << material.m_transparency << std::endl;
        fout << "                }" << std::endl;
        fout << "            }" << std::endl;
        fout << "            geometry IndexedFaceSet {" << std::endl;
        fout << "                ccw TRUE" << std::endl;
        fout << "                solid TRUE" << std::endl;
        fout << "                convex TRUE" << std::endl;
        if (nV > 0)
        {
            fout << "                coord DEF co Coordinate {" << std::endl;
            fout << "                    point [" << std::endl;
            for (size_t v = 0; v < nV; v+=3)
            {
                fout << "                        " << points[v+0] << " "
                                                   << points[v+1] << " "
                                                   << points[v+2] << "," << std::endl;
            }
            fout << "                    ]" << std::endl;
            fout << "                }" << std::endl;
        }
        if (nT > 0)
        {
            fout << "                coordIndex [ " << std::endl;
            for (size_t f = 0; f < nT; f+=3)
            {
                fout << "                        " << triangles[f+0] << ", "
                                                   << triangles[f+1] << ", "
                                                   << triangles[f+2] << ", -1," << std::endl;
            }
            fout << "                ]" << std::endl;
        }
        fout << "            }" << std::endl;
        fout << "        }" << std::endl;
        fout << "    ]" << std::endl;
        fout << "}" << std::endl;
        return true;
    }
    else
    {
        logger.Log("Can't open file\n");
        return false;
    }
}
