#include "ImportURDF.h"
#include "FastXml.h"
#include "StringHelper.h"
#include "URDF_DOM.h"
#include "MeshFactory.h"
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <stdlib.h>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <assert.h>

// Documentation for URDF XML is here: http://wiki.ros.org/urdf/XML/

#pragma warning(disable:4100 4505)

#define MAX_STACK 32

using namespace URDF_DOM;

namespace IMPORT_URDF
{


	void normalizePath(char *foo)
	{
		while (*foo)
		{
			if (*foo == '\\')
			{
				*foo = '/';
			}
			foo++;
		}
	}

	char *nextSlash(char *scan)
	{
		while (*scan && *scan != '/')
		{
			scan++;
		}
		if (*scan == 0)
		{
			return nullptr;
		}
		return scan;
	}

	static bool splitPath(const char *_fileName, const char *_assetName, std::string &path, std::string &asset)
	{
		bool ret = false;
		char fileName[512];
		char assetName[512];
		strncpy(fileName, _fileName, 512);
		strncpy(assetName, _assetName, 512);
		normalizePath(fileName);
		normalizePath(assetName);
		char *prefix = strstr(assetName, "//");
		if (prefix)
		{
			prefix++; // skip the first slash but not the second slash...
			char *endOfBlock = nextSlash(prefix + 1);
			if (endOfBlock)
			{
				char specFind[512];
				char *dest = specFind;
				*dest++ = *prefix++;
				while (*prefix != '/')
				{
					*dest++ = *prefix++;
				}
				*dest++ = *prefix++;
				*dest = 0;
				char *match = strstr(fileName, specFind);
				if (match)
				{
					match += strlen(specFind)-1;
					*match = 0;
					path = std::string(fileName);
					asset = std::string(endOfBlock + 1);
					ret = true;
				}
			}
		}
		return ret;
	}

	// 157 unique element types
	enum ElementType {
		ET_ACTUATOR,
		ET_ALWAYSON,
		ET_ALWAYS_ON,
		ET_ATTACH_STEPS,
		ET_AXIS,
		ET_BODYNAME,
		ET_BOUNCE,
		ET_BOX,
		ET_BUMPERTOPICNAME,
		ET_CALIBRATION,
		ET_CAMERA,
		ET_CAMERAINFOTOPICNAME,
		ET_CAMERANAME,
		ET_CENTER,
		ET_CHARGERATE,
		ET_CHARGEVOLTAGE,
		ET_CHILD,
		ET_CLIP,
		ET_COLLISION,
		ET_COLOR,
		ET_COMPENSATOR,
		ET_CONTACT,
		ET_CX,
		ET_CXPRIME,
		ET_CY,
		ET_CYLINDER,
		ET_DAMPING,
		ET_DETACH_STEPS,
		ET_DISCHARGERATE,
		ET_DISCHARGEVOLTAGE,
		ET_DISTORTION,
		ET_DISTORTIONK1,
		ET_DISTORTIONK2,
		ET_DISTORTIONK3,
		ET_DISTORTIONT1,
		ET_DISTORTIONT2,
		ET_DYNAMICS,
		ET_FAR,
		ET_FAR_CLIP,
		ET_FILTERTEXTURENAME,
		ET_FLEXJOINT,
		ET_FOCALLENGTH,
		ET_FORMAT,
		ET_FOV,
		ET_FRAMENAME,
		ET_FRICTION,
		ET_FUDGEFACTOR,
		ET_FULLCHARGECAPACITY,
		ET_GAP_JOINT,
		ET_GAUSSIANNOISE,
		ET_GAZEBO,
		ET_GEOMETRY,
		ET_GRASP_CHECK,
		ET_GRAVITY,
		ET_GRIPPER,
		ET_GRIPPER_LINK,
		ET_HACKBASELINE,
		ET_HARDWAREINTERFACE,
		ET_HEIGHT,
		ET_HOKUYOMININTENSITY,
		ET_HORIZONTAL,
		ET_HORIZONTAL_FOV,
		ET_IMAGE,
		ET_IMAGETOPICNAME,
		ET_IMPLICITSPRINGDAMPER,
		ET_INERTIA,
		ET_INERTIAL,
		ET_IXX,
		ET_IXY,
		ET_IXZ,
		ET_IYY,
		ET_IYZ,
		ET_IZZ,
		ET_JOINT,
		ET_K1,
		ET_K2,
		ET_K3,
		ET_KD,
		ET_KP,
		ET_LEFTACTUATOR,
		ET_LIMIT,
		ET_LINK,
		ET_MASS,
		ET_MATERIAL,
		ET_MAX,
		ET_MAXVEL,
		ET_MAX_ANGLE,
		ET_MAX_CONTACTS,
		ET_MEAN,
		ET_MECHANICALREDUCTION,
		ET_MESH,
		ET_MIMIC,
		ET_MIN,
		ET_MINDEPTH,
		ET_MIN_ANGLE,
		ET_MIN_CONTACT_COUNT,
		ET_MU1,
		ET_MU2,
		ET_NEAR,
		ET_NEAR_CLIP,
		ET_NOISE,
		ET_ODE,
		ET_ORIGIN,
		ET_P1,
		ET_P2,
		ET_PALM_LINK,
		ET_PARENT,
		ET_PASSIVE_JOINT,
		ET_PLUGIN,
		ET_POLLSERVICENAME,
		ET_POSE,
		ET_POWERSTATERATE,
		ET_POWERSTATETOPIC,
		ET_PROJECTOR,
		ET_PROJECTORTOPICNAME,
		ET_PROVIDEFEEDBACK,
		ET_RANGE,
		ET_RAY,
		ET_RESOLUTION,
		ET_RIGHTACTUATOR,
		ET_ROBOT,
		ET_ROBOTNAMESPACE,
		ET_ROLLJOINT,
		ET_RPYOFFSET,
		ET_RPYOFFSETS,
		ET_SAFETY_CONTROLLER,
		ET_SAMPLES,
		ET_SCAN,
		ET_SELFCOLLIDE,
		ET_SENSOR,
		ET_SERVICENAME,
		ET_SIMULATED_ACTUATED_JOINT,
		ET_SPHERE,
		ET_STDDEV,
		ET_STOPKD,
		ET_STOPKP,
		ET_SURFACE,
		ET_TEXTURE,
		ET_TEXTURENAME,
		ET_TEXTURETOPICNAME,
		ET_THREAD_PITCH,
		ET_TIMEOUT,
		ET_TOPIC,
		ET_TOPICNAME,
		ET_TRANSMISSION,
		ET_TURNGRAVITYOFF,
		ET_TYPE,
		ET_UPDATERATE,
		ET_UPDATE_RATE,
		ET_USE_SIMULATED_GRIPPER_JOINT,
		ET_VERTICAL,
		ET_VISUAL,
		ET_VISUALIZE,
		ET_WIDTH,
		ET_XYZ,
		ET_XYZOFFSET,
		ET_XYZOFFSETS,
		ET_LAST
	};

	// 59 unique attribute types
	enum AttributeType {
		AT_A,
		AT_B,
		AT_DAMPING,
		AT_EFFORT,
		AT_FALLING,
		AT_FILENAME,
		AT_FRICTION,
		AT_GEAR_RATIO,
		AT_H,
		AT_IXX,
		AT_IXY,
		AT_IXZ,
		AT_IYY,
		AT_IYZ,
		AT_IZZ,
		AT_JOINT,
		AT_KD_MOTOR,
		AT_K_BELT,
		AT_K_POSITION,
		AT_K_VELOCITY,
		AT_L0,
		AT_LAMBDA_COMBINED,
		AT_LAMBDA_JOINT,
		AT_LAMBDA_MOTOR,
		AT_LENGTH,
		AT_LINK,
		AT_LOWER,
		AT_MASS_MOTOR,
		AT_MECHANICALREDUCTION,
		AT_MECHANICAL_REDUCTION,
		AT_MULTIPLIER,
		AT_NAME,
		AT_OFFSET,
		AT_PASSIVE_ACTUATED_JOINT,
		AT_PHI0,
		AT_R,
		AT_RADIUS,
		AT_REFERENCE,
		AT_RGBA,
		AT_RISING,
		AT_RPY,
		AT_SCALE,
		AT_SCREW_REDUCTION,
		AT_SIMULATED_REDUCTION,
		AT_SIZE,
		AT_SOFT_LOWER_LIMIT,
		AT_SOFT_UPPER_LIMIT,
		AT_T0,
		AT_THETA0,
		AT_TYPE,
		AT_UPPER,
		AT_VALUE,
		AT_VELOCITY,
		AT_XMLNS_CONTROLLER,
		AT_XMLNS_INTERFACE,
		AT_XMLNS_JOINT,
		AT_XMLNS_SENSOR,
		AT_XMLNS_XACRO,
		AT_XYZ,
		AT_LAST
	};

	struct ElementStruct
	{
		ElementType 	mType;
		const char		*mName;
	};

