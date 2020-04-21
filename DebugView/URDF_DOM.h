#ifndef URDF_DOM_H
#define URDF_DOM_H

#include <vector>
#include <string>

namespace MESH_FACTORY
{
	class Mesh;
}

// Defines the URDF object model
namespace URDF_DOM
{


typedef std::vector< std::string > StringVector;

// A simple 2d vector
class Vec2
{
public:
	float	x{ 0 };
	float	y{ 0 };
};

// A simple 3d vector
class Vec3
{
public:
	float	x{ 0 };
	float	y{ 0 };
	float	z{ 0 };
};

// A simple 4d vector
class Vec4
{
	float	x{ 0 };
	float	y{ 0 };
	float	z{ 0 };
	float	w{ 0 };
};

class Limit
{
public:
	float		mEffort{ 0 };
	float		mVelocity{ 0 };
	float		mLower{ 0 };
	float		mUpper{ 0 };
};

typedef std::vector< Limit > LimitVector;

class SafetyController
{
public:
	float		mKVelocity{ 0 };
	float		mKPosition{ 0 };
	float		mSoftLowerLimit{ 0 };
	float		mSoftUpperLimit{ 0 };
};

typedef std::vector< SafetyController > SafetyControllerVector;

class Calibration
{
public:
	float	mRising{ 0 };
	float	mFalling{ 0 };
};

typedef std::vector< Calibration > CalibrationVector;

class Dynamics
{
public:
	void clear(void)
	{
		Dynamics c;
		*this = c;
	}
	float	mDamping{ 0 };
	float	mFriction{ 0 };
};

typedef std::vector< Dynamics > DynamicsVector;

class Mimic
{
public:
	std::string	mJoint;
	float		mMultiplier{ 0 };
	float		mOffset{ 0 };
};

typedef std::vector< Mimic > MimicVector;

class Axis
{
public:
	Vec3		mAxis;
	DynamicsVector	mDynamics;
};

class Pose
{
public:
	Vec3	mPosition;		// position
	Vec3	mRPY;			// roll-pitch-yaw
};

class GraspCheck
{
public:
	void clear(void)
	{
		GraspCheck c;
		*this = c;
	}
	uint32_t	mAttachSteps{ 0 };
	uint32_t	mDetachSteps{ 0 };
	uint32_t	mMinContactCount{ 0 };
};

class GripperLink
{
public:
	enum Type
	{
		GL_GRIPPER_LINK,
		GL_PALM_LINK,
		GL_LAST
	};
	void clear(void)
	{
		GripperLink c;
		*this = c;
	}
	Type			mType{ GL_LAST };	// Type of link
	std::string		mLink;				// Name of link
};

typedef std::vector< GripperLink > GripperLinkVector;
typedef std::vector< GraspCheck > GraspCheckVector;

class Gripper
{
public:
	void clear(void)
	{
		Gripper c;
		*this = c;
	}
	std::string			mName;
	GraspCheckVector	mGraspChecks;
	GripperLinkVector	mGripperLinks;
};

typedef std::vector< Gripper > GripperVector;

class Joint
{
public:
	enum Type
	{
		JT_REVOLUTE,
		JT_CONTINUOUS,
		JT_PRISMATIC,
		JT_FIXED,
		JT_FLOATING,
		JT_PLANAR,
		JT_LAST
	};
	void clear(void)
	{
		Joint c;
		*this;
	}

	std::string		mName;			// Name of the joint
	Type			mType{ JT_LAST };			// Type of joint
	float			mThreadPitch{ 0 }; // whatever this is
	Axis			mAxis;			// axis information for this joint
	std::string		mChild;			// The child of this joint (really unnecessary..)
	std::string		mParent;		// The parent of this joint
	LimitVector		mLimits;
	SafetyControllerVector	mSafetyControllers; // any safety controllers
	CalibrationVector	mCalibrations;	// any calibrations
	DynamicsVector	mDynamics;
	MimicVector		mMimics;
	Pose			mPose;
};

typedef std::vector< Joint > JointVector;

// mass space inertia tensor in some format I have not yet deciphered
class Inertia
{
public:
	float	mIXX{ 1 };
	float	mIXY{ 1 };
	float	mIXZ{ 1 };
	float	mIYY{ 1 };
	float	mIYZ{ 1 };
	float	mIZZ{ 1 };
};

class Inertial
{
public:
	Inertial(void)
	{
	}
	float		mMass{ 1 };		// Mass of this object
	Inertia		mInertia;		// Mass space inertia tensor
	Pose		mPose;
};

class Geometry
{
public:
	enum Type
	{
		GT_MESH,
		GT_BOX,
		GT_SPHERE,
		GT_CAPSULE,
		GT_PLANE,
		GT_CYLINDER,
		GT_LAST
	};

