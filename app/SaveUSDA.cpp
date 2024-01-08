#include "SaveUSDA.h"

#include <float.h>
#include <math.h>
#include <vector>
#include <cstdio>

#ifdef _MSC_VER
#pragma warning(disable:4100 4996)
#endif

static double computePlane(const double* A, const double* B, const double* C, double* n) // returns D
{
    double vx = (B[0] - C[0]);
    double vy = (B[1] - C[1]);
    double vz = (B[2] - C[2]);

    double wx = (A[0] - B[0]);
    double wy = (A[1] - B[1]);
    double wz = (A[2] - B[2]);

    double vw_x = vy * wz - vz * wy;
    double vw_y = vz * wx - vx * wz;
    double vw_z = vx * wy - vy * wx;

    double mag = (double)sqrt((vw_x * vw_x) + (vw_y * vw_y) + (vw_z * vw_z));

    if (mag < 0.000001f)
    {
        mag = 0;
    }
    else
    {
        mag = 1.0f / mag;
    }

    double x = vw_x * mag;
    double y = vw_y * mag;
    double z = vw_z * mag;


    double D = 0.0f - ((x * A[0]) + (y * A[1]) + (z * A[2]));

    n[0] = x;
    n[1] = y;
    n[2] = z;

    return D;
}

static double normalize(double* n) // normalize this vector
{
    double dist = (double)sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (dist > 0.0000001f)
    {
        double mag = 1.0f / dist;
        n[0] *= mag;
        n[1] *= mag;
        n[2] *= mag;
    }
    else
    {
        n[0] = 1;
        n[1] = 0;
        n[2] = 0;
    }

    return dist;
}


class V3
{
public:
    double x;
    double y;
    double z;
};

class SaveUSDAImpl : public SaveUSDA
{
public:
    SaveUSDAImpl(const char *fname)
    {
        mFph = fopen(fname,"wb");
        if ( mFph )
        {
            writeHeader(mFph);
        }
    }

    virtual ~SaveUSDAImpl(void)
    {
        if ( mFph )
        {
            writeFooter(mFph);
            fclose(mFph);
        }
    }

    virtual bool saveMesh(const char *meshName,
                          const SimpleMesh &mesh,
                          const float meshColor[3]) final
    {
        bool ret = false;

        if ( mFph )
        {
            ret = true;
            writeMesh(meshName,mesh,meshColor,mFph);
        }

        return ret;
    }

    virtual void release(void) final
    {
        delete this;
    }

    bool isValid(void) const
    {
        return mFph ? true :false;
    }

    void writeHeader(FILE *fph)
    {
        fprintf(fph, "#usda 1.0\n");

        fprintf(fph, "(\n");
        fprintf(fph, "    defaultPrim = \"World\"\n");
        fprintf(fph, "    endTimeCode = 100\n");
        fprintf(fph, "    metersPerUnit = 0.009999999776482582\n");
        fprintf(fph, "    startTimeCode = 0\n");
        fprintf(fph, "    timeCodesPerSecond = 24\n");
        fprintf(fph, "    upAxis = \"Y\"\n");
        fprintf(fph, ")\n");
        fprintf(fph, "\n");

        fprintf(fph, "def Xform \"World\"\n");
        fprintf(fph, "{\n");

// *** Xform for rigid body

        fprintf(fph,"    def Xform \"ConvexDecomposition\" (\n");
//        fprintf(fph,"        prepend apiSchemas = [\"PhysicsRigidBodyAPI\", \"PhysxRigidBodyAPI\"]\n");
        fprintf(fph,"    )\n");
        fprintf(fph,"    {\n");
        fprintf(fph,"        bool physics:kinematicEnabled = 0\n");
        fprintf(fph,"        bool physics:rigidBodyEnabled = 1\n");
        fprintf(fph,"        double3 xformOp:rotateXYZ = (0, 0, 0)\n");
        fprintf(fph,"        double3 xformOp:scale = (1, 1, 1)\n");
        fprintf(fph,"        double3 xformOp:translate = (0, 0, 0)\n");
        fprintf(fph,"        uniform token[] xformOpOrder = [\"xformOp:translate\", \"xformOp:rotateXYZ\", \"xformOp:scale\"]\n");

//

    }