	static ElementStruct gElements[ET_LAST] =
	{
		ET_ACTUATOR, "actuator",
		ET_ALWAYSON, "alwaysOn",
		ET_ALWAYS_ON, "always_on",
		ET_ATTACH_STEPS, "attach_steps",
		ET_AXIS, "axis",
		ET_BODYNAME, "bodyName",
		ET_BOUNCE, "bounce",
		ET_BOX, "box",
		ET_BUMPERTOPICNAME, "bumperTopicName",
		ET_CALIBRATION, "calibration",
		ET_CAMERA, "camera",
		ET_CAMERAINFOTOPICNAME, "cameraInfoTopicName",
		ET_CAMERANAME, "cameraName",
		ET_CENTER, "center",
		ET_CHARGERATE, "chargeRate",
		ET_CHARGEVOLTAGE, "chargeVoltage",
		ET_CHILD, "child",
		ET_CLIP, "clip",
		ET_COLLISION, "collision",
		ET_COLOR, "color",
		ET_COMPENSATOR, "compensator",
		ET_CONTACT, "contact",
		ET_CX, "Cx",
		ET_CXPRIME, "CxPrime",
		ET_CY, "Cy",
		ET_CYLINDER, "cylinder",
		ET_DAMPING, "damping",
		ET_DETACH_STEPS, "detach_steps",
		ET_DISCHARGERATE, "dischargeRate",
		ET_DISCHARGEVOLTAGE, "dischargeVoltage",
		ET_DISTORTION, "distortion",
		ET_DISTORTIONK1, "distortionK1",
		ET_DISTORTIONK2, "distortionK2",
		ET_DISTORTIONK3, "distortionK3",
		ET_DISTORTIONT1, "distortionT1",
		ET_DISTORTIONT2, "distortionT2",
		ET_DYNAMICS, "dynamics",
		ET_FAR, "far",
		ET_FAR_CLIP, "far_clip",
		ET_FILTERTEXTURENAME, "filterTextureName",
		ET_FLEXJOINT, "flexJoint",
		ET_FOCALLENGTH, "focalLength",
		ET_FORMAT, "format",
		ET_FOV, "fov",
		ET_FRAMENAME, "frameName",
		ET_FRICTION, "friction",
		ET_FUDGEFACTOR, "fudgeFactor",
		ET_FULLCHARGECAPACITY, "fullChargeCapacity",
		ET_GAP_JOINT, "gap_joint",
		ET_GAUSSIANNOISE, "gaussianNoise",
		ET_GAZEBO, "gazebo",
		ET_GEOMETRY, "geometry",
		ET_GRASP_CHECK, "grasp_check",
		ET_GRAVITY, "gravity",
		ET_GRIPPER, "gripper",
		ET_GRIPPER_LINK, "gripper_link",
		ET_HACKBASELINE, "hackBaseline",
		ET_HARDWAREINTERFACE, "hardwareInterface",
		ET_HEIGHT, "height",
		ET_HOKUYOMININTENSITY, "hokuyoMinIntensity",
		ET_HORIZONTAL, "horizontal",
		ET_HORIZONTAL_FOV, "horizontal_fov",
		ET_IMAGE, "image",
		ET_IMAGETOPICNAME, "imageTopicName",
		ET_IMPLICITSPRINGDAMPER, "implicitSpringDamper",
		ET_INERTIA, "inertia",
		ET_INERTIAL, "inertial",
		ET_IXX, "ixx",
		ET_IXY, "ixy",
		ET_IXZ, "ixz",
		ET_IYY, "iyy",
		ET_IYZ, "iyz",
		ET_IZZ, "izz",
		ET_JOINT, "joint",
		ET_K1, "k1",
		ET_K2, "k2",
		ET_K3, "k3",
		ET_KD, "kd",
		ET_KP, "kp",
		ET_LEFTACTUATOR, "leftActuator",
		ET_LIMIT, "limit",
		ET_LINK, "link",
		ET_MASS, "mass",
		ET_MATERIAL, "material",
		ET_MAX, "max",
		ET_MAXVEL, "maxVel",
		ET_MAX_ANGLE, "max_angle",
		ET_MAX_CONTACTS, "max_contacts",
		ET_MEAN, "mean",
		ET_MECHANICALREDUCTION, "mechanicalReduction",
		ET_MESH, "mesh",
		ET_MIMIC, "mimic",
		ET_MIN, "min",
		ET_MINDEPTH, "minDepth",
		ET_MIN_ANGLE, "min_angle",
		ET_MIN_CONTACT_COUNT, "min_contact_count",
		ET_MU1, "mu1",
		ET_MU2, "mu2",
		ET_NEAR, "near",
		ET_NEAR_CLIP, "near_clip",
		ET_NOISE, "noise",
		ET_ODE, "ode",
		ET_ORIGIN, "origin",
		ET_P1, "p1",
		ET_P2, "p2",
		ET_PALM_LINK, "palm_link",
		ET_PARENT, "parent",
		ET_PASSIVE_JOINT, "passive_joint",
		ET_PLUGIN, "plugin",
		ET_POLLSERVICENAME, "pollServiceName",
		ET_POSE, "pose",
		ET_POWERSTATERATE, "powerStateRate",
		ET_POWERSTATETOPIC, "powerStateTopic",
		ET_PROJECTOR, "projector",
		ET_PROJECTORTOPICNAME, "projectorTopicName",
		ET_PROVIDEFEEDBACK, "provideFeedback",
		ET_RANGE, "range",
		ET_RAY, "ray",
		ET_RESOLUTION, "resolution",
		ET_RIGHTACTUATOR, "rightActuator",
		ET_ROBOT, "robot",
		ET_ROBOTNAMESPACE, "robotNamespace",
		ET_ROLLJOINT, "rollJoint",
		ET_RPYOFFSET, "rpyOffset",
		ET_RPYOFFSETS, "rpyOffsets",
		ET_SAFETY_CONTROLLER, "safety_controller",
		ET_SAMPLES, "samples",
		ET_SCAN, "scan",
		ET_SELFCOLLIDE, "selfCollide",
		ET_SENSOR, "sensor",
		ET_SERVICENAME, "serviceName",
		ET_SIMULATED_ACTUATED_JOINT, "simulated_actuated_joint",
		ET_SPHERE, "sphere",
		ET_STDDEV, "stddev",
		ET_STOPKD, "stopKd",
		ET_STOPKP, "stopKp",
		ET_SURFACE, "surface",
		ET_TEXTURE, "texture",
		ET_TEXTURENAME, "textureName",
		ET_TEXTURETOPICNAME, "textureTopicName",
		ET_THREAD_PITCH, "thread_pitch",
		ET_TIMEOUT, "timeout",
		ET_TOPIC, "topic",
		ET_TOPICNAME, "topicName",
		ET_TRANSMISSION, "transmission",
		ET_TURNGRAVITYOFF, "turnGravityOff",
		ET_TYPE, "type",
		ET_UPDATERATE, "updateRate",
		ET_UPDATE_RATE, "update_rate",
		ET_USE_SIMULATED_GRIPPER_JOINT, "use_simulated_gripper_joint",
		ET_VERTICAL, "vertical",
		ET_VISUAL, "visual",
		ET_VISUALIZE, "visualize",
		ET_WIDTH, "width",
		ET_XYZ, "xyz",
		ET_XYZOFFSET, "xyzOffset",
		ET_XYZOFFSETS, "xyzOffsets",
	};

	struct AttributeStruct
	{
		AttributeType mType;
		const char	  *mName;
	};

	static AttributeStruct gAttributes[AT_LAST] =
	{
		AT_A, "a",
		AT_B, "b",
		AT_DAMPING, "damping",
		AT_EFFORT, "effort",
		AT_FALLING, "falling",
		AT_FILENAME, "filename",
		AT_FRICTION, "friction",
		AT_GEAR_RATIO, "gear_ratio",
		AT_H, "h",
		AT_IXX, "ixx",
		AT_IXY, "ixy",
		AT_IXZ, "ixz",
		AT_IYY, "iyy",
		AT_IYZ, "iyz",
		AT_IZZ, "izz",
		AT_JOINT, "joint",
		AT_KD_MOTOR, "kd_motor",
		AT_K_BELT, "k_belt",
		AT_K_POSITION, "k_position",
		AT_K_VELOCITY, "k_velocity",
		AT_L0, "L0",
		AT_LAMBDA_COMBINED, "lambda_combined",
		AT_LAMBDA_JOINT, "lambda_joint",
		AT_LAMBDA_MOTOR, "lambda_motor",
		AT_LENGTH, "length",
		AT_LINK, "link",
		AT_LOWER, "lower",
		AT_MASS_MOTOR, "mass_motor",
		AT_MECHANICALREDUCTION, "mechanicalReduction",
		AT_MECHANICAL_REDUCTION, "mechanical_reduction",
		AT_MULTIPLIER, "multiplier",
		AT_NAME, "name",
		AT_OFFSET, "offset",
		AT_PASSIVE_ACTUATED_JOINT, "passive_actuated_joint",
		AT_PHI0, "phi0",
		AT_R, "r",
		AT_RADIUS, "radius",
		AT_REFERENCE, "reference",
		AT_RGBA, "rgba",
		AT_RISING, "rising",
		AT_RPY, "rpy",
		AT_SCALE, "scale",
		AT_SCREW_REDUCTION, "screw_reduction",
		AT_SIMULATED_REDUCTION, "simulated_reduction",
		AT_SIZE, "size",
		AT_SOFT_LOWER_LIMIT, "soft_lower_limit",
		AT_SOFT_UPPER_LIMIT, "soft_upper_limit",
		AT_T0, "t0",
		AT_THETA0, "theta0",
		AT_TYPE, "type",
		AT_UPPER, "upper",
		AT_VALUE, "value",
		AT_VELOCITY, "velocity",
		AT_XMLNS_CONTROLLER, "xmlns:controller",
		AT_XMLNS_INTERFACE, "xmlns:interface",
		AT_XMLNS_JOINT, "xmlns:joint",
		AT_XMLNS_SENSOR, "xmlns:sensor",
		AT_XMLNS_XACRO, "xmlns:xacro",
		AT_XYZ, "xyz",
	};

