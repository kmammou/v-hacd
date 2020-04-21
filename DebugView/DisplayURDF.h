#ifndef DISPLAY_URDF_H
#define DISPLAY_URDF_H

namespace RENDER_DEBUG
{
	class RenderDebug;
}

namespace DISPLAY_URDF
{

class DisplayURDF
{
public:

	static DisplayURDF *create(void);

	// Import a URDF file
	virtual void		importURDF(const char *fname) = 0;

	// Render the contents of the URDF file using the RenderDebug interface and at the provided scale
	virtual void	render(RENDER_DEBUG::RenderDebug *renderDebug,float scale,bool showSourceMesh,bool showCollisionMesh) = 0;

	// debug print link information 
	virtual void	debugLinks(void) = 0;

	virtual void release(void) = 0;

protected:
	virtual ~DisplayURDF(void)
	{
	}
};

}	  // end of DISPLAY_URDF namespace

#endif
