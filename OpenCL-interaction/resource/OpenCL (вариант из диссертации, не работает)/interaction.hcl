/**
* Взаимодействие частиц.
* Частицы 'peOut' в обсчитываемой ячейке получают новое состояние.
* Использует сформированные шагом PREPARE_INTERACTION данные.
*/
inline void interaction(
    __global ParticleEntity* peOut,
    __global WMap* wm,
    const float dt,
    // Методу передаётся именно координаты ячейки, а не uid частицы
    const uint xCell,
    const uint zCell,
    // = xCell + zCell * N
    const uint coordCell
) {
    // Частицы, состояние которых будет изменено.
    // Вынесены отдельно, чтобы сохранить состояние частиц при расчёте
    // влияния друг на друга.
    ParticleEntity peOutT[ MAX_WMAP_CELL ];
    // Свои UID частицы хранят отдельно.
    int uidOrSignT[ MAX_WMAP_CELL ];

    // Ячейка, частицы в которой надо посчитать
    const WMap cell = wm[coordCell];
    
    // Плотность вещества в ячейке
    float densityCell = 0.0f;
    
    // Цикл по конкректным частицам в ячейке.
    for (size_t n = 0; n < MAX_WMAP_CELL; n++) {
        const int uidOrSign = cell.uidOrSign[n];
        uidOrSignT[n] = uidOrSign;
        // Все частицы записываются в ячейку последовательно => встреча '-1'
        // означает, что больше частиц в ячейке нет
        if (uidOrSign == -1) {
            break;
        }
        
        const uint uid = uidOrSign;
        
        // Обсчитываемая частица: данные в ней будут обновлены.
        // Собираем результат в 'peOutT' ниже (для понятного кода).
        ParticleEntity p = peOut[uid];
        
        // Декларация переменных для ускорения расчётов ниже.
        const float2 coord = (float2)( p.coordX, p.coordZ );
        const float2 acceleration = (float2)( p.accelerationX, p.accelerationZ );
        const float2 velocity = (float2)( p.velocityX, p.velocityZ );
        const float pressureRel = p.pressure - PRESSURE_0;
        const float density2 = p.density * p.density;


        // Переменные для накопления результата.
        // a - сокр. от Accumulation
        float aDensity = 0.0;
        float aPressure = 0.0;
        float2 aVelocity = (float2)( 0.0f, 0.0f );
        float2 aCoord = (float2)( 0.0f, 0.0f );
        float aViscosity = 0.0f;

        // Счётчик соседей
        size_t neighbor = 0;


        // Считаем влияние соседей.
        // Просматриваем центральную ячейку (помимо этой, в ней могут быть
        // другие частицы) и ячейки рядом.
        for (size_t q = 0; q <= 8; q++) {
            const int coordCellNear = nearCell(xCell, zCell, q);
            // Ячейка лежит вне обсчитываемого поля (крайняя), пропускаем
            if (coordCellNear == -1) {
                continue;
            }

            const WMap cellNear = wm[coordCellNear];

            // Просматриваем все частицы в этой ячейке.
            // Находим соседа, с которым хотим взаимодействовать.
            for (size_t k = 0; k < MAX_WMAP_CELL; k++) {
                // Дальше повторяется обёртка внешнего цикла. Аналогичные
                // комментарии упущены.
                
                const int uidOrSign = cellNear.uidOrSign[k];
                if (uidOrSign == -1) {
                    break;
                }

                const uint uidNear = uidOrSign;

                // Проверяем, что сосед не является обсчитываемой частицей
                if (uidNear == uid) {
                    continue;
                }

                // Сосед
                const ParticleEntity pNear = peOut[uidNear];
                neighbor++;
                
                // Влияние соседа на частицу.
                // См. декларацию переменных выше.
                
                // Декларация переменных для ускорения расчётов ниже.
                const float2 coordNear = (float2)( pNear.coordX, pNear.coordZ );
                // @todo optimize Перенести за вычислением 'w'.
                const float2 velocityNear = (float2)( pNear.velocityX, pNear.velocityZ );
                const float pressureRelNear = pNear.pressure - PRESSURE_0;
                
                // Расстояние между частицами и сглаживающее ядро.
                // @todo optimize distance(x, y) = sqrt( dot(x, y) )
                //       Исп. вместе с занесением ядра в тело цикла.
                const float r = distance( coord, coordNear ) +
                    // Страховка от деления на '0'.
                    1e-4;
                const float r2 = r * r;
                const float2 rij = coord - coordNear;
                // @todo optimize Внести в цикл, избавившись от лишнего деления
                const float w = sphW( r, SPH_H );
                if ( zero( w ) ) {
                    // Сосед слишком далеко, чтобы влиять на частицу
                    break;
                }
                
                // Считаем влияние
                
                // Градиент сглаживающего ядра определяется через алгебраическую
                // производную сглаживающего ядра
                // @see SPH / стр. 34 (1.9)
                const float2 gradW = rij / r;

                // Относительная скорость
                const float2 vij = velocity - velocityNear;

                // Плотность
                /* - Плотность считаем через уравнение неразрывности. См. ниже.
                // @see SPH / стр. 40 (2.5)
                aDensity += pNear.mass * w;
                */
                
                // Плотность через уравнение неразрывности
                // @see SPH / стр. 42 (2.13)
                aDensity += pNear.mass * dot( vij, gradW );

                // Давление
                // @see SPH / стр. 40 (2.6)
                const float density2Near = pNear.density * pNear.density;
                aPressure += p.density * pNear.mass * (
                    pressureRel / density2 + pressureRelNear / density2Near
                ) * w;
                // или @see SPH / стр. 39 (2.2)
                //aPressure += pNear.mass / pNear.density * pressureRelNear * w;
                
                // Скорость
                // @see SPH / Стр. 40 (2.7) или SPH / Стр. 55 (2.23)
                // @todo Добавить вязкость.
                // @todo optimize Использовать выражение из давления.
                aVelocity += - pNear.mass * (
                    p.pressure / density2 + pNear.pressure / density2Near
                ) * gradW;
                
                // Вязкий член
                /* - Сейчас используем простой. См. ниже.
                // @see SPH / Стр. 48 (2.20)
                const float vrC = dot( vij, rij );
                const float dijC = p.density * pNear.density;
                const float vdijC = (p.viscosity + pNear.viscosity) / dij;
                // @see SPH / Стр. 45 (2.18)
                const float vijC = SPH_H * vr / ( dot( rij, rij ) + 0.01 * SPH_H * SPH_H );
                const float halfDensity = (p.density + pNear.density) / 2;
                // @todo ? Правильно записана формула?
                aViscosity += pNear.mass * (
                    isless( vrC, 0 )
                      ? (
                            dot( vdijC * coord / r2, vij ) +
                            SPH_BETA * vijC * vijC / halfDensity * w
                        )
                      : (
                            dot ( vdijC, vij)
                        )
                );
                */
                const float vrC = dot( vij, rij );
                aViscosity += pNear.mass * w * (
                    isless( vrC, 0 )
                      ? (
                            - SPH_ALPHA * 
                            SPH_BETA * vijC * vijC / halfDensity * w
                        )
                      : (
                            dot ( vdijC, vij)
                        )
                );
                
                // Перемещение
                // Исп. сглаживающая скорость
                // @see SPH / стр. 55 (2.26)
                const float2 vji = - vij;
                aCoord += velocity + SPH_EPSILON * ( pNear.mass / halfDensity * vji * w );
                

                densityCell += pNear.mass * w;
                
            } // for (size_t k = 0; k < MAX_WMAP_CELL; k++)

        } // for (size_t q = 0; q <= 8; q++)
        
        
        // Обновляем частицу.
        // Учитываем и силы, действующие на частицу.
        // Ускорение посчитано в calcCharacteristicParticle().
        p.density += aDensity * dt;
        p.pressure += aPressure * dt;

        p.velocityX += aVelocity.x * dt + p.accelerationX * dt;
        p.velocityZ += aVelocity.y * dt + p.accelerationZ * dt;
        
        p.coordX += aCoord.x * dt + p.accelerationX * dt * dt / 2;
        p.coordZ += aCoord.y * dt + p.accelerationZ * dt * dt / 2;

        // @test
        //p.coordX += -40.0f;
        //p.coordZ += -20.0f;

        
        // Влияние на частицу выше посчитали.
        // Теперь корректируем... и запоминаем.
        peOutT[n] = cropCoordToBorder( p );
        
    } // for (size_t n = 0; n < MAX_WMAP_CELL; n++)
    
    
    
    // Обновляем информацию о частицах, которые находились в ячейке.
    // Чтобы не захватить мусор позаботились вначале.
    for (size_t n = 0; n < MAX_WMAP_CELL; n++) {
        if (uidOrSignT[n] == -1) {
            break;
        }
        
        const ParticleEntity p = peOutT[n];
        const uint uid = uidOrSignT[n];
        peOut[uid] = p;
    }
    
}
