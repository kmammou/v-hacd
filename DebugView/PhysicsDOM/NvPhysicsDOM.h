#ifndef NV_PHYSICS_DOM_H
#define NV_PHYSICS_DOM_H

// A plugin interface to instantiate, simulate, and query physics on multiple
// physics engines using the generic physics Data-Object-Model
#include <stdint.h>

namespace RENDER_DEBUG
{
	class RenderDebug;
}

namespace PHYSICS_DOM
{
	class PhysicsDOM;
	class NodeState;
	class PhysicsDOMInstance;
}

namespace PHYSICS_DOM
{

#define PHYSICS_DOM_VERSION_NUMBER 110

class PhysicsDOMContainer
{
public:
	virtual const PHYSICS_DOM::PhysicsDOM *getPhysicsDOM(void) = 0;
	virtual void release(void) = 0;
};


// Instantiate the PhysX SDK, create a scene, and a ground plane
class NvPhysicsDOM
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
	virtual float simulate(bool showPhysics) = 0;

	virtual void setCommandCallback(CommandCallback *cc) = 0;

	// Return the render debug interface if available
	virtual RENDER_DEBUG::RenderDebug *getRenderDebug(void) = 0;

	virtual void setDragForce(float force) = 0;

	virtual void setPauseState(bool state) = 0;
	virtual bool getPauseState(void) const = 0;

	// serialize the current state to an XML file
	virtual void serializeXML(const char *fname) = 0;

	// Debug feature; create some default box stacks
	virtual void createDefaultStacks(void) = 0;

	// Create some of everything so we can serialize the scenes and get a detailed
	// XML output for analysis
	virtual void createSomeOfEverything(void) = 0;

	// Load this physics DOM
	virtual PhysicsDOMInstance *loadPhysicsDOM(const PHYSICS_DOM::PhysicsDOM &physicsDOM,const char *physicsEngine="physx") = 0;


	// Parses a PhysX RepX XML file and loads the contents into a PhysicsDOM container
	virtual PhysicsDOMContainer *importPhysXDOM(const char *fname) = 0;

	// Release the PhysXFramework interface
	virtual void release(void) = 0;

protected:
	virtual ~NvPhysicsDOM(void)
	{
	}
};

NvPhysicsDOM *createNvPhysicsDOM(uint32_t versionNumber,const char *dllName);


};

#endif
