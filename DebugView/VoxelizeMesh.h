#pragma once

#include <stdint.h>
#include "VHACD.h"

namespace RENDER_DEBUG
{
class	RenderDebug;
}

namespace voxelizemesh
{

class VoxelizeMesh
{
public:
	static VoxelizeMesh *create(uint32_t vertexCount,uint32_t triangleCount,const double *vertices,const uint32_t *indices,uint32_t resolution,RENDER_DEBUG::RenderDebug *renderDebug,VHACD::IVHACD::Parameters &params);

	virtual void visualize(bool showSurface,bool showInside,bool showOutside,bool showUndefined) = 0;

	virtual void computeInsideVoxels(void) = 0;

	virtual void snapVoxelToMesh(void) = 0;

	virtual void release(void) = 0;
protected:
	virtual ~VoxelizeMesh(void)
	{
	}
};

}
