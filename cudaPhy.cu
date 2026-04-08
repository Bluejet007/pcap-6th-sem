cudaGraphicsMapResources(1, &g_cudaPos[read]);
cudaGraphicsMapResources(1, &g_cudaVel[read]);
cudaGraphicsMapResources(1, &g_cudaPos[write]);
cudaGraphicsMapResources(1, &g_cudaVel[write]);

float4* posIn;
float4* posOut;
float4* velIn;
float4* velOut;
size_t size;

// Get raw pointers (zero-copy!)
cudaGraphicsResourceGetMappedPointer((void**)&posIn,  &size, g_cudaPos[read]);
cudaGraphicsResourceGetMappedPointer((void**)&posOut, &size, g_cudaPos[write]);
cudaGraphicsResourceGetMappedPointer((void**)&velIn,  &size, g_cudaVel[read]);
cudaGraphicsResourceGetMappedPointer((void**)&velOut, &size, g_cudaVel[write]);

// launch kernel <<<>>> here

cudaGraphicsUnmapResources(...);