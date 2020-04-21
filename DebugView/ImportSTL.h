#ifndef IMPORT_STL_H
#define IMPORT_STL_H

#include <stdint.h>
// Simple code snippet to import mesh data from an 'STL' file
// https://en.wikipedia.org/wiki/STL_(file_format)
namespace IMPORT_STL
{

// This is the format of a triangle in the STL format:
// https://en.wikipedia.org/wiki/STL_(file_format)#Binary_STL
class StlTriangle
{
public:
	float		mNormal[3];		// The normal for this triangle
	float		mP1[3];			// Point 1
	float		mP2[3];			// Point 1
	float		mP3[3];			// Point 1
	uint16_t	mColor;
};



class ImportSTL
{
public:

	// Create an instance of the StlImport class with the provided source
	// data.  The file will be parsed and converted into an indexed triangle mesh
	// suitable for rendering or collision detection and other geometric calculations
	static ImportSTL *create(void);

	// Import a triangle mesh, returns the number of triangles found in the source data
	// returns 0 if the file was corrupt or otherwise no triangles found.
	// 'data' points to the memory representing the STL file
	// 'dataLen' is the length of the file
	virtual const StlTriangle * importMesh(const void *data, uint32_t dataLen,uint32_t &triangleCount) = 0;


	// Release the StlImport interface
	virtual void release(void) = 0;

protected:
	virtual ~ImportSTL(void)
	{

	}
};

} // End of STL_IMPORT namespace

#endif
