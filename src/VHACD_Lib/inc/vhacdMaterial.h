/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#ifndef VHACD_MATERIAL_H
#define VHACD_MATERIAL_H
#include <VHACDVersion.h>
#include <VHACDVector.h>
namespace VHACD
{
    struct Material
    {

        Vec3<double>                                            m_diffuseColor;
        double                                                  m_ambientIntensity;
        Vec3<double>                                            m_specularColor;
        Vec3<double>                                            m_emissiveColor;
        double                                                  m_shininess;
        double                                                  m_transparency;
		Material(void)
		{
			m_diffuseColor.X()  = 0.5;
			m_diffuseColor.Y()  = 0.5;
			m_diffuseColor.Z()  = 0.5;
			m_specularColor.X() = 0.5;
			m_specularColor.Y() = 0.5;
			m_specularColor.Z() = 0.5;
			m_ambientIntensity  = 0.4;
			m_emissiveColor.X() = 0.0;
			m_emissiveColor.Y() = 0.0;
			m_emissiveColor.Z() = 0.0;
			m_shininess         = 0.4;
			m_transparency      = 0.0;
		};
	};
}
#endif