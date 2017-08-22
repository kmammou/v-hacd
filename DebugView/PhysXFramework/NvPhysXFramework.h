#ifndef NV_PHYSX_FRAMEWORK_H
#define NV_PHYSX_FRAMEWORK_H

// The PhysX framework is a DLL which provides a very simple interface to perform
// physics simulations using the PhysX SDK.  It also contains bindings to do debug
// visualizations of the current simulation using the NvRenderDebug remote visualization
// API and/or PVD (the PhysX Visual Debugger)
// This is not intended to be a full function physics API but rather a minimal simple
// system for getting basic demos to work.  The intial version was written specifically 
// to improve and test the V-HACD (voxelized hierarchical convex decomposition library) results.
// @see: https://github.com/kmammou/v-hacd

#include <stdint.h>

namespace RENDER_DEBUG
{
	class RenderDebug;
}

namespace NV_PHYSX_FRAMEWORK
{

#define PHYSX_FRAMEWORK_VERSION_NUMBER 1

// Instantiate the PhysX SDK, create a scene, and a ground plane
class PhysXFramework
{
public:
	class CommandCallback
	{
	public:
		/**
		*\brief Optional callback to the application to process an arbitrary console command.

		This allows the application to process an incoming command from the server.  If the application consumes the command, then it will not be passed on
		to the rest of the default processing.  Return true to indicate that you consumed the command, false if you did not.

		\return Return true if your application consumed the command, return false if it did not.
		*/
		virtual bool processDebugCommand(uint32_t argc, const char **argv) = 0;
	};

	// Returns delta time since last simulation step
	virtual float simulate(void) = 0;

	// Create a default series of stacked boxes for testing purposes
	virtual void createDefaultStacks(void) = 0;

	virtual void setCommandCallback(CommandCallback *cc) = 0;

	// Return the render debug interface if available
	virtual RENDER_DEBUG::RenderDebug *getRenderDebug(void) = 0;

	// Release the PhysXFramework interface
	virtual void release(void) = 0;

protected:
	virtual ~PhysXFramework(void)
	{
	}
};

PhysXFramework *createPhysXFramework(uint32_t versionNumber,const char *dllName);


};

#endif
