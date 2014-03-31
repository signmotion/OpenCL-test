/**
* Сравнивание значений с заданной точностью.
*/
__constant const float PRECISION = 0.001f;

inline bool equal(float a, float b) {
    return isequal( a, b );
}

inline bool zero(float a) {
    return isless( a, PRECISION );
}




/**
* @return Признак, что указанная точка лежит у соотв. названию метода
* границы матрицы N x M. N, M задаются в pragma.hcl
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
* @return Одномерный индекс указанной (относительно текущей) ячейки.
*         Если ячейка выходит за границы мира, возвращает индекс
*         текущей ячейки.
*
* Расположение ячеек:
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
* @return Агрегатное состояние материи.
*/
inline enum PState calcState( const uchar matter, const float temperature ) {
    // @todo Рассчитывать агр. сост. согласно декларации материи.
    return (
        (matter == 1) ? GAS :
        (matter == 0) ? VACUUM :
        SOLID
    );
}



/**
* @return Признак: Газ или Вакуум.
*/
inline bool gasOrVacuum( const enum PState state, const uchar matter ) {
    return ( (state == GAS) || (matter == VACUUM) );
}



/**
* @return Давление газа.
*
* p = D / M * R * T
*   где М - молярная масса вещества (воздух = 29 г/моль)
*   R - газовая постоянная
*   D - плотность
*/
inline float calcGasPressure( const float density, const float molarMass ) {
    return density / molarMass * R * (-C0);

}



/**
* @return Плотность газа.
*
* D = p * M / R / T
* @see calcGasPressure()
*/
inline float calcGasDensity( const float pressure, const float molarMass ) {
    return pressure * molarMass / R / (-C0);
}

