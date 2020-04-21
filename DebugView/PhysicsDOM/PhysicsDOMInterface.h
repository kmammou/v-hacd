#ifndef PHYSICS_DOM_INTERFACE_H
#define PHYSICS_DOM_INTERFACE_H

namespace PHYSICS_DOM
{

class NodeState
{
public:
	// Returns the current position and orientation for this node
	// false if unable to return this information for this type of node
	virtual bool getPose(float pos[3],float quat[4]) = 0;
};

// When a PhysicsDOM is instanced into a physics engine; this class is used
// to interact with the running simulation
class PhysicsDOMInstance
{
public:
	// Does a named lookup of a node with this ID and, if found, returns the 'NodeState'
	// interface which can be used to query current state of this object
	virtual PHYSICS_DOM::NodeState *getNodeState(const char *nodeId) = 0;

	// Runtime body pair filtering, 
	virtual bool shouldCollide(uint32_t bodyA, uint32_t bodyB) = 0;

	virtual void release(void) = 0;
};


} // end of PHYSICS_DOM interface

#endif
