inline void caclFlowCellGas(
    uint fc[2], 
    const uint nc[], const uchar nm[], enum PState ns[],
    const float np[]
) {
    const float pressure = np[0];

    // ���������� �����, ���� ����� ������ ���
    uint outflowCell = 0;
    // ���������� �����, ������ ����� ���� ���
    uint inflowCell = 0;
    //if ( gasOrVacuum(state, matter) ) { - ����� ��� ������ ����� �� ����������.
    for (uint cell = 1; cell <= 8; cell++) {
        const uint in = nc[cell];
        const uchar nearMatter = nm[cell];
        const enum PState nearState = ns[cell];
        
        if ( gasOrVacuum(nearState, nearMatter) ) {
            const float nearPresure = np[cell];
            // ������ ��������
            if ( !equal( pressure, nearPresure ) ) {
                //(pressure > nearPresure) ? outflowCell++ : inflowCell++;
                if (pressure > nearPresure) {
                    outflowCell++;
                } else {
                    inflowCell++;
                }
                continue;
            }
        }
    }
    //}

    fc[0] = outflowCell;
    fc[1] = inflowCell;
}




/**
* @return ����� ������, ������� �� ������� �������� ������� ��� 
*         ���������� ����������� ������.
* ���������, ��� ������ ����� ��������� ������ ��� �� ���,
* ��� � ����� ������; ��� (���� � ������ ������) ������,
* ������� �������� ��� ������ ����� �����.
*/
inline uint calcFillCellGas(
    const uint i,
    const uint nc[], const uchar nm[], enum PState ns[],
    const float np[],
    const uint random
) {
    const uchar matter = nm[0];
    enum PState state = ns[0];

    if ( !gasOrVacuum(state, matter) ) {
        return 0;
    }

    const float pressure = np[0];

    // �������� ���������� ������.
    // ������ ������� ������������: ������ ����
    // ����� � ���� �������.
    bool fillCell[9] = {
        true,
        false, false, false, false,
        false, false, false, false,
    };
    for (uint cell = 1; cell <= 8; cell++) {
        const uint in = nc[cell];
        const uchar nearMatter = nm[cell];
        const enum PState nearState = ns[cell];

        if (nearState == GAS) {
            const float nearPresure = np[cell];
            const float flow = nearPresure - pressure;
            // �������� �����
            fillCell[cell] = (flow > 0);
        }
    }
    
    // �������� �� ��������� ����� ������
    uint shift = random % 9;
    for (uint cell = 0; cell <= 8; cell++) {
        uint index = cell + shift;
        if (index > 8) {
            index -= 8;
        }
        if ( fillCell[ index ] ) {
            return index;
        }
    }
    
    // ������ ����������� �� ����������
    return 0;
}






/**
* ���� �������� ������ ������� ����.
* @see ����. � calcNewPressureGas()
*/
inline void calcNewForceSpeedGas(
    float2* newF, float2* newV,
    const uint nc[], const uchar nm[], enum PState ns[],
    const float2 nf[], const float2 speed[],
    const float np[]
) {
    // @todo .....

}







/**
* �������, ������� ����� ��������� ������ ����� �������.
* ���������, ��� ������ ����� ��������� ������ ��� �� ���,
* ��� � ����� ������; ��� (���� � ������ ������) ������,
* ������� �������� ��� ������ ����� �����.
*/
inline void calcNewPressureGas(
    float* newP,
    const uint nc[], const uchar nm[], enum PState ns[],
    const float2 nf[], const float2 speed[],
    const float np[]
) {
    enum PState state = ns[0];
    const float pressure = np[0];

    /* - �� ��������. ���������������.
    // ���������, ��� ��� �������� � ������ ��������� � ���������� =
    // = f(pressure) / "���������� �����, ���� ����� ������ ���"
    // ������������ �������� ��� ���� ����� �����.
    const float density = s4f[i].density;
    float deltaPresure = 0.0;
    for (uint cell = 1; cell <= 8; cell++) {
        const uint in = nearCell( x, z, cell );  //nc[cell];
        const uchar nearMatter = s1u[in].matter;  //nm[cell];
        const enum PState nearState = calcState( nearMatter, temperature );  //ns[cell];
        
        if ( gasOrVacuum(nearState, nearMatter) ) {
            const float nearPresure = s4f[in].pressure;
            // ����� = "��� �������� � ������� ���� ������"
            // '0' ��������� ����
            const float flow = nearPresure - pressure;
            if (flow < 0) {
                // 'outflowCell' ���������� ������� ��������� �������� ����
                const float halfFlow = flow / outflowCell / 2;
                s4fOut[in].pressure += halfFlow;
                s4fOut[in].density += calcGasDensity( halfFlow, molarMass );
                // ��������� �������� �������� �� ����� ��������� ���������
                deltaPresure += halfFlow;
            }
            if (flow < 0) {
                s4uOut[i].color = 0xff0000ff;
                return;
            }
        }
    }

    float newDensity = 
        zero(deltaPresure) ? density :
        calcGasDensity( deltaPresure, molarMass );
    */
    

    // ���������, ��� ��� �������� � ������ ��������� � ���������� =
    // = f(pressure) / "8 �����, ���� ����� ������ ���"
    // ������������ �������� �������� ��� ����������� ������.
    float currentPressure = pressure;
    for (uint cell = 1; cell <= 8; cell++) {
        const uint in = nc[cell];
        const uchar nearMatter = nm[cell];
        const enum PState nearState = ns[cell];
        
        if ( gasOrVacuum(nearState, nearMatter) ) {
            const float nearPresure = np[cell];
            // ����� = "��� �������� � ������� ���� ������".
            // ��������� ��������. �� ��������� ������ ����� �����,
            // ��� ��� �������� ��������.
            const float flow = nearPresure - currentPressure;
            if ( !zero( flow ) )
            {
                // 'outflowCell' ���������� ������� ��������� �������� ����
                // ���������: outflowCell ������ = 8 (���������� �����������)
                const float halfFlow = flow / 8 / 2;
                currentPressure += halfFlow;
            }
        }
    }

    // @see const.hcl
    *newP = currentPressure * GAS_FADING;
}







