#include "ImportSTL.h"
#include "InParser.h"
#include <string.h>
#include <vector>

// Simple code snippet to import mesh data from an 'STL' file
// https://en.wikipedia.org/wiki/STL_(file_format)
namespace IMPORT_STL
{

static float fm_computePlane(const float *A, const float *B, const float *C, float *n) // returns D
{
	float vx = (B[0] - C[0]);
	float vy = (B[1] - C[1]);
	float vz = (B[2] - C[2]);

	float wx = (A[0] - B[0]);
	float wy = (A[1] - B[1]);
	float wz = (A[2] - B[2]);

	float vw_x = vy * wz - vz * wy;
	float vw_y = vz * wx - vx * wz;
	float vw_z = vx * wy - vy * wx;

	float mag = (float)sqrt((vw_x * vw_x) + (vw_y * vw_y) + (vw_z * vw_z));

	if (mag < 0.000001f)
	{
		mag = 0;
	}
	else
	{
		mag = 1.0f / mag;
	}

	float x = vw_x * mag;
	float y = vw_y * mag;
	float z = vw_z * mag;


	float D = 0.0f - ((x*A[0]) + (y*A[1]) + (z*A[2]));

	n[0] = x;
	n[1] = y;
	n[2] = z;

	return D;
}

class Vec3
{
public:
	float	mPos[3];
};

typedef std::vector< Vec3 > Vec3Vector;

	
	typedef std::vector< StlTriangle > StlTriangleVector;

	class ImportSTLImpl : public ImportSTL, public IN_PARSER::InPlaceParserInterface
	{
	public:
		ImportSTLImpl(void)
		{

		}
		~ImportSTLImpl(void)
		{

		}


		// Binary files begin with an 80 byte header
		// https://en.wikipedia.org/wiki/STL_(file_format)#Binary_STL
		bool isAscii(const uint8_t * bytes,uint32_t fileSize)
		{
			if (fileSize < 80)
			{
				return true; // binary files have at least an 80 char header.
			}
			// if all 80 bytes are ASCII, we assume it is an ASCII file.
			bool ret = true;
			for (uint32_t i = 0; i <fileSize; i++)
			{
				uint8_t c = bytes[i];
				if (c == 13 || c == 10 || c == 9) // these are all allowed, tab, line feed, carraige return
				{
					continue;
				}
				if (c < 32 || c > 127)
				{
					ret = false;
					break;
				}
			}
			return ret;
		}

		virtual uint32_t ParseLine(uint32_t lineno, uint32_t argc, const char **argv) final  // return TRUE to continue parsing, return FALSE to abort parsing process
		{
			uint32_t ret = 0;

			(lineno);
			if (argc)
			{
				const char *cmd = argv[0];
				if (strcmp(cmd, "solid") == 0)// defines the start of a mesh definition
				{

				}
				else if (strcmp(cmd, "endsolid") == 0)// defines the start of a mesh definition
				{

				}
				else if (strcmp(cmd, "facet") == 0)
				{
					if (argc == 5 && strcmp(argv[1], "normal") == 0)
					{
						mHaveNormal = true;
						mNormal[0] = float(atof(argv[2]));
						mNormal[1] = float(atof(argv[3]));
						mNormal[2] = float(atof(argv[4]));
					}
				}
				else if (strcmp(cmd, "outer") == 0)
				{

				}
				else if (strcmp(cmd, "endloop") == 0)
				{
					if (mPolygon.size() >= 3)
					{
						if (!mHaveNormal)
						{
							const Vec3 &p1 = mPolygon[0];
							const Vec3 &p2 = mPolygon[1];
							const Vec3 &p3 = mPolygon[2];
							fm_computePlane(p1.mPos, p2.mPos, p3.mPos,mNormal);
							mHaveNormal = false;
						}
						// ok..now we create the STL triangles for this polygon..
						uint32_t pcount = uint32_t(mPolygon.size());
						const Vec3 &p1 = mPolygon[0];
						StlTriangle tri;
						tri.mNormal[0] = mNormal[0];
						tri.mNormal[1] = mNormal[1];
						tri.mNormal[2] = mNormal[2];
						tri.mP1[0] = p1.mPos[0];
						tri.mP1[1] = p1.mPos[1];
						tri.mP1[2] = p1.mPos[2];



						tri.mColor = 0;
						for (uint32_t i = 2; i < pcount; i++)
						{
							const Vec3 &p2 = mPolygon[i-1];

							tri.mP2[0] = p2.mPos[0];
							tri.mP2[1] = p2.mPos[1];
							tri.mP2[2] = p2.mPos[2];

							const Vec3 &p3 = mPolygon[i];

							tri.mP3[0] = p3.mPos[0];
							tri.mP3[1] = p3.mPos[1];
							tri.mP3[2] = p3.mPos[2];
							mTriangles.push_back(tri);
						}
					}
					mPolygon.clear();
				}
				else if (strcmp(cmd, "vertex") == 0)
				{
					if (argc == 4)
					{
						Vec3 v;
						v.mPos[0] = float(atof(argv[1]));
						v.mPos[1] = float(atof(argv[2]));
						v.mPos[2] = float(atof(argv[3]));
						mPolygon.push_back(v);
					}
				}
			}
			return ret;
		}

