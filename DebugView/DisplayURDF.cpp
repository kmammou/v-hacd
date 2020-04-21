#include "DisplayURDF.h"
#include "ImportURDF.h"
#include "MeshFactory.h"
#include "URDF_DOM.h"
#include "NvRenderDebug.h"

#pragma warning(disable:4100)

namespace DISPLAY_URDF
{

class DisplayURDFImpl : public DisplayURDF
{
public:
	DisplayURDFImpl(void)
	{
		mMeshFactory = MESH_FACTORY::MeshFactory::create();
	}

	virtual ~DisplayURDFImpl(void)
	{
		if (mImportURDF)
		{
			mImportURDF->release();
		}
		if (mMeshFactory)
		{
			mMeshFactory->release();
		}
	}

	// Import a URDF file
	virtual void		importURDF(const char *fname) final
	{
		mURDF = nullptr;
		if (mImportURDF)
		{
			mImportURDF->release();
			mImportURDF = nullptr;
		}
		mImportURDF = IMPORT_URDF::ImportURDF::create();
		mURDF = mImportURDF->importURDF(fname, mMeshFactory);
	}

	// Render the contents of the URDF file using the RenderDebug interface and at the provided scale
	virtual void	render(RENDER_DEBUG::RenderDebug *renderDebug,float scale,bool showSourceMesh,bool showCollisionMesh) final
	{
		if (!mURDF) return;
		// Ok, let's walk through the nodes and display the visual meshes for each...
		for (auto &i : mURDF->mRobots)
		{
			renderRobot(i,renderDebug,scale,showSourceMesh,showCollisionMesh);
		}
	}

	void renderRobot(const URDF_DOM::Robot &r,RENDER_DEBUG::RenderDebug *renderDebug, float scale,bool showSourceMesh,bool showCollisionMesh)
	{
		for (auto &i : r.mLinks)
		{
			renderLink(i, renderDebug, scale, showSourceMesh, showCollisionMesh);
		}
		for (auto &i : r.mJoints)
		{
			renderJoint(i, renderDebug, scale);
		}
	}

	void renderJoint(const URDF_DOM::Joint &j,RENDER_DEBUG::RenderDebug *renderDebug, float scale)
	{
		float pos[3];
		pos[0] = j.mPose.mPosition.x * scale;
		pos[1] = j.mPose.mPosition.y * scale;
		pos[2] = j.mPose.mPosition.z * scale;
		renderDebug->debugSphere(pos, 0.02f);
	}

	void renderLink(const URDF_DOM::Link &l, RENDER_DEBUG::RenderDebug *renderDebug, float scale,bool showSourceMesh,bool showCollisionMesh)
	{
		if (showSourceMesh)
		{
			renderRepresentation(l.mPose, l.mVisual, renderDebug, scale);
		}
		if (showCollisionMesh)
		{
			renderRepresentation(l.mPose, l.mCollision, renderDebug, scale);
		}
	}

	void renderRepresentation(const URDF_DOM::Pose &pose, const URDF_DOM::Representation &r, RENDER_DEBUG::RenderDebug *renderDebug, float scale)
	{
		if (r.mMesh)
		{
			renderDebug->pushRenderState();
			renderDebug->addToCurrentState(RENDER_DEBUG::DebugRenderState::SolidWireShaded);
			renderDebug->setCurrentColor(0xFFFF00, 0xFFFFFF);
			uint32_t subMeshCount = r.mMesh->getSubMeshCount();
			for (uint32_t i = 0; i < subMeshCount; i++)
			{
				const MESH_FACTORY::SubMesh *sm = r.mMesh->getSubMesh(i);
				if (sm)
				{
					for (uint32_t j = 0; j < sm->mTriangleCount; j++)
					{
						uint32_t i1 = sm->mMeshIndices[j * 3 + 0];
						uint32_t i2 = sm->mMeshIndices[j * 3 + 1];
						uint32_t i3 = sm->mMeshIndices[j * 3 + 2];
						float p1[3];
						float p2[3];
						float p3[3];
						auto getPos = [this](const MESH_FACTORY::Vertex &v, float *pos,float scale, const URDF_DOM::Pose &pose)
						{
							pos[0] = (v.mPosition[0]+pose.mPosition.x) * scale;
							pos[1] = (v.mPosition[1]+pose.mPosition.y) * scale;
							pos[2] = (v.mPosition[2]+pose.mPosition.z) * scale;
						};
						getPos(sm->mMeshVertices[i1], p1, scale, pose);
						getPos(sm->mMeshVertices[i2], p2, scale, pose);
						getPos(sm->mMeshVertices[i3], p3, scale, pose);
						renderDebug->debugTri(p1, p2, p3);
					}
				}
			}
			renderDebug->popRenderState();
		}
	}

