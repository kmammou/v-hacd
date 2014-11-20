/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <iosfwd>
#include <btConvexHullComputer.h>
#include <vhacdMesh.h>

namespace VHACD
{
    Mesh::Mesh()
    {
        m_diag = 1.0;
    }
    Mesh::~Mesh()
    {
    }
    bool Mesh::SaveVRML2(const std::string & fileName, const Vec3<Real> * colors) const
    {
        std::cout << "Saving " <<  fileName << std::endl;
        std::ofstream fout(fileName.c_str());
        if (fout.is_open()) 
        {
            const Material material;
            
            if (SaveVRML2(fout, material, colors))
            {
                fout.close();
                return true;
            }
            return false;
        }
        return false;
    }

    bool Mesh::SaveVRML2(std::ofstream & fout, const Material & material, const Vec3<Real> * colors) const
    {        
        if (fout.is_open()) 
        {
            fout.setf(std::ios::fixed, std::ios::floatfield);
            fout.setf(std::ios::showpoint);
            fout.precision(6);
            size_t nV = m_points.Size();
            size_t nT = m_triangles.Size();
            fout <<"#VRML V2.0 utf8" << std::endl;
            fout <<"" << std::endl;
            fout <<"# Vertices: " << nV << std::endl;        
            fout <<"# Triangles: " << nT << std::endl;        
            fout <<"" << std::endl;
            fout <<"Group {" << std::endl;
            fout <<"    children [" << std::endl;
            fout <<"        Shape {" << std::endl;
            fout <<"            appearance Appearance {" << std::endl;
            fout <<"                material Material {" << std::endl;
            fout <<"                    diffuseColor "      << material.m_diffuseColor[0]      << " " 
                                                            << material.m_diffuseColor[1]      << " "
                                                            << material.m_diffuseColor[2]      << std::endl;  
            fout <<"                    ambientIntensity "  << material.m_ambientIntensity      << std::endl;
            fout <<"                    specularColor "     << material.m_specularColor[0]     << " " 
                                                            << material.m_specularColor[1]     << " "
                                                            << material.m_specularColor[2]     << std::endl; 
            fout <<"                    emissiveColor "     << material.m_emissiveColor[0]     << " " 
                                                            << material.m_emissiveColor[1]     << " "
                                                            << material.m_emissiveColor[2]     << std::endl; 
            fout <<"                    shininess "         << material.m_shininess             << std::endl;
            fout <<"                    transparency "      << material.m_transparency          << std::endl;
            fout <<"                }" << std::endl;
            fout <<"            }" << std::endl;
            fout <<"            geometry IndexedFaceSet {" << std::endl;
            fout <<"                ccw TRUE" << std::endl;
            fout <<"                solid TRUE" << std::endl;
            fout <<"                convex TRUE" << std::endl;
            if (colors && nT>0)
            {
                fout <<"                colorPerVertex FALSE" << std::endl;
                fout <<"                color Color {" << std::endl;
                fout <<"                    color [" << std::endl;
                for(size_t c = 0; c < nT; c++)
                {
                    fout <<"                        " << colors[c][0] << " " 
                                                      << colors[c][1] << " " 
                                                      << colors[c][2] << "," << std::endl;
                }
                fout <<"                    ]" << std::endl;
                fout <<"                }" << std::endl;
                        }
            if (nV > 0) 
            {
                fout <<"                coord DEF co Coordinate {" << std::endl;
                fout <<"                    point [" << std::endl;
                for(size_t v = 0; v < nV; v++)
                {
                    fout <<"                        " << m_points[v][0] << " " 
                                                      << m_points[v][1] << " " 
                                                      << m_points[v][2] << "," << std::endl;
                }
                fout <<"                    ]" << std::endl;
                fout <<"                }" << std::endl;
            }
            if (nT > 0) 
            {
                fout <<"                coordIndex [ " << std::endl;
                for(size_t f = 0; f < nT; f++)
                {
                    fout <<"                        " << m_triangles[f][0] << ", " 
                                                      << m_triangles[f][1] << ", "                                                  
                                                      << m_triangles[f][2] << ", -1," << std::endl;
                }
                fout <<"                ]" << std::endl;
            }
            fout <<"            }" << std::endl;
            fout <<"        }" << std::endl;
            fout <<"    ]" << std::endl;
            fout <<"}" << std::endl;    
            return true;
        }
        return false;
    }
    bool Mesh::SaveOFF(const std::string & fileName) const
    {
        std::cout << "Saving " <<  fileName << std::endl;
        std::ofstream fout(fileName.c_str());
        if (fout.is_open()) 
        {           
            size_t nV = m_points.Size();
            size_t nT = m_triangles.Size();
            fout <<"OFF" << std::endl;
            fout << nV << " " << nT << " " << 0<< std::endl;
            for(size_t v = 0; v < nV; v++)
            {
                fout << m_points[v][0] << " " 
                     << m_points[v][1] << " " 
                     << m_points[v][2] << std::endl;
            }
            for(size_t f = 0; f < nT; f++)
            {
                fout <<"3 " << m_triangles[f][0] << " " 
                            << m_triangles[f][1] << " "
                            << m_triangles[f][2] << std::endl;
            }
            fout.close();
            return true;
        }
        return false;
    }

    bool Mesh::LoadOFF(const std::string & fileName, bool invert) 
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
                m_points.Resize(nv);
                m_triangles.Resize(nf);
                Vec3<Real> coord;
                float x,y,z;
                for (long p = 0; p < nv ; p++) 
                {
                    fscanf(fid, "%f", &x);
                    fscanf(fid, "%f", &y);
                    fscanf(fid, "%f", &z);
                    m_points[p][0] = x;
                    m_points[p][1] = y;
                    m_points[p][2] = z;
                }
                int i, j, k, s;
                for (long t = 0; t < nf ; ++t) {
                    fscanf(fid, "%i", &s);
                    if (s == 3)
                    {
                        fscanf(fid, "%i", &i);
                        fscanf(fid, "%i", &j);
                        fscanf(fid, "%i", &k);
                        m_triangles[t][0] = i;
                        if (invert)
                        {
                            m_triangles[t][1] = k;
                            m_triangles[t][2] = j;
                        }
                        else
                        {
                            m_triangles[t][1] = j;
                            m_triangles[t][2] = k;
                        }
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

    Real Mesh::ComputeVolume() const
    {
        const long nV = (long) GetNPoints();
        if (nV == 0)
        {
            return 0.0;
        }       
        Vec3<Real> bary(0.0, 0.0, 0.0);
        for(long v = 0; v < nV; v++)
        {
            bary +=  GetPoint(v);
        }
        bary /= static_cast<Real>(nV);
        
        const long nT = (long) GetNTriangles();
        Vec3<Real> ver0, ver1, ver2;
        Real totalVolume = 0.0;
        for(long t = 0; t < nT; t++)
        {
            const Vec3<long> & tri = GetTriangle(t);
            ver0 = GetPoint(tri[0]);
            ver1 = GetPoint(tri[1]);
            ver2 = GetPoint(tri[2]);
            totalVolume += ComputeVolume4(ver0, ver1, ver2, bary);
        }
        return totalVolume/6.0;
    }

}