		bool importMeshAscii(const uint8_t* bytes,uint32_t fileSize)
		{
			// https://en.wikipedia.org/wiki/STL_(file_format)
			bool ret = false;
			if (fileSize)
			{
				IN_PARSER::InPlaceParser ipp;
				char *tempData = new char[fileSize + 1];
				memcpy(tempData, bytes, fileSize);
				tempData[fileSize] = 0;
				ipp.SetSourceData(tempData, fileSize);
				ipp.Parse(this);
			}

			return ret;
		}

		bool importMeshBinary(const uint8_t* bytes, uint32_t fileSize)
		{
			// https://en.wikipedia.org/wiki/STL_(file_format)

			// Header is of size 80 bytes as per
			// https://en.wikipedia.org/wiki/STL_(file_format)#Binary_STL
			const uint32_t headerSize = 80;
			uint32_t numTris = *((uint32_t*)(bytes + headerSize));

			// Iterate through triangles:
			const uint8_t *stlTriBytes = bytes + headerSize + sizeof(uint32_t);
			const int stlBinaryTriangleSize = (12 * sizeof(float) + sizeof(uint16_t));
			// Compute the EOF based on number of triangles specified.
			// If that goes beyond the end of the file, return false
			uint32_t sizeOfMesh = headerSize + sizeof(uint32_t) + (numTris * stlBinaryTriangleSize);
			if (sizeOfMesh > fileSize)
			{
				return false;
			}
			mTriangles.clear();
			mTriangles.reserve(numTris);
			for (uint32_t ti = 0; ti < numTris; ti++)
			{
				// Structs are padded ( at least in the windows build ), so we have to use StlBinaryTriangleSize
				// to correctly iterate through the ptr.  sizeof(StlBinaryTriangle) is padded to 4-byte multiple.
				const StlTriangle &tri = *(StlTriangle*)(stlTriBytes + ti * stlBinaryTriangleSize);
				mTriangles.push_back(tri);
			}

			return true;
		}


		// Import a triangle mesh, returns the number of triangles found in the source data
		// returns 0 if the file was corrupt or otherwise no triangles found.
		// 'data' points to the memory representing the STL file
		// 'dataLen' is the length of the file
		virtual const StlTriangle *importMesh(const void *_data, uint32_t dataLen,uint32_t &triCount) final
		{
			const StlTriangle *ret = nullptr;

			mTriangles.clear();
			const uint8_t *data = (const uint8_t *)(_data);
			if (isAscii(data, dataLen))
			{
				importMeshAscii(data, dataLen);
			}
			else
			{
				importMeshBinary(data, dataLen);
			}
			triCount = uint32_t(mTriangles.size());
			if (triCount)
			{
				ret = &mTriangles[0];
			}
			
			return ret;
		}


		// Release the ImportSTL interface
		virtual void release(void) final
		{
			delete this;
		}

	private:
		StlTriangleVector	mTriangles;
		// Scratch variables for parsing ASCII STL
		bool				mHaveNormal{ false };
		float				mNormal[3];
		Vec3Vector			mPolygon;
	};

// Create an instance of the ImportSTL class with the provided source
// data.  The file will be parsed and converted into an indexed triangle mesh
// suitable for rendering or collision detection and other geometric calculations
ImportSTL *ImportSTL::create(void)
{
	ImportSTLImpl *s = new ImportSTLImpl;
	return static_cast<ImportSTL *>(s);
}


} // End of IMPORT_STL namespace