    void writeFooter(FILE *fph)
    {


        fprintf(fph,"    }\n");

        fprintf(fph, "    def DistantLight \"DistantLight\" (\n");
        fprintf(fph, "        apiSchemas = [\"ShapingAPI\"]\n");
        fprintf(fph, "    )\n");
        fprintf(fph, "    {\n");
        fprintf(fph, "        float angle = 1\n");
        fprintf(fph, "        float intensity = 3000\n");
        fprintf(fph, "        float shaping:cone:angle = 180\n");
        fprintf(fph, "        float shaping:cone:softness\n");
        fprintf(fph, "        float shaping:focus\n");
        fprintf(fph, "        color3f shaping:focusTint\n");
        fprintf(fph, "        asset shaping:ies:file\n");
        fprintf(fph, "        double3 xformOp:rotateXYZ = (315, 0, 0)\n");
        fprintf(fph, "        double3 xformOp:scale = (1, 1, 1)\n");
        fprintf(fph, "        double3 xformOp:translate = (0, 0, 0)\n");
        fprintf(fph, "        uniform token[] xformOpOrder = [\"xformOp:translate\", \"xformOp:rotateXYZ\", \"xformOp:scale\"]\n");
        fprintf(fph, "    }\n");

// Ground Plane goes here
#if 0
    fprintf(fph,"    def Xform \"GroundPlane\"\n");
    fprintf(fph,"    {\n");
    fprintf(fph,"        quatf xformOp:orient = (1, 0, 0, 0)\n");
    fprintf(fph,"        float3 xformOp:scale = (1, 1, 1)\n");
    fprintf(fph,"        double3 xformOp:translate = (0, 0, 0)\n");
    fprintf(fph,"        uniform token[] xformOpOrder = [\"xformOp:translate\", \"xformOp:orient\", \"xformOp:scale\"]\n");
    fprintf(fph,"\n");
    fprintf(fph,"        def Mesh \"CollisionMesh\"\n");
    fprintf(fph,"        {\n");
    fprintf(fph,"            uniform bool doubleSided = 0\n");
    fprintf(fph,"            int[] faceVertexCounts = [4]\n");
    fprintf(fph,"            int[] faceVertexIndices = [0, 1, 2, 3]\n");
    fprintf(fph,"            normal3f[] normals = [(0, 1, 0), (0, 1, 0), (0, 1, 0), (0, 1, 0)]\n");
    fprintf(fph,"            point3f[] points = [(-2500, 0, -2500), (2500, 0, -2500), (2500, 0, 2500), (-2500, 0, 2500)]\n");
    fprintf(fph,"            color3f[] primvars:displayColor = [(0.5, 0.5, 0.5)]\n");
    fprintf(fph,"        }\n");
    fprintf(fph,"\n");
    fprintf(fph,"        def Plane \"CollisionPlane\" (\n");
    fprintf(fph,"            prepend apiSchemas = [\"PhysicsCollisionAPI\"]\n");
    fprintf(fph,"        )\n");
    fprintf(fph,"        {\n");
    fprintf(fph,"            uniform token axis = \"Y\"\n");
    fprintf(fph,"            uniform token purpose = \"guide\"\n");
    fprintf(fph,"        }\n");
    fprintf(fph,"    }\n");
#endif
//

        fprintf(fph, "}\n");
    }

