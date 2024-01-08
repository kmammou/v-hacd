#pragma once

// Just a stupid little utility to save out a triangle mesh
// as a USDA file. Primarily for debugging.
#include <stdint.h>

class SimpleMesh
{
public:
    uint32_t    mVertexCount{0};
    const double      *mVertices{nullptr};
    uint32_t    mTriangleCount{0};
    const uint32_t    *mIndices{nullptr};
};

class SimpleSphere
{
public:
    double  mCenter[3];
    double  mRadius{ 0 };
};


class SaveUSDA
{
public:
    static SaveUSDA *create(const char *fname);

    virtual bool saveMesh(const char *meshName,
                          const SimpleMesh &mesh,
                          const float meshColor[3]) = 0;

    virtual bool saveSphere(const SimpleSphere &s) = 0;

    virtual void release(void) = 0;

protected:
    virtual ~SaveUSDA(void)
    {
    }
};
