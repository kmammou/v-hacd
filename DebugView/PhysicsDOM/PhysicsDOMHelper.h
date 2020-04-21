#ifndef PHYSICS_DOM_HELPER_H
#define PHYSICS_DOM_HELPER_H

#include "PhysicsDOMDef.h"
#include "NvMath/NvMath.h"
#include <string>

// This header file provides helper functions to work with the DOM class
namespace PHYSICS_DOM
{

	NV_MATH::NvTransform getNvTransform(const PHYSICS_DOM::Pose &p)
	{
		return NV_MATH::NvTransform(NV_MATH::NvVec3(p.p.x, p.p.y, p.p.z), NV_MATH::NvQuat(p.q.x, p.q.y, p.q.z, p.q.w));
	}

	PHYSICS_DOM::Pose getPose(const NV_MATH::NvTransform &p)
	{
		PHYSICS_DOM::Pose ret;
		ret.p = PHYSICS_DOM::Vec3(p.p.x, p.p.y, p.p.z);
		ret.q = PHYSICS_DOM::Quat(p.q.x, p.q.y, p.q.z, p.q.w);
		return ret;
	}

	PHYSICS_DOM::Pose getInversePose(const PHYSICS_DOM::Pose &ref,	// Reference frame
									const PHYSICS_DOM::Pose &worldPose) // World pose to be inverse transformed into the reference frame
	{
		NV_MATH::NvTransform inverse = getNvTransform(ref).getInverse();
		NV_MATH::NvTransform ret = inverse*getNvTransform(worldPose);
		return getPose(ret);
	}

	const PHYSICS_DOM::NodeDef *locateNode(const PHYSICS_DOM::PhysicsDOMDef &dom, const std::string id)
	{
		// Search all collections for a node with this ID
		for (auto &i : dom.mCollections)
		{
			for (auto &j : i->mNodes)
			{
				if (j->mId == id)
				{
					return j;
				}
			}
		}
		// Search all scenes for a node with this ID
		for (auto &i : dom.mScenes)
		{
			for (auto &j : i->mNodes)
			{
				if (j->mId == id)
				{
					return j;
				}
			}
		}
		return nullptr;
	}

}

#endif