	typedef std::unordered_map< std::string, ElementType > ElementTypeMap;
	typedef std::unordered_map< std::string, AttributeType > AttributeTypeMap;

	ElementTypeMap gElementsMap;
	AttributeTypeMap gAttributesMap;



	static void initMaps(void)
	{
		for (auto &i : gElements)
		{
			gElementsMap[std::string(i.mName)] = i.mType;
		}
		for (auto &i : gAttributes)
		{
			gAttributesMap[std::string(i.mName)] = i.mType;
		}
	}

	static ElementType getElementType(const char *str)
	{
		ElementType ret = ET_LAST;

		ElementTypeMap::iterator found = gElementsMap.find(std::string(str));
		if (found != gElementsMap.end())
		{
			ret = (*found).second;
		}
		return ret;
	}

	static AttributeType getAttributeType(const char *str)
	{
		AttributeType ret = AT_LAST;

		AttributeTypeMap::iterator found = gAttributesMap.find(std::string(str));
		if (found != gAttributesMap.end())
		{
			ret = (*found).second;
		}
		return ret;
	}
	static const char *getElementName(ElementType t)
	{
		const char *ret = "**UNKONWN-ELEMENT-TYPE**";
		if (t < ET_LAST)
		{
			ret = gElements[t].mName;
		}
		return ret;
	}

	static const char *getAttributeName(AttributeType t)
	{
		const char *ret = "**UNKONWN-ATTRIBUTE-TYPE**";
		if (t < AT_LAST)
		{
			ret = gAttributes[t].mName;
		}
		return ret;
	}


	static Joint::Type getJointType(const char *str)
	{
		Joint::Type ret = Joint::JT_LAST;

		if (strcmp(str, "revolute") == 0)
		{
			ret = Joint::JT_REVOLUTE;
		}
		else if (strcmp(str, "continuous") == 0)
		{
			ret = Joint::JT_CONTINUOUS;
		}
		else if (strcmp(str, "prismatic") == 0)
		{
			ret = Joint::JT_PRISMATIC;
		}
		else if (strcmp(str, "fixed") == 0)
		{
			ret = Joint::JT_FIXED;
		}
		else if (strcmp(str, "floating") == 0)
		{
			ret = Joint::JT_FLOATING;
		}
		else if (strcmp(str, "planar") == 0)
		{
			ret = Joint::JT_PLANAR;
		}

		return ret;
	}

class ImportURDFImpl :public ImportURDF, public FAST_XML::FastXml::Callback
{
public:
	ImportURDFImpl(void)
	{
		initMaps(); // initialize the lookup tables for elements and attributes
	}

	virtual ~ImportURDFImpl(void)
	{

	}

	void reportError(uint32_t lineno, const char *fmt, ...)
	{
		va_list         args;
		char            buffer[4096];
		va_start(args, fmt);
		STRING_HELPER::stringFormatV(buffer, sizeof(buffer), fmt, args);
		va_end(args);
		printf("[ImportURDF:ERROR:Lineno:%d]%s\n", lineno, buffer);
	}

	void attributeCheck(ElementType type,uint32_t lineno, uint32_t acount, const char **argv)
	{
		if (acount == 0) return;
		reportError(lineno, "Encountered %d attribute(s) for element <%s> when none were expected.", acount, getElementName(type));
		for (uint32_t i = 0; i < acount; i++)
		{
			const char *key = argv[i * 2 + 0];
			const char *value = argv[i * 2 + 1];
			AttributeType at = getAttributeType(key);
			reportError(lineno, "Attr[%d] %s=%s", i, key, value);
			if (at == AT_LAST)
			{
				reportError(lineno, "Attribute '%s' not recognized.", key);
			}
		}
	}

	void nestingError(uint32_t lineno,ElementType type, ElementType expected, ElementType parent)
	{
		reportError(lineno, "Nesting error <%s> expected parent <%s> (or others) but found <%s>",
			getElementName(type),
			getElementName(expected),
			getElementName(parent));
	}

