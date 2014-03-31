// Вспомогательные методы.


/**
* Сравнение значений с заданной точностью.
*/
__constant const float PRECISION_POSITIVE = 1e-5f;
__constant const float PRECISION_NEGATIVE = - 1e-5f;

inline bool zero(float a) {
    return  ( isless( a, PRECISION_POSITIVE ) && isgreater( a, PRECISION_NEGATIVE ) );
}

inline bool lZero(float a) {
    return isless( a, PRECISION_NEGATIVE );
}

inline bool leZero(float a) {
    return lZero( a ) || zero( a );
}

inline bool gZero(float a) {
    return isgreater( a, PRECISION_POSITIVE );
}

inline bool geZero(float a) {
    return gZero( a ) || zero( a );
}

inline bool equal(float a, float b) {
    return zero( a - b );
}

inline bool positiveOne(float a) {
    return zero( a - 1 );
}

inline bool negativeOne(float a) {
    return zero( a + 1 );
}

inline bool one(float a) {
    return positiveOne( a ) || negativeOne( a );
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
* @return Признак, что частица представляет собой вакуум.
*/
inline bool mVacuum( const enum UIDMatter matter ) {
    return (matter == MATTER_VACUUM);
}

inline bool vacuum( const ParticleEntity p ) {
    return mVacuum( p.matter );
}



/**
* @return Признак, что частица находится в данном агрегатном состоянии.
*
* (!) Состояние проверяется по значению, посчитанному для частицы ранее.
* @see calcState()
*/
inline bool solid( const ParticleEntity p ) {
    return (p.state == SOLID);
}

inline bool liquid( const ParticleEntity p ) {
    return (p.state == LIQUID);
}

inline bool gas( const ParticleEntity p ) {
    return (p.state == GAS);
}

inline bool plasma( const ParticleEntity p ) {
    return (p.state == PLASMA);
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
inline int near(
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
inline int nearCell(
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
* @return Единичный вектор силы, действующий на частицу (x, y) со стороны
*         указанной ячейки.
*
* @see near()
*/
inline float2 nearForce(
    const uint cell
) {
    switch ( cell ) {
        case 1:  return (float2)( -1.0f, 0.0f );
        case 2:  return (float2)( -0.707f, 0.707f );
        case 3:  return (float2)( 0.0f, 1.0f );
        case 4:  return (float2)( 0.707f, 0.707f );
        case 5:  return (float2)( 1.0f, 0.0f );
        case 6:  return (float2)( 0.707f, -0.707f );
        case 7:  return (float2)( 0.0f, -1.0f );
        case 8:  return (float2)( -0.707f, -0.707f );
        case 0:  return (float2)( 0.0f, 0.0f );
    }

    return (float2)( -1.0f, -1.0f );
}





/**
* @return Значение ячейки 'k' матрицы соседей 'nb'.
*         Диапазон возвращаемых значений - [0.0; 1.0].
*         Диапазон 'k' - целые числа [1; 8]
*
* Если ячейка недопустима, возвращает -1.0
*
* @see struct.h / struct NB
*/
inline float getNB( const ParticleEntity p, size_t k ) {
    // @todo optimize Убрать проверку.
    if ( (k < 1) || (k > 8) ) {
        return -1.0f;
    }

    const size_t shift = (k > 4) ? (k - 5) : (k - 1);
    const uint c = (k > 4) ? p.nb.nb58 : p.nb.nb14;

    // Извлекаем и нормализуем
    const uint v = (uint)(
        (shift == 0) ? c : ( c >> (shift * 8) )
    ) & 0xFF;

    return (float)v / 0xFF;
}



/**
* @return Значение ячейки матрицы соседей 'nb', которая соприкасается
*         с ячейкой 'k'.
*
* Если ячейка недопустима, возвращает -1.0
* Метод используется для получения степени влияния соседней частицы.
*
* @see getNB()
* @see struct.h / struct NB
*/
inline float getNBNear( const ParticleEntity p, size_t kThis ) {
    switch ( kThis ) {
        case 1:  return getNB( p, 5 );
        case 2:  return getNB( p, 6 );
        case 3:  return getNB( p, 7 );
        case 4:  return getNB( p, 8 );
        case 5:  return getNB( p, 1 );
        case 6:  return getNB( p, 2 );
        case 7:  return getNB( p, 3 );
        case 8:  return getNB( p, 4 );
    }

    // Недопустимый номер ячейки
    return -1.0f;
}



/**
* Сбрасывает у частицы матрицу соседей 'NB' в ноль.
*
* @see setNB()
*/
inline void clearNB( ParticleEntity* p ) {
    p->nb.nb14 = p->nb.nb58 = 0x00000000;
}



/**
* Устанавливает у частицы значение ячейки 'k' матрицы соседей 'NB'.
*
* @param k      Целые числа [1; 8]
* @param value  Диапазон [0.0; 1.0].
*
* Если ячейка недопустима, сбрасывает всех соседей в 0.0
* Если передаётся значение вне заданного диапазона, оно ограничивается
* рамками диапазона.
* Точность хранимого значения = 1 / 255 (т.е. ~ 2 знака).
*
* @see struct.h / struct NB
*/
inline void setNB( ParticleEntity* p, size_t k, float value ) {
    // @todo optimize Убрать проверку.
    if ( (k < 1) || (k > 8) ) {
        clearNB( p );
        return;
    }

    const float valueClamp = clamp( value, 0.0f, 1.0f );
    const size_t shift = k - ( (k > 4) ? 5 : 1 );

    // Подготавливаем
    const uint mask =
        (shift == 0) ? 0xFFFFFF00 :
          (shift == 1) ? 0xFFFF00FF :
            (shift == 2) ? 0xFF00FFFF : 0x00FFFFFF;
    const uint c = ( (k > 4) ? p->nb.nb58 : p->nb.nb14 ) & mask;

    // Нормализуем и устанавливаем
    const uint v = (uint)floor( valueClamp * 0xFF + 0.5f );
    const uint vc = c | (
        (shift == 0) ? v : ( v << (shift * 8) )
    );
    (k > 4)
      ? ( p->nb.nb58 = vc )
      : ( p->nb.nb14 = vc );
}



/**
* Инициализирует матрицу переноса указанным значением.
*
* @param value  Диапазон [0.0; 1.0].
*
* @see setNB()
*/
inline void fillNB( ParticleEntity* p, float value ) {
    // @todo optimize Можно быстрее.
    for (size_t k = 1; k <= 8; ++k) {
        setNB( p, k, value );
    }
}




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
    const ParticleEntity p
) {
    return (
        solid( p ) ? mSolid[ p.matter ].color :
        liquid( p ) ? mLiquid[ p.matter ].color :
        gas( p ) ? mGas[ p.matter ].color :
        mPlasma[ p.matter ].color
    );
}






/**
* @return Давление газа.
*
* p = D / M * R * T
*   где М - молярная масса вещества (воздух = 29 г/моль)
*   R - газовая постоянная
*   D - плотность
*   T - температура
*/
inline float calcGasPressure( const float density, const float molarMass, const float temperature ) {
    return
        zero( molarMass )
          ? 0.0f
          : density / molarMass * R * (temperature - C0);
}



/**
* @return Плотность газа.
*
* D = p * M / R / T
* @see calcGasPressure()
*/
inline float calcGasDensity( const float pressure, const float molarMass, const float temperature ) {
    const float t = temperature - C0;
    return zero( t ) ? 0.0f : ( pressure * molarMass / R / t );
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
    if ( vacuum( p ) ) {
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
* Рассчитывает характеристики для полученной частицы,
* зависящие от её агрегатного состояния.
* @see struct ParticleEntity
*/
inline void calcCharacteristic(
    __global MatterSolid* mSolid,
    __global MatterLiquid* mLiquid,
    __global MatterGas* mGas,
    __global MatterPlasma* mPlasma,
    ParticleEntity* p
) {
    if ( vacuum( *p ) ) {
        // Для вакуума считать нечего.
        // @todo optimize Не обнулять характеристики частицы - всегда проверяем по vacuum( p ).
        p->state = VACUUM;
        return;
    }

    // Вычисляем характеристики
    p->state = calcState( mSolid, mLiquid, mGas, mPlasma, *p );

    const bool zeroMass = zero( p->mass );
    p->accelerationX = zeroMass ? 0.0f : ( DEFAULT_FORCE_X / p->mass );
    p->accelerationZ = zeroMass ? 0.0f : ( DEFAULT_FORCE_Z / p->mass );

    p->density = p->mass / V_PARTICLE;

    const float molarMass = molarMassMatter(
        mGas, mPlasma, p->matter, p->state
    );
    // Молярная масса для "негазовых" состояний = 0.0
    p->pressure = calcGasPressure( p->density, molarMass, p->temperature );

    // Скорость расширения материи (для состояний GAS и PLASMA)
    // @todo Связать с коэффициентом.
    if ( gas( *p ) || plasma( *p ) ) {
        p->expansion = p->pressure;
    } else {
        p->expansion = 0.0f;
    }

}



/**
* Рассчитывает характеристики частицы.
* @see calcCharacteristic()
*/
inline void calcCharacteristicParticle(
    __global MatterSolid* mSolid,
    __global MatterLiquid* mLiquid,
    __global MatterGas* mGas,
    __global MatterPlasma* mPlasma,
    __global WMap* wm,
    // Координата частицы в матрице
    const uint i,
    // Порядковый номер частицы в ячейке
    const uint n
) {
    ParticleEntity p = wm[i].pe[n];
    calcCharacteristic( mSolid, mLiquid, mGas, mPlasma, &p );
    wm[i].pe[n] = p;
}




/**
* Вызывает calcCharacteristicParticle() для каждой частицы в ячейке.
*/
inline void calcCharacteristicParticleInCell(
    __global MatterSolid* mSolid,
    __global MatterLiquid* mLiquid,
    __global MatterGas* mGas,
    __global MatterPlasma* mPlasma,
    __global WMap* wm,
    // Координата частицы в матрице
    const uint i
) {
    for (size_t n = 0; n < MAX_WMAP_CELL; ++n) {
        calcCharacteristicParticle(
            mSolid, mLiquid, mGas, mPlasma, wm, i, n
        );
    }
}







/**
* Формирует матрицу переноса для частицы в агр. сост. GAS.
* @see transferSolid()
*
* Газ стремится занять всё доступное ему пространство. Здесь - 8 соседних ячеек.
*/
inline void transferGas(
    ParticleEntity* p
) {
    fillNB( p, 1.0f / 8 );
}




/**
* Формирует матрицу переноса для частицы в агр. сост. SOLID.
* Матрица переноса определяет, как распределится масса частицы (в ячейке) под
* влиянием действующего вектора скорости при ПОЛНОМ перемещении частицы в
* соседнюю ячейку.
* Матрица переноса строится для 8-ми соседей и удовлетворяет след. требованиям:
*   - Значения положительные и лежат в пределах [0.0, 1.0]
*   - '0' означает, что переноса в сторону этого соседа нет
*   - Сумма значений в матрице равна 0 (когда вектор скорости == 0) или 1
*
* @see Расположение ячеек в near()
*/
inline void transferSolid(
    ParticleEntity* p,
    const float2 velocity
) {
    clearNB( p );

    // Нормализуем скорость
    // @todo optimize Есть в OpenCL готовое решение?
    const bool negativeX = signbit( velocity.x );
    const bool negativeZ = signbit( velocity.y );
    const float vAbsMax = max(
        select( velocity.x, -velocity.x, negativeX ),
        select( velocity.y, -velocity.y, negativeZ )
    );
    if ( zero( vAbsMax ) ) {
        // В ноль сбросили выше
        return;
    }
    const float2 vn = (float2)( velocity.x / vAbsMax, velocity.y / vAbsMax );
    
    /*
    // Определяем максимальную координату (== 1.0)
    const bool xGreater = isgreater( velocity.x, velocity.y );
    const float сMax = select( velocity.y, velocity.x, xGreater );
    const float cMin = select( velocity.x, velocity.y, xGreater );
    */

    // Здесь знаем наверняка, что одно из значений - не нулевое (см. выше)
    const bool positiveX = !negativeX;
    const bool positiveZ = !negativeZ;
    
    if ( zero( vn.x ) ) {
        const size_t k = select( 3, 7, positiveZ );
        setNB( p, k, 1.0f );
        return;
    }
    
    if ( zero( vn.y ) ) {
        const size_t k = select( 5, 1, positiveX );
        setNB( p, k, 1.0f );
        return;
    }
    
    
    const bool oneX = one( vn.x );
    const bool oneZ = one( vn.y );

    if ( oneX ) {
        // Считаем углы
        float c2 = 0.0f;
        float c4 = 0.0f;
        float c6 = 0.0f;
        float c8 = 0.0f;
        // 2
        if ( positiveX && negativeZ ) {
            c2 = -vn.y;
            setNB( p, 2, c2 );
        }
        // 4
        if ( negativeX && negativeZ ) {
            c4 = -vn.y;
            setNB( p, 4, c4 );
        }
        // 6
        if ( negativeX && positiveZ ) {
            c6 = vn.y;
            setNB( p, 6, c6 );
        }
        // 8
        if ( positiveX && positiveZ ) {
            c8 = vn.y;
            setNB( p, 8, c8 );
        }

        // Считаем бока
        // 1
        if ( positiveX ) {
            setNB( p, 1, vn.x - c8 - c2 );
        }
        // 3
        /* - Всегда == 0
        if ( negativeZ ) {
            setNB( p, 3, -vn.y - c2 - c4 );
        }
        */
        // 5
        if ( positiveX ) {
            setNB( p, 5, -vn.x - c4 - c6 );
        }
        // 7
        /* - Всегда == 0
        if ( negativeZ ) {
            setNB( p, 7, vn.y - c6 - c8 );
        }
        */

        return;
    }


    if ( oneZ ) {
        // Считаем углы
        float c2 = 0.0f;
        float c4 = 0.0f;
        float c6 = 0.0f;
        float c8 = 0.0f;
        // 2
        if ( positiveX && negativeZ ) {
            c2 = vn.x;
            setNB( p, 2, c2 );
        }
        // 4
        if ( negativeX && negativeZ ) {
            c4 = -vn.x;
            setNB( p, 4, c4 );
        }
        // 6
        if ( negativeX && positiveZ ) {
            c6 = -vn.x;
            setNB( p, 6, c6 );
        }
        // 8
        if ( positiveX && positiveZ ) {
            c8 = vn.x;
            setNB( p, 8, c8 );
        }

        // Считаем бока
        // 1
        /* - Всегда == 0
        if ( positiveX ) {
        }
        */
        // 3
        if ( negativeZ ) {
            setNB( p, 3, -vn.y - c2 - c4 );
        }
        // 5
        /* - Всегда == 0
        if ( positiveX ) {
        }
        */
        // 7
        if ( negativeZ ) {
            setNB( p, 7, vn.y - c6 - c8 );
        }

        return;
    }

}
