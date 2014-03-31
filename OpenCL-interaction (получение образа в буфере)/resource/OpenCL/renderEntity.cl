/**
* √енерирует изображение по сущност€м мира.
*/
__kernel void renderEntity(
        __global float4* out,
        const uint N,
        const uint M
) {
    //const int tx = get_local_id(0);		// Cuda equivalent : threadIdx.x
    //const int ty = get_local_id(1);		// Cuda equivalent : threadIdx.y
    //const int bw = get_local_size(0);	// Cuda equivalent : blockDim.x
    //const int bh = get_local_size(1);	// Cuda equivalent : blockDim.y
    const int x = get_global_id(0);		// Cuda equivalent : blockIdx.x*bw + tx
    const int y = get_global_id(1);		// Cuda equivalent : blockIdx.y*bh + ty

    // ѕри размещении мира в сетке, может понадобитьс€ захватить чуть больше
    // места. ѕровер€ем здесь, чтобы не выйти за пределы.
    // @todo optimize ”брать и посмотреть разницу.
	if ( (x >= N) || (y >= M) ) {
        return;
    };

    float u = x / (float)N;
    float v = y / (float)M;
    u = u * 2.0f - 1.0f;
    v = v * 2.0f - 1.0f;

    // calculate simple sine wave pattern
    float time = (float)( x * N + M );
    float freq = 4.0f;
    float w = sin( u * freq + time ) * cos( v * freq + time ) * 0.5f;

    const uint i = y * N + x;
    out[i] = (float4)( u, w/2, v, 1.0f ); 
}