    void writeMesh(const char *meshName,
                   const SimpleMesh &mesh,
                   const float meshColor[3],
                   FILE *fph)
    {
        std::vector< V3 > normals;
        normals.resize(mesh.mTriangleCount);
        for (uint32_t i=0; i<mesh.mTriangleCount; i++)
        {
            uint32_t i1 = mesh.mIndices[i * 3 + 0];
            uint32_t i2 = mesh.mIndices[i * 3 + 1];
            uint32_t i3 = mesh.mIndices[i * 3 + 2];
            const double *p1 = &mesh.mVertices[i1*3];
            const double *p2 = &mesh.mVertices[i2*3];
            const double *p3 = &mesh.mVertices[i3*3];
            computePlane(p1,p2,p3,&normals[i].x);
        }


        fprintf(fph, "    def Mesh \"%s\" (\n", meshName);

        fprintf(fph, "            )\n");

        fprintf(fph, "    {\n");

        fprintf(fph,"        color3f[] primvars:displayColor = [(%0.9f,%0.9f,%0.9f)]\n", meshColor[0], meshColor[1], meshColor[2]);

        fprintf(fph, "        int[] faceVertexCounts = [");
        for (uint32_t i = 0; i < mesh.mTriangleCount; i++)
        {
            fprintf(fph, "%d", 3);
            if ((i + 1) < mesh.mTriangleCount)
            {
                fprintf(fph, ", ");
            }
        }
        fprintf(fph, "]\n");

        fprintf(fph, "        int[] faceVertexIndices = [");
        for (uint32_t i = 0; i < mesh.mTriangleCount; i++)
        {
            uint32_t i1 = mesh.mIndices[i * 3 + 0];
            uint32_t i2 = mesh.mIndices[i * 3 + 1];
            uint32_t i3 = mesh.mIndices[i * 3 + 2];
            fprintf(fph, "%d, %d, %d", i1, i2, i3);
            if ((i + 1) < mesh.mTriangleCount)
            {
                fprintf(fph, ", ");
            }
            if (((i + 1) % 8) == 0)
            {
                fprintf(fph, "\n");
                fprintf(fph, "            ");
            }
        }
        fprintf(fph, "]\n");

        // Normals here...
        {
            fprintf(fph,"        normal3f[] normals = [");
            for (uint32_t i=0; i<mesh.mTriangleCount; i++)
            {
                const double *p = &normals[i].x;
                fprintf(fph, "(%0.9f, %0.9f, %0.9f)", p[0], p[1], p[2]);
                if ((i + 1) < mesh.mTriangleCount)
                {
                    fprintf(fph, ", ");
                }
                if (((i + 1) % 8) == 0)
                {
                    fprintf(fph, "\n");
                    fprintf(fph, "            ");
                }
            }
            fprintf(fph, "] (\n");
            fprintf(fph,"                interpolation = \"uniform\"\n");
            fprintf(fph,"            )\n");
        }

        fprintf(fph, "        point3f[] points = [");
        for (uint32_t i = 0; i < mesh.mVertexCount; i++)
        {
            const double *p = &mesh.mVertices[i * 3];
            fprintf(fph, "(%0.9f, %0.9f, %0.9f)", p[0], p[1], p[2]);
            if ((i + 1) < mesh.mVertexCount)
            {
                fprintf(fph, ", ");
            }
            if (((i + 1) % 8) == 0)
            {
                fprintf(fph, "\n");
                fprintf(fph, "            ");
            }
        }
        fprintf(fph, "]\n");

        double translate[3] = {0,0,0};

        // compute the direction vector between this object and the parent.
        fprintf(fph, "        uniform token subdivisionScheme = \"none\"\n");
        fprintf(fph, "            double3 xformOp:rotateXYZ = (0, 0, 0)\n");
        fprintf(fph, "            double3 xformOp:scale = (1, 1, 1)\n");
        fprintf(fph, "            double3 xformOp:translate = (0, 0, 0)\n");
        fprintf(fph, "            uniform token[] xformOpOrder = [\"xformOp:translate\", \"xformOp:rotateXYZ\", \"xformOp:scale\"]\n");

        fprintf(fph, "    }\n");

    }

    virtual bool saveSphere(const SimpleSphere &s) final
    {
        if ( !mFph ) return false;
        fprintf(mFph,"\tdef Sphere \"Sphere_%03d\" (\n", mSphereIndex);
        fprintf(mFph,"\t    prepend apiSchemas = [\"PhysicsCollisionAPI\", \"PhysxCollisionAPI\"]\n");
	    fprintf(mFph,"\t)\n");
        fprintf(mFph,"\t{\n");
        fprintf(mFph,"\t\tfloat3[] extent = [(%0.9f, %0.9f, %0.9f), (%0.9f, %0.9f, %0.9f)]\n",-s.mRadius,-s.mRadius,-s.mRadius,s.mRadius,s.mRadius,s.mRadius);
        fprintf(mFph,"\t\tbool physics:collisionEnabled = 1\n");
        fprintf(mFph,"\t\tdouble radius = %0.9f\n", s.mRadius);
        fprintf(mFph,"\t\tcustom bool refinementEnableOverride = 1\n");
        fprintf(mFph,"\t\tcustom int refinementLevel = 2\n");
//        fprintf(mFph,"token visibility = \"invisible\"\n");
        fprintf(mFph,"\t\tdouble3 xformOp:rotateXYZ = (0, 0, 0)\n");
        fprintf(mFph,"\t\tdouble3 xformOp:scale = (1, 1, 1)\n");
        fprintf(mFph,"\t\tdouble3 xformOp:translate = (%0.9f, %0.9f, %0.9f)\n",s.mCenter[0],s.mCenter[1],s.mCenter[2]);
        fprintf(mFph,"\t\tuniform token[] xformOpOrder = [\"xformOp:translate\", \"xformOp:rotateXYZ\", \"xformOp:scale\"]\n");
        fprintf(mFph, "\t}\n");
        fprintf(mFph,"\n");
        mSphereIndex++;
        return true;
    }

    FILE *mFph{nullptr};
    uint32_t    mSphereIndex{0};
};

SaveUSDA *SaveUSDA::create(const char *fname)
{
    auto ret = new SaveUSDAImpl(fname);
    if ( !ret->isValid() )
    {
        delete ret;
        ret =nullptr;
    }
    return static_cast< SaveUSDA *>(ret);
}
