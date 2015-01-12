__kernel void ComputePartialVolumes(__global short4 * voxels,
                                      const  int      numVoxels,
                                      const  float4   plane,
                                      const  float4   minBB,
                                    __local  uint4 *  localPartialVolumes,
                                    __global uint4 *  partialVolumes)
{
    int localId   = get_local_id(0);
    int groupSize = get_local_size(0);
    int i0 = get_global_id(0) << 2;
    uint  pVol[4] = { 0, 0, 0, 0 };
    short4 voxel;
    float4 pt;
    float  d;
    for (int i = 0; (i0+i < numVoxels) && (i < 4); ++i)
    {
        voxel   = voxels[i+i0];
        pt      = (float4)(voxel.s0 * minBB.s3 + minBB.s0, voxel.s1 * minBB.s3 + minBB.s1, voxel.s2 * minBB.s3 + minBB.s2, 1.0f);
        d       = dot(plane, pt);
        pVol[i] = d >= 0.0f;
    }
    localPartialVolumes[localId] = *((uint4 *)(pVol));
    barrier(CLK_LOCAL_MEM_FENCE);

    for (int i = groupSize >> 1; i > 0; i >>= 1)
    {
        if (localId < i)
        {
            localPartialVolumes[localId] += localPartialVolumes[localId + i];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    if (localId == 0)
    {
        partialVolumes[get_group_id(0)] = localPartialVolumes[0];
    }
}

__kernel void ComputePartialSums(__global uint4 * data,
                                   const  int     dataSize,
                                 __local  uint4 * partialSums) 
{

    int globalId  = get_global_id(0);
    int localId   = get_local_id(0);
    int groupSize = get_local_size(0);
    if (globalId < dataSize)
    {
        partialSums[localId] = data[globalId];
    }
    else
    {
        partialSums[localId] = (0, 0, 0, 0);
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    for (int i = groupSize >> 1; i > 0; i >>= 1)
    {
        if (localId < i)
        {
            partialSums[localId] += partialSums[localId + i];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (localId == 0)
    {
        data[get_group_id(0)] = partialSums[0];
    }
}