/**
* �������������� �����.
* ���� ������� ��� ����� ���� �������.
* ����������� ���� �� ���������.
*
* �������:
*   - ����� ���������� ��� ��������� GAS ��� ������� VACUUM
*   -  I/O ������ ��� ����������������
*/
void gas(
    __global DataS1U* s1uOut,
    __global DataS4U* s4uOut,
    __global DataS4F* s4fOut,
    __global DataW8F* w8fOut,
    const float dt,
    const uint random,
    __global DataS1U* s1u,
    __global DataS4U* s4u,
    __global DataS4F* s4f,
    __global DataW8F* w8f,
    const uint x, const uint z,
    const uint nc[], const uchar nm[], enum PState ns[],
    const float2 nf[], const float2 nv[],
    const float temperature
    /* - ��������� ��� ���.
    __global DataTempSU* tempSU,
    __global DataTempSF* tempSF,
    __global DataTempWU* tempWU,
    __global DataTempWF* tempWF
    */
) {
    const uint i = nc[0];
    const uchar matter = nm[0];
    enum PState state = ns[0];

    // ������ ��� �������� �������:
    //   - �������� � ������� �����
    float np[9];
    np[0] = s4f[i].pressure;
    for (uint cell = 1; cell <= 8; cell++) {
        np[cell] = s4f[ nc[cell] ].pressure;
    }
    const float pressure = np[0];

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


    // ������ �������� � �� ���� - ���������.
    // ��� ����� ���������������� � ����. ������:
    //   - ������
    //   - ��� ��� �� �������
    // ��� ����, ���� �������� � ������ ����� ������ �������� �������� ����,
    // ������ ���� ���������� � ������. ���� ������, "������� ����������"
    // �� ����������.

    // @todo ������� �������� ����� ��� ������ �������.
    // ������ ���� �������� ����� �������.
    const float molarMass = 29.0f / 1000.0f;

    
    // I. �������������� ������ ��� �������.

    // ���������� ����� ������[0] / ����[1] ����� ���
    uint flowCell[2];
    caclFlowCellGas(
        flowCell,
        nc, nm, ns, np
    );
    const uint outflowCell = flowCell[0];
    const uint inflowCell = flowCell[1];

    //barrier( CLK_GLOBAL_MEM_FENCE );

    // ��� �� �����
    if ( (outflowCell == 0) && (inflowCell == 0) ) {
        return;
    }


    // �������, ������� �������� ������ ����� �������.
    uint iFC = calcFillCellGas(
        i,
        nc, nm, ns, np,
        random
    );
    const uchar fillMatter = nm[ iFC ];
    const uint iFillCell = nc[ iFC ];
    const uint fillColor = s4u[ iFillCell ].color;

    //barrier( CLK_GLOBAL_MEM_FENCE );

    /*
    // @todo optimize ������� �� ������ �����������.
    if (fillMatter == VACUUM) {
        s4uOut[i].color = 0xff0000ff; return;
        return;
    }
    */

    
    // II. ������������ �������� ����.

    // ���� ����� ������ �������� ����
    /* - @todo .....
    const float force = nf[0];
    float newForce = force;
    const float speed = nv[0];
    float newSpeed = speed;
    calcNewForceGas(
        &newForce, &newSpeed,
        nc, nm, ns, nf, nv, np
    );
    */

    
    // ��������� ����� �������� � ������
    float newPressure = pressure;
    calcNewPressureGas(
        &newPressure,
        nc, nm, ns, nf, nv, np
    );
    


    // ��������� �������� ������ �� ����� ��������� ���������
    const float density = s4f[i].density;
    float newDensity =
        equal(pressure, newPressure) ? density :
        calcGasDensity( newPressure, molarMass );


    uchar newMatter = fillMatter;
    uint newColor = fillColor;
    if (newDensity >= DENSITY_BORDER) {
        if (state == VACUUM) {
            // ���� � ������ � �������� ����� ���, ������ ����������� ���� �����
            newMatter = fillMatter;
            newColor = fillColor;
        }
    } else {
        if (state == GAS) {
            // ��� ������ ������ ��������� ���� ������ ������ ������
            newMatter = 0;  // ������
            newColor = 0x000000ff;
            newDensity = 0.0f;
            newPressure = 0.0f;
        }
    }

    s1uOut[i].matter = newMatter;
    s4uOut[i].color = newColor;
    s4fOut[i].density = newDensity;
    s4fOut[i].pressure = newPressure;

}
