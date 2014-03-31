/**
* ����������� �������� � �������� ���������.
*/
__constant const float PRECISION = 0.001f;

inline bool equal(float a, float b) {
    return isequal( a, b );
}

inline bool zero(float a) {
    return isless( a, PRECISION );
}




/**
* @return �������, ��� ��������� ����� ����� � �����. �������� ������
* ������� ������� N x M. N, M �������� � pragma.hcl
*/
inline bool borderRight( const uint x, const uint z ) {
    return (x == N - 1);
}

inline bool borderBottom( const uint x, const uint z ) {
    return (z == 0);
}

inline bool borderLeft( const uint x, const uint z ) {
    return (x == 0);
}

inline bool borderTop( const uint x, const uint z ) {
    return (z == M - 1);
}




/**
* @return ���������� ������ ��������� (������������ �������) ������.
*         ���� ������ ������� �� ������� ����, ���������� ������
*         ������� ������.
*
* ������������ �����:
*   6   7   8   .   .   N
*   5   i   1
*   4   3   2
*   .
*   .
*   M
* 
*/
inline uint near(
    const uint x, const uint z,
    const int dx, const int dz
) {
    const uint i = x + z * N;
    if ( borderRight(x, z) && (dx > 0) ) {
        return 0;
    }
    if ( borderBottom(x, z) && (dz < 0) ) {
        return 0;
    }
    if ( borderLeft(x, z) && (dx < 0) ) {
        return 0;
    }
    if ( borderTop(x, z) && (dz > 0) ) {
        return 0;
    }

    return i + (dx + dz * N);
}


inline uint nearCell(
    const uint x, const uint z,
    const uint cell
) {
    switch ( cell ) {
        case 1:  return near( x, z, +1, 0 );
        case 2:  return near( x, z, +1, -1 );
        case 3:  return near( x, z, 0, -1 );
        case 4:  return near( x, z, -1, -1 );
        case 5:  return near( x, z, -1, 0 );
        case 6:  return near( x, z, -1, +1 );
        case 7:  return near( x, z, 0, +1 );
        case 8:  return near( x, z, +1, +1 );
    }

    return near( x, z, 0, 0 );
}






/**
* @return ���������� ��������� �������.
*/
inline enum PState calcState( const uchar matter, const float temperature ) {
    // @todo ������������ ���. ����. �������� ���������� �������.
    return (
        (matter == 1) ? GAS :
        (matter == 0) ? VACUUM :
        SOLID
    );
}



/**
* @return �������: ��� ��� ������.
*/
inline bool gasOrVacuum( const enum PState state, const uchar matter ) {
    return ( (state == GAS) || (matter == VACUUM) );
}



/**
* @return �������� ����.
*
* p = D / M * R * T
*   ��� � - �������� ����� �������� (������ = 29 �/����)
*   R - ������� ����������
*   D - ���������
*/
inline float calcGasPressure( const float density, const float molarMass ) {
    return density / molarMass * R * (-C0);

}



/**
* @return ��������� ����.
*
* D = p * M / R / T
* @see calcGasPressure()
*/
inline float calcGasDensity( const float pressure, const float molarMass ) {
    return pressure * molarMass / R / (-C0);
}

