#include "pragma.hcl"
#include "const.hcl"
#include "default.hcl"
#include "struct.hcl"
#include "helper.hcl"
#include "interaction/gas.hcl"

// @todo optimize ��������� �������������� ��������� � ��������.


/**
* ��������� ���������� ����� ����.
*/
inline void createImage(
    __global float4* out,
    const uint i,
    const uchar matter,
    enum PState state,
    const uint color,
    const float density
) {

#ifndef TEST
    // ���� ��������� � RGBA
    // @source http://rclermont.blogspot.com/2010/02/convert-long-into-byte-array-in-cc.html
    const uchar cr = (int)((color >> 24) & 0xFF);
    const uchar cg = (int)((color >> 16) & 0xFF);
    const uchar cb = (int)((color >> 8) & 0xFF);
    // �����-����� ������������ ����������
    //const uchar ca = (int)((color & 0xFF));

    // ������������� ������.
    // �������� ���� � ����� ������� � ����������� ��
    // ��������� � ���. ��������� -> #00000000.
    const float K =
        (state == GAS) ? 1.0f :
        5.0f;
    float colorDiff = (density / K);
    colorDiff =
        (colorDiff > 1.0) ? 1.0 :
        (colorDiff < 0.3) ? 0.3 : colorDiff;
    const float r = (float)cr / 255.0f * colorDiff;
    const float g = (float)cg / 255.0f * colorDiff;
    const float b = (float)cb / 255.0f * colorDiff;
    const float a = 1.0f;  //colorDiff;  // * 1.0f;
    
#if 0
    // ���� "�������"
    const float r = (matter == 0) ? 0.0f : 0x7f / 255.0f ;
    const float g = (matter == 0) ? 0.0f : 0xc7 / 255.0f;
    const float b = (matter == 0) ? 0.0f : 0xff / 255.0f;
    const float a = (matter == 0) ? 0.0f : 0x80 / 255.0f;
#endif

    out[i] = (float4)( r, g, b, a );
    
#else
// TEST

    float u = x / (float)N;
    float v = y / (float)M;
    u = u * 2.0f - 1.0f;
    v = v * 2.0f - 1.0f;

    // calculate simple sine wave pattern
    float time = (float)( x * N + M );
    float freq = 4.0f;
    float w = sin( u * freq + time ) * cos( v * freq + time ) * 0.5f;

    out[i] = (float4)( u, w/2, v, 1.0f );

#endif

}






