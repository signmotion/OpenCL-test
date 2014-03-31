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
    // ����� ������������ ������
    __global WMap* wm,              // @5
    // ��� ����� ���������?
    const enum RenderWhat what,     // @6
    // ���������� �������, �� ������� ��������� ���������� ��������� ����.
    // ��� ���������� ����������� ������ �������� ������������.
    const float dt,                 // @7
    // ��������� �����: ����� ���� ������������ � �������
    const uint random,              // @8
    // ��������������� �����.
    __global WMap* wmT              // @9
) {

    // ������ - ���������� ��������� ������ ����
    const uint x = get_global_id( 0 );
    const uint z = get_global_id( 1 );

    // ��� ���������� ���� � �����, ����� ������������ ��������� ���� ������
    // �����. ��������� �����, ����� �� ����� �� �������.
    // @todo optimize ������ �������� ����� � �������� ����. ������ �������, ������� �������.
	if ( (x >= N) || (z >= M) ) {
        return;
    };


    const uint i = x + z * N;

    
    switch ( what ) {
        case IMAGE_WMAP:
            // ����� ������� ��������, ������� ����� �������� ��� �������
            // @todo optimize ������?
            calcCharacteristicParticleInCell(
                mSolid, mLiquid, mGas, mPlasma, wm, i
            );
            imageWMap(
                imageOut,
                mSolid, mLiquid, mGas, mPlasma,
                // � ������ ���� ����� ���������� ��������� ������
                wm,
                i
            );
            return;
            


        case PREPARE_INTERACTION:
            // ������������ �������� ������� �� ��������� ������ � ��������
            // ��� ����� �������������� � ������� ���������.
            calcCharacteristicParticleInCell(
                mSolid, mLiquid, mGas, mPlasma, wm, i
            );
            return;


            
        case GO_INTERACTION_I:
            // �������� � ���������, ������� ��������� � PREPARE_INTERACTION.
            // ��������� ���������� � 'wmT'.
            interactionI(
                wmT,
                wm,
                dt,
                x, z, i
            );
            return;



        case GO_INTERACTION_II:
            interactionII(
                wmT,
                wm,
                dt,
                x, z, i
            );
            return;



        case GO_INTERACTION_III:
            // @todo ...
            return;



        case COMMIT_INTERACTION:
            /* - ��� ����������: �������� ����� � GO_INTERACTION.
            for (size_t n = 0; n < MAX_WMAP_CELL; ++n) {
                wm[i].pe[n] = wmT[i].pe[n];
            }
            */
            break;



        default:
            // ����������� �������� 'what'
            // @todo ...
            return;

    } // switch ( what )



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



}