	Type			mType{ GT_LAST };
	std::string		mMesh;		// mesh associated with this geometry
	Vec3			mDimensions;	// If a box, x,y,z, is a cylinder radius,length, if a sphere x=radius
};

// Visual and/or collision representation
class Representation
{
public:
	std::string	mName;
	MESH_FACTORY::Mesh		*mMesh{ nullptr };	// Triangle mesh associated with this representation
	Pose		mPose;
	Geometry	mGeometry;		// The geometry associated with this visual and rigid body
	std::string	mMaterial;		// material associated with this visual
};

class SimulatedActuatedJoint
{
public:
	std::string		mName;
	std::string		mPassiveActuatedJoint;
	float			mSimulatedReduction{ 0 };
};

typedef std::vector< SimulatedActuatedJoint > SimulatedActuatedJointVector;

class Compensator
{
public:
	float		mKBelt{ 0 };
	float		mKDMotor{ 0 };
	float		mLambdaCombined{ 0 };
	float		mLambdaJoint{ 0 };
	float		mLambdaMotor{ 0 };
	float		mMassMotor{ 0 };
};

typedef std::vector< Compensator > CompensatorVector;

class Actuator
{
public:
	enum Type
	{
		AT_RIGHT_ACTUATOR,
		AT_LEFT_ACTUATOR,
		AT_FLEX_JOINT,
		AT_ROLL_JOINT,
		AT_LAST
	};
	std::string	mName;
	Type		mType{ AT_LAST };			// type of actuator
	float		mMechanicalReduction{ 0 };
};

typedef std::vector< Actuator > ActuatorVector;

class GapJoint
{
public:
	std::string	mName;
	float		mL0{ 0 };
	float		mA{ 0 };
	float		mB{ 0 };
	float		mGearRatio{ 0 };
	float		mH{ 0 };
	float		mMechanicalReduction{ 0 };
	float		mPhi0{ 0 };
	float		mR{ 0 };
	float		mScrewReduction{ 0 };
	float		mT0{ 0 };
	float		mTheta0{ 0 };
};

typedef std::vector< GapJoint > GapJointVector;

class Transmission
{
public:
	void clear(void)
	{
		Transmission c;
		*this = c;
	}
	bool					mUseSimulatedGripperJoint{ false };
	std::string				mName;
	std::string				mType;
	std::string				mActuator;
	StringVector			mJoints;
	StringVector			mPassiveJoints;
	SimulatedActuatedJointVector	mSimulatedActuatedJoints;
	float					mMechanicalReduction{ 0 };
	CompensatorVector		mCompensators;
	ActuatorVector			mActuators;
	GapJointVector			mGapJoints;
};

typedef std::vector< Transmission > TransmissionVector;

class Link
{
public:
	void clear(void)
	{
		Link c;
		*this = c;
	}
	std::string		mName;
	std::string		mType;
	Inertial		mInertial;		// Some rigid body parameters
	Representation	mVisual;		// The visual representation
	Representation	mCollision;		// The collision representation
	Pose			mPose;
	bool			mGravity{ true };

};

typedef std::vector< Link > LinkVector;

class Color
{
public:
	bool haveColor(void) const
	{
		return mRed != 1 || mGreen != 1 || mBlue != 1 || mAlpha != 1;
	}
	float	mRed{ 1 };
	float	mGreen{ 1 };
	float	mBlue{ 1 };
	float	mAlpha{ 1 };
};

class Material
{
public:
	void clear(void)
	{
		Material c;
		*this = c;
	}
	std::string	mName;
	std::string	mTexture;
	Color		mColor;
};

typedef std::vector< Material > MaterialVector;

// A bag of properties are associated with plugins and gazebos.
// A key-value pair represents one of those property pairs which have not
// been encoded into a specific C++ class
class KeyValuePair
{
public:
	std::string	mKey;		// The type as a key
	std::string	mValue;		// The value as a string
};

// A vector of KeyValuePairs
typedef std::vector< KeyValuePair > KeyValuePairVector;

// Specifies a Gazebo plugin.
// A 'plugin' has a filename (a shared object .so file in many cases)
// And the name of the plugin
// Followed by a basket of key-value pairs that the plugin might consume
class Plugin
{
public:

	void clear(void)
	{
		Plugin c;
		*this = c;
	}

	std::string			mFileName;			// Filename of the plugin
	std::string			mName;				// The name of the plugin
	KeyValuePairVector	mKeyValuePairs;		// The list of key-value pairs associated with this plugin
};

typedef std::vector< Plugin > PluginVector;

class ScanDimensions
{
public:
	float			mSamples{ 0 };
	float			mResolution{ 0 };
	float			mMinAngle{ 0 };
	float			mMaxAngle{ 0 };
};

class ScanRange
{
public:
	float		mMin{ 0 };
	float		mMax{ 0 };
	float		mResolution{ 0 };
};

class Scan
{
public:
	ScanDimensions	mHorizontal;
	ScanRange		mRange;
};

class Ray
{
public:
	Scan			mScan;
};

class Contact
{
public:
	std::string	mCollision;
};

class Image
{
public:
	std::string	mFormat;
	uint32_t	mWidth{ 0 };
	uint32_t	mHeight{ 0 };
};

class Clip
{
public:
	float	mNear{ 0 };
	float	mFar{ 0 };
};

class Camera
{
public:
	void clear(void)
	{
		Camera c;
		*this = c;
	}
	float	mHorizontalFOV{ 0 };
	Image	mImage;
	Clip	mClip;
};

typedef std::vector< Camera > CameraVector;

class Projector
{
public:
	void clear(void)
	{
		Projector c;
		*this = c;
	}
	std::string	mName;
	Pose		mPose;
	std::string	mTexture;
	float		mFOV{ 0 };
	float		mNearClip{ 0 };
	float		mFarClip{ 0 };
};

typedef std::vector< Projector > ProjectorVector;

class Sensor
{
public:
	Sensor(void)
	{
	}
	void clear(void)
	{
		Sensor c;
		*this = c;
	}
	std::string		mName;
	std::string		mType;
	bool			mAlwaysOn{ false };
	bool			mAlwaysUpdate{ false };
	float			mUpdateRate{ 0 };
	Pose			mPose;
	bool			mVisualize{ false };
	Contact			mContact;
	Ray				mRay;
	CameraVector	mCameras;			// if this sensor is/has a camera
	PluginVector	mPlugins;
};


typedef std::vector< Sensor > SensorVector;

// A Gazebo object
class  Gazebo
{
public:
	void clear(void)
	{
		Gazebo c;
		*this = c;
	}
	std::string			mReference;
	std::string			mMaterial;
	ProjectorVector		mProjectors;
	PluginVector		mPlugins;
	SensorVector		mSensors;
	JointVector			mJoints;
	LinkVector			mLinks;
	GripperVector		mGrippers;
	KeyValuePairVector	mKeyValuePairs;
};

typedef std::vector< Gazebo > GazeboVector;

// A Robot object
class Robot
{
public:
	void clear(void)
	{
		Robot c;
		*this = c;
	}

	std::string			mName;				// name of the robot
	std::string			mXMLNSController;	// The robot controller schema
	std::string			mXMLNSInterface;	// The robot interface schema
	std::string			mXMLNSJoint;		// The joint schema
	std::string			mXMLNSSensor;		// The sensor schema
	std::string			mXMLNSXacro;		// Xacro schema?
	StringVector		mComments;			// comments about this robot
	GazeboVector		mGazebos;			// Data packages for Gazebo
	MaterialVector		mMaterials;			// list of materials
	LinkVector			mLinks;				// list of nodes in the robot
	JointVector			mJoints;			// list of joints in the robot
	TransmissionVector	mTransmissions;		// List of transmissions (motors which drive joints/links?)
};

typedef std::vector< Robot > RobotVector;

// The main base class for the URDF DOM
class URDF
{
public:
	void clear(void)
	{
		URDF c;
		*this = c;
	}
	RobotVector		mRobots;				// List of robots in this URDF spec
	StringVector	mComments;				// Comments found in the main body of the URDF file
};




};


#endif
