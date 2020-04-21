#include "SkeletonRig.h"
#include <stdio.h>


#ifdef _MSC_VER

#include <windows.h>

namespace skeletonrig
{


skeletonrig::SkeletonRigFactory *createSkeletonRigPlugin(uint32_t versionNumber,const char *dllName)
{
    skeletonrig::SkeletonRigFactory *ret = NULL;
    UINT errorMode = 0;
    errorMode = SEM_FAILCRITICALERRORS;
    UINT oldErrorMode = SetErrorMode(errorMode);
    HMODULE module = LoadLibraryA(dllName);
    SetErrorMode(oldErrorMode);
    if ( module )
    {
        void *proc = GetProcAddress(module,"createSkeletonRig");
        if ( proc )
        {
            typedef void * (__cdecl * NX_GetToolkit)(uint32_t versionNumber);
            ret = (SkeletonRigFactory *)((NX_GetToolkit)proc)(versionNumber);
        }
    }
    return ret;
}


} // end of namespace


#endif