	// return true to continue processing the XML document, false to skip.
	virtual bool processElement(
		const char *elementName,   // name of the element
		uint32_t argc,         // number of attributes pairs
		const char **argv,         // list of attributes.
		const char  *elementData,  // element data, null if none
		uint32_t lineno) final  // line number in the source XML file
	{
		if (argc & 1) // if it's odd
		{
			reportError(lineno, "Attribute key/value pair mismatch");
			argc--;
		}
		uint32_t acount = argc / 2;
		mCurrentType = getElementType(elementName);
		mPreviousPreviousType = mStackLocation >= 2 ? mTypeStack[mStackLocation - 2] : ET_LAST;
		mPreviousType = mStackLocation ? mTypeStack[mStackLocation - 1] : ET_LAST;
		mTypeStack[mStackLocation] = mCurrentType;
		mStackLocation++;
		if (mStackLocation > MAX_STACK)
		{
			mStackLocation = MAX_STACK;
			reportError(lineno, "ElementTypes nested too deeply!");
		}


		switch ( mCurrentType )
		{
			case ET_BOUNCE:
			case ET_CENTER:
			case ET_DISTORTION:
			case ET_FRICTION:
			case ET_HARDWAREINTERFACE:
			case ET_IMPLICITSPRINGDAMPER:
			case ET_K1:
			case ET_K2:
			case ET_K3:
			case ET_MAX_CONTACTS:
			case ET_MEAN:
			case ET_NOISE:
			case ET_ODE:
			case ET_P1:
			case ET_P2:
			case ET_ROBOTNAMESPACE:
			case ET_STDDEV:
			case ET_SURFACE:
			case ET_TOPIC:
			case ET_TYPE:
			case ET_VERTICAL:
				reportError(lineno, "Element type (%s) not yet implemented", elementName);
				break;
			// These are all key/value pair property items
			// They are not fully parsed into an object model; simply saved as an 
			// bag of properties for the current gazebo plugin 
			case ET_ALWAYSON:
			case ET_ALWAYS_ON:
			case ET_UPDATERATE:
			case ET_UPDATE_RATE:
			case ET_TIMEOUT:
			case ET_POWERSTATERATE:
			case ET_POWERSTATETOPIC:
			case ET_FULLCHARGECAPACITY:
			case ET_DISCHARGERATE:
			case ET_CHARGERATE:
			case ET_DISCHARGEVOLTAGE:
			case ET_CHARGEVOLTAGE:
			case ET_GAUSSIANNOISE:
			case ET_TOPICNAME:
			case ET_FRAMENAME:
			case ET_HOKUYOMININTENSITY:
			case ET_MU1:
			case ET_MU2:
			case ET_KD:
			case ET_KP:
			case ET_MAXVEL:
			case ET_MINDEPTH:
			case ET_SELFCOLLIDE:
			case ET_BUMPERTOPICNAME:
			case ET_BODYNAME:
			case ET_XYZOFFSETS:
			case ET_RPYOFFSETS:
			case ET_XYZOFFSET:
			case ET_RPYOFFSET:
			case ET_SERVICENAME:
			case ET_CAMERANAME:
			case ET_IMAGETOPICNAME:
			case ET_CAMERAINFOTOPICNAME:
			case ET_POLLSERVICENAME:
			case ET_CXPRIME:
			case ET_CX:
			case ET_CY:
			case ET_FOCALLENGTH:
			case ET_DISTORTIONK1:
			case ET_DISTORTIONK2:
			case ET_DISTORTIONK3:
			case ET_DISTORTIONT1:
			case ET_DISTORTIONT2:
			case ET_HACKBASELINE:
			case ET_TURNGRAVITYOFF:
			case ET_TEXTURENAME:
			case ET_FILTERTEXTURENAME:
			case ET_TEXTURETOPICNAME:
			case ET_PROJECTORTOPICNAME:
			case ET_STOPKD:
			case ET_STOPKP:
			case ET_FUDGEFACTOR:
			case ET_PROVIDEFEEDBACK:
				if (mPreviousType == ET_PLUGIN)
				{
					attributeCheck(mCurrentType, lineno, acount, argv);
					KeyValuePair kvp;
					kvp.mKey = std::string(elementName);
					if (elementData)
					{
						kvp.mValue = std::string(elementData);
					}
					mCurrentPlugin.mKeyValuePairs.push_back(kvp);
				}
				else if (mPreviousType == ET_GAZEBO)
				{
					KeyValuePair kvp;
					kvp.mKey = std::string(elementName);
					if (acount)
					{
						for (uint32_t i = 0; i < acount; i++)
						{
							const char *key = argv[i * 2 + 0];
							const char *value = argv[i * 2 + 1];
							AttributeType at = getAttributeType(key);
							switch (at)
							{
							case AT_VALUE:
								kvp.mValue = std::string(value);
								break;
							default:
								reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
								break;
							}
						}
					}
					else if (elementData)
					{
						kvp.mValue = std::string(elementData);
					}
					mCurrentGazebo.mKeyValuePairs.push_back(kvp);
				}
				else if (mPreviousType == ET_SENSOR)
				{
					attributeCheck(mCurrentType, lineno, acount, argv);
					switch (mCurrentType)
					{
					case ET_ALWAYS_ON:
						mCurrentSensor.mAlwaysOn = STRING_HELPER::getBool(elementData);
						break;
					case ET_UPDATE_RATE:
						mCurrentSensor.mUpdateRate = STRING_HELPER::getFloatValue(elementData, nullptr);
						break;
					default:
						nestingError(lineno, mCurrentType, ET_SENSOR, mPreviousType);
						break;
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_PLUGIN, mPreviousType);
				}
				break;
			case ET_USE_SIMULATED_GRIPPER_JOINT:
				attributeCheck(mCurrentType, lineno, acount, argv); // Not expecting attributes
				if (mPreviousType == ET_TRANSMISSION)
				{
					if (elementData && strlen(elementData))
					{
						mCurrentTransmission.mUseSimulatedGripperJoint = STRING_HELPER::getBool(elementData);
					}
					else
					{
						mCurrentTransmission.mUseSimulatedGripperJoint = true;
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_TRANSMISSION, mPreviousType);
				}
				break;
			case ET_PASSIVE_JOINT:
				if (mPreviousType == ET_TRANSMISSION)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_NAME:
							mCurrentTransmission.mPassiveJoints.push_back(std::string(value));
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}

				}
				else
				{
					nestingError(lineno, mCurrentType, ET_TRANSMISSION, mPreviousType);
				}
				break;
			case ET_GAP_JOINT:
				if (mPreviousType == ET_TRANSMISSION)
				{
					GapJoint j;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						float v = STRING_HELPER::getFloatValue(value, nullptr);
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_L0:
							j.mL0 = v;
							break;
						case AT_A:
							j.mA = v;
							break;
						case AT_B:
							j.mB = v;
							break;
						case AT_GEAR_RATIO:
							j.mGearRatio = v;
							break;
						case AT_H:
							j.mH = v;
							break;
						case AT_NAME:
							j.mName = std::string(value);
							break;
						case AT_PHI0:
							j.mPhi0 = v;
							break;
						case AT_R:
							j.mR = v;
							break;
						case AT_SCREW_REDUCTION:
							j.mScrewReduction = v;
							break;
						case AT_T0:
							j.mT0 = v;
							break;
						case AT_THETA0:
							j.mTheta0 = v;
							break;
						case AT_MECHANICAL_REDUCTION:
							j.mMechanicalReduction = v;
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
					mCurrentTransmission.mGapJoints.push_back(j);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_TRANSMISSION, mPreviousType);
				}
				break;
			case ET_DAMPING:
				attributeCheck(mCurrentType, lineno, acount, argv); // Not expecting attributes
				if (mPreviousType == ET_DYNAMICS)
				{
					if (mPreviousPreviousType == ET_AXIS)
					{
						mCurrentDynamics.mDamping = STRING_HELPER::getFloatValue(elementData, nullptr);
					}
					else
					{
						nestingError(lineno, mCurrentType, ET_AXIS, mPreviousPreviousType);
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_DYNAMICS, mPreviousType);
				}
				break;
			case ET_ATTACH_STEPS:
			case ET_DETACH_STEPS:
			case ET_MIN_CONTACT_COUNT:
				if (mPreviousType == ET_GRASP_CHECK)
				{
					uint32_t count = (uint32_t)atoi(elementData);
					switch (mCurrentType)
					{
						case ET_ATTACH_STEPS:
							mCurrentGraspCheck.mAttachSteps = count;
							break;
						case ET_DETACH_STEPS:
							mCurrentGraspCheck.mDetachSteps = count;
							break;
						case ET_MIN_CONTACT_COUNT:
							mCurrentGraspCheck.mMinContactCount = count;
							break;
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_GRIPPER, mPreviousType);
				}
				break;
			case ET_GRIPPER_LINK:
			case ET_PALM_LINK:
				attributeCheck(mCurrentType, lineno, acount, argv); // Not expecting attributes
				if (mPreviousType == ET_GRIPPER)
				{
					if ( mCurrentType == ET_GRIPPER_LINK)
						mCurrentGripperLink.mType = GripperLink::GL_GRIPPER_LINK;
					else
						mCurrentGripperLink.mType = GripperLink::GL_PALM_LINK;
					mCurrentGripperLink.mLink = std::string(elementData);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_GRIPPER, mPreviousType);
				}
				break;
			case ET_GRASP_CHECK:
				attributeCheck(mCurrentType, lineno, acount, argv); // Not expecting attributes
				if (mPreviousType == ET_GRIPPER)
				{
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_GRIPPER, mPreviousType);
				}
				break;
			case ET_GRIPPER:
				if (mPreviousType == ET_GAZEBO)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_NAME:
							mCurrentGripper.mName = std::string(value);
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_GAZEBO, mPreviousType);
				}
				break;
			case ET_IXX:
			case ET_IXY:
			case ET_IXZ:
			case ET_IYY:
			case ET_IYZ:
			case ET_IZZ:
				attributeCheck(mCurrentType, lineno, acount, argv); // Not expecting attributes
				if (mPreviousType == ET_INERTIA)
				{
					if ( mPreviousPreviousType == ET_INERTIAL )
					{
						float v = STRING_HELPER::getFloatValue(elementData, nullptr);
						switch (mCurrentType)
						{
						case ET_IXX:
							mCurrentLink.mInertial.mInertia.mIXX = v;
							break;
						case ET_IXY:
							mCurrentLink.mInertial.mInertia.mIXY = v;
							break;
						case ET_IXZ:
							mCurrentLink.mInertial.mInertia.mIXZ = v;
							break;
						case ET_IYY:
							mCurrentLink.mInertial.mInertia.mIYY = v;
							break;
						case ET_IYZ:
							mCurrentLink.mInertial.mInertia.mIYZ = v;
							break;
						case ET_IZZ:
							mCurrentLink.mInertial.mInertia.mIZZ = v;
							break;
						}
					}
					else
					{
						reportError(lineno, "Expected to be nested within an <inertial> element.");
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_INERTIA, mPreviousType);
				}
				break;
			case ET_GRAVITY:
				attributeCheck(mCurrentType, lineno, acount, argv); // Don't expect attributes on a gravity element
				if (mPreviousType == ET_LINK)
				{
					mCurrentLink.mGravity = STRING_HELPER::getBool(elementData);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_INERTIAL, mPreviousType);
				}
				break;
			case ET_MIMIC:
				if (mPreviousType == ET_JOINT)
				{
					Mimic m;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_JOINT:
							m.mJoint = std::string(value);
							break;
						case AT_MULTIPLIER:
							m.mMultiplier = STRING_HELPER::getFloatValue(value, nullptr);
							break;
						case AT_OFFSET:
							m.mOffset = STRING_HELPER::getFloatValue(value, nullptr);
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
					mCurrentJoint.mMimics.push_back(m);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_JOINT, mPreviousType);
				}
				break;
			case ET_LEFTACTUATOR:
			case ET_RIGHTACTUATOR:
			case ET_FLEXJOINT:
			case ET_ROLLJOINT:
				if (mPreviousType == ET_TRANSMISSION)
				{
					Actuator a;
					switch (mCurrentType)
					{
						case ET_LEFTACTUATOR:
							a.mType = Actuator::AT_LEFT_ACTUATOR;
							break;
						case ET_RIGHTACTUATOR:
							a.mType = Actuator::AT_RIGHT_ACTUATOR;
							break;
						case ET_FLEXJOINT:
							a.mType = Actuator::AT_FLEX_JOINT;
							break;
						case ET_ROLLJOINT:
							a.mType = Actuator::AT_ROLL_JOINT;
							break;
						default:
							reportError(lineno, "Unknown actuator type '%s'", elementName);
							break;
					}
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_NAME:
							a.mName = std::string(value);
							break;
						case AT_MECHANICALREDUCTION:
							a.mMechanicalReduction = STRING_HELPER::getFloatValue(value, nullptr);
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
					mCurrentTransmission.mActuators.push_back(a);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_TRANSMISSION, mPreviousType);
				}
				break;
			case ET_COMPENSATOR:
				if (mPreviousType == ET_TRANSMISSION)
				{
					Compensator c;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						float v = STRING_HELPER::getFloatValue(value, nullptr);
						switch (at)
						{
							case AT_K_BELT:
								c.mKBelt = v;
								break;
							case AT_KD_MOTOR:
								c.mKDMotor = v;
								break;
							case AT_LAMBDA_COMBINED:
								c.mLambdaCombined = v;
								break;
							case AT_LAMBDA_MOTOR:
								c.mLambdaMotor = v;
								break;
							case AT_LAMBDA_JOINT:
								c.mLambdaJoint = v;
								break;
							case AT_MASS_MOTOR:
								c.mMassMotor = v;
								break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
					mCurrentTransmission.mCompensators.push_back(c);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_PLUGIN, mPreviousType);
				}
				break;
			case ET_XYZ:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_AXIS )
				{
					if (mPreviousPreviousType == ET_JOINT)
					{
						STRING_HELPER::getVec3(elementData, nullptr,
							mCurrentJoint.mAxis.mAxis.x,
							mCurrentJoint.mAxis.mAxis.y,
							mCurrentJoint.mAxis.mAxis.z);
					}
					else
					{
						reportError(lineno, "Expected <axis> element to be under <joint> element.");
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_JOINT, mPreviousType);
				}
				break;
			case ET_FOV:
			case ET_NEAR_CLIP:
			case ET_FAR_CLIP:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_PROJECTOR)
				{
					float v = STRING_HELPER::getFloatValue(elementData, nullptr);
					switch (mCurrentType)
					{
					case ET_NEAR_CLIP:
						mCurrentProjector.mNearClip = v;
						break;
					case ET_FAR_CLIP:
						mCurrentProjector.mFarClip = v;
						break;
					case ET_FOV:
						mCurrentProjector.mFOV = v;
						break;
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_PROJECTOR, mPreviousType);
				}
				break;
			case ET_CLIP:
			case ET_IMAGE:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_CAMERA)
				{
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_CAMERA, mPreviousType);
				}
				break;
			case ET_NEAR:
			case ET_FAR:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_CLIP)
				{
					if (mPreviousPreviousType == ET_CAMERA)
					{
						switch (mCurrentType)
						{
						case ET_NEAR:
							mCurrentCamera.mClip.mNear = STRING_HELPER::getFloatValue(elementData, nullptr);
							break;
						case ET_FAR:
							mCurrentCamera.mClip.mFar = STRING_HELPER::getFloatValue(elementData, nullptr);
							break;
						}
					}
					else
					{
						reportError(lineno, "Expected element <%s> to be within an <clip> element and parent of <camera> but found <%s>",
							elementName, getElementName(mPreviousPreviousType));
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_IMAGE, mPreviousType);
				}
				break;
			case ET_FORMAT:
			case ET_WIDTH:
			case ET_HEIGHT:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_IMAGE)
				{
					if (mPreviousPreviousType == ET_CAMERA)
					{
						switch (mCurrentType)
						{
						case ET_FORMAT:
							mCurrentCamera.mImage.mFormat = std::string(elementData);
							break;
						case ET_WIDTH:
							mCurrentCamera.mImage.mWidth = (uint32_t)atoi(elementData);
							break;
						case ET_HEIGHT:
							mCurrentCamera.mImage.mHeight = (uint32_t)atoi(elementData);
							break;
						}
					}
					else
					{
						reportError(lineno, "Expected element <%s> to be within an <image> element and parent of <camera> but found <%s>",
							elementName, getElementName(mPreviousPreviousType));
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_IMAGE, mPreviousType);
				}
				break;
			case ET_PROJECTOR:
				if (mPreviousType == ET_GAZEBO)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_NAME:
							mCurrentProjector.mName = std::string(value);
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else if (mPreviousType == ET_PLUGIN) // if it's just the projector property on a plugin
				{
					attributeCheck(mCurrentType, lineno, acount, argv); // We don't expect attributes in this case
					KeyValuePair kvp;
					kvp.mKey = std::string(elementName);
					kvp.mValue = std::string(elementData);
					mCurrentPlugin.mKeyValuePairs.push_back(kvp); // add the projector key/value pair
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_GAZEBO, mPreviousType);
				}
				break;
			case ET_CAMERA:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_SENSOR)
				{
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_SENSOR, mPreviousType);
				}
				break;
			case ET_HORIZONTAL_FOV:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_CAMERA)
				{
					mCurrentCamera.mHorizontalFOV = STRING_HELPER::getFloatValue(elementData, nullptr);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_CAMERA, mPreviousType);
				}
				break;
			case ET_CONTACT:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_SENSOR)
				{

				}
				else
				{
					nestingError(lineno, mCurrentType, ET_SENSOR, mPreviousType);
				}
				break;
			case ET_MECHANICALREDUCTION:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_TRANSMISSION)
				{
					mCurrentTransmission.mMechanicalReduction = STRING_HELPER::getFloatValue(elementData, nullptr);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_TRANSMISSION, mPreviousType);
				}
				break;
			case ET_ACTUATOR:
				if (mPreviousType == ET_TRANSMISSION)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_NAME:
							mCurrentTransmission.mActuator = std::string(value);
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_TRANSMISSION, mPreviousType);
				}
				break;
			case ET_SIMULATED_ACTUATED_JOINT:
				if (mPreviousType == ET_TRANSMISSION)
				{
					SimulatedActuatedJoint j;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_NAME:
							j.mName = std::string(value);
							break;
						case AT_SIMULATED_REDUCTION:
							j.mSimulatedReduction = STRING_HELPER::getFloatValue(value, nullptr);
							break;
						case AT_PASSIVE_ACTUATED_JOINT:
							j.mPassiveActuatedJoint = std::string(value);
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
					mCurrentTransmission.mSimulatedActuatedJoints.push_back(j);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_TRANSMISSION, mPreviousType);
				}
				break;
			case ET_MIN:
			case ET_MAX:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_RANGE)
				{
					float v = STRING_HELPER::getFloatValue(elementData, nullptr);
					switch (mCurrentType)
					{
					case ET_MIN:
						mCurrentSensor.mRay.mScan.mRange.mMin = v;
						break;
					case ET_MAX:
						mCurrentSensor.mRay.mScan.mRange.mMax = v;
						break;
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_RANGE, mPreviousType);
				}
				break;
			case ET_SAFETY_CONTROLLER:
				if (mPreviousType == ET_JOINT)
				{
					SafetyController c;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						float v = STRING_HELPER::getFloatValue(value, nullptr);
						switch (at)
						{
						case AT_K_VELOCITY:
							c.mKVelocity = v;
							break;
						case AT_K_POSITION:
							c.mKPosition = v;
							break;
						case AT_SOFT_LOWER_LIMIT:
							c.mSoftLowerLimit = v;
							break;
						case AT_SOFT_UPPER_LIMIT:
							c.mSoftUpperLimit = v;
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
					mCurrentJoint.mSafetyControllers.push_back(c);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_JOINT, mPreviousType);
				}
				break;
			case ET_DYNAMICS:
				if (mPreviousType == ET_JOINT)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_DAMPING:
							mCurrentDynamics.mDamping = STRING_HELPER::getFloatValue(value, nullptr);
							break;
						case AT_FRICTION:
							mCurrentDynamics.mFriction = STRING_HELPER::getFloatValue(value, nullptr);
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else if (mPreviousType == ET_AXIS)
				{
					// dynamics field under axis is allowed
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_JOINT, mPreviousType);
				}
				break;
			case ET_CALIBRATION:
				if (mPreviousType == ET_JOINT)
				{
					Calibration c;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						float v = STRING_HELPER::getFloatValue(value, nullptr);
						switch (at)
						{
						case AT_RISING:
							c.mRising = v;
							break;
						case AT_FALLING:
							c.mFalling = v;
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
					mCurrentJoint.mCalibrations.push_back(c);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_JOINT, mPreviousType);
				}
				break;
			case ET_LIMIT:
				if (mPreviousType == ET_JOINT)
				{
					Limit l;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_EFFORT:
							l.mEffort = STRING_HELPER::getFloatValue(value, nullptr);
							break;
						case AT_VELOCITY:
							l.mVelocity = STRING_HELPER::getFloatValue(value, nullptr);
							break;
						case AT_LOWER:
							l.mLower = STRING_HELPER::getFloatValue(value, nullptr);
							break;
						case AT_UPPER:
							l.mUpper = STRING_HELPER::getFloatValue(value, nullptr);
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
					mCurrentJoint.mLimits.push_back(l);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_JOINT, mPreviousType);
				}
				break;
			case ET_RANGE:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_RAY )
				{

				}
				else
				{
					nestingError(lineno, mCurrentType, ET_SCAN, mPreviousType);
				}
				break;
			case ET_MAX_ANGLE:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_HORIZONTAL)
				{
					mCurrentSensor.mRay.mScan.mHorizontal.mMaxAngle = STRING_HELPER::getFloatValue(elementData, nullptr);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_HORIZONTAL, mPreviousType);
				}
				break;
			case ET_MIN_ANGLE:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_HORIZONTAL)
				{
					mCurrentSensor.mRay.mScan.mHorizontal.mMinAngle = STRING_HELPER::getFloatValue(elementData, nullptr);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_HORIZONTAL, mPreviousType);
				}
				break;
			case ET_RESOLUTION:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_HORIZONTAL)
				{
					mCurrentSensor.mRay.mScan.mHorizontal.mResolution = STRING_HELPER::getFloatValue(elementData, nullptr);
				}
				else if (mPreviousType == ET_RANGE)
				{
					if (mPreviousPreviousType == ET_RAY)
					{
						mCurrentSensor.mRay.mScan.mRange.mResolution = STRING_HELPER::getFloatValue(elementData, nullptr);
					}
					else
					{
						nestingError(lineno, mCurrentType, ET_RAY, mPreviousType);
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_HORIZONTAL, mPreviousType);
				}
				break;
			case ET_SAMPLES:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_HORIZONTAL)
				{
					mCurrentSensor.mRay.mScan.mHorizontal.mSamples = STRING_HELPER::getFloatValue(elementData,nullptr);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_HORIZONTAL, mPreviousType);
				}
				break;
			case ET_HORIZONTAL:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_SCAN)
				{

				}
				else
				{
					nestingError(lineno, mCurrentType, ET_HORIZONTAL, mPreviousType);
				}
				break;
			case ET_SCAN:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_RAY)
				{

				}
				else
				{
					nestingError(lineno, mCurrentType, ET_SCAN, mPreviousType);
				}
				break;
			case ET_RAY:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_SENSOR)
				{

				}
				else
				{
					nestingError(lineno, mCurrentType, ET_RAY, mPreviousType);
				}
				break;
			case ET_VISUALIZE:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_SENSOR)
				{
					mCurrentSensor.mVisualize = STRING_HELPER::getBool(elementData);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_VISUALIZE, mPreviousType);
				}
				break;
			case ET_POSE:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_SENSOR || mPreviousType == ET_PROJECTOR || mPreviousType == ET_INERTIAL || mPreviousType == ET_LINK || mPreviousType == ET_JOINT)
				{
					Pose *dest = nullptr;
					switch (mPreviousType)
					{
					case ET_SENSOR:
						dest = &mCurrentSensor.mPose;
						break;
					case ET_PROJECTOR:
						dest = &mCurrentProjector.mPose;
						break;
					case ET_INERTIAL:
						dest = &mCurrentLink.mInertial.mPose;
						break;
					case ET_LINK:
						dest = &mCurrentLink.mPose;
						break;
					case ET_JOINT:
						dest = &mCurrentJoint.mPose;
						break;
					}

					const char *next = nullptr;
					const char *scan = elementData;
					for (uint32_t i = 0; i < 6; i++)
					{
						float v = STRING_HELPER::getFloatValue(scan, &next);
						if (next == nullptr)
						{
							break;
						}
						switch (i)
						{
							case 0:
								dest->mPosition.x = v;
								break;
							case 1:
								dest->mPosition.y = v;
								break;
							case 2:
								dest->mPosition.z = v;
								break;
							case 3:
								dest->mRPY.x = v;
								break;
							case 4:
								dest->mRPY.y = v;
								break;
							case 5:
								dest->mRPY.z = v;
								break;
						}
						scan = next;
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_POSE, mPreviousType);
				}
				break;
			case ET_SENSOR:
				if (mPreviousType == ET_GAZEBO)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_NAME:
							mCurrentSensor.mName = std::string(value);
							break;
						case AT_TYPE:
							mCurrentSensor.mType = std::string(value);
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_GAZEBO, mPreviousType);
				}
				break;
			case ET_AXIS:
				if (mPreviousType == ET_JOINT)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_XYZ:
							STRING_HELPER::getVec3(value, nullptr, mCurrentJoint.mAxis.mAxis.x, mCurrentJoint.mAxis.mAxis.y, mCurrentJoint.mAxis.mAxis.z);
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_JOINT, mPreviousType);
				}
				break;
			case ET_THREAD_PITCH:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_JOINT)
				{
					attributeCheck(mCurrentType,lineno, acount, argv);
					mCurrentJoint.mThreadPitch = STRING_HELPER::getFloatValue(elementData, nullptr);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_JOINT, mPreviousType);
				}
				break;
			case ET_CHILD:
			case ET_PARENT:
				if (mPreviousType == ET_JOINT)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_LINK:
							if (mCurrentType == ET_CHILD)
							{
								mCurrentJoint.mChild = std::string(value);
							}
							else
							{
								mCurrentJoint.mParent = std::string(value);
							}
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_JOINT, mPreviousType);
				}
				break;
			case ET_JOINT:
				if (mPreviousType == ET_ROBOT || mPreviousType == ET_GAZEBO )
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
							case AT_NAME:
								mCurrentJoint.mName = std::string(value);
								break;
							case AT_TYPE:
								mCurrentJoint.mType = getJointType(value);
								break;
							default:
								reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
								break;
						}
					}
				}
				else if (mPreviousType == ET_TRANSMISSION)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_NAME:
							mCurrentTransmission.mJoints.push_back(std::string(value));
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_ROBOT, mPreviousType);
				}
				break;
			case ET_CYLINDER:
				if (mPreviousType == ET_GEOMETRY)
				{
					Representation *rep = nullptr;
					switch (mPreviousPreviousType)
					{
					case ET_VISUAL:
						rep = &mCurrentLink.mVisual;
						break;
					case ET_COLLISION:
						rep = &mCurrentLink.mCollision;
						break;
					default:
						reportError(lineno, "Encountered a geometry with unknown parent type (%s)", getElementName(mPreviousPreviousType));
						break;
					}
					if (rep)
					{
						rep->mGeometry.mType = Geometry::GT_CYLINDER;
					}
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_LENGTH:
							if (rep)
							{
								rep->mGeometry.mDimensions.y = STRING_HELPER::getFloatValue(value, nullptr);
							}
							break;
						case AT_RADIUS:
							if (rep)
							{
								rep->mGeometry.mDimensions.x = STRING_HELPER::getFloatValue(value, nullptr);
							}
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_GEOMETRY, mPreviousType);
				}
				break;
			case ET_SPHERE:
				if (mPreviousType == ET_GEOMETRY)
				{
					Representation *rep = nullptr;
					switch (mPreviousPreviousType)
					{
					case ET_VISUAL:
						rep = &mCurrentLink.mVisual;
						break;
					case ET_COLLISION:
						rep = &mCurrentLink.mCollision;
						break;
					default:
						reportError(lineno, "Encountered a geometry with unknown parent type (%s)", getElementName(mPreviousPreviousType));
						break;
					}
					if (rep)
					{
						rep->mGeometry.mType = Geometry::GT_SPHERE;
					}
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_RADIUS:
							if (rep)
							{
								rep->mGeometry.mDimensions.x = STRING_HELPER::getFloatValue(value, nullptr);
							}
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_GEOMETRY, mPreviousType);
				}
				break;
			case ET_BOX:
				if (mPreviousType == ET_GEOMETRY)
				{
					Representation *rep = nullptr;
					switch (mPreviousPreviousType)
					{
					case ET_VISUAL:
						rep = &mCurrentLink.mVisual;
						break;
					case ET_COLLISION:
						rep = &mCurrentLink.mCollision;
						break;
					default:
						reportError(lineno, "Encountered a geometry with unknown parent type (%s)", getElementName(mPreviousPreviousType));
						break;
					}
					if (rep)
					{
						rep->mGeometry.mType = Geometry::GT_BOX;
					}
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_SIZE:
						{
							Vec3 size;
							STRING_HELPER::getVec3(value, nullptr, size.x, size.y, size.z);
							if (rep)
							{
								rep->mGeometry.mDimensions = size;
							}
						}
						break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_GEOMETRY, mPreviousType);
				}
				break;
			case ET_MESH:
				if (mPreviousType == ET_GEOMETRY )
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_FILENAME:
						{
							if (mPreviousPreviousType == ET_VISUAL)
							{
								mCurrentLink.mVisual.mGeometry.mMesh = std::string(value);
								mCurrentLink.mVisual.mGeometry.mType = Geometry::GT_MESH;
							}
							else if (mPreviousPreviousType == ET_COLLISION)
							{
								mCurrentLink.mCollision.mGeometry.mMesh = std::string(value);
								mCurrentLink.mCollision.mGeometry.mType = Geometry::GT_MESH;
							}
							else
							{
								reportError(lineno, "Got mesh filename but not within a <visual> or <collision> element");
							}
						}
						break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_GEOMETRY, mPreviousType);
				}
				break;
			case ET_INERTIA:
				if (mPreviousType == ET_INERTIAL)
				{
					Inertia inertia;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
							case AT_IXX:
							{
								inertia.mIXX = STRING_HELPER::getFloatValue(value, nullptr);
							}
							break;
							case AT_IXY:
							{
								inertia.mIXY = STRING_HELPER::getFloatValue(value, nullptr);
							}
							break;
							case AT_IXZ:
							{
								inertia.mIXZ = STRING_HELPER::getFloatValue(value, nullptr);
							}
							break;
							case AT_IYY:
							{
								inertia.mIYY = STRING_HELPER::getFloatValue(value, nullptr);
							}
							break;
							case AT_IYZ:
							{
								inertia.mIYZ = STRING_HELPER::getFloatValue(value, nullptr);
							}
							break;
							case AT_IZZ:
							{
								inertia.mIZZ = STRING_HELPER::getFloatValue(value, nullptr);
							}
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
					switch (mPreviousType)
					{
						case ET_INERTIAL:
							mCurrentLink.mInertial.mInertia = inertia;
							break;
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_INERTIAL, mPreviousType);
				}
				break;
			case ET_GEOMETRY:
				attributeCheck(mCurrentType, lineno, acount, argv);
				if (mPreviousType == ET_VISUAL || mPreviousType == ET_COLLISION )
				{
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_INERTIAL, mPreviousType);
				}
				break;
			case ET_ORIGIN:
				if (mPreviousType == ET_INERTIAL || 
					mPreviousType == ET_VISUAL || 
					mPreviousType == ET_COLLISION ||
					mPreviousType == ET_JOINT )
				{
					Pose *dest = nullptr;
					switch (mPreviousType)
					{
					case ET_INERTIAL:
						dest = &mCurrentLink.mInertial.mPose;
						break;
					case ET_VISUAL:
						dest = &mCurrentLink.mVisual.mPose;
						break;
					case ET_COLLISION:
						dest = &mCurrentLink.mCollision.mPose;
						break;
					case ET_JOINT:
						dest = &mCurrentJoint.mPose;
						break;
					}

					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_XYZ:
						{
							STRING_HELPER::getVec3(value, nullptr,
								dest->mPosition.x,
								dest->mPosition.y,
								dest->mPosition.z);
						}
						break;
						case AT_RPY:
						{
							STRING_HELPER::getVec3(value, nullptr,
								dest->mRPY.x,
								dest->mRPY.y,
								dest->mRPY.z);
						}
						break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_INERTIAL, mPreviousType);
				}
				break;
			case ET_MASS:
				if (mPreviousType == ET_INERTIAL)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
							case AT_VALUE:
								{
									mCurrentLink.mInertial.mMass = STRING_HELPER::getFloatValue(value, nullptr);
								}
								break;
							default:
								reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
								break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_INERTIAL, mPreviousType);
				}
				break;
			case ET_INERTIAL:
			case ET_VISUAL:
			case ET_COLLISION:
				{
					std::string name;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_NAME:
							name = std::string(value);
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
					if (mPreviousType == ET_LINK)
					{
						if (mPreviousPreviousType == ET_COLLISION)
						{
							mCurrentLink.mCollision.mName = name;
						}
					}
					else if (mPreviousType == ET_CONTACT)
					{
						mCurrentSensor.mContact.mCollision = std::string(elementData);
					}
					else
					{
						nestingError(lineno, mCurrentType, ET_LINK, mPreviousType);
					}
				}
				break;
			case ET_TRANSMISSION:
				if (mPreviousType == ET_ROBOT)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_NAME:
							mCurrentTransmission.mName = std::string(value);
							break;
						case AT_TYPE:
							mCurrentTransmission.mType = std::string(value);
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_ROBOT, mPreviousType);
				}
				break;
			case ET_LINK:
				if (mPreviousType == ET_ROBOT || mPreviousType == ET_GAZEBO )
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_NAME:
							mCurrentLink.mName = std::string(value);
							break;
						case AT_TYPE:
							mCurrentLink.mType = std::string(value);
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_ROBOT, mPreviousType);
				}
				break;
			case ET_TEXTURE:
				if (mPreviousType == ET_MATERIAL)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
							case AT_FILENAME:
								{
									mCurrentMaterial.mTexture = std::string(value);
								}
								break;
							default:
								reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
								break;
						}
					}
				}
				else if (mPreviousType == ET_PROJECTOR)
				{
					attributeCheck(mCurrentType, lineno, acount, argv);
					mCurrentProjector.mTexture = std::string(elementData);
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_MATERIAL, mPreviousType);
				}
				break;
			case ET_COLOR:
				if (mPreviousType == ET_MATERIAL)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_RGBA:
							{
							STRING_HELPER::getVec4(value, nullptr,
								mCurrentMaterial.mColor.mRed,
								mCurrentMaterial.mColor.mGreen,
								mCurrentMaterial.mColor.mBlue,
								mCurrentMaterial.mColor.mAlpha);
							}
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_MATERIAL, mPreviousType);
				}
				break;
			case ET_MATERIAL:
				if (mPreviousType == ET_ROBOT || mPreviousType == ET_VISUAL || mPreviousType == ET_GAZEBO )
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
							case AT_NAME:
								if (mPreviousType == ET_ROBOT)
								{
									mCurrentMaterial.mName = std::string(value);
								}
								else if (mPreviousType == ET_VISUAL)
								{
									mCurrentLink.mVisual.mMaterial = std::string(value);
								}
								else if (mPreviousType == ET_GAZEBO)
								{
									reportError(lineno, "For Gazebo we expect a 'value' attribute not 'name'");
								}
								break;
							case AT_VALUE:
								if (mPreviousType == ET_GAZEBO)
								{
									mCurrentGazebo.mMaterial = std::string(value);
								}
								else
								{
									reportError(lineno, "Only expect a 'value' attribute when inside a <gazebo> element");
								}
								break;
							default:
								reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
								break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_ROBOT, mPreviousType);
				}
				break;
			case ET_PLUGIN:
				if (mPreviousType == ET_GAZEBO || mPreviousType == ET_SENSOR )
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
							case AT_FILENAME:
								mCurrentPlugin.mFileName = std::string(value);
								break;
							case AT_NAME:
								mCurrentPlugin.mName = std::string(value);
								break;
							default:
								reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
								break;
						}
					}
				}
				else
				{
					nestingError(lineno, mCurrentType, ET_GAZEBO, mPreviousType);
				}
				break;
			case ET_GAZEBO:
				if (mPreviousType == ET_ROBOT)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_REFERENCE:
							mCurrentGazebo.mReference = std::string(value);
							break;
						default:
							reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
							break;
						}
					}

				}
				else
				{
					nestingError(lineno, mCurrentType, ET_ROBOT, mPreviousType);
				}
				break;
			case ET_ROBOT:
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
							case AT_NAME:
								mCurrentRobot.mName = std::string(value);
								break;
							case AT_XMLNS_CONTROLLER:
								mCurrentRobot.mXMLNSController = std::string(value);
								break;
							case AT_XMLNS_INTERFACE:
								mCurrentRobot.mXMLNSInterface = std::string(value);
								break;
							case AT_XMLNS_JOINT:
								mCurrentRobot.mXMLNSJoint = std::string(value);
								break;
							case AT_XMLNS_SENSOR:
								mCurrentRobot.mXMLNSSensor = std::string(value);
								break;
							case AT_XMLNS_XACRO:
								mCurrentRobot.mXMLNSXacro = std::string(value);
								break;
							default:
								reportError(lineno, "Unexpected attribute '%s' for element '%s'", key, elementName);
								break;
						}
					}
				}
				break;
			default:
				attributeCheck(mCurrentType, lineno, acount, argv);
				// If we are within a plugin element than any list of children elements
				// even if they are unknown get added as key-value-pairs to the current
				// plugin
				if (mPreviousType == ET_PLUGIN || mPreviousType == ET_SENSOR )
				{
					KeyValuePair kvp;
					kvp.mKey = std::string(elementName);
					if (elementData)
					{
						kvp.mValue = std::string(elementData);
					}
					mCurrentPlugin.mKeyValuePairs.push_back(kvp);
				}
				else
				{
					reportError(lineno, "Unknown element type (%s)", elementName);
				}
				break;
		}


		return true;
	}

	// Parses this XML and accumulates all of the unique element and attribute names
	virtual const URDF_DOM::URDF *importURDF(const char *xmlName,MESH_FACTORY::MeshFactory *meshFactory) final
	{
		mURDF.clear();
		FAST_XML::FastXml *f = FAST_XML::FastXml::create();
		f->processXml(xmlName, this);
		f->release();

		loadMeshes(xmlName,meshFactory);

		return &mURDF;
	}

	void loadMeshes(const char *xmlName,MESH_FACTORY::MeshFactory *meshFactory)
	{
		if (meshFactory)
		{
			for (auto &i : mURDF.mRobots)
			{
				for (auto &j : i.mLinks)
				{
					getMesh(xmlName,j.mVisual, meshFactory);
					getMesh(xmlName, j.mCollision, meshFactory);
				}
				for (auto &j : i.mGazebos)
				{
					for (auto &k : j.mLinks)
					{
						getMesh(xmlName, k.mVisual, meshFactory);
						getMesh(xmlName, k.mCollision, meshFactory);
					}
				}
			}
		}
	}

	bool getMesh(const char *xmlName,Representation &r, MESH_FACTORY::MeshFactory *meshFactory)
	{
		bool ret = false;

		// if the mesh name is not empty
		if (r.mGeometry.mMesh.size() && meshFactory )
		{
			std::string pathName;
			std::string assetName;
			if (splitPath(xmlName, r.mGeometry.mMesh.c_str(), pathName, assetName))
			{
				r.mMesh = meshFactory->importMesh(assetName.c_str(), pathName.c_str());
				if (r.mMesh)
				{
					ret = true;
				}
			}
			else
			{
				reportError(0, "Failed to properly figure out the meshName and path name from meshName(%s) xmlFileName(%s)", r.mGeometry.mMesh.c_str(), xmlName);
			}
		}

		return ret;
	}

	virtual void release(void) final
	{
		delete this;
	}

	virtual bool processComment(const char *comment)final  // encountered a comment in the XML
	{
		switch (mCurrentType)
		{
			case ET_ROBOT:
				mCurrentRobot.mComments.push_back(std::string(comment));
				break;
			case ET_LAST:
				mURDF.mComments.push_back(std::string(comment));
				break;
		}
		return true;
	}

	void reportCloseError(uint32_t lineno,ElementType type,ElementType expected,ElementType previousType)
	{
		reportError(lineno, "Got element-close <%s> without matching parent <%s> instead found <%s>",
			getElementName(type),
			getElementName(expected),
			getElementName(previousType));
	}

	// 'element' is the name of the element that is being closed.
	// depth is the recursion depth of this element.
	// Return true to continue processing the XML file.
	// Return false to stop processing the XML file; leaves the read pointer of the stream right after this close tag.
	// The bool 'isError' indicates whether processing was stopped due to an error, or intentionally canceled early.
	virtual bool processClose(const char *element, uint32_t depth, bool &isError, uint32_t lineno) final	  // process the 'close' indicator for a previously encountered element
	{
		// We pop the element type stack and revise the current and previous type variables
		if ((depth + 1) != mStackLocation)
		{
			reportError(lineno, "Element Stack is messed up.");
		}
		ElementType type = getElementType(element);
		if (mStackLocation)
		{
			mStackLocation--;
			if (mTypeStack[mStackLocation] != type)
			{
				reportError(lineno, "ElementClose did not match the previous element open! Invalid XML file.");
				mStackLocation++; // don't pop the stack, this was a mismatched close
				return true;
			}
			else
			{
				mPreviousPreviousType = ET_LAST;
				mCurrentType = mTypeStack[mStackLocation];
				if (mStackLocation)
				{
					mPreviousType = mTypeStack[mStackLocation - 1];
					if ((mStackLocation - 1))
					{
						mPreviousPreviousType = mTypeStack[mStackLocation - 2];
					}
				}
				else
				{
					mPreviousType = ET_LAST;
				}
			}
		}
		switch (type)
		{
			case ET_ROBOT:
				mURDF.mRobots.push_back(mCurrentRobot);
				mCurrentRobot.clear();
				break;
			case ET_CAMERA:
				if (mPreviousType == ET_SENSOR)
				{
					mCurrentSensor.mCameras.push_back(mCurrentCamera);
					mCurrentCamera.clear();
				}
				else
				{
					reportCloseError(lineno, type, ET_SENSOR, mPreviousType);
				}
				break;
			case ET_DYNAMICS:
				if (mPreviousType == ET_JOINT)
				{
					mCurrentJoint.mDynamics.push_back(mCurrentDynamics);
					mCurrentDynamics.clear();
				}
				else if (mPreviousType == ET_AXIS && mPreviousPreviousType == ET_JOINT )
				{
					mCurrentJoint.mAxis.mDynamics.push_back(mCurrentDynamics);
					mCurrentDynamics.clear();
				}
				else
				{
					reportCloseError(lineno, type, ET_JOINT, mPreviousType);
				}
				break;
			case ET_GRIPPER_LINK:
				if (mPreviousType == ET_GRIPPER)
				{
					mCurrentGripper.mGripperLinks.push_back(mCurrentGripperLink);
					mCurrentGripperLink.clear();
				}
				else
				{
					reportCloseError(lineno, type, ET_GRIPPER, mPreviousType);
				}
				break;
			case ET_GRASP_CHECK:
				if (mPreviousType == ET_GRIPPER)
				{
					mCurrentGripper.mGraspChecks.push_back(mCurrentGraspCheck);
					mCurrentGraspCheck.clear();
				}
				else
				{
					reportCloseError(lineno, type, ET_GRIPPER, mPreviousType);
				}
				break;
			case ET_GRIPPER:
				if (mPreviousType == ET_GAZEBO)
				{
					mCurrentGazebo.mGrippers.push_back(mCurrentGripper);
					mCurrentGripper.clear();
				}
				else
				{
					reportCloseError(lineno, type, ET_GAZEBO, mPreviousType);
				}
				break;
			case ET_PROJECTOR:
				if (mPreviousType == ET_GAZEBO)
				{
					mCurrentGazebo.mProjectors.push_back(mCurrentProjector);
					mCurrentProjector.clear();
				}
				else if (mPreviousType == ET_PLUGIN)
				{
					// If it's nested in a plugin; then the projector element was already added to the key/value pair basket
				}
				else
				{
					reportCloseError(lineno, type, ET_GAZEBO, mPreviousType);
				}
				break;
			case ET_SENSOR:
				if (mPreviousType == ET_GAZEBO)
				{
					mCurrentGazebo.mSensors.push_back(mCurrentSensor);
					mCurrentSensor.clear();
				}
				else
				{
					reportCloseError(lineno, type, ET_GAZEBO, mPreviousType);
				}
				break;
			case ET_GAZEBO:
				if (mPreviousType == ET_ROBOT)
				{
					mCurrentRobot.mGazebos.push_back(mCurrentGazebo);
					mCurrentGazebo.clear();
				}
				else
				{
					reportCloseError(lineno, type, ET_ROBOT, mPreviousType);
				}
				break;
			case ET_PLUGIN:
				if (mPreviousType == ET_GAZEBO)
				{
					mCurrentGazebo.mPlugins.push_back(mCurrentPlugin);
					mCurrentPlugin.clear();
				}
				else if (mPreviousType == ET_SENSOR)
				{
					mCurrentSensor.mPlugins.push_back(mCurrentPlugin);
					mCurrentPlugin.clear();
				}
				else
				{
					reportCloseError(lineno, type, ET_GAZEBO, mPreviousType);
				}
				break;
			case ET_MATERIAL:
				if (mPreviousType == ET_ROBOT)
				{
					mCurrentRobot.mMaterials.push_back(mCurrentMaterial);
					mCurrentMaterial.clear();
				}
				else if (mPreviousType == ET_VISUAL || mPreviousType == ET_GAZEBO )
				{
					// materials associated with visual elements are already handled
				}
				else
				{
					reportCloseError(lineno, type, ET_ROBOT, mPreviousType);
				}
				break;
			case ET_TRANSMISSION:
				if (mPreviousType == ET_ROBOT)
				{
					mCurrentRobot.mTransmissions.push_back(mCurrentTransmission);
					mCurrentTransmission.clear();
				}
				else
				{
					reportCloseError(lineno, type, ET_ROBOT, mPreviousType);
				}
				break;
			case ET_LINK:
				if (mPreviousType == ET_ROBOT)
				{
					mCurrentRobot.mLinks.push_back(mCurrentLink);
					mCurrentLink.clear();
				}
				else if (mPreviousType == ET_GAZEBO)
				{
					mCurrentGazebo.mLinks.push_back(mCurrentLink);
					mCurrentLink.clear();
				}
				else
				{
					reportCloseError(lineno, type, ET_ROBOT, mPreviousType);
				}
				break;
			case ET_JOINT:
				if (mPreviousType == ET_ROBOT)
				{
					mCurrentRobot.mJoints.push_back(mCurrentJoint);
					mCurrentJoint.clear();
				}
				else if (mPreviousType == ET_GAZEBO)
				{
					mCurrentGazebo.mJoints.push_back(mCurrentJoint);
					mCurrentJoint.clear();
				}
				else if (mPreviousType == ET_TRANSMISSION)
				{
					// already handled
				}
				else
				{
					reportCloseError(lineno, type, ET_ROBOT, mPreviousType);
				}
				break;
		}
		return true;
	}

private:
	uint32_t			mStackLocation{ 0 };
	ElementType			mCurrentType{ ET_LAST };	// The current element type we are processing
	ElementType			mPreviousType{ ET_LAST };	// The previous element type (parent node type)
	ElementType			mPreviousPreviousType{ ET_LAST }; // two up the call stack
	ElementType			mTypeStack[MAX_STACK];
	URDF				mURDF;
	Robot				mCurrentRobot;
	Gazebo				mCurrentGazebo;
	Plugin				mCurrentPlugin;	// The current plugin we are parsing
	Material			mCurrentMaterial;
	Link				mCurrentLink;
	Joint				mCurrentJoint;
	Sensor				mCurrentSensor;
	Projector			mCurrentProjector;
	Transmission		mCurrentTransmission;
	Gripper				mCurrentGripper;
	GraspCheck			mCurrentGraspCheck;
	GripperLink			mCurrentGripperLink;
	Dynamics			mCurrentDynamics;
	Camera				mCurrentCamera;
};

ImportURDF *ImportURDF::create(void)
{
	ImportURDFImpl *in = new ImportURDFImpl;
	return static_cast<ImportURDF *>(in);
}


}
