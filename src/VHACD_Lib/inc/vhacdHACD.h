/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#ifndef VHACD_VHACD_H
#define VHACD_VHACD_H
#include <vhacdVersion.h>
#include <vhacdVector.h>
#include <vhacdMesh.h>
#include <vhacdVolume.h>
#include <vhacdSArray.h>

namespace VHACD
{
    typedef void (*CallBackFunction)(const char *);

    bool ApproximateConvexDecomposition(VoxelSet * const                inputVSet, 
                                            const int                   depth, 
                                            const int                   planeDownsampling,
                                            const int                   convexhullDownsampling, 
                                            const Real                  alpha, 
                                            const Real                  beta, 
                                            const Real                  concavityThreshold,
                                            SArray< VoxelSet * >      & parts, 
                                            const CallBackFunction      callBack);
    bool ApproximateConvexDecomposition(TetrahedronSet * const                inputTSet, 
                                            const int                         depth, 
                                            const int                         planeDownsampling,
                                            const int                         convexhullDownsampling, 
                                            const Real                        alpha, 
                                            const Real                        beta, 
                                            const Real                        concavityThreshold,
                                            const bool                        pca,
                                            SArray< TetrahedronSet * >      & parts, 
                                            const CallBackFunction            callBack);
}
#endif