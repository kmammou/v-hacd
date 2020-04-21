#include "ImportDAE.h"
#include "FastXml.h"
#include "StringHelper.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

#pragma warning(disable:4100 4505 4840)

#define MAX_STACK 32
#define SHOW_ERROR 0

namespace IMPORT_DAE
{
	typedef std::vector< ImportDAE::Vertex > VertexVector;
	typedef std::vector< uint32_t > Uint32Vector;

	static void  fm_matrixMultiply(const float *pA, const float *pB, float *pM)
	{
		float a = pA[0 * 4 + 0] * pB[0 * 4 + 0] + pA[0 * 4 + 1] * pB[1 * 4 + 0] + pA[0 * 4 + 2] * pB[2 * 4 + 0] + pA[0 * 4 + 3] * pB[3 * 4 + 0];
		float b = pA[0 * 4 + 0] * pB[0 * 4 + 1] + pA[0 * 4 + 1] * pB[1 * 4 + 1] + pA[0 * 4 + 2] * pB[2 * 4 + 1] + pA[0 * 4 + 3] * pB[3 * 4 + 1];
		float c = pA[0 * 4 + 0] * pB[0 * 4 + 2] + pA[0 * 4 + 1] * pB[1 * 4 + 2] + pA[0 * 4 + 2] * pB[2 * 4 + 2] + pA[0 * 4 + 3] * pB[3 * 4 + 2];
		float d = pA[0 * 4 + 0] * pB[0 * 4 + 3] + pA[0 * 4 + 1] * pB[1 * 4 + 3] + pA[0 * 4 + 2] * pB[2 * 4 + 3] + pA[0 * 4 + 3] * pB[3 * 4 + 3];

		float e = pA[1 * 4 + 0] * pB[0 * 4 + 0] + pA[1 * 4 + 1] * pB[1 * 4 + 0] + pA[1 * 4 + 2] * pB[2 * 4 + 0] + pA[1 * 4 + 3] * pB[3 * 4 + 0];
		float f = pA[1 * 4 + 0] * pB[0 * 4 + 1] + pA[1 * 4 + 1] * pB[1 * 4 + 1] + pA[1 * 4 + 2] * pB[2 * 4 + 1] + pA[1 * 4 + 3] * pB[3 * 4 + 1];
		float g = pA[1 * 4 + 0] * pB[0 * 4 + 2] + pA[1 * 4 + 1] * pB[1 * 4 + 2] + pA[1 * 4 + 2] * pB[2 * 4 + 2] + pA[1 * 4 + 3] * pB[3 * 4 + 2];
		float h = pA[1 * 4 + 0] * pB[0 * 4 + 3] + pA[1 * 4 + 1] * pB[1 * 4 + 3] + pA[1 * 4 + 2] * pB[2 * 4 + 3] + pA[1 * 4 + 3] * pB[3 * 4 + 3];

		float i = pA[2 * 4 + 0] * pB[0 * 4 + 0] + pA[2 * 4 + 1] * pB[1 * 4 + 0] + pA[2 * 4 + 2] * pB[2 * 4 + 0] + pA[2 * 4 + 3] * pB[3 * 4 + 0];
		float j = pA[2 * 4 + 0] * pB[0 * 4 + 1] + pA[2 * 4 + 1] * pB[1 * 4 + 1] + pA[2 * 4 + 2] * pB[2 * 4 + 1] + pA[2 * 4 + 3] * pB[3 * 4 + 1];
		float k = pA[2 * 4 + 0] * pB[0 * 4 + 2] + pA[2 * 4 + 1] * pB[1 * 4 + 2] + pA[2 * 4 + 2] * pB[2 * 4 + 2] + pA[2 * 4 + 3] * pB[3 * 4 + 2];
		float l = pA[2 * 4 + 0] * pB[0 * 4 + 3] + pA[2 * 4 + 1] * pB[1 * 4 + 3] + pA[2 * 4 + 2] * pB[2 * 4 + 3] + pA[2 * 4 + 3] * pB[3 * 4 + 3];

		float m = pA[3 * 4 + 0] * pB[0 * 4 + 0] + pA[3 * 4 + 1] * pB[1 * 4 + 0] + pA[3 * 4 + 2] * pB[2 * 4 + 0] + pA[3 * 4 + 3] * pB[3 * 4 + 0];
		float n = pA[3 * 4 + 0] * pB[0 * 4 + 1] + pA[3 * 4 + 1] * pB[1 * 4 + 1] + pA[3 * 4 + 2] * pB[2 * 4 + 1] + pA[3 * 4 + 3] * pB[3 * 4 + 1];
		float o = pA[3 * 4 + 0] * pB[0 * 4 + 2] + pA[3 * 4 + 1] * pB[1 * 4 + 2] + pA[3 * 4 + 2] * pB[2 * 4 + 2] + pA[3 * 4 + 3] * pB[3 * 4 + 2];
		float p = pA[3 * 4 + 0] * pB[0 * 4 + 3] + pA[3 * 4 + 1] * pB[1 * 4 + 3] + pA[3 * 4 + 2] * pB[2 * 4 + 3] + pA[3 * 4 + 3] * pB[3 * 4 + 3];

		pM[0] = a;
		pM[1] = b;
		pM[2] = c;
		pM[3] = d;

		pM[4] = e;
		pM[5] = f;
		pM[6] = g;
		pM[7] = h;

		pM[8] = i;
		pM[9] = j;
		pM[10] = k;
		pM[11] = l;

		pM[12] = m;
		pM[13] = n;
		pM[14] = o;
		pM[15] = p;
	}

	static void  fm_setScale(float x, float y, float z, float *fscale) // apply scale to the matrix.
	{
		fscale[0 * 4 + 0] = x;
		fscale[1 * 4 + 1] = y;
		fscale[2 * 4 + 2] = z;
	}


	static void  fm_setTranslation(float *matrix,float x,float y,float z)
	{
		matrix[12] = x;
		matrix[13] = y;
		matrix[14] = z;
	}

	static void  fm_getTranslation(const float *matrix, float &x, float &y, float &z)
	{
		x = matrix[12];
		y = matrix[13];
		z = matrix[14];
	}


	static void fm_identity(float matrix[16]) // set 4x4 matrix to identity.
	{
		matrix[0 * 4 + 0] = 1;
		matrix[1 * 4 + 1] = 1;
		matrix[2 * 4 + 2] = 1;
		matrix[3 * 4 + 3] = 1;

		matrix[1 * 4 + 0] = 0;
		matrix[2 * 4 + 0] = 0;
		matrix[3 * 4 + 0] = 0;

		matrix[0 * 4 + 1] = 0;
		matrix[2 * 4 + 1] = 0;
		matrix[3 * 4 + 1] = 0;

		matrix[0 * 4 + 2] = 0;
		matrix[1 * 4 + 2] = 0;
		matrix[3 * 4 + 2] = 0;

		matrix[0 * 4 + 3] = 0;
		matrix[1 * 4 + 3] = 0;
		matrix[2 * 4 + 3] = 0;

	}


	class VertexCompare
	{
	public:
		bool operator()(const ImportDAE::Vertex &v1, const ImportDAE::Vertex &v2) const
		{
			if (v1.mPosition[0] < v2.mPosition[0])
				return false;
			if (v1.mPosition[0] > v2.mPosition[0])
				return true;
			if (v1.mPosition[1] < v2.mPosition[1])
				return false;
			if (v1.mPosition[1] > v2.mPosition[1])
				return true;
			if (v1.mPosition[2] < v2.mPosition[2])
				return false;
			if (v1.mPosition[2] > v2.mPosition[2])
				return true;

			if (v1.mNormal[0] < v2.mNormal[0])
				return false;
			if (v1.mNormal[0] > v2.mNormal[0])
				return true;
			if (v1.mNormal[1] < v2.mNormal[1])
				return false;
			if (v1.mNormal[1] > v2.mNormal[1])
				return true;
			if (v1.mNormal[2] < v2.mNormal[2])
				return false;
			if (v1.mNormal[2] > v2.mNormal[2])
				return true;

			if (v1.mTexel[0] < v2.mTexel[0])
				return false;
			if (v1.mTexel[0] > v2.mTexel[0])
				return true;
			if (v1.mTexel[1] < v2.mTexel[1])
				return false;
			if (v1.mTexel[1] > v2.mTexel[1])
				return true;

			return false;
		}
	};

	typedef std::map< ImportDAE::Vertex, uint32_t, VertexCompare > VertexIndexMap;

	class VertexIndex
	{
	public:
		void clear(void)
		{
			mCount = 0;
			mVertexIndexMap.clear();
		}
		uint32_t getVertexIndex(const ImportDAE::Vertex &v)
		{
			uint32_t ret = mCount;
			VertexIndexMap::iterator found = mVertexIndexMap.find(v);
			if (found == mVertexIndexMap.end())
			{
				mVertexIndexMap[v] = mCount;
				mCount++;
			}
			else
			{
				ret = (*found).second;
			}
			return ret;
		}
		uint32_t		mCount{ 0 };
		VertexIndexMap	mVertexIndexMap;
	};



	class ImportSubMesh : public ImportDAE::SubMesh
	{
	public:
		void addVertex(const ImportDAE::Vertex &v)
		{
			uint32_t vcount = uint32_t(mVertices.size());
			uint32_t index = mVertexIndex.getVertexIndex(v);
			if ( index == vcount )
			{
				mVertices.push_back(v);
			}
			mIndices.push_back(index);
		}

		void setup(void)
		{
			mVertexIndex.clear();
			mMaterialName = mMaterial.c_str();
			mVertexCount = uint32_t(mVertices.size());
			mTriangleCount = uint32_t(mIndices.size() / 3);
			mMeshVertices = nullptr;
			if (mVertexCount)
			{
				mMeshVertices = &mVertices[0];
			}
			mMeshIndices = nullptr;
			if (mTriangleCount)
			{
				mMeshIndices = &mIndices[0];
			}
		}

		VertexIndex		mVertexIndex;
		std::string		mMaterial;		// The name of the material this sub-mesh uses
		VertexVector	mVertices;
		Uint32Vector	mIndices;
	};

	typedef std::vector< ImportSubMesh > ImportSubMeshVector;

	class ImportMesh : public ImportDAE::MeshImport
	{
	public:

		// Return the total number of submeshes
		virtual uint32_t		getSubMeshCount(void) const final
		{
			return uint32_t(mSubMeshes.size());
		}

		// Returns a pointer to a specific submesh
		virtual const ImportDAE::SubMesh	*getSubMesh(uint32_t index) const final
		{
			const ImportDAE::SubMesh *ret = nullptr;

			if (index < mSubMeshes.size())
			{
				ret = &mSubMeshes[index];
			}

			return ret;
		}

		void setup(void)
		{
			mMeshName = mName.c_str();
			for (auto &i : mSubMeshes)
			{
				i.setup();
			}
		}

		std::string			mName;		// The name of this mesh
		ImportSubMeshVector	mSubMeshes; // The array of sub-meshes (each one associated with a different material)
	};

	typedef std::vector< ImportMesh > ImportMeshVector;

	// 250 unique element types
	enum ElementType {
		ET_ACCESSOR,
		ET_ADAPT_THRESH,
		ET_AMBIENT,
		ET_AMBIENT_DIFFUSE_LOCK,
		ET_AMBIENT_DIFFUSE_TEXTURE_LOCK,
		ET_AMOUNT,
		ET_ANIMATION,
		ET_APPLY_REFLECTION_DIMMING,
		ET_AREA_SHAPE,
		ET_AREA_SIZE,
		ET_AREA_SIZEY,
		ET_AREA_SIZEZ,
		ET_ASPECT_RATIO,
		ET_ASSET,
		ET_ATMOSPHERE_ON,
		ET_ATMOSPHERE_OPACITY,
		ET_ATM_DISTANCE_FACTOR,
		ET_ATM_EXTINCTION_FACTOR,
		ET_ATM_TURBIDITY,
		ET_ATT1,
		ET_ATT2,
		ET_ATTENUATION_FAR_END,
		ET_ATTENUATION_NEAR_END,
		ET_ATTENUATION_NEAR_START,
		ET_AUTHOR,
		ET_AUTHORING_TOOL,
		ET_BACKSCATTERED_LIGHT,
		ET_BIAS,
		ET_BIND_MATERIAL,
		ET_BIND_SHAPE_MATRIX,
		ET_BIND_VERTEX_INPUT,
		ET_BLEND_MODE,
		ET_BLINN,
		ET_BLUE,
		ET_BOX,
		ET_BUFFERS,
		ET_BUFFLAG,
		ET_BUFSIZE,
		ET_BUFTYPE,
		ET_BUMP,
		ET_BUMPINTERP,
		ET_CAMERA,
		ET_CHANNEL,
		ET_CLIPEND,
		ET_CLIPSTA,
		ET_COLLADA,
		ET_COLOR,
		ET_COMMENTS,
		ET_COMPRESSTHRESH,
		ET_CONSTANT_ATTENUATION,
		ET_CONTRAST,
		ET_CONTRIBUTOR,
		ET_CONTROLLER,
		ET_CONTROL_VERTICES,
		ET_COPYRIGHT,
		ET_COVERAGEU,
		ET_COVERAGEV,
		ET_CREATED,
		ET_DECAY_FALLOFF,
		ET_DECAY_TYPE,
		ET_DGNODE_TYPE,
		ET_DIFFUSE,
		ET_DIFFUSE_SOFTEN,
		ET_DIFFUSE_SPECULAR_LOCK,
		ET_DIM_LEVEL,
		ET_DIRECTIONAL,
		ET_DIST,
		ET_DOUBLE_SIDED,
		ET_EDGE,
		ET_EDGE_ENABLE,
		ET_EDGE_EXPONENT,
		ET_EFFECT,
		ET_EMISSION,
		ET_END_TIME,
		ET_ENERGY,
		ET_EXTENDED_SHADER,
		ET_EXTRA,
		ET_FACE,
		ET_FALLOFF,
		ET_FALLOFF_TYPE,
		ET_FAST,
		ET_FILTER,
		ET_FILTERTYPE,
		ET_FILTER_COLOR,
		ET_FLAG,
		ET_FLOAT,
		ET_FLOAT_ARRAY,
		ET_FORMAT,
		ET_FRAME_RATE,
		ET_GAMMA,
		ET_GENERATEUVS,
		ET_GEOMETRY,
		ET_GREEN,
		ET_HALO_INTENSITY,
		ET_HEIGHT,
		ET_HEIGHTSEGMENTS,
		ET_HORIZON_BRIGHTNESS,
		ET_HOTSPOT_BEAM,
		ET_IMAGE,
		ET_IMAGE_SEQUENCE,
		ET_INDEX_OF_REFRACTION,
		ET_INIT_FROM,
		ET_INPUT,
		ET_INSTANCE_CAMERA,
		ET_INSTANCE_CONTROLLER,
		ET_INSTANCE_EFFECT,
		ET_INSTANCE_GEOMETRY,
		ET_INSTANCE_LIGHT,
		ET_INSTANCE_MATERIAL,
		ET_INSTANCE_VISUAL_SCENE,
		ET_INTERESTDIST,
		ET_JOINTS,
		ET_LAMBERT,
		ET_LENGTH,
		ET_LENGTHSEGMENTS,
		ET_LIBRARY_ANIMATIONS,
		ET_LIBRARY_CAMERAS,
		ET_LIBRARY_CONTROLLERS,
		ET_LIBRARY_EFFECTS,
		ET_LIBRARY_GEOMETRIES,
		ET_LIBRARY_IMAGES,
		ET_LIBRARY_LIGHTS,
		ET_LIBRARY_MATERIALS,
		ET_LIBRARY_VISUAL_SCENES,
		ET_LIGHT,
		ET_LINEAR_ATTENUATION,
		ET_LUMINOUS,
		ET_MAGFILTER,
		ET_MATERIAL,
		ET_MATRIX,
		ET_MESH,
		ET_METAL,
		ET_MINFILTER,
		ET_MIRRORU,
		ET_MIRRORV,
		ET_MODE,
		ET_MODIFIED,
		ET_MULTIPLIER,
		ET_NAME_ARRAY,
		ET_NEWPARAM,
		ET_NODE,
		ET_NOISEU,
		ET_NOISEV,
		ET_OFFSETU,
		ET_OFFSETV,
		ET_OPACITY,
		ET_OPACITY_TYPE,
		ET_OPTICS,
		ET_ORIGINALMAYANODEID,
		ET_P,
		ET_PARAM,
		ET_PERSPECTIVE,
		ET_PHONG,
		ET_POINT,
		ET_POLYLIST,
		ET_POST_INFINITY,
		ET_PRE_INFINITY,
		ET_PROFILE_COMMON,
		ET_QUADRATIC_ATTENUATION,
		ET_RAY_SAMP,
		ET_RAY_SAMPY,
		ET_RAY_SAMPZ,
		ET_RAY_SAMP_METHOD,
		ET_RAY_SAMP_TYPE,
		ET_RED,
		ET_REFLECT,
		ET_REFLECTION_LEVEL,
		ET_REFLECTIVE,
		ET_REFLECTIVITY,
		ET_REPEATU,
		ET_REPEATV,
		ET_ROTATE,
		ET_ROTATEFRAME,
		ET_ROTATEUV,
		ET_SAMP,
		ET_SAMPLER,
		ET_SAMPLER2D,
		ET_SCALE,
		ET_SCENE,
		ET_SCENE_BOUNDING_MAX,
		ET_SCENE_BOUNDING_MIN,
		ET_SHADER,
		ET_SHADHALOSTEP,
		ET_SHADING_COEFFICIENTS,
		ET_SHADOW_ATTRIBUTES,
		ET_SHADOW_B,
		ET_SHADOW_COLOR,
		ET_SHADOW_G,
		ET_SHADOW_R,
		ET_SHADSPOTSIZE,
		ET_SHIFTX,
		ET_SHIFTY,
		ET_SHININESS,
		ET_SI_AMBIENCE,
		ET_SI_SCENE,
		ET_SKELETON,
		ET_SKIN,
		ET_SKYBLENDFAC,
		ET_SKYBLENDTYPE,
		ET_SKY_COLORSPACE,
		ET_SKY_EXPOSURE,
		ET_SOFT,
		ET_SOFTEN,
		ET_SOURCE,
		ET_SOURCE_DATA,
		ET_SPECULAR,
		ET_SPLINE,
		ET_SPOTBLEND,
		ET_SPOTSIZE,
		ET_SPREAD,
		ET_STAGGER,
		ET_START_TIME,
		ET_SUN_BRIGHTNESS,
		ET_SUN_EFFECT_TYPE,
		ET_SUN_INTENSITY,
		ET_SUN_SIZE,
		ET_SURFACE,
		ET_TECHNIQUE,
		ET_TECHNIQUE_COMMON,
		ET_TEXTURE,
		ET_TRANSLATE,
		ET_TRANSLATEFRAMEU,
		ET_TRANSLATEFRAMEV,
		ET_TRANSPARENCY,
		ET_TRANSPARENT,
		ET_TRIANGLES,
		ET_TYPE,
		ET_UNIT,
		ET_UP_AXIS,
		ET_USE_FAR_ATTENUATION,
		ET_USE_NEAR_ATTENUATION,
		ET_USE_SELF_ILLUM_COLOR,
		ET_V,
		ET_VCOUNT,
		ET_VERTEX_WEIGHTS,
		ET_VERTICES,
		ET_VISUAL_SCENE,
		ET_WIDTH,
		ET_WIDTHSEGMENTS,
		ET_WIRE_SIZE,
		ET_WIRE_UNITS,
		ET_WRAPU,
		ET_WRAPV,
		ET_XFOV,
		ET_XSI_CAMERA,
		ET_XSI_PARAM,
		ET_YFOV,
		ET_YF_DOFDIST,
		ET_ZFAR,
		ET_ZNEAR,
		ET_LAST
	};

	// 26 unique attribute types
	enum AttributeType {
		AT_CLOSED,
		AT_COUNT,
		AT_DEPTH,
		AT_ID,
		AT_INPUT_SEMANTIC,
		AT_INPUT_SET,
		AT_MATERIAL,
		AT_METER,
		AT_NAME,
		AT_OFFSET,
		AT_OPAQUE,
		AT_PROFILE,
		AT_SEMANTIC,
		AT_SET,
		AT_SID,
		AT_SOURCE,
		AT_STRIDE,
		AT_SYMBOL,
		AT_TARGET,
		AT_TEXCOORD,
		AT_TEXTURE,
		AT_TYPE,
		AT_URL,
		AT_VERSION,
		AT_XMLNS,
		AT_XMLNS_XSI,
		AT_LAST
	};

	struct ElementStruct
	{
		ElementType 	mType;
		const char		*mName;
	};

	static ElementStruct gElements[ET_LAST] =
	{
		ET_ACCESSOR, "accessor",
		ET_ADAPT_THRESH, "adapt_thresh",
		ET_AMBIENT, "ambient",
		ET_AMBIENT_DIFFUSE_LOCK, "ambient_diffuse_lock",
		ET_AMBIENT_DIFFUSE_TEXTURE_LOCK, "ambient_diffuse_texture_lock",
		ET_AMOUNT, "amount",
		ET_ANIMATION, "animation",
		ET_APPLY_REFLECTION_DIMMING, "apply_reflection_dimming",
		ET_AREA_SHAPE, "area_shape",
		ET_AREA_SIZE, "area_size",
		ET_AREA_SIZEY, "area_sizey",
		ET_AREA_SIZEZ, "area_sizez",
		ET_ASPECT_RATIO, "aspect_ratio",
		ET_ASSET, "asset",
		ET_ATMOSPHERE_ON, "atmosphere_on",
		ET_ATMOSPHERE_OPACITY, "atmosphere_opacity",
		ET_ATM_DISTANCE_FACTOR, "atm_distance_factor",
		ET_ATM_EXTINCTION_FACTOR, "atm_extinction_factor",
		ET_ATM_TURBIDITY, "atm_turbidity",
		ET_ATT1, "att1",
		ET_ATT2, "att2",
		ET_ATTENUATION_FAR_END, "attenuation_far_end",
		ET_ATTENUATION_NEAR_END, "attenuation_near_end",
		ET_ATTENUATION_NEAR_START, "attenuation_near_start",
		ET_AUTHOR, "author",
		ET_AUTHORING_TOOL, "authoring_tool",
		ET_BACKSCATTERED_LIGHT, "backscattered_light",
		ET_BIAS, "bias",
		ET_BIND_MATERIAL, "bind_material",
		ET_BIND_SHAPE_MATRIX, "bind_shape_matrix",
		ET_BIND_VERTEX_INPUT, "bind_vertex_input",
		ET_BLEND_MODE, "blend_mode",
		ET_BLINN, "blinn",
		ET_BLUE, "blue",
		ET_BOX, "box",
		ET_BUFFERS, "buffers",
		ET_BUFFLAG, "bufflag",
		ET_BUFSIZE, "bufsize",
		ET_BUFTYPE, "buftype",
		ET_BUMP, "bump",
		ET_BUMPINTERP, "bumpInterp",
		ET_CAMERA, "camera",
		ET_CHANNEL, "channel",
		ET_CLIPEND, "clipend",
		ET_CLIPSTA, "clipsta",
		ET_COLLADA, "COLLADA",
		ET_COLOR, "color",
		ET_COMMENTS, "comments",
		ET_COMPRESSTHRESH, "compressthresh",
		ET_CONSTANT_ATTENUATION, "constant_attenuation",
		ET_CONTRAST, "contrast",
		ET_CONTRIBUTOR, "contributor",
		ET_CONTROLLER, "controller",
		ET_CONTROL_VERTICES, "control_vertices",
		ET_COPYRIGHT, "copyright",
		ET_COVERAGEU, "coverageU",
		ET_COVERAGEV, "coverageV",
		ET_CREATED, "created",
		ET_DECAY_FALLOFF, "decay_falloff",
		ET_DECAY_TYPE, "decay_type",
		ET_DGNODE_TYPE, "dgnode_type",
		ET_DIFFUSE, "diffuse",
		ET_DIFFUSE_SOFTEN, "diffuse_soften",
		ET_DIFFUSE_SPECULAR_LOCK, "diffuse_specular_lock",
		ET_DIM_LEVEL, "dim_level",
		ET_DIRECTIONAL, "directional",
		ET_DIST, "dist",
		ET_DOUBLE_SIDED, "double_sided",
		ET_EDGE, "edge",
		ET_EDGE_ENABLE, "edge_enable",
		ET_EDGE_EXPONENT, "edge_exponent",
		ET_EFFECT, "effect",
		ET_EMISSION, "emission",
		ET_END_TIME, "end_time",
		ET_ENERGY, "energy",
		ET_EXTENDED_SHADER, "extended_shader",
		ET_EXTRA, "extra",
		ET_FACE, "face",
		ET_FALLOFF, "falloff",
		ET_FALLOFF_TYPE, "falloff_type",
		ET_FAST, "fast",
		ET_FILTER, "filter",
		ET_FILTERTYPE, "filtertype",
		ET_FILTER_COLOR, "filter_color",
		ET_FLAG, "flag",
		ET_FLOAT, "float",
		ET_FLOAT_ARRAY, "float_array",
		ET_FORMAT, "format",
		ET_FRAME_RATE, "frame_rate",
		ET_GAMMA, "gamma",
		ET_GENERATEUVS, "generateuvs",
		ET_GEOMETRY, "geometry",
		ET_GREEN, "green",
		ET_HALO_INTENSITY, "halo_intensity",
		ET_HEIGHT, "height",
		ET_HEIGHTSEGMENTS, "heightsegments",
		ET_HORIZON_BRIGHTNESS, "horizon_brightness",
		ET_HOTSPOT_BEAM, "hotspot_beam",
		ET_IMAGE, "image",
		ET_IMAGE_SEQUENCE, "image_sequence",
		ET_INDEX_OF_REFRACTION, "index_of_refraction",
		ET_INIT_FROM, "init_from",
		ET_INPUT, "input",
		ET_INSTANCE_CAMERA, "instance_camera",
		ET_INSTANCE_CONTROLLER, "instance_controller",
		ET_INSTANCE_EFFECT, "instance_effect",
		ET_INSTANCE_GEOMETRY, "instance_geometry",
		ET_INSTANCE_LIGHT, "instance_light",
		ET_INSTANCE_MATERIAL, "instance_material",
		ET_INSTANCE_VISUAL_SCENE, "instance_visual_scene",
		ET_INTERESTDIST, "interestdist",
		ET_JOINTS, "joints",
		ET_LAMBERT, "lambert",
		ET_LENGTH, "length",
		ET_LENGTHSEGMENTS, "lengthsegments",
		ET_LIBRARY_ANIMATIONS, "library_animations",
		ET_LIBRARY_CAMERAS, "library_cameras",
		ET_LIBRARY_CONTROLLERS, "library_controllers",
		ET_LIBRARY_EFFECTS, "library_effects",
		ET_LIBRARY_GEOMETRIES, "library_geometries",
		ET_LIBRARY_IMAGES, "library_images",
		ET_LIBRARY_LIGHTS, "library_lights",
		ET_LIBRARY_MATERIALS, "library_materials",
		ET_LIBRARY_VISUAL_SCENES, "library_visual_scenes",
		ET_LIGHT, "light",
		ET_LINEAR_ATTENUATION, "linear_attenuation",
		ET_LUMINOUS, "luminous",
		ET_MAGFILTER, "magfilter",
		ET_MATERIAL, "material",
		ET_MATRIX, "matrix",
		ET_MESH, "mesh",
		ET_METAL, "metal",
		ET_MINFILTER, "minfilter",
		ET_MIRRORU, "mirrorU",
		ET_MIRRORV, "mirrorV",
		ET_MODE, "mode",
		ET_MODIFIED, "modified",
		ET_MULTIPLIER, "multiplier",
		ET_NAME_ARRAY, "Name_array",
		ET_NEWPARAM, "newparam",
		ET_NODE, "node",
		ET_NOISEU, "noiseU",
		ET_NOISEV, "noiseV",
		ET_OFFSETU, "offsetU",
		ET_OFFSETV, "offsetV",
		ET_OPACITY, "opacity",
		ET_OPACITY_TYPE, "opacity_type",
		ET_OPTICS, "optics",
		ET_ORIGINALMAYANODEID, "originalMayaNodeId",
		ET_P, "p",
		ET_PARAM, "param",
		ET_PERSPECTIVE, "perspective",
		ET_PHONG, "phong",
		ET_POINT, "point",
		ET_POLYLIST, "polylist",
		ET_POST_INFINITY, "post_infinity",
		ET_PRE_INFINITY, "pre_infinity",
		ET_PROFILE_COMMON, "profile_COMMON",
		ET_QUADRATIC_ATTENUATION, "quadratic_attenuation",
		ET_RAY_SAMP, "ray_samp",
		ET_RAY_SAMPY, "ray_sampy",
		ET_RAY_SAMPZ, "ray_sampz",
		ET_RAY_SAMP_METHOD, "ray_samp_method",
		ET_RAY_SAMP_TYPE, "ray_samp_type",
		ET_RED, "red",
		ET_REFLECT, "reflect",
		ET_REFLECTION_LEVEL, "reflection_level",
		ET_REFLECTIVE, "reflective",
		ET_REFLECTIVITY, "reflectivity",
		ET_REPEATU, "repeatU",
		ET_REPEATV, "repeatV",
		ET_ROTATE, "rotate",
		ET_ROTATEFRAME, "rotateFrame",
		ET_ROTATEUV, "rotateUV",
		ET_SAMP, "samp",
		ET_SAMPLER, "sampler",
		ET_SAMPLER2D, "sampler2D",
		ET_SCALE, "scale",
		ET_SCENE, "scene",
		ET_SCENE_BOUNDING_MAX, "scene_bounding_max",
		ET_SCENE_BOUNDING_MIN, "scene_bounding_min",
		ET_SHADER, "shader",
		ET_SHADHALOSTEP, "shadhalostep",
		ET_SHADING_COEFFICIENTS, "shading_coefficients",
		ET_SHADOW_ATTRIBUTES, "shadow_attributes",
		ET_SHADOW_B, "shadow_b",
		ET_SHADOW_COLOR, "shadow_color",
		ET_SHADOW_G, "shadow_g",
		ET_SHADOW_R, "shadow_r",
		ET_SHADSPOTSIZE, "shadspotsize",
		ET_SHIFTX, "shiftx",
		ET_SHIFTY, "shifty",
		ET_SHININESS, "shininess",
		ET_SI_AMBIENCE, "SI_Ambience",
		ET_SI_SCENE, "SI_Scene",
		ET_SKELETON, "skeleton",
		ET_SKIN, "skin",
		ET_SKYBLENDFAC, "skyblendfac",
		ET_SKYBLENDTYPE, "skyblendtype",
		ET_SKY_COLORSPACE, "sky_colorspace",
		ET_SKY_EXPOSURE, "sky_exposure",
		ET_SOFT, "soft",
		ET_SOFTEN, "soften",
		ET_SOURCE, "source",
		ET_SOURCE_DATA, "source_data",
		ET_SPECULAR, "specular",
		ET_SPLINE, "spline",
		ET_SPOTBLEND, "spotblend",
		ET_SPOTSIZE, "spotsize",
		ET_SPREAD, "spread",
		ET_STAGGER, "stagger",
		ET_START_TIME, "start_time",
		ET_SUN_BRIGHTNESS, "sun_brightness",
		ET_SUN_EFFECT_TYPE, "sun_effect_type",
		ET_SUN_INTENSITY, "sun_intensity",
		ET_SUN_SIZE, "sun_size",
		ET_SURFACE, "surface",
		ET_TECHNIQUE, "technique",
		ET_TECHNIQUE_COMMON, "technique_common",
		ET_TEXTURE, "texture",
		ET_TRANSLATE, "translate",
		ET_TRANSLATEFRAMEU, "translateFrameU",
		ET_TRANSLATEFRAMEV, "translateFrameV",
		ET_TRANSPARENCY, "transparency",
		ET_TRANSPARENT, "transparent",
		ET_TRIANGLES, "triangles",
		ET_TYPE, "type",
		ET_UNIT, "unit",
		ET_UP_AXIS, "up_axis",
		ET_USE_FAR_ATTENUATION, "use_far_attenuation",
		ET_USE_NEAR_ATTENUATION, "use_near_attenuation",
		ET_USE_SELF_ILLUM_COLOR, "use_self_illum_color",
		ET_V, "v",
		ET_VCOUNT, "vcount",
		ET_VERTEX_WEIGHTS, "vertex_weights",
		ET_VERTICES, "vertices",
		ET_VISUAL_SCENE, "visual_scene",
		ET_WIDTH, "width",
		ET_WIDTHSEGMENTS, "widthsegments",
		ET_WIRE_SIZE, "wire_size",
		ET_WIRE_UNITS, "wire_units",
		ET_WRAPU, "wrapU",
		ET_WRAPV, "wrapV",
		ET_XFOV, "xfov",
		ET_XSI_CAMERA, "XSI_Camera",
		ET_XSI_PARAM, "xsi_param",
		ET_YFOV, "yfov",
		ET_YF_DOFDIST, "YF_dofdist",
		ET_ZFAR, "zfar",
		ET_ZNEAR, "znear",
	};

	struct AttributeStruct
	{
		AttributeType mType;
		const char	  *mName;
	};

	static AttributeStruct gAttributes[AT_LAST] =
	{
		AT_CLOSED, "closed",
		AT_COUNT, "count",
		AT_DEPTH, "depth",
		AT_ID, "id",
		AT_INPUT_SEMANTIC, "input_semantic",
		AT_INPUT_SET, "input_set",
		AT_MATERIAL, "material",
		AT_METER, "meter",
		AT_NAME, "name",
		AT_OFFSET, "offset",
		AT_OPAQUE, "opaque",
		AT_PROFILE, "profile",
		AT_SEMANTIC, "semantic",
		AT_SET, "set",
		AT_SID, "sid",
		AT_SOURCE, "source",
		AT_STRIDE, "stride",
		AT_SYMBOL, "symbol",
		AT_TARGET, "target",
		AT_TEXCOORD, "texcoord",
		AT_TEXTURE, "texture",
		AT_TYPE, "type",
		AT_URL, "url",
		AT_VERSION, "version",
		AT_XMLNS, "xmlns",
		AT_XMLNS_XSI, "xmlns:xsi",
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


typedef std::vector< std::string > StringVector;

class ImportDAEImpl : public ImportDAE, public FAST_XML::FastXml::Callback
{
public:
	ImportDAEImpl(void)
	{
		initMaps(); // initialize the lookup tables
	}

	virtual ~ImportDAEImpl(void)
	{

	}

	class IdName
	{
	public:
		std::string		mId;
		std::string		mName;
	};


	class InstanceEffect
	{
	public:
		void clear(void)
		{
			InstanceEffect empty;
			*this = empty;
		}
		std::string	mURL;
	};

	typedef std::vector< InstanceEffect > InstanceEffectVector;

	class Material : public IdName
	{
	public:
		void clear(void)
		{
			Material empty;
			*this = empty;
		}
		InstanceEffectVector	mInstanceEffects;
	};

	typedef std::vector< Material > MaterialVector;

	class LibraryMaterials
	{
	public:
		MaterialVector	mMaterials;
	};

	class LibraryImages
	{
	public:
	};

	class LibraryControllers
	{
	public:
	};


	class Vec4
	{
	public:
		float	x{ 0 };
		float	y{ 0 };
		float	z{ 0 };
		float	w{ 0 };
	};

	class Vec3
	{
	public:
		float	x{ 0 };
		float	y{ 0 };
		float	z{ 0 };
	};


	class Color
	{
	public:
		std::string		mSid;
		Vec4			mColor;
	};

	class Shininess
	{
	public:
		std::string	mSid;
		float		mShininess{ 0 };
	};

	class IndexOfRefraction
	{
	public:
		std::string	mSid;
		float		mIndexOfRefraction{ 0 };
	};

	class Phong
	{
	public:
		Color		mDiffuse;
		Color		mSpecular;
		Color		mEmission;
		Color		mAmbient;
		Shininess	mShininess;
		IndexOfRefraction	mIndexOfRefraction;
	};

	class Technique : public IdName
	{
	public:
		Phong		mPhong;
	};

	class ProfileCommon
	{
	public:
		Technique	mTechnique;
	};

	class Effect : public IdName
	{
	public:

		void clear(void)
		{
			Effect empty;
			*this = empty;
		}

		ProfileCommon	mProfileCommon;
	};

	typedef std::vector< Effect > EffectVector;

	class LibraryEffects
	{
	public:
		EffectVector	mEffects;
	};

	typedef std::vector< float > FloatVector;


	class FloatArray : public IdName
	{
	public:
		void clear(void)
		{
			FloatArray c;
			*this = c;
		}
		FloatVector	mFloatArray;
	};

	enum ParamType
	{
		PT_FLOAT,
		PT_LAST
	};

	static ParamType getParamType(const char *p)
	{
		ParamType ret = PT_LAST;
		if (strcmp(p, "float") == 0)
		{
			ret = PT_FLOAT;
		}
		return ret;
	}


	enum ParamNameType
	{
		PNT_X,
		PNT_Y,
		PNT_Z,
		PNT_S,
		PNT_T,
		PNT_LAST
	};

	static ParamNameType getParamNameType(const char *p)
	{
		ParamNameType ret = PNT_LAST;
		if (strcmp(p, "X") == 0)
		{
			ret = PNT_X;
		}
		else if (strcmp(p, "Y") == 0)
		{
			ret = PNT_Y;
		}
		else if (strcmp(p, "Z") == 0)
		{
			ret = PNT_Z;
		}
		else if (strcmp(p, "S") == 0)
		{
			ret = PNT_S;
		}
		else if (strcmp(p, "T") == 0)
		{
			ret = PNT_T;
		}
		return ret;
	}


	class Param
	{
	public:
		ParamNameType	mNameType{ PNT_LAST };
		ParamType		mType{ PT_LAST };
	};

	typedef std::vector< Param > ParamVector;

	class SourceTechniqueCommon
	{
	public:
		uint32_t	mCount{ 0 };
		uint32_t	mStride{ 0 };
		std::string	mSource;
		ParamVector	mParams;
	};

	class Source : public IdName
	{
	public:
		void clear(void)
		{
			Source c;
			*this = c;
		}
		FloatArray				mFloatArray;		// Array of floats associated with this source
		SourceTechniqueCommon	mSourceTechniqueCommon;
	};

	typedef std::vector< Source > SourceVector;

	enum SemanticType
	{
		ST_POSITION,
		ST_VERTEX,
		ST_NORMAL,
		ST_TEXCOORD,
		ST_LAST
	};

	static SemanticType getSemanticType(const char *s)
	{
		SemanticType ret = ST_LAST;
		if (strcmp(s, "POSITION") == 0)
		{
			ret = ST_POSITION;
		}
		else if (strcmp(s, "VERTEX") == 0)
		{
			ret = ST_VERTEX;
		}
		else if (strcmp(s, "NORMAL") == 0)
		{
			ret = ST_NORMAL;
		}
		else if (strcmp(s, "TEXCOORD") == 0)
		{
			ret = ST_TEXCOORD;
		}
		return ret;
	}

	class Input
	{
	public:
		void clear(void)
		{
			Input c;
			*this = c;
		}
		SemanticType	mSemantic{ ST_LAST };
		std::string		mSource;
		uint32_t		mOffset{ 0 };
		uint32_t		mSet{ 0 };
	};

	typedef std::vector< Input > InputVector;

	class Vertices : public IdName
	{
	public:
		void clear(void)
		{
			Vertices c;
			*this = c;
		}
		InputVector	mInputs;
	};

	typedef std::vector<Vertices> VerticesVector;


	class TrianglesPolyList
	{
	public:
		void clear(void)
		{
			TrianglesPolyList c;
			*this = c;
		}
		uint32_t			mCount{ 0 };
		std::string			mMaterial;
		InputVector			mInputs;
		Uint32Vector		mIndices;
		Uint32Vector		mVcount;
	};

	typedef std::vector< TrianglesPolyList > TrianglesVector;

	class Mesh
	{
	public:
		void clear(void)
		{
			Mesh c;
			*this = c;
		}

		const Source *findVertexSource(const std::string &sourceName,SemanticType stype) const
		{
			const Source *ret = nullptr;

			for (const auto &i : mVertices)
			{
				if (i.mId == sourceName)
				{
					for (const auto &j : i.mInputs)
					{
						if (j.mSemantic == stype )
						{
							const char *sn = j.mSource.c_str();
							if (*sn == '#')
							{
								sn++;
							}
							ret = findSource(std::string(sn));
						}
					}
				}
			}

			return ret;
		}

		const Source *findSource(const std::string &sourceName) const
		{
			const Source *ret = nullptr;

			for (const auto &i : mSources)
			{
				if (i.mId == sourceName)
				{
					ret = &i;
					break;
				}
			}

			return ret;
		}

		SourceVector		mSources;
		VerticesVector	mVertices;
		TrianglesVector	mTriangles;
	};

	typedef std::vector< Mesh > MeshVector;

	class Geometry : public IdName
	{
	public:
		void clear(void)
		{
			Geometry c;
			*this = c;
		}
		MeshVector	mMeshes;
	};

	typedef std::vector< Geometry > GeometryVector;

	class LibraryGeometries
	{
	public:
		GeometryVector	mGeometries;
	};

	enum NodeType
	{
		NT_NODE,
		NT_LAST
	};

	static NodeType getNodeType(const char *n)
	{
		NodeType ret = NT_LAST;
		if (strcmp(n, "NODE") == 0)
		{
			ret = NT_NODE;
		}
		return ret;
	}

	class InstanceMaterial
	{
	public:
		std::string		mSymbol;
		std::string		mTarget;
	};

	class InstanceGeometry
	{
	public:
		void clear(void)
		{
			InstanceGeometry c;
			*this = c;
		}
		std::string	mURL;
		InstanceMaterial	mInstanceMaterial;
	};

	typedef std::vector< InstanceGeometry > InstanceGeometryVector;

	class Matrix
	{
	public:
		Matrix(void)
		{
			fm_identity(mMatrix);
		}
		std::string	mSID;
		float		mMatrix[16];
	};

	class Node : public IdName
	{
	public:
		void clear(void)
		{
			Node c;
			*this = c;
		}

		NodeType				mType{ NT_LAST };
		Matrix					mMatrix;
		InstanceGeometryVector	mInstanceGeometries;
	};

	typedef std::vector< Node > NodeVector;
	typedef std::vector< SceneNode > SceneNodeVector;

	class VisualScene : public IdName
	{
	public:
		void clear(void)
		{
			VisualScene c;
			*this = c;
		}

		NodeVector		mNodes;
	};

	typedef std::vector< VisualScene > VisualSceneVector;

	class LibraryVisualScenes
	{
	public:
		VisualSceneVector	mVisualScenes;
	};

	class InstanceVisualScene
	{
	public:
		std::string	mURL;
	};

	typedef std::vector< InstanceVisualScene > InstanceVisualSceneVector;

	class Scene
	{
	public:
		InstanceVisualSceneVector	mInstanceVisualScenes;
	};

	class Contributor
	{
	public:
		std::string	mAuthor;
		std::string	mAuthoringTool;
		std::string	mComments;
		std::string	mSourceData;
	};

	enum UnitType
	{
		UT_METER,
		UT_LAST
	};

	static UnitType getUnitType(const char *u)
	{
		UnitType ret = UT_LAST;
		if (strcmp(u, "meter") == 0)
		{
			ret = UT_METER;
		}
		return ret;
	}

	class Unit 
	{
	public:
		UnitType	mType{ UT_LAST };
		float		mValue{ 0 };
	};

	class Asset
	{
	public:
		Contributor	mContributor;
		Unit		mUnit;
		std::string	mCreated;
		std::string	mModified;
		std::string	mUpAxis;
	};

	class Collada
	{
	public:
		std::string			mSchemaLocation;	// The URL location of the COLLADA schema
		std::string			mSchemaVersion;		// The version number of the COLLADA schema
		Asset				mAsset;				// The asset portion of the COLLADA file
		LibraryImages		mLibraryImages;		// Declared but not yet implemented
		LibraryControllers	mLibraryControllers;// Declared but not yet implemented
		LibraryMaterials	mLibraryMaterials;	// Library of materials
		LibraryEffects		mLibraryEffects;	// Library of effects
		LibraryGeometries	mLibraryGeometries;
		LibraryVisualScenes	mLibraryVisualScenes;
		Scene				mScene;
	};


	void reportError(uint32_t lineno,const char *fmt, ...)
	{
#if SHOW_ERROR
		va_list         args;
		char            buffer[4096];
		va_start(args, fmt);
		STRING_HELPER::stringFormatV(buffer, sizeof(buffer), fmt, args);
		va_end(args);
		printf("[ImportDAE:ERROR:Lineno:%d]%s\n", lineno, buffer);
#endif
	}

	// Import as a single mesh the contents of this DAE file
	// This snippet only reads the basic mesh data, not nodes,
	// animation data, skinning, etc. etc.  Just simple meshes
	// suitable for importing URDF files
	virtual void importDAE(const void *data, uint32_t dlen) final
	{
		clear();
		FAST_XML::FastXml *f = FAST_XML::FastXml::create();
		f->processXml(data, dlen, this);
		processMeshes();
		f->release();
	}

	virtual void importDAE(const char *fname) final
	{
		clear();
		FAST_XML::FastXml *f = FAST_XML::FastXml::create();
		f->processXml(fname, this);
		processMeshes();
		f->release();
	}

	virtual void release(void) final
	{
		delete this;
	}

	// XML parser callbacks
	virtual bool processComment(const char *comment) final // encountered a comment in the XML
	{
		return true;
	}
	// 'element' is the name of the element that is being closed.
	// depth is the recursion depth of this element.
	// Return true to continue processing the XML file.
	// Return false to stop processing the XML file; leaves the read pointer of the stream right after this close tag.
	// The bool 'isError' indicates whether processing was stopped due to an error, or intentionally canceled early.
	virtual bool processClose(const char *element, uint32_t depth, bool &isError,uint32_t lineno) final	  // process the 'close' indicator for a previously encountered element
	{
		// We pop the element type stack and revise the current and previous type variables
		if ( (depth+1) != mStackLocation)
		{
			reportError(lineno,"Element Stack is messed up.");
		}
		ElementType type = getElementType(element);
		if (mStackLocation)
		{
			mStackLocation--;
			if (mTypeStack[mStackLocation] != type)
			{
				reportError(lineno,"ElementClose did not match the previous element open! Invalid XML file.");
				mStackLocation++; // don't pop the stack, this was a mismatched close
				return true;
			}
			else
			{
				mCurrentType = mTypeStack[mStackLocation];
				if (mStackLocation)
				{
					mPreviousType = mTypeStack[mStackLocation - 1];
				}
				else
				{
					mPreviousType = ET_LAST;
				}
			}
		}
		switch (type)
		{
			case ET_TECHNIQUE_COMMON:
				if (mPreviousType == ET_BIND_MATERIAL || mPreviousType == ET_SOURCE)
				{
					mHaveTechniqueCommon = false;
				}
				else
				{
					reportError(lineno, "Got element-close <%s> without matching parent <%s> or <%s> instead found <%s>",
						getElementName(type),
						getElementName(ET_BIND_MATERIAL),
						getElementName(ET_SOURCE),
						getElementName(mPreviousType));
				}
				break;
			case ET_INSTANCE_EFFECT:
				if (mPreviousType == ET_MATERIAL)
				{
					mCurrentMaterial.mInstanceEffects.push_back(mCurrentInstanceEffect);
					mCurrentInstanceEffect.clear();
				}
				else
				{
					reportError(lineno,"Got element-close <%s> without matching parent <%s> instead found <%s>",
						getElementName(type),
						getElementName(ET_MATERIAL),
						getElementName(mPreviousType));
				}
				break;
			case ET_MATERIAL:
				if (mPreviousType == ET_LIBRARY_MATERIALS)
				{
					mCollada.mLibraryMaterials.mMaterials.push_back(mCurrentMaterial);
					mCurrentMaterial.clear();
				}
				else
				{
					reportError(lineno, "Got element-close <%s> without matching parent <%s> instead found <%s>",
						getElementName(type),
						getElementName(ET_LIBRARY_MATERIALS),
						getElementName(mPreviousType));
				}
				break;
			case ET_EFFECT:
				if (mPreviousType == ET_LIBRARY_EFFECTS)
				{
					mCollada.mLibraryEffects.mEffects.push_back(mCurrentEffect);
					mCurrentEffect.clear();
				}
				else
				{
					reportError(lineno, "Got element-close <%s> without matching parent <%s> instead found <%s>",
						getElementName(type),
						getElementName(ET_LIBRARY_EFFECTS),
						getElementName(mPreviousType));
				}
				break;
			case ET_GEOMETRY:
				if (mPreviousType == ET_LIBRARY_GEOMETRIES)
				{
					mCollada.mLibraryGeometries.mGeometries.push_back(mCurrentGeometry);
					mCurrentGeometry.clear();
				}
				else
				{
					reportError(lineno, "Got element-close <%s> without matching parent <%s> instead found <%s>",
						getElementName(type),
						getElementName(ET_LIBRARY_GEOMETRIES),
						getElementName(mPreviousType));
				}
				break;
			case ET_MESH:
				if (mPreviousType == ET_GEOMETRY)
				{
					mCurrentGeometry.mMeshes.push_back(mCurrentMesh);
					mCurrentMesh.clear();
				}
				else
				{
					reportError(lineno, "Got element-close <%s> without matching parent <%s> instead found <%s>",
						getElementName(type),
						getElementName(ET_GEOMETRY),
						getElementName(mPreviousType));
				}
				break;
			case ET_SOURCE:
				if (mPreviousType == ET_MESH)
				{
					mCurrentMesh.mSources.push_back(mCurrentSource);
					mCurrentSource.clear();
					mHaveFloatArray = false; // clear the semaphore
					mHaveTechniqueCommon = false;
					mHaveAccessor = false;
				}
				else
				{
					reportError(lineno, "Got element-close <%s> without matching parent <%s> instead found <%s>",
						getElementName(type),
						getElementName(ET_MESH),
						getElementName(mPreviousType));
				}
				break;
			case ET_TRIANGLES:
			case ET_POLYLIST:
				if (mPreviousType == ET_MESH)
				{
					mCurrentMesh.mTriangles.push_back(mCurrentTriangles);
					mCurrentTriangles.clear();
				}
				else
				{
					reportError(lineno, "Got element-close <%s> without matching parent <%s> instead found <%s>",
						getElementName(type),
						getElementName(ET_MESH),
						getElementName(mPreviousType));
				}
				break;
			case ET_VERTICES:
				if (mPreviousType == ET_MESH)
				{
					mCurrentMesh.mVertices.push_back(mCurrentVertices);
					mCurrentVertices.clear();
				}
				else
				{
					reportError(lineno, "Got element-close <%s> without matching parent <%s> instead found <%s>",
						getElementName(type),
						getElementName(ET_MESH),
						getElementName(mPreviousType));
				}
				break;
			case ET_VISUAL_SCENE:
				if (mPreviousType == ET_LIBRARY_VISUAL_SCENES)
				{
					mCollada.mLibraryVisualScenes.mVisualScenes.push_back(mCurrentVisualScene);
					mCurrentVisualScene.clear();
				}
				else
				{
					reportError(lineno, "Got element-close <%s> without matching parent <%s> instead found <%s>",
						getElementName(type),
						getElementName(ET_LIBRARY_VISUAL_SCENES),
						getElementName(mPreviousType));
				}
				break;
			case ET_NODE:
				if (mPreviousType == ET_VISUAL_SCENE)
				{
					mCurrentVisualScene.mNodes.push_back(mCurrentNode);
					mCurrentNode.clear();
				}
				else
				{
					reportError(lineno, "Got element-close <%s> without matching parent <%s> instead found <%s>",
						getElementName(type),
						getElementName(ET_VISUAL_SCENE),
						getElementName(mPreviousType));
				}
				break;
			case ET_INSTANCE_GEOMETRY:
				if (mPreviousType == ET_NODE)
				{
					mCurrentNode.mInstanceGeometries.push_back(mCurrentInstanceGeometry);
					mCurrentInstanceGeometry.clear();
				}
				else
				{
					reportError(lineno, "Got element-close <%s> without matching parent <%s> instead found <%s>",
						getElementName(type),
						getElementName(ET_NODE),
						getElementName(mPreviousType));
				}
				break;
			case ET_BIND_MATERIAL:
				if (mPreviousType == ET_INSTANCE_GEOMETRY )
				{
					mHaveBindMaterial = false;
				}
				else
				{
					reportError(lineno, "Got element-close <%s> without matching parent <%s> instead found <%s>",
						getElementName(type),
						getElementName(ET_INSTANCE_GEOMETRY),
						getElementName(mPreviousType));
				}
				break;
		}

		return true;
	}

	void attributeCheck(uint32_t acount, uint32_t lineno,const char *elementName)
	{
		if (acount)
		{
			reportError(lineno, "Element: %s has attributes which are unexpected.", elementName);
		}
	}

	// To continue processing the XML document, false to skip.
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
			reportError(lineno,"ElementTypes nested too deeply!");
		}

		switch (mCurrentType)
		{
			case ET_ADAPT_THRESH:
			case ET_AMBIENT_DIFFUSE_LOCK:
			case ET_AMBIENT_DIFFUSE_TEXTURE_LOCK:
			case ET_AMOUNT:
			case ET_ANIMATION:
			case ET_APPLY_REFLECTION_DIMMING:
			case ET_AREA_SHAPE:
			case ET_AREA_SIZE:
			case ET_AREA_SIZEY:
			case ET_AREA_SIZEZ:
			case ET_ASPECT_RATIO:
			case ET_ATMOSPHERE_ON:
			case ET_ATMOSPHERE_OPACITY:
			case ET_ATM_DISTANCE_FACTOR:
			case ET_ATM_EXTINCTION_FACTOR:
			case ET_ATM_TURBIDITY:
			case ET_ATT1:
			case ET_ATT2:
			case ET_ATTENUATION_FAR_END:
			case ET_ATTENUATION_NEAR_END:
			case ET_ATTENUATION_NEAR_START:
			case ET_BACKSCATTERED_LIGHT:
			case ET_BIAS:
			case ET_BIND_SHAPE_MATRIX:
			case ET_BIND_VERTEX_INPUT:
			case ET_BLEND_MODE:
			case ET_BLINN:
			case ET_BLUE:
			case ET_BOX:
			case ET_BUFFERS:
			case ET_BUFFLAG:
			case ET_BUFSIZE:
			case ET_BUFTYPE:
			case ET_BUMP:
			case ET_BUMPINTERP:
			case ET_CAMERA:
			case ET_CHANNEL:
			case ET_CLIPEND:
			case ET_CLIPSTA:
			case ET_COMPRESSTHRESH:
			case ET_CONSTANT_ATTENUATION:
			case ET_CONTRAST:
			case ET_CONTROLLER:
			case ET_CONTROL_VERTICES:
			case ET_COPYRIGHT:
			case ET_COVERAGEU:
			case ET_COVERAGEV:
			case ET_DECAY_FALLOFF:
			case ET_DECAY_TYPE:
			case ET_DGNODE_TYPE:
			case ET_DIFFUSE_SOFTEN:
			case ET_DIFFUSE_SPECULAR_LOCK:
			case ET_DIM_LEVEL:
			case ET_DIRECTIONAL:
			case ET_DIST:
			case ET_DOUBLE_SIDED:
			case ET_EDGE:
			case ET_EDGE_ENABLE:
			case ET_EDGE_EXPONENT:
			case ET_END_TIME:
			case ET_ENERGY:
			case ET_EXTENDED_SHADER:
			case ET_EXTRA:
			case ET_FACE:
			case ET_FALLOFF:
			case ET_FALLOFF_TYPE:
			case ET_FAST:
			case ET_FILTER:
			case ET_FILTERTYPE:
			case ET_FILTER_COLOR:
			case ET_FLAG:
			case ET_FORMAT:
			case ET_FRAME_RATE:
			case ET_GAMMA:
			case ET_GENERATEUVS:
			case ET_GREEN:
			case ET_HALO_INTENSITY:
			case ET_HEIGHT:
			case ET_HEIGHTSEGMENTS:
			case ET_HORIZON_BRIGHTNESS:
			case ET_HOTSPOT_BEAM:
			case ET_IMAGE:
			case ET_IMAGE_SEQUENCE:
			case ET_INIT_FROM:
			case ET_INSTANCE_CAMERA:
			case ET_INSTANCE_CONTROLLER:
			case ET_INSTANCE_LIGHT:
			case ET_INTERESTDIST:
			case ET_JOINTS:
			case ET_LAMBERT:
			case ET_LENGTH:
			case ET_LENGTHSEGMENTS:
			case ET_LIBRARY_ANIMATIONS:
			case ET_LIBRARY_CAMERAS:
			case ET_LIBRARY_LIGHTS:
			case ET_LIGHT:
			case ET_LINEAR_ATTENUATION:
			case ET_LUMINOUS:
			case ET_MAGFILTER:
			case ET_METAL:
			case ET_MINFILTER:
			case ET_MIRRORU:
			case ET_MIRRORV:
			case ET_MODE:
			case ET_MULTIPLIER:
			case ET_NAME_ARRAY:
			case ET_NEWPARAM:
			case ET_NOISEU:
			case ET_NOISEV:
			case ET_OFFSETU:
			case ET_OFFSETV:
			case ET_OPACITY:
			case ET_OPACITY_TYPE:
			case ET_OPTICS:
			case ET_ORIGINALMAYANODEID:
			case ET_PERSPECTIVE:
			case ET_POINT:
			case ET_POST_INFINITY:
			case ET_PRE_INFINITY:
			case ET_QUADRATIC_ATTENUATION:
			case ET_RAY_SAMP:
			case ET_RAY_SAMPY:
			case ET_RAY_SAMPZ:
			case ET_RAY_SAMP_METHOD:
			case ET_RAY_SAMP_TYPE:
			case ET_RED:
			case ET_REFLECT:
			case ET_REFLECTION_LEVEL:
			case ET_REFLECTIVE:
			case ET_REFLECTIVITY:
			case ET_REPEATU:
			case ET_REPEATV:
			case ET_ROTATEFRAME:
			case ET_ROTATEUV:
			case ET_SAMP:
			case ET_SAMPLER:
			case ET_SAMPLER2D:
			case ET_SCENE_BOUNDING_MAX:
			case ET_SCENE_BOUNDING_MIN:
			case ET_SHADER:
			case ET_SHADHALOSTEP:
			case ET_SHADING_COEFFICIENTS:
			case ET_SHADOW_ATTRIBUTES:
			case ET_SHADOW_B:
			case ET_SHADOW_COLOR:
			case ET_SHADOW_G:
			case ET_SHADOW_R:
			case ET_SHADSPOTSIZE:
			case ET_SHIFTX:
			case ET_SHIFTY:
			case ET_SI_AMBIENCE:
			case ET_SI_SCENE:
			case ET_SKELETON:
			case ET_SKIN:
			case ET_SKYBLENDFAC:
			case ET_SKYBLENDTYPE:
			case ET_SKY_COLORSPACE:
			case ET_SKY_EXPOSURE:
			case ET_SOFT:
			case ET_SOFTEN:
			case ET_SPLINE:
			case ET_SPOTBLEND:
			case ET_SPOTSIZE:
			case ET_SPREAD:
			case ET_STAGGER:
			case ET_START_TIME:
			case ET_SUN_BRIGHTNESS:
			case ET_SUN_EFFECT_TYPE:
			case ET_SUN_INTENSITY:
			case ET_SUN_SIZE:
			case ET_SURFACE:
			case ET_TEXTURE:
			case ET_TRANSLATEFRAMEU:
			case ET_TRANSLATEFRAMEV:
			case ET_TRANSPARENCY:
			case ET_TRANSPARENT:
			case ET_TYPE:
			case ET_USE_FAR_ATTENUATION:
			case ET_USE_NEAR_ATTENUATION:
			case ET_USE_SELF_ILLUM_COLOR:
			case ET_V:
			case ET_VERTEX_WEIGHTS:
			case ET_WIDTH:
			case ET_WIDTHSEGMENTS:
			case ET_WIRE_SIZE:
			case ET_WIRE_UNITS:
			case ET_WRAPU:
			case ET_WRAPV:
			case ET_XFOV:
			case ET_XSI_CAMERA:
			case ET_XSI_PARAM:
			case ET_YFOV:
			case ET_YF_DOFDIST:
			case ET_ZFAR:
			case ET_ZNEAR:
				reportError(lineno, "Element: %s not yet supported.", elementName);
				break;
			case ET_SCALE:
				if (mPreviousType == ET_NODE)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
						case AT_SID:
							if (strcmp(value, "scale") == 0)
							{
								
							}
							else
							{
								reportError(lineno, "Expected <sid> to be 'scale' found '%s'", value);
							}
							break;
						default:
							reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
							break;
						}
					}
					float sx, sy, sz;
					// Get the translation component
					STRING_HELPER::getVec3(elementData, nullptr, sx, sy, sz);
					float scaleMatrix[16];
					fm_identity(scaleMatrix);
					fm_setScale(sx, sy, sz, scaleMatrix);
					float px, py, pz;
					fm_getTranslation(mCurrentNode.mMatrix.mMatrix, px, py, pz); // get the old translation
					fm_setTranslation(mCurrentNode.mMatrix.mMatrix, 0, 0, 0);
					fm_matrixMultiply(mCurrentNode.mMatrix.mMatrix, scaleMatrix, mCurrentNode.mMatrix.mMatrix);
					fm_setTranslation(mCurrentNode.mMatrix.mMatrix, px, py, pz);
				}
				else
				{
					reportError(lineno, "Nesting error <translate> element does not have <node> element as a parent.");
				}
				break;
			case ET_ROTATE:
				if (mPreviousType == ET_NODE)
				{
					uint32_t matrixIndex = 0xFFFFFFFF;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
						case AT_SID:
							if (strcmp(value, "rotateX") == 0 )
							{
								matrixIndex = 0;
							}
							if (strcmp(value, "rotateY") == 0)
							{
								matrixIndex = 4;
							}
							if (strcmp(value, "rotateZ") == 0)
							{
								matrixIndex = 8;
							}
							else
							{
								reportError(lineno, "Expected <sid> to be 'rotateX'or 'rotateY' or 'rotateZ' found '%s'", value);
							}
							break;
						default:
							reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
							break;
						}
					}
					if (matrixIndex != 0xFFFFFFFF)
					{
						// Get the translation component
						STRING_HELPER::getVec4(elementData, nullptr,
							mCurrentNode.mMatrix.mMatrix[matrixIndex + 0],
							mCurrentNode.mMatrix.mMatrix[matrixIndex + 1],
							mCurrentNode.mMatrix.mMatrix[matrixIndex + 2],
							mCurrentNode.mMatrix.mMatrix[matrixIndex + 3]);
					}
				}
				else
				{
					reportError(lineno, "Nesting error <translate> element does not have <node> element as a parent.");
				}
				break;
			case ET_TRANSLATE:
				if (mPreviousType == ET_NODE)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
						case AT_SID:
							if (strcmp(value, "translate") == 0)
							{

							}
							else
							{
								reportError(lineno, "Expected <sid> to be 'translate' found '%s'", value);
							}
							break;
						default:
							reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
							break;
						}
					}
					// Get the translation component
					STRING_HELPER::getVec3(elementData,nullptr,
						mCurrentNode.mMatrix.mMatrix[12],
						mCurrentNode.mMatrix.mMatrix[13],
						mCurrentNode.mMatrix.mMatrix[14]);
				}
				else
				{
					reportError(lineno, "Nesting error <translate> element does not have <node> element as a parent.");
				}
				break;
			case ET_MATRIX:
				if (mPreviousType == ET_NODE)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
							case AT_SID:
								mCurrentNode.mMatrix.mSID = std::string(value);
								break;
							default:
								reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
								break;
						}
					}
					const char *scan = elementData;
					const char *next = nullptr;
					Matrix m;
					for (uint32_t i = 0; i < 16; i++)
					{
						if (scan)
						{
							float v = STRING_HELPER::getFloatValue(scan, &next);
							m.mMatrix[i] = v;
							mCurrentSource.mFloatArray.mFloatArray.push_back(v);
							if (next)
							{
								scan = next;
							}
						}
						else
						{
							m.mMatrix[i] = 0;
						}
					}
					mCurrentNode.mMatrix = m;
				}
				else
				{
					reportError(lineno, "Nesting error <matrix> element does not have <node> element as a parent.");
				}
				break;
			case ET_LIBRARY_CONTROLLERS:
				if (mPreviousType == ET_COLLADA)
				{
					attributeCheck(acount, lineno, elementName);
				}
				else
				{
					reportError(lineno, "Nesting error <library_controllers> element does not have <COLLADA> element as a parent.");
				}
				break;
			case ET_LIBRARY_IMAGES:
				if (mPreviousType == ET_COLLADA)
				{
					attributeCheck(acount, lineno, elementName);
				}
				else
				{
					reportError(lineno, "Nesting error <library_images> element does not have <COLLADA> element as a parent.");
				}
				break;
			case ET_UNIT:
				if (mPreviousType == ET_ASSET)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
							case AT_NAME:
								mCollada.mAsset.mUnit.mType = getUnitType(value);
								if (mCollada.mAsset.mUnit.mType == UT_LAST)
								{
									reportError(lineno, "Unknown unit type (%s)", value);
								}
								break;
							case AT_METER:
								mCollada.mAsset.mUnit.mValue = float(atof(value));
								break;
							default:
								reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
								break;
						}
					}
				}
				else
				{
					reportError(lineno, "Nesting error <unit> element does not have <asset> element as a parent.");
				}
				break;
			case ET_AUTHOR:
				if (mPreviousType == ET_CONTRIBUTOR)
				{
					attributeCheck(acount, lineno, elementName);
					mCollada.mAsset.mContributor.mAuthor = std::string(elementData);
				}
				else
				{
					reportError(lineno, "Nesting error <scene> element does not have <COLLADA> element as a parent.");
				}
				break;
			case ET_INSTANCE_VISUAL_SCENE:
				if (mPreviousType == ET_SCENE )
				{
					InstanceVisualScene ivs;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
							case AT_URL:
								ivs.mURL = std::string(value);
								break;
							default:
								reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
								break;
						}
					}
					mCollada.mScene.mInstanceVisualScenes.push_back(ivs);
				}
				else
				{
					reportError(lineno, "Nesting error <instance_material> element does not have <technique_common> element as a parent.");
				}
				break;
			case ET_SCENE:
				if (mPreviousType == ET_COLLADA)
				{
					attributeCheck(acount, lineno, elementName);
				}
				else
				{
					reportError(lineno, "Nesting error <scene> element does not have <COLLADA> element as a parent.");
				}
				break;
			case ET_INSTANCE_MATERIAL:
				if (mPreviousType == ET_TECHNIQUE_COMMON)
				{
					InstanceMaterial im;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
						case AT_SYMBOL:
							im.mSymbol = std::string(value);
							break;
						case AT_TARGET:
							im.mTarget = std::string(value);
							break;
						default:
							reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
							break;
						}
					}
					mCurrentInstanceGeometry.mInstanceMaterial = im;
				}
				else
				{
					reportError(lineno, "Nesting error <instance_material> element does not have <technique_common> element as a parent.");
				}
				break;
			case ET_BIND_MATERIAL:
				if (mPreviousType == ET_INSTANCE_GEOMETRY)
				{
					if (mHaveBindMaterial)
					{
						reportError(lineno, "Unexpected element <%s> found more than once inside parent <%s>", elementName, getElementName(mPreviousType));
					}
					mHaveBindMaterial = true;
					attributeCheck(acount, lineno, elementName);
				}
				else
				{
					reportError(lineno, "Nesting error <bind_material> element does not have <instance_geometry> element as a parent.");
				}
				break;
			case ET_INSTANCE_GEOMETRY:
				if (mPreviousType == ET_NODE)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
						case AT_URL:
							mCurrentInstanceGeometry.mURL = std::string(value);
							break;
						default:
							reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
							break;
						}
					}
				}
				else
				{
					reportError(lineno, "Nesting error <instance_geometry> element does not have <node> element as a parent.");
				}
				break;
			case ET_NODE:
				if (mPreviousType == ET_VISUAL_SCENE)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
						case AT_ID:
							mCurrentNode.mId = std::string(value);
							break;
						case AT_NAME:
							mCurrentNode.mName = std::string(value);
							break;
						case AT_TYPE:
							mCurrentNode.mType = getNodeType(value);
							if (mCurrentNode.mType == NT_LAST)
							{
								reportError(lineno, "Unknown node type(%s) encountered", value);
							}
							break;
						default:
							reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
							break;
						}
					}
				}
				else
				{
					reportError(lineno, "Nesting error <node> element does not have <visual_scene> element as a parent.");
				}
				break; 
			case ET_VISUAL_SCENE:
				if (mPreviousType == ET_LIBRARY_VISUAL_SCENES)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
						case AT_ID:
							mCurrentVisualScene.mId = std::string(value);
							break;
						case AT_NAME:
							mCurrentVisualScene.mName = std::string(value);
							break;
						default:
							reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
							break;
						}
					}
				}
				else
				{
					reportError(lineno, "Nesting error <visual_scene> element does not have <library_visual_scenes> element as a parent.");
				}
				break;
			case ET_LIBRARY_VISUAL_SCENES:
				if (mPreviousType == ET_COLLADA )
				{
					attributeCheck(acount, lineno, elementName);
				}
				else
				{
					reportError(lineno, "Nesting error <library_visual_scenes> element does not have <COLLADA> element as a parent.");
				}
				break;
			case ET_P:
				if (mPreviousType == ET_TRIANGLES || mPreviousType == ET_POLYLIST )
				{
					attributeCheck(acount, lineno, elementName);
					const char *scan = elementData;
					while (scan)
					{
						const char *next;
						uint32_t v = STRING_HELPER::getUint32Value(scan, &next);
						mCurrentTriangles.mIndices.push_back(v);
						scan = next;
					}
				}
				else
				{
					reportError(lineno, "Nesting error <p> element does not have <triangles> element as a parent.");
				}
				break;
			case ET_VCOUNT:
				if (mPreviousType == ET_POLYLIST)
				{
					attributeCheck(acount, lineno, elementName);
					const char *scan = elementData;
					while (scan)
					{
						const char *next;
						uint32_t v = STRING_HELPER::getUint32Value(scan, &next);
						mCurrentTriangles.mVcount.push_back(v);
						scan = next;
					}
				}
				else
				{
					reportError(lineno, "Nesting error <vcount> element does not have <polylist> element as a parent.");
				}
				break;
			case ET_TRIANGLES:
			case ET_POLYLIST:
				if (mPreviousType == ET_MESH)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
						case AT_COUNT:
							mCurrentTriangles.mCount = atoi(value);
							break;
						case AT_MATERIAL:
							mCurrentTriangles.mMaterial = std::string(value);
							break;
						default:
							reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
							break;
						}
					}
				}
				else
				{
					reportError(lineno, "Nesting error <%s> element does not have <mesh> element as a parent.", getElementName(mCurrentType));
				}
				break;
			case ET_INPUT:
				// Both vertices elements and triangles elements have inputs
				// However vertices are only expected to have a single input
				// while triangles have multiple inputs
				if (mPreviousType == ET_VERTICES || mPreviousType == ET_TRIANGLES || mPreviousType == ET_POLYLIST)
				{
					Input input;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
						case AT_SEMANTIC:
							input.mSemantic = getSemanticType(value);
							if (input.mSemantic == ST_LAST)
							{
								reportError(lineno, "Unknown Semantic type '%s' encountered.", value);
							}
							break;
						case AT_SOURCE:
							input.mSource = std::string(value);
							break;
						case AT_OFFSET:
							input.mOffset = atoi(value);
							break;
						case AT_SET:
							input.mSet = atoi(value);
							break;
						default:
							reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
							break;
						}
					}
					if (mPreviousType == ET_VERTICES)
					{
						mCurrentVertices.mInputs.push_back(input);
					}
					else
					{
						mCurrentTriangles.mInputs.push_back(input);
					}
				}
				else
				{
					reportError(lineno, "Nesting error <vertices> element does not have <mesh> element as a parent.");
				}
				break;
			case ET_VERTICES:
				if (mPreviousType == ET_MESH)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
							case AT_ID:
								mCurrentVertices.mId = std::string(value);
								break;
							case AT_NAME:
								mCurrentVertices.mName = std::string(value);
								break;
							default:
								reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
								break;
						}
					}
				}
				else
				{
					reportError(lineno, "Nesting error <vertices> element does not have <mesh> element as a parent.");
				}
				break;
			case ET_PARAM:
				if (mPreviousType == ET_ACCESSOR)
				{
					Param p;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
						case AT_NAME:
							p.mNameType = getParamNameType(value);
							if (p.mNameType == PNT_LAST)
							{
								reportError(lineno, "Unknown Param name type '%s'", value);
							}
							break;
						case AT_TYPE:
							p.mType = getParamType(value);
							if (p.mType == PT_LAST)
							{
								reportError(lineno, "Unknown param type '%s'", value);
							}
							break;
						default:
							reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
							break;
						}
					}
					mCurrentSource.mSourceTechniqueCommon.mParams.push_back(p);
				}
				else
				{
					reportError(lineno, "Nesting error <param> element does not have <accessor> element as a parent.");
				}
				break;
			case ET_ACCESSOR:
				if (mPreviousType == ET_TECHNIQUE_COMMON)
				{
					if (mHaveAccessor)
					{
						reportError(lineno, "Unexpected element <%s> found more than once inside parent <%s>", elementName, getElementName(mPreviousType));
					}
					mHaveAccessor = true;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
							case AT_STRIDE:
								mCurrentSource.mSourceTechniqueCommon.mStride = atoi(value);
								break;
							case AT_SOURCE:
								mCurrentSource.mSourceTechniqueCommon.mSource = std::string(value);
								break;
							case AT_COUNT:
								mCurrentSource.mSourceTechniqueCommon.mCount = atoi(value);
								break;
							default:
								reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
								break;
						}
					}
				}
				else
				{
					reportError(lineno, "Nesting error <accessor> element does not have <technique_common> element as a parent.");
				}
				break;
			case ET_TECHNIQUE_COMMON:
				if (mPreviousType == ET_SOURCE || mPreviousType == ET_BIND_MATERIAL )
				{
					//
					if (mHaveTechniqueCommon)
					{
						reportError(lineno, "Unexpected element <%s> found more than once inside parent <%s>", elementName, getElementName(mPreviousType));
					}
					mHaveTechniqueCommon = true;
				}
				else
				{
					reportError(lineno, "Nesting error <technique_common> element does not have <source> element as a parent.");
				}
				break;
			case ET_FLOAT_ARRAY:
				if (mPreviousType == ET_SOURCE)
				{
					if (mHaveFloatArray)
					{
						reportError(lineno,"Unexpected element <%s> found more than once inside parent <%s>", elementName, getElementName(mPreviousType));
					}
					mHaveFloatArray = true;
					uint32_t count = 0;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
							case AT_ID:
								mCurrentSource.mFloatArray.mId = std::string(value);
								break;
							case AT_NAME:
								mCurrentSource.mFloatArray.mName = std::string(value);
								break;
							case AT_COUNT:
								count = atoi(value);
								break;
							default:
								reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
								break;
							}
					}
					const char *scan = elementData;
					const char *next = nullptr;
					for (uint32_t i = 0; i < count; i++)
					{
						if (scan)
						{
							float v = STRING_HELPER::getFloatValue(scan, &next);
							mCurrentSource.mFloatArray.mFloatArray.push_back(v);
							if (next)
							{
								scan = next;
							}
						}
						else
						{
							mCurrentSource.mFloatArray.mFloatArray.push_back(0.0);
						}
					}
				}
				else
				{
					reportError(lineno, "Nesting error <float_array> element does not have <source> element as a parent.");
				}
				break;
			case ET_SOURCE:
				if (mPreviousType == ET_MESH)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
						case AT_ID:
							mCurrentSource.mId = std::string(value);
							break;
						case AT_NAME:
							mCurrentSource.mName = std::string(value);
							break;
						default:
							reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
							break;
						}
					}
				}
				else
				{
					reportError(lineno, "Nesting error <source> element does not have <mesh> element as a parent.");
				}
				break;
			case ET_MESH:
				if (mPreviousType == ET_GEOMETRY)
				{
					//
					attributeCheck(acount, lineno, elementName);
				}
				else
				{
					reportError(lineno, "Nesting error <mesh> element does not have <geometry> element as a parent.");
				}
				break;
			case ET_GEOMETRY:
				if (mPreviousType == ET_LIBRARY_GEOMETRIES)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						AttributeType at = getAttributeType(key);
						const char *value = argv[i * 2 + 1];
						switch (at)
						{
						case AT_ID:
							mCurrentGeometry.mId = std::string(value);
							break;
						case AT_NAME:
							mCurrentGeometry.mName = std::string(value);
							break;
						default:
							reportError(lineno, "Unsupported attribute type '%s' in <%s>", key, elementName);
							break;
						}
					}
				}
				else
				{
					reportError(lineno, "Nesting error <geometry> element does not have <library_geometries> element as a parent.");
				}
				break;
			case ET_LIBRARY_GEOMETRIES:
				if (mPreviousType == ET_COLLADA)
				{
					attributeCheck(acount, lineno, elementName);
				}
				else
				{
					reportError(lineno, "Nesting error <library_geometries> element does not have <COLLADA> element as a parent.");
				}
				break;
			case ET_COLLADA:
				for (uint32_t i = 0; i <acount; i++)
				{
					const char *key		= argv[i * 2 + 0];
					AttributeType at = getAttributeType(key);
					const char *value	= argv[i * 2 + 1];
					switch (at)
					{
						case AT_XMLNS:
							mCollada.mSchemaLocation = std::string(value);
							break;
						case AT_VERSION:
							mCollada.mSchemaVersion = std::string(value);
							break;
						default:
							reportError(lineno, "Unknown attribute in the COLLADA element");
							break;
					}
				}
				break;
			case ET_ASSET:
				attributeCheck(acount, lineno, elementName);
				if (mPreviousType != ET_COLLADA)
				{
					reportError(lineno, "Nesting error <asset> element does not have COLLADA element as a parent.");
				}
				break;
			case ET_CONTRIBUTOR:
				attributeCheck(acount, lineno, elementName);
				if (mPreviousType != ET_ASSET)
				{
					reportError(lineno, "Nesting error <contributor> element does not have <asset> element as a parent.");
				}
				break;
			case ET_AUTHORING_TOOL:
				if (mPreviousType == ET_CONTRIBUTOR)
				{
					attributeCheck(acount, lineno, elementName);
					mCollada.mAsset.mContributor.mAuthoringTool = std::string(elementData);
				}
				else
				{
					reportError(lineno, "Nesting error <authoring_tool> element does not have <contributor> element as a parent.");
				}
				break;
			case ET_COMMENTS:
				if (mPreviousType == ET_CONTRIBUTOR)
				{
					attributeCheck(acount, lineno, elementName);
					mCollada.mAsset.mContributor.mComments = std::string(elementData);
				}
				else
				{
					reportError(lineno, "Nesting error <comments> element does not have <contributor> element as a parent.");
				}
				break;
			case ET_SOURCE_DATA:
				if (mPreviousType == ET_CONTRIBUTOR)
				{
					attributeCheck(acount, lineno, elementName);
					mCollada.mAsset.mContributor.mSourceData = std::string(elementData);
				}
				else
				{
					reportError(lineno, "Nesting error <source_data> element does not have <contributor> element as a parent.");
				}
				break;
			case ET_CREATED:
				if (mPreviousType == ET_ASSET)
				{
					attributeCheck(acount, lineno, elementName);
					mCollada.mAsset.mCreated = std::string(elementData);
				}
				else
				{
					reportError(lineno, "Nesting error <created> element does not have <asset> element as a parent.");
				}
				break;
			case ET_MODIFIED:
				if (mPreviousType == ET_ASSET)
				{
					attributeCheck(acount, lineno, elementName);
					mCollada.mAsset.mModified = std::string(elementData);
				}
				else
				{
					reportError(lineno, "Nesting error <modified> element does not have <asset> element as a parent.");
				}
				break;
			case ET_UP_AXIS:
				if (mPreviousType == ET_ASSET)
				{
					attributeCheck(acount, lineno, elementName);
					mCollada.mAsset.mUpAxis = std::string(elementData);
				}
				else
				{
					reportError(lineno, "Nesting error <up_axis> element does not have <asset> element as a parent.");
				}
				break;
			case ET_LIBRARY_MATERIALS:
				if (mPreviousType == ET_COLLADA)
				{
					attributeCheck(acount, lineno, elementName);
				}
				else
				{
					reportError(lineno, "Nesting error <library_materials> element does not have <COLLADA> element as a parent.");
				}
				break;
			case ET_MATERIAL:
				if (mPreviousType == ET_LIBRARY_MATERIALS)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_ID:
							mCurrentMaterial.mId = std::string(value);
							break;
						case AT_NAME:
							mCurrentMaterial.mName = std::string(value);
							break;
						default:
							reportError(lineno, "Unknown attribute '%s' in element <material>", key);
							break;
						}
					}
				}
				else
				{
					reportError(lineno, "Nesting error <material> element does not have <library_materials> element as a parent.");
				}
				break;
			case ET_INSTANCE_EFFECT:
				if (mPreviousType == ET_MATERIAL)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
						case AT_URL:
							mCurrentInstanceEffect.mURL = std::string(value);
							break;
						default:
							reportError(lineno, "Unknown attribute '%s' in element <instance_effect>", key);
							break;
						}
					}
				}
				else
				{
					reportError(lineno, "Nesting error <instance_effect> element does not have <material> element as a parent.");
				}
				break;
			case ET_LIBRARY_EFFECTS:
				if (mPreviousType == ET_COLLADA)
				{
					attributeCheck(acount, lineno, elementName);
				}
				else
				{
					reportError(lineno, "Nesting error <library_effects> element does not have <COLLADA> element as a parent.");
				}
				break;
			case ET_EFFECT:
				if (mPreviousType == ET_LIBRARY_EFFECTS)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
							case AT_ID:
								mCurrentEffect.mId = std::string(value);
								break;
							case AT_NAME:
								mCurrentEffect.mName = std::string(value);
								break;
							default:
								reportError(lineno, "Unknown attribute '%s' in <effect>", key);
								break;
						}
					}
				}
				else
				{
					reportError(lineno, "Nesting error <effect> element does not have <library_effects> element as a parent.");
				}
				break;
			case ET_PROFILE_COMMON:
				if (mPreviousType == ET_EFFECT)
				{
					attributeCheck(acount, lineno, elementName);
				}
				else
				{
					reportError(lineno, "Nesting error <profile_COMMON> element does not have <effect> element as a parent.");
				}
				break;
			case ET_TECHNIQUE:
				if (mPreviousType == ET_PROFILE_COMMON)
				{
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
							case AT_SID:
								mCurrentEffect.mProfileCommon.mTechnique.mId = std::string(value);
								break;
							default:
								reportError(lineno, "Unknown attribute '%s' in <technique>", key);
								break;
						}
					}
				}
				else
				{
					reportError(lineno, "Nesting error <effect> element does not have <library_effects> element as a parent.");
				}
				break;
			case ET_PHONG:
				if (mPreviousType == ET_TECHNIQUE)
				{
					attributeCheck(acount, lineno, elementName);
				}
				else
				{
					reportError(lineno, "Nesting error <phong> element does not have <technique> element as a parent.");
				}
				break;
			case ET_DIFFUSE:
			case ET_EMISSION:
			case ET_AMBIENT:
			case ET_SPECULAR:
			case ET_SHININESS:
			case ET_INDEX_OF_REFRACTION:
				if (mPreviousType == ET_PHONG)
				{
					attributeCheck(acount, lineno, elementName);
				}
				else
				{
					reportError(lineno, "Nesting error <%s> element does not have <phong> element as a parent.", getElementName(mCurrentType));
				}
				break;
			case ET_COLOR:
				if (mPreviousPreviousType == ET_PHONG)
				{
					Color c;
					for (uint32_t i = 0; i < acount; i++)
					{
						const char *key = argv[i * 2 + 0];
						const char *value = argv[i * 2 + 1];
						AttributeType at = getAttributeType(key);
						switch (at)
						{
							case AT_SID:
								c.mSid = std::string(value);
								break;
							default:
								reportError(lineno, "Unknown attribute '%s' in <color> element.", key);
								break;
						}
					}
					STRING_HELPER::getVec4(elementData, nullptr, c.mColor.x, c.mColor.y, c.mColor.z, c.mColor.w);
					switch (mPreviousType)
					{
						case ET_DIFFUSE:
							mCurrentEffect.mProfileCommon.mTechnique.mPhong.mDiffuse = c;
							break;
						case ET_SPECULAR:
							mCurrentEffect.mProfileCommon.mTechnique.mPhong.mSpecular = c;
							break;
						case ET_EMISSION:
							mCurrentEffect.mProfileCommon.mTechnique.mPhong.mEmission = c;
							break;
						case ET_AMBIENT:
							mCurrentEffect.mProfileCommon.mTechnique.mPhong.mAmbient = c;
							break;
						default:
							reportError(lineno,"Unknown color type.");
							break;
					}
				}
				else
				{
					reportError(lineno, "Nesting error <color> element does not have <phong> element as a parent's parent.");
				}
				break;
			case ET_FLOAT:
				if (mPreviousPreviousType == ET_PHONG)
				{
					if (mPreviousType == ET_SHININESS)
					{
						for (uint32_t i = 0; i < acount; i++)
						{
							const char *key = argv[i * 2 + 0];
							const char *value = argv[i * 2 + 1];
							AttributeType at = getAttributeType(key);
							switch (at)
							{
								case AT_SID:
									mCurrentEffect.mProfileCommon.mTechnique.mPhong.mShininess.mSid = std::string(value);
									break;
								default:
									reportError(lineno, "Unknown attribute '%s' in <float> element.", key);
									break;
							}
						}
						mCurrentEffect.mProfileCommon.mTechnique.mPhong.mShininess.mShininess = STRING_HELPER::getFloatValue(elementData, nullptr);
					}
					else if (mPreviousType == ET_INDEX_OF_REFRACTION)
					{
						for (uint32_t i = 0; i < acount; i++)
						{
							const char *key = argv[i * 2 + 0];
							const char *value = argv[i * 2 + 1];
							AttributeType at = getAttributeType(key);
							switch (at)
							{
							case AT_SID:
								mCurrentEffect.mProfileCommon.mTechnique.mPhong.mIndexOfRefraction.mSid = std::string(value);
								break;
							default:
								reportError(lineno, "Unknown attribute '%s' in <float> element.", key);
								break;
							}
						}
						mCurrentEffect.mProfileCommon.mTechnique.mPhong.mIndexOfRefraction.mIndexOfRefraction = STRING_HELPER::getFloatValue(elementData, nullptr);
					}
					else
					{
						reportError(lineno, "Unexepcted <float> element; not under <shininess>");
					}
				}
				else
				{
					reportError(lineno, "Unexpected <float> element; not under <phong>");
				}
				break;
			default:
				reportError(lineno, "Unknown ElementType: %s", elementName);
				break;
		}
		return true;
	}

	// process the XML declaration header
	virtual bool processXmlDeclaration(
		uint32_t argc,
		const char ** argv,
		const char  * elementData,
		uint32_t lineno) final
	{
		for (uint32_t i = 0; i < argc; i++)
		{
			const char *key = argv[i * 2 + 0];
			const char *value = argv[i * 2 + 1];

			if (strcmp(key, "version") == 0)
			{
				mVersion = std::string(value);
				if ( strcmp(value,"1.0") != 0 )
				{
					reportError(lineno,"Unknown version provided: %s; expected 1.0", value);
				}
			}
			else if (strcmp(key, "encoding") == 0)
			{
				mEncoding = std::string(value);
				if (strcmp(value, "utf-8") != 0)
				{
					reportError(lineno, "Unknown encoding format (%s) expected 'utf-8'", value);
				}
			}
			else
			{
				reportError(lineno, "Unknown XmlDeclaration Attribute: %s=%s", key, value);
			}
		}
		return true;
	}

	virtual bool processDoctype(
		const char * rootElement, //Root element tag
		const char * type,        //SYSTEM or PUBLIC
		const char * fpi,         //Formal Public Identifier
		const char * uri) final         //Path to schema file
	{
		return true;
	}

	// Build the actual triangle meshes from the COLLADA file inputs
	void processMeshes(void)
	{
		for (auto &i : mCollada.mLibraryGeometries.mGeometries)
		{
			ImportMesh im;
			im.mName = i.mId; // get the name of the mesh
			for (auto &j : i.mMeshes)
			{
				// For each batch of triangles...
				for (auto &k : j.mTriangles)
				{
					ImportSubMesh sm;
					sm.mMaterial = k.mMaterial; // assign the material

					const Input *positions = nullptr;
					const Source *positionsSource = nullptr;
					const Input *normals = nullptr;
					const Source *normalsSource = nullptr;
					const Input *texels = nullptr;
					const Source *texelsSource = nullptr;
					for (auto &l : k.mInputs)
					{
						const char *sourceName = l.mSource.c_str();
						if (*sourceName == '#')
						{
							sourceName++;
						}
						std::string _sourceName(sourceName);

						// Validate that this source works for texture co-ordinates
						// Expected names and types
						auto validateTexelsSource = [this](const Source *&texelsSource)
						{
							// Validate that this 'source' works as texels input
							// We expect it to be in a standardized layout only
							const SourceTechniqueCommon &s = texelsSource->mSourceTechniqueCommon;
							uint32_t vcount = uint32_t(texelsSource->mFloatArray.mFloatArray.size() / 2);
							if (s.mCount == vcount &&
								s.mStride == 2 &&
								s.mParams.size() == 2 &&
								s.mParams[0].mNameType == PNT_S &&
								s.mParams[0].mType == PT_FLOAT &&
								s.mParams[1].mNameType == PNT_T &&
								s.mParams[1].mType == PT_FLOAT)
							{
								// all standard!
							}
							else
							{
								texelsSource = nullptr;
								reportError(0, "Normals source is in a non-standard layout!");
							}
						};

						// Validate that this source works as normals input
						auto validateNormalsSource = [this](const Source *&normalsSource)
						{
							// Validate that this 'source' works as normals input
							// We expect it to be in a standardized layout only
							const SourceTechniqueCommon &s = normalsSource->mSourceTechniqueCommon;
							uint32_t vcount = uint32_t(normalsSource->mFloatArray.mFloatArray.size() / 3);
							if (s.mCount == vcount &&
								s.mStride == 3 &&
								s.mParams.size() == 3 &&
								s.mParams[0].mNameType == PNT_X &&
								s.mParams[0].mType == PT_FLOAT &&
								s.mParams[1].mNameType == PNT_Y &&
								s.mParams[1].mType == PT_FLOAT &&
								s.mParams[2].mNameType == PNT_Z &&
								s.mParams[2].mType == PT_FLOAT)
							{
								// all standard!
							}
							else
							{
								normalsSource = nullptr;
								reportError(0, "Normals source is in a non-standard layout!");
							}
						};

						// 
						auto validatePositionsSource =[this](const Source *&positionsSource)
						{
							// Validate that this 'source' works as position input
							// We expect it to be in a standardized layout only
							const SourceTechniqueCommon &s = positionsSource->mSourceTechniqueCommon;
							uint32_t vcount = uint32_t(positionsSource->mFloatArray.mFloatArray.size() / 3);
							if (s.mCount == vcount &&
								s.mStride == 3 &&
								s.mParams.size() == 3 &&
								s.mParams[0].mNameType == PNT_X &&
								s.mParams[0].mType == PT_FLOAT &&
								s.mParams[1].mNameType == PNT_Y &&
								s.mParams[1].mType == PT_FLOAT &&
								s.mParams[2].mNameType == PNT_Z &&
								s.mParams[2].mType == PT_FLOAT)
							{
								// all standard!
							}
							else
							{
								positionsSource = nullptr;
								reportError(0, "Positions source is in a non-standard layout!");
							}
						};


						switch (l.mSemantic)
						{
							case ST_VERTEX:
								positionsSource = j.findVertexSource(_sourceName,ST_POSITION);
								if (positionsSource)
								{
									positions = &l;
									validatePositionsSource(positionsSource);
								}
								else
								{
									reportError(0, "Failed to locate positionsSource(%s)", _sourceName);
								}
								// See if normals are part of the VERTEX list
								if (normals == nullptr)
								{
									normalsSource = j.findVertexSource(_sourceName, ST_NORMAL);
									if (normalsSource)
									{
										normals = &l;
										validateNormalsSource(normalsSource);
									}
								}
								// See if normals are part of the VERTEX list
								if (texels == nullptr)
								{
									texelsSource = j.findVertexSource(_sourceName, ST_TEXCOORD);
									if (texelsSource)
									{
										texels = &l;
										validateTexelsSource(texelsSource);
									}
								}
								break;
							case ST_POSITION:
								positions = &l;
								positionsSource = j.findSource(_sourceName);
								if (normalsSource)
								{
									validatePositionsSource(positionsSource);
								}
								else
								{
									reportError(0, "Failed to locate normalsSource(%s)", _sourceName);
								}
								break;
							case ST_NORMAL:
								normals = &l;
								normalsSource = j.findSource(_sourceName);
								if (normalsSource)
								{
									validateNormalsSource(normalsSource);
								}
								else
								{
									reportError(0, "Failed to locate normalsSource(%s)", _sourceName);
								}
								break;
							case ST_TEXCOORD:
								texels = &l;
								texelsSource = j.findSource(_sourceName);
								if (texelsSource)
								{
									validateTexelsSource(texelsSource);
								}
								else
								{
									reportError(0, "Failed to locate texelsSource(%s)", _sourceName);
								}
								break;
						}
					}
					// If it's triangles instead of polygons
					uint32_t icount = uint32_t(k.mIndices.size());
					uint32_t tcount = k.mCount; // number of triangles...
					uint32_t index = 0;
					uint32_t scount = uint32_t(k.mInputs.size());
					// For each triangle 

					auto composeVertex = [this](TrianglesPolyList &k,
												ImportDAE::Vertex &v,
												uint32_t index,
												uint32_t scount,
												const Input *positions,
												const Source *positionsSource,
												const Input *normals,
												const Source *normalsSource,
												const Input *texels,
												const Source *texelsSource)
					{
						// Get the triangle indices
						const uint32_t *indices = &k.mIndices[index];
						if (positions && positionsSource)
						{
							uint32_t i0 = positions->mOffset;
							if (i0 < scount) // must be less than the number of inputs
							{
								// Position lookup...
								uint32_t i1 = indices[i0];
								if ((i1 * 3) < positionsSource->mFloatArray.mFloatArray.size())
								{
									const float *p = &positionsSource->mFloatArray.mFloatArray[i1 * 3];
									v.mPosition[0] = p[0];
									v.mPosition[1] = p[1];
									v.mPosition[2] = p[2];
								}
							}
						}
						if (normals && normalsSource)
						{
							uint32_t i0 = normals->mOffset;
							if (i0 < scount) // must be less than the number of inputs
							{
								// Position lookup...
								uint32_t i1 = indices[i0];
								if ((i1 * 3) < normalsSource->mFloatArray.mFloatArray.size())
								{
									const float *p = &normalsSource->mFloatArray.mFloatArray[i1 * 3];
									v.mNormal[0] = p[0];
									v.mNormal[1] = p[1];
									v.mNormal[2] = p[2];
								}
							}
						}
						if (texels && texelsSource)
						{
							uint32_t i0 = texels->mOffset;
							if (i0 < scount) // must be less than the number of inputs
							{
								// Position lookup...
								uint32_t i1 = indices[i0];
								if ((i1 * 2) < texelsSource->mFloatArray.mFloatArray.size())
								{
									const float *p = &texelsSource->mFloatArray.mFloatArray[i1 * 2];
									v.mTexel[0] = p[0];
									v.mTexel[1] = p[1];
								}
							}
						}
					};

					if (k.mVcount.empty())
					{
						for (uint32_t l = 0; l < tcount && index < icount; l++)
						{
							// The 3 triangle mesh vertices we are assembling...
							Vertex tri[3];
							// For each point in the triangle...
							for (uint32_t m = 0; m < 3 && index < icount; m++)
							{
								Vertex &v = tri[m];
								composeVertex(k, v, index, scount, positions, positionsSource, normals, normalsSource, texels, texelsSource);
								sm.addVertex(v);
								index += scount; // advance to the next set of indices..
							}
						}
					}
					else
					{
						uint32_t pcount = uint32_t(k.mVcount.size());
						if (pcount == tcount && pcount )
						{
							const uint32_t *polylist = &k.mVcount[0];
							#define MAX_POLY_VERTS 32
							for (uint32_t l = 0; l < tcount && index < icount; l++)
							{
								// The 3 triangle mesh vertices we are assembling...
								Vertex tri[MAX_POLY_VERTS];
								uint32_t pvcount = *polylist++;
								if ( pvcount >= 3 && pvcount < MAX_POLY_VERTS )
								{
									for (uint32_t m = 0; m < pvcount && index < icount; m++)
									{
										Vertex &v = tri[m];
										composeVertex(k, v, index, scount, positions, positionsSource, normals, normalsSource, texels, texelsSource);
										index += scount; // advance to the next set of indices..
									}
									// Convert the polgyon into a series of triangles here
									const Vertex &v1 = tri[0];
									Vertex v2 = tri[1];
									for (uint32_t n = 2; n < pvcount; n++)
									{
										const Vertex &v3 = tri[n];
										sm.addVertex(v1);
										sm.addVertex(v2);
										sm.addVertex(v3);
										v2 = v3;
									}
								}
								else
								{
									if (pvcount < 3)
									{
										reportError(0, "Invalid polygon vertex count; must contain at least 3 points; found (%d)", pvcount);
									}
									else
									{
										reportError(0, "Polygon vertex count value (%d) exceeds maximum size this importer handles (%d)", pvcount, MAX_POLY_VERTS);
									}
								}
							}
						}
						else
						{
							reportError(0, "Polylist count attribute (%d) doesn't match the actual number of polylist values found (%d)",tcount, pcount);
						}
					}
					im.mSubMeshes.push_back(sm);
				}
			}
			mImportMeshes.push_back(im);
		}
		// Initialize the public interfaces to query the contents of the results
		for (auto &i : mImportMeshes)
		{
			i.setup();
		}
	}

	// Export the mesh as a Wavefront OBJ file
	virtual void exportOBJ(const char *fname,const char *matName) final
	{
		FILE *fph = fopen(fname, "wb");
		if (fph == nullptr) return;
		{
			FILE *fpmat = fopen(matName, "wb");
			if (fpmat)
			{
				for (auto &i : mImportMeshes)
				{
					for (auto &j : i.mSubMeshes)
					{
						fprintf(fpmat, "newmtl %s\r\n", j.mMaterial.c_str());
					}
				}
				fclose(fpmat);
			}
		}

		fprintf(fph, "#Author       : %s\r\n", mCollada.mAsset.mContributor.mAuthor.c_str());
		fprintf(fph, "#AuthoringTool: %s\r\n", mCollada.mAsset.mContributor.mAuthoringTool.c_str());
		fprintf(fph, "#Comments     : %s\r\n", mCollada.mAsset.mContributor.mComments.c_str());
		fprintf(fph, "#SourceData   : %s\r\n", mCollada.mAsset.mContributor.mSourceData.c_str());
		fprintf(fph, "#Created      : %s\r\n", mCollada.mAsset.mCreated.c_str());
		fprintf(fph, "#Modified     : %s\r\n", mCollada.mAsset.mModified.c_str());
		fprintf(fph, "#UpAxis       : %s\r\n", mCollada.mAsset.mUpAxis.c_str());

		fprintf(fph, "mtllib %s", matName);

		uint32_t baseIndex = 1;
		for (auto &i : mImportMeshes)
		{
			fprintf(fph, "# Mesh: %s\r\n", i.mName.c_str());
			for (auto &j : i.mSubMeshes)
			{
				for (auto &k : j.mVertices)
				{
					fprintf(fph, "v %f %f %f\r\n", k.mPosition[0], k.mPosition[1], k.mPosition[2]);
				}
				for (auto &k : j.mVertices)
				{
					fprintf(fph, "vt %f %f\r\n", k.mTexel[0], k.mTexel[1]);
				}
				for (auto &k : j.mVertices)
				{
					fprintf(fph, "vn %f %f %f\r\n", k.mNormal[0], k.mNormal[1], k.mNormal[2]);
				}
				fprintf(fph, "usemtl %s\r\n", j.mMaterial.c_str());
				uint32_t tcount = uint32_t(j.mIndices.size() / 3);
				if (tcount)
				{
					const uint32_t *indices = &j.mIndices[0];
					for (uint32_t l = 0; l < tcount; l++)
					{
						uint32_t i1 = indices[0] + baseIndex;
						uint32_t i2 = indices[1] + baseIndex;
						uint32_t i3 = indices[2] + baseIndex;
						fprintf(fph, "f %d/%d/%d %d/%d/%d %d/%d/%d\r\n",
							i1, i1, i1,
							i2, i2, i2,
							i3, i3, i3);
						indices += 3;
					}
				}
				baseIndex += uint32_t(j.mVertices.size());
			}
		}

		fclose(fph);
	}

	void clear(void)
	{
		ImportDAEImpl c;
		*this = c;
	}

	// Return the total number of meshes imported
	virtual uint32_t getMeshCount(void) const final
	{
		return uint32_t(mImportMeshes.size());
	}

	// Return the mesh
	virtual const MeshImport *getMeshImport(uint32_t index) const final
	{
		const MeshImport *ret = nullptr;

		if (index < mImportMeshes.size())
		{
			ret = &mImportMeshes[index];
		}

		return ret;
	}

	virtual uint32_t getVisualSceneCount(void) const final
	{
		return uint32_t(mCollada.mLibraryVisualScenes.mVisualScenes.size());
	}

	virtual uint32_t getVisualSceneNodeCount(uint32_t index, const char *&sceneId, const char *&sceneName) const
	{
		uint32_t ret = 0;

		sceneId = nullptr;
		sceneName = nullptr;
		if (index < mCollada.mLibraryVisualScenes.mVisualScenes.size())
		{
			const VisualScene &vs = mCollada.mLibraryVisualScenes.mVisualScenes[index];
			ret = uint32_t(vs.mNodes.size());
			sceneId = vs.mId.c_str();
			sceneName = vs.mName.c_str();
		}

		return ret;
	}

	virtual const SceneNode * getVisualSceneNode(uint32_t sceneIndex, uint32_t nodeIndex) final
	{
		const SceneNode *ret = nullptr;

		SceneNode c;
		mCurrentSceneNode = c;

		if (sceneIndex < mCollada.mLibraryVisualScenes.mVisualScenes.size())
		{
			const VisualScene &vs = mCollada.mLibraryVisualScenes.mVisualScenes[sceneIndex];
			if (nodeIndex < vs.mNodes.size())
			{
				const Node &n = vs.mNodes[nodeIndex];
				mCurrentSceneNode.mName = n.mName.c_str();
				mCurrentSceneNode.mId = n.mId.c_str();
				memcpy(mCurrentSceneNode.mMatrix, n.mMatrix.mMatrix, sizeof(n.mMatrix)); // copy the matrix transform
				mCurrentMeshImports.clear();
				for (auto &i : n.mInstanceGeometries)
				{
					const char *url = i.mURL.c_str();
					if (*url == '#') url++;
					for (auto &j : mImportMeshes)
					{
						if (strcmp(url, j.mName.c_str()) == 0 )
						{
							ImportDAE::MeshImport *mi = static_cast<ImportDAE::MeshImport *>(&j);
							mCurrentMeshImports.push_back(mi);
							break;
						}
					}
				}
				mCurrentSceneNode.mMeshCount = uint32_t(mCurrentMeshImports.size());
				if (mCurrentSceneNode.mMeshCount)
				{
					mCurrentSceneNode.mMeshes = &mCurrentMeshImports[0];
				}

				ret = &mCurrentSceneNode;
			}
		}

		return ret;
	}


private:
	typedef std::vector< MeshImport * > MeshImportVector;
	MeshImportVector	mCurrentMeshImports;
	SceneNode			mCurrentSceneNode;		// Used to return scene node queries..
	uint32_t			mStackLocation{ 0 };
	ElementType			mCurrentType{ ET_LAST };	// The current element type we are processing
	ElementType			mPreviousType{ ET_LAST };	// The previous element type (parent node type)
	ElementType			mPreviousPreviousType{ ET_LAST }; // two up the call stack
	ElementType			mTypeStack[MAX_STACK];
	std::string			mVersion;			// Overall version string
	std::string			mEncoding;			// Encoding (only support utf-8 for now)
	Collada				mCollada;			// The main container class for the COLLADA objects
	InstanceEffect		mCurrentInstanceEffect;
	Material			mCurrentMaterial;	// Current material properties being parsed
	Effect				mCurrentEffect;
	Geometry			mCurrentGeometry;
	Mesh				mCurrentMesh;
	Source				mCurrentSource;
	Vertices			mCurrentVertices;
	TrianglesPolyList	mCurrentTriangles;
	VisualScene			mCurrentVisualScene;
	Node				mCurrentNode;
	InstanceGeometry	mCurrentInstanceGeometry;
	// Semaphores for elements we only expect to see one time within a block
	bool				mHaveTechniqueCommon{ false };
	bool				mHaveFloatArray{ false };
	bool				mHaveAccessor{ false };
	bool				mHaveBindMaterial{ false };
	// parsed mesh data
	ImportMeshVector	mImportMeshes;
};

ImportDAE *ImportDAE::create(void)
{
	ImportDAEImpl *d = new ImportDAEImpl;
	return static_cast<ImportDAE *>(d);
}


} // end of IMPORT_DAE namespace
