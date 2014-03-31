// Вспомогательные методы.


/**
* Сравнение значений с заданной точностью.
*/
__constant const float PRECISION_POSITIVE = 1e-5f;
__constant const float PRECISION_NEGATIVE = - 1e-5f;

inline bool zero(float a) {
    return  ( isless( a, PRECISION_POSITIVE ) && isgreater( a, PRECISION_NEGATIVE ) );
}

inline bool equal(float a, float b) {
    return zero( a - b );
}




/**
* @return Признак, что указанная точка лежит у соотв. названию метода
* границы матрицы N x M. N, M задаются в pragma.hcl
*/
inline bool borderRight( const uint x, const uint z ) {
    return (x == (N - 1));
}

inline bool borderBottom( const uint x, const uint z ) {
    return (z == 0);
}

inline bool borderLeft( const uint x, const uint z ) {
    return (x == 0);
}

inline bool borderTop( const uint x, const uint z ) {
    return (z == (M - 1));
}


/**
* @return Признак, что указанная точка лежит внутри матрицы N x M.
* == !outsideMatrix()
*/
inline bool insideMatrix( const int x, const int z ) {
    return (
        (x >= 0) && (x < N)
     && (z >= 0) && (z < M)
    );
}



/**
* @return Признак, что указанная точка лежит вне матрицы N x M.
* == !insideMatrix()
*/
inline bool outsideMatrix( const int x, const int z ) {
    return (
        (z < 0) || (z >= M)
     || (x < 0) || (x >= N)
    );
}





/**
* @return Одномерный индекс указанной (относительно текущей) ячейки.
*         Если ячейка выходит за границы мира, возвращает -1.
*
* Расположение ячеек:
*   6   7   8   .   .   N
*   5   0   1
*   4   3   2
*   .
*   .
*   M
* 
*/
inline uint near(
    const int x, const int z,
    const int dx, const int dz
) {
    return (
        ( outsideMatrix( x + dx, z + dz ) || outsideMatrix( x, z ) )
          ? -1
          : (x + dx) + (z + dz) * N
    );
}



/**
* @see near()
*/
inline uint nearCell(
    const int x, const int z,
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
        case 0:  return near( x, z, 0, 0 );
    }

    return -1;
}




/**
* @return Частица, координаты которой не выходят за пределы мира.
*         При превышении пределов, частица помещается на границу.
*/
inline ParticleEntity cropCoordToBorder(const ParticleEntity p) {
    ParticleEntity pNew = p;

    if ( isless( p.coordX, 0) ) {
        pNew.coordX = 0.0f;
    } else if ( isgreaterequal( p.coordX, N) ) {
        pNew.coordX = N - 1;
    }

    if ( isless( p.coordZ, 0) ) {
        pNew.coordZ = 0.0f;
    } else if ( isgreaterequal( p.coordZ, M) ) {
        pNew.coordZ = M - 1;
    }

    return pNew;
}





/**
* @return Агрегатное состояние материи при указанной температуре.
*//* - Вычисляется при построении WMap.
     - @see HOST/fillWMap()
inline enum PState calcState(
    __global MatterSolid* mSolid,
    __global MatterLiquid* mLiquid,
    __global MatterGas* mGas,
    __global MatterPlasma* mPlasma,
    const uint matter,
    const float temperature
) {
    if (temperature < mSolid[matter].tt) {
        return SOLID;
    }
    
    if (temperature < mLiquid[matter].tt) {
        return LIQUID;
    }

    if (temperature < mGas[matter].tt) {
        return GAS;
    }

    return PLASMA;
}
*/


/**
* @return Агрегатное состояние материи для указанной частицы.
*/
/* - Агр. сост. получаем из ParticleEntity
inline enum PState getState(
    const uint uid,
    const ParticleEntity p,
    __global WMap* wm
) {

    // Вакуум не имеет состояния
    if (p.matter == VACUUM) {
        return VACUUM;
    }
    
    // Вышедшие за пределы частицы не учитываем
    // @todo optimize Проверку можно убрать.
    if ( outsideMatrix( p.coordX, p.coordZ ) ) {
        return VACUUM;
    }
    
    const int iCoord = convert_int( p.coordX ) + convert_int( p.coordZ ) * N;
    const WMap wmThis = wm[iCoord];
    
    // Ищем частицу в ячейке по UID
    int foundN = -1;
    for (size_t n = 0; n < MAX_WMAP_CELL; n++) {
        if (wmThis.uid[n] == uid) {
            foundN = n;
            break;
        } else if (wmThis.uid[n] == MATTER_VACUUM) {
            // Частица уже не будет найдена
            return VACUUM;
        }
    }
    
    // Частица не найдена
    if (foundN == -1) {
        return VACUUM;
    }
    
    return wmThis.state[foundN];
}
*/




/**
* @return Молярная масса материи.
*         Для состояний, отличных от GAS и PLASMA всегда = 0.
*/
inline float molarMassMatter(
    __global MatterGas* mGas,
    __global MatterPlasma* mPlasma,
    const uint matter,
    const enum PState state
) {
    return (
        (state == GAS) ? mGas[matter].molarMass :
        (state == PLASMA) ? mPlasma[matter].molarMass :
        0.0f
    );
}




/**
* @return Цвет материи.
*/
inline uint colorMatter(
    __global MatterSolid* mSolid,
    __global MatterLiquid* mLiquid,
    __global MatterGas* mGas,
    __global MatterPlasma* mPlasma,
    const uint matter,
    const enum PState state
) {
    return (
        (state == SOLID) ? mSolid[matter].color :
        (state == LIQUID) ? mLiquid[matter].color :
        (state == GAS) ? mGas[matter].color :
        mPlasma[matter].color
    );
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
    return (molarMass > 0.0f) ? density / molarMass * R * (-C0) : 0.0f;

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




/**
* @return Вычисляет агрегатное состояние частицы по температуре.
*/
inline enum PState calcState(
    __global MatterSolid* mSolid,
    __global MatterLiquid* mLiquid,
    __global MatterGas* mGas,
    __global MatterPlasma* mPlasma,
    const ParticleEntity p
) {
    // Вакуум не имеет состояния
    if (p.matter == MATTER_VACUUM) {
        return VACUUM;
    }

    if (p.temperature < mSolid[ p.matter ].tt) {
        return SOLID;
    }
    
    if (p.temperature < mLiquid[ p.matter ].tt) {
        return LIQUID;
    }

    if (p.temperature < mGas[ p.matter ].tt) {
        return GAS;
    }

    return PLASMA;
}




/**
* Рассчитывает характеристики частицы, зависящие от её агрегатного состояния.
* @see struct ParticleEntity
*/
inline ParticleEntity calcCharacteristicParticle(
    __global MatterSolid* mSolid,
    __global MatterLiquid* mLiquid,
    __global MatterGas* mGas,
    __global MatterPlasma* mPlasma,
    const ParticleEntity p
) {
    // Для вакуума считать нечего
    if (p.matter == MATTER_VACUUM) {
        return p;
    }

    ParticleEntity pNew = p;

    pNew.state = calcState( mSolid, mLiquid, mGas, mPlasma, p );
    
    pNew.accelerationX = p.forceX / p.mass;
    pNew.accelerationZ = p.forceZ / p.mass;

    pNew.density = p.mass / V_PARTICLE;
    
    const float molarMass =
        molarMassMatter( mGas, mPlasma, p.matter, pNew.state );
    // Молярная масса для "негазовых" состояний = 0.0
    pNew.pressure = calcGasPressure( pNew.density, molarMass);
    
    return pNew;
}




/**
* Рассчитывает скорость частицы, опираясь на поготовленные
* calcCharacteristicParticle() значения.
*/
/* - Считается отдельно.
inline ParticleEntity calcSpeedParticle(
    const float dt,
    const ParticleEntity p
) {
    ParticleEntity pNew = p;
    
    pNew.speedX += p.accelerationX * dt;
    pNew.speedZ += p.accelerationZ * dt;

    return pNew;
}
*/
