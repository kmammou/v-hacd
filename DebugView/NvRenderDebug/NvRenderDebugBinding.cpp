#include "NvRenderDebug.h"
#include <stdio.h>

#ifdef _MSC_VER

#include <windows.h>

namespace RENDER_DEBUG
{


RENDER_DEBUG::RenderDebug *createRenderDebug(RenderDebug::Desc &desc)
{
	RENDER_DEBUG::RenderDebug *ret = NULL;
	UINT errorMode = 0;
	errorMode = SEM_FAILCRITICALERRORS;
	UINT oldErrorMode = SetErrorMode(errorMode);
	HMODULE module = LoadLibraryA(desc.dllName);
	SetErrorMode(oldErrorMode);
	if ( module )
	{
		void *proc = GetProcAddress(module,"createRenderDebugExport");
		if ( proc )
		{
			typedef void * (__cdecl * NX_GetToolkit)(const RenderDebug::Desc &desc);
			ret = (RenderDebug *)((NX_GetToolkit)proc)(desc);
		}
		else
		{
			desc.errorCode = "Unable to located the 'createRenderDebug' export symbol";
		}
	}
	else
	{
		desc.errorCode = "LoadLibrary failed; could not load the requested RenderDebug DLL; are you sure you have the correct file name?";
	}
	return ret;
}


}; // end of namespace

#else

namespace RENDER_DEBUG
{

	RENDER_DEBUG::RenderDebug *createRenderDebug(RenderDebug::Desc & /*desc*/)
	{
		RENDER_DEBUG::RenderDebug *ret = NULL;
		return ret;
	}
}; // end of namespace

#endif