#include "pragma.hcl"
#include "default.hcl"
#include "const.hcl"
#include "sph.hcl"
#include "struct.hcl"
#include "helper.hcl"
#include "image.hcl"
#include "interaction.hcl"



/**
* �������� �����, ����������� �������������� ��������� � �����������
* ���������� ����� ����.
* ������ ���� = N x M (��. pragma.hcl).
* ��������� ������ ������ ������� �� ��������� 'what'. ������, �����������������
* ����� ������� ����� ������:
*   1. �� UID �������. ��� �� ����� ��������� ����� N x M ������. ������
*      ������� ��� ������������� ���� �������� UID, ������� ����� ���� �������� ��
*      ������� get_global_id(0) + get_global_id(1) * N
*   2. �� ������� ���� WMap. � ������ ��������� �� ����� MAX_WMAP_CELL ������.
*      ����������� � ������� CPU.
* ������� ����� ���������� ������������ ������ � ����� ������.
*
* ������������������ ����� 'what' � ���� �� �����. ��������������� ������� ��
* ���������� (��. interaction.cpp): ����� ����� ��� �����������.
*/
__kernel void render(
    // ���������� ����� ����.
    // 1 ������� = 4 ������: RGBA, float
    __global float4* imageOut,      // @0
    // ������ � �������.
    // ���������� ������� ���� ������ 256.
    __global MatterSolid* mSolid,   // @1
    __global MatterLiquid* mLiquid, // @2
    __global MatterGas* mGas,       // @3
    __global MatterPlasma* mPlasma, // @4
    // �������� ����. �������� ���� ������� �� �������.
    __global ParticleEntity* pe,    // @5
    // ����� ������������ ������
    __global WMap* wm,              // @6
    // ��� ����� ���������?
    const enum RenderWhat what,     // @7
    // ���������� �������, �� ������� ��������� ���������� ��������� ����.
    // ��� ���������� ����������� ������ �������� ������������.
    const float dt,                 // @8
    // ��������� �����: ����� ���� ������������ � �������
    const uint random,              // @9
    // ��������������� �����.
    // �������� ���� ����� ����������� ������� 'time'.
    __global ParticleEntity* peOut  // @10
) {
    // ����� ��� �� �����, ��� ���������������� 'a' � 'b'
    const uint a = get_global_id( 0 );
    const uint b = get_global_id( 1 );

    // ��� ���������� ���� � �����, ����� ������������ ��������� ���� ������
    // �����. ��������� �����, ����� �� ����� �� �������.
    // ����� � ��� ������: UID �� ����� ��������� ������� ���� (��. �������� render() ).
    // @todo optimize ������ �������� ����� � �������� ����. ������ �������, ������� �������.
	if ( (a >= N) || (b >= M) ) {
        return;
    };


    // ������: ����� ������� ����� �������� � ���������� ���������������.
    // ��� ��������� - ������������� ��� �������� ������ ���� ����.
    const uint uid = a + b * N;
    const uint coordCell = uid;  // = a + b * N;
    
    
    switch ( what ) {
        case IMAGE_WMAP:
            imageWMap(
                imageOut,
                mSolid, mLiquid, mGas, mPlasma,
                pe,
                wm[coordCell],
                coordCell
            );
            return;


        case CLEAR_MATRIX:
            // ������� ������ ������
            //imageOut[coordCell] = (float4)( 0.0f, 0.0f, 0.0f, 0.0f );

            return;
            
            
        case COMMIT_INTERACTION:
            pe[uid] = peOut[uid];
            break;
    }
    
    
            
    // �������� �������
    const ParticleEntity p = pe[uid];

    // �� ������� �������, ������� ����� �� ������� ����
    if ( outsideMatrix( p.coordX, p.coordZ ) ) {
        return;
    };

    
    /*
    if (i == 0) {
        for (size_t z = 0; z < 1; z++) {
            for (size_t x = 0; x < N; x++) {
                const size_t i = x + z * N;
                imageOut[i] = (float4)( 1.0f, 1.0f, 1.0f, 1.0f );
            }
        }
    }
    */

    /*
    if (pe[i].mass > 0.0f) {
        imageOut[i] = (float4)( 1.0f, 1.0f, 1.0f, 1.0f );
    } else {
        imageOut[i] = (float4)( 1.0f, 0.0f, 0.0f, 1.0f );
    }
    return;
    */



    // � switch ������ ������������� ���������� - ����������� �����.
    //ParticleEntity pOut;

    switch ( what ) {
        case PREPARE_INTERACTION:
            // ���������� ������ ����� ���������� ������ HOST/interaction()/fillWMap()

            // ������������ �������� ������� �� ��������� ������ � ��������
            // ��� ����� �������������� � ������� ���������.
            peOut[uid] = calcCharacteristicParticle(
                mSolid, mLiquid, mGas, mPlasma, p
            );
            return;


        case GO_INTERACTION:
            if ( !zero( dt ) ) {
                // ������� �������, ����������� � PREPARE_INTERACTION
                interaction(
                    peOut,
                    wm,
                    dt,
                    a, b, coordCell
                );
            }
            return;


        default:
            // ����������� �������� 'what'
            // @todo ...
            return;
    }

    //barrier( CLK_GLOBAL_MEM_FENCE );

}