	virtual void release(void) final
	{
		delete this;
	}

	void debugRepresentation(FILE *fph,const URDF_DOM::Representation &r, const char *label)
	{
		fprintf(fph,"%s(%s): (%0.4f,%0.4f,%0.4f) (%0.4f,%0.4f,%0.4f)\r\n",
			label,
			r.mName.c_str(),
			r.mPose.mPosition.x,
			r.mPose.mPosition.y,
			r.mPose.mPosition.z,
			r.mPose.mRPY.x,
			r.mPose.mRPY.y,
			r.mPose.mRPY.z);
		switch (r.mGeometry.mType)
		{
		case URDF_DOM::Geometry::GT_MESH:
			fprintf(fph,"Mesh: %s\r\n", r.mGeometry.mMesh.c_str());
			break;
		case URDF_DOM::Geometry::GT_BOX:
			fprintf(fph,"Box(%0.4f,%0.4f,%0.4f)\r\n",
				r.mGeometry.mDimensions.x,
				r.mGeometry.mDimensions.y,
				r.mGeometry.mDimensions.z);
			break;
		case URDF_DOM::Geometry::GT_SPHERE:
			fprintf(fph,"Sphere(%0.4f)\r\n",
				r.mGeometry.mDimensions.x);
			break;
		case URDF_DOM::Geometry::GT_CYLINDER:
			fprintf(fph,"Cylinder(%0.4f,%0.4f)\r\n",
				r.mGeometry.mDimensions.x,
				r.mGeometry.mDimensions.y);
			break;

		}
	}

	void debugLink(const URDF_DOM::Link &link,FILE *fph)
	{
		fprintf(fph,"==========================================================\r\n");
		fprintf(fph,"Link: %s\r\n", link.mName.c_str());
		fprintf(fph,"     Pose: (%0.4f,%0.4f,%0.4f) (%0.4f,%0.4f,%02.f)\r\n",
			link.mPose.mPosition.x,
			link.mPose.mPosition.y,
			link.mPose.mPosition.z,
			link.mPose.mRPY.x,
			link.mPose.mRPY.y,
			link.mPose.mRPY.z);
		debugRepresentation(fph,link.mVisual, "Visual");
		debugRepresentation(fph,link.mCollision, "Collision");
		fprintf(fph,"==========================================================\r\n");
		fprintf(fph,"\r\n");
	}

	void debugJoint(const URDF_DOM::Joint &j, FILE *fph)
	{
		fprintf(fph, "==========================================================\r\n");
		fprintf(fph, "Joint: %s\r\n", j.mName.c_str());
		fprintf(fph, "Child: %s\r\n", j.mChild.c_str());
		fprintf(fph, "Parent: %s\r\n", j.mParent.c_str());
		fprintf(fph, "     Pose: (%0.4f,%0.4f,%0.4f) (%0.4f,%0.4f,%02.f)\r\n",
			j.mPose.mPosition.x,
			j.mPose.mPosition.y,
			j.mPose.mPosition.z,
			j.mPose.mRPY.x,
			j.mPose.mRPY.y,
			j.mPose.mRPY.z);
		fprintf(fph, "==========================================================\r\n");
		fprintf(fph, "\r\n");

	}

	// debug print link information 
	virtual void	debugLinks(void) final 
	{
		if (!mURDF) return;
		if (!mURDF->mRobots.size()) return;
		FILE *fph = fopen("DebugLinks.txt", "wb");
		if (fph == nullptr) return;
		const URDF_DOM::Robot &r = mURDF->mRobots[0];
		for (const auto &i : r.mLinks)
		{
			debugLink(i,fph);
		}
		for (const auto &i : r.mJoints)
		{
			debugJoint(i, fph);
		}
		fclose(fph);
	}

	MESH_FACTORY::MeshFactory	*mMeshFactory{ nullptr };
	IMPORT_URDF::ImportURDF		*mImportURDF{ nullptr };
	const URDF_DOM::URDF		*mURDF{ nullptr };
};


DisplayURDF *DisplayURDF::create(void)
{
	DisplayURDFImpl *d = new DisplayURDFImpl;
	return static_cast< DisplayURDF *>(d);
}

}	  // end of DISPLAY_URDF namespace