/**
* ���������� ����������� �� ��������� ����.
*
* ��������� � ���� �������� ������ ��������� �� ������� 'out'.
* @see struct.h
*/
__kernel void render(
        __global float4* out,
        // ���������� ������� ����: ������ == 256
        //const uint nMatter,
        // ������ � �������
        __global Matter* md,
        // �������� ����. ������ ������� = N x M (��. pragma.hcl).
        __global DataS1U* s1u,
        __global DataS4U* s4u,
        __global DataS4F* s4f,
        __global DataW8F* w8f,
        // ���������� �������, �� ������� ��������� ���������� ��������� ����.
        const float dt,
        // ��������� �����: ������������ ���������� ��������
        const uint random,
        // �������� ���� ����� ����������� ������� 'time'.
        __global DataS1U* s1uOut,
        __global DataS4U* s4uOut,
        __global DataS4F* s4fOut,
        __global DataW8F* w8fOut
        /* - ��������� ��� ���.
        // ��������� ������ ��� �������� ����������.
        // ����� �������������� ������� �������� �� ������ ����������.
        __global DataTempSU* tempSU,
        __global DataTempSF* tempSF,
        __global DataTempWU* tempWU,
        __global DataTempWF* tempWF
        */
) {
    //const int tx = get_local_id(0);	// Cuda equivalent : threadIdx.x
    //const int ty = get_local_id(1);	// Cuda equivalent : threadIdx.y
    //const int bw = get_local_size(0);	// Cuda equivalent : blockDim.x
    //const int bh = get_local_size(1);	// Cuda equivalent : blockDim.y
    const int x = get_global_id(0);		// Cuda equivalent : blockIdx.x*bw + tx
    const int z = get_global_id(1);		// Cuda equivalent : blockIdx.y*bh + ty

    // ��� ���������� ���� � �����, ����� ������������ ��������� ���� ������
    // �����. ��������� �����, ����� �� ����� �� �������.
    // @todo optimize ������ �������� ����� � �������� ����. ������, ���������� �������.
	if ( (x >= N) || (z >= M) ) {
        return;
    };

    const uint i = z * N + x;
    
    // ������� ������� �������� � ������. ������: ������
    // ��������� �� ������� 'out'.
    const uchar matter = s1u[i].matter;
    const float temperature = s4f[i].temperature;
    const enum PState state = calcState( matter, temperature );

    // ����������� ��������������
    //if (dt >= 0.1f)
    {
        // �������������� ������
        s1uOut[i] = s1u[i];
        s4uOut[i] = s4u[i];
        s4fOut[i] = s4f[i];
        w8fOut[i] = w8f[i];
        barrier( CLK_GLOBAL_MEM_FENCE );

        
        // ������ ��� �������� �������:
        //   - ������ ����� �����
        uint nc[9];
        //   - ������� � ������� �����
        uchar nm[9];
        //   - ���������� ��������� ������� � ������� �����
        enum PState ns[9];
        //   - ����
        float2 nf[9];
        //   - ��������
        float2 nv[9];
        nc[0] = i;
        nm[0] = matter;
        ns[0] = state;
        nf[0] = w8f[i].force;
        nv[0] = w8f[i].speed;
        for (uint cell = 1; cell <= 8; cell++) {
            nc[cell] = nearCell( x, z, cell );
            const uint ind = nc[cell];
            nm[cell] = s1u[ ind ].matter;
            ns[cell] = calcState( nm[cell], temperature );
            nf[cell] = w8f[ ind ].force;
            nv[cell] = w8f[ ind ].speed;
        }

        // �������������� ������ �����. ���������

        //solid();
        //liquid();

        if ( gasOrVacuum(state, matter) ) {
            gas(
                s1uOut, s4uOut, s4fOut, w8fOut,
                dt,
                random,
                md,
                s1u, s4u, s4f, w8f,
                x, z,
                nc, nm, ns, nf, nv,
                temperature
            );
        }

        //plasma();

        // ��� ���������� ������ �������
        barrier( CLK_GLOBAL_MEM_FENCE );


        // ����� ��������� �������� ���� ������� ��������� �������� ����� ����
        s1u[i] = s1uOut[i];
        s4u[i] = s4uOut[i];
        s4f[i] = s4fOut[i];
        w8f[i] = w8fOut[i];
        barrier( CLK_GLOBAL_MEM_FENCE );
    }


    // test
    // ����������� �����
    /*
    if ( (x == N/2) && (z == M/2) ) {
        const int Q = 25;
        for (int dx = -Q; dx <= Q * 2; dx++) {
            s4u[ near( x, z, dx, 0 ) ].color = 0xff0000ff;
        }
        for (int dz = -Q; dz <= Q * 2; dz++) {
            s4u[ near( x, z, 0, dz ) ].color = 0x00ff00ff;
        }
        for (int dxz = -Q; dxz <= Q * 2; dxz++) {
            s4u[ near( x, z, dxz, dxz ) ].color = 0x0000ffff;
        }
        for (int dxz = -Q; dxz <= Q * 2; dxz++) {
            s4u[ near( x, z, dxz, -dxz ) ].color = 0xffff00ff;
        }
        for (int cell = 1; cell <= 8; cell++) {
            s4u[ nearCell( x, z, cell ) ].color = 0xff00ffff;
        }
        s4u[i].color = 0xffffffff;
    }
    */
    

    // ��������� ���������� �����
    //if (dt < 0.1f)
    {
        const uint iMD = matter * MAX_COUNT_PSTATE + state;
        const uint color = md[ iMD ].color;
        const float density = s4f[i].density;
        createImage(out, i, matter, state, color, density);
    }

}
