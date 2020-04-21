#ifndef IMPORT_URDF_H
#define IMPORT_URDF_H

// Import a URDF file and convert it into a consumable object model
namespace URDF_DOM
{
class URDF;
};

namespace MESH_FACTORY
{
	class MeshFactory;
}

namespace IMPORT_URDF
{

class ImportURDF
{
public:
	static ImportURDF *create(void);

	virtual const URDF_DOM::URDF *importURDF(const char *xmlName, // Name of URDF file to load
											MESH_FACTORY::MeshFactory *meshFactory) = 0; // Optional 'MeshFactory' to load and represent individual triangle meshes

	virtual void release(void) = 0;
};

}

#endif
