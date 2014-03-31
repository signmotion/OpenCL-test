/**
* Взаимодействие частиц.
* Использует сформированные шагом PREPARE_INTERACTION данные.
*/



/**
* Считаем "сколько себя передаём соседям".
* wm -> wmT
*/
inline void interactionI(
    __global WMap* wmT,
    __global WMap* wm,
    const float dt,
    const uint x,
    const uint z,
    // = x + z * N
    const uint i
) {
    /* @test
    wmT[i].pe[0] = wm[i].pe[0];
    return;
    */
    
    // Взаимодействие всегда требует времени
    // @todo optimize Можно убрать.
    if ( zero( dt ) ) {
        return;
    }


    // Ячейка может содержать до MAX_WMAP_CELL частиц из РАЗНОЙ материи.
    // Взаимодействуем с каждой из них.
    /* - Нет. Пока работаем с одним пластом.
    for (size_t n = 0; n < MAX_WMAP_CELL; ++n) {
    */
    const size_t n = 0;

    ParticleEntity p = wm[i].pe[n];

    // Вакуум здесь нас не интересует
    if ( vacuum( p ) ) {
        wmT[i].pe[n] = p;
        return;
    }
    
    
    // Константы для ускорения обработки
    // Скорость указывает на скорость движения массы частицы (вектор
    // заполнения массой соседних ячеек)
    const float2 velocity = (float2)(p.velocityX, p.velocityZ);
    
    
    // Скорость и направление изменения массы этой частицы
    float2 massDt = (float2)0.0f;
    
    
    // Считаем влияние соседей
    for (size_t k = 1; k <= 8; ++k) {
        const int iNear = nearCell( x, z, k );
        // Частица может быть на границе мира
        if (iNear == -1) {
            continue;
        }

        const ParticleEntity pNear = wm[iNear].pe[n];
        if ( vacuum( pNear ) ) {
            // Вакуум не влияет
            continue;
        }

        const float2 velocityNear = (float2)(pNear.velocityX, pNear.velocityZ);
        const float2 vij = velocity - velocityNear;
        massDt += pNear.mass * vij;

    } // for (size_t k = 1; k <= 8; ++k)


    // Результирующая скорость изменения массы частицы
    // Сила перемещает частицу
    const float2 force = (
        (float2)(DEFAULT_FORCE_X, DEFAULT_FORCE_Z)
    );
    /* - Уже посчитано в calcCharacteristicParticle().
    const float2 acceleration = 
        zero( p.mass ) ? 0.0f : ( force / p.mass );
    */
    const float2 acceleration = (float2)(p.accelerationX, p.accelerationZ);
    const float2 vForce = velocity + acceleration * dt;
    massDt += vForce;
    
    // Строим матрицу переноса
    //transferGas( &p );
    transferSolid( &p, massDt );
    
    // Остальное - см. interactionII()

    /* @test
    const size_t k = 1;
    setNB( &p, k, 0.01f );
    const float v = getNB( &p, k );
    setNB( &p, k, v );
    */


    // Сохраняем результат
    //p.velocityX = massDt.x;
    //p.velocityZ = massDt.y;
    wmT[i].pe[n] = p;

}








/**
* Считаем "какую часть каждого соседа приняли".
* Обновляем себя.
* wmT -> wm
*
* @see interactionI()
*/
inline void interactionII(
    __global WMap* wmT,
    __global WMap* wm,
    const float dt,
    const uint x,
    const uint z,
    // = x + z * N
    const uint i
) {
    /* @test
    wm[i].pe[0] = wmT[i].pe[0];
    return;
    */


    // Взаимодействие всегда требует времени
    // @todo optimize Можно убрать.
    if ( zero( dt ) ) {
        return;
    }
    

    // Ячейка может содержать до MAX_WMAP_CELL частиц из РАЗНОЙ материи.
    // Взаимодействуем с каждой из них.
    /* - Нет. Пока поработаем с одним пластом.
    for (size_t n = 0; n < MAX_WMAP_CELL; ++n) {
    */
    const size_t n = 0;

    // Работаем с обновлённой ячейкой
    ParticleEntity p = wmT[i].pe[n];

    // Вакуум тоже участвует в переносе.
    const bool pVacuum = vacuum( p );

    // Какую массу принимает себе частица от соседей
    float incomingMass = 0.0f;

    // Признак, что ячейка получила в процессе переноса новую материю
    bool addNewMatter = false;

    // Считаем перенос плотности (== массы) соседей
    for (size_t k = 1; k <= 8; ++k) {
        const int iNear = nearCell( x, z, k );
        // Частица может быть на границе мира
        if (iNear == -1) {
            // 0 == "массу от соседа не приняли"
            setNB( &p, k, 0.0f );
            continue;
        }

        // Работаем с обновлённой ячейкой
        ParticleEntity pNear = wmT[iNear].pe[n];
        const bool pNearVacuum = vacuum( pNear );

        // Получаем значение из ячейки матрицы переноса, с которой соприкасаемся
        // @see struct.h / struct NB
        const float transfer = getNBNear( pNear, k );
        if ( zero( transfer ) ) {
            // Нет переноса
            // 0 == здесь: "сосед ничего не передаёт"
            setNB( &p, k, 0.0f );
            continue;
        }

        if ( pNearVacuum ) {
            // Вакуум не переносится
            // @todo optimize Можно не вызывать setNB(), т.к. всё равно соседняя
            //       ячейка это пропустит. Т.о. перенесём получение transfer ниже.
            // 1 == "приняли всё, что сосед хотел передать; здесь - ничего"
            setNB( &p, k, 1.0f );
            continue;
        }

        // Смотрим, сколько массы соседней частицы можем принять к себе в ячейку.
        // Работаем только с частицами одной материи. Сейчас не смешиваем их.
        
        /*
        // @todo Расширить: большее количество частиц, перенос другой материи.
        if (p.matter != pNear.matter) {
            // 0 == "массу от соседа не приняли"
            setNB( &p, k, 0 );
            continue;
        }
        */

        // @todo Расширить: условия переноса.

        // Газ - расширяется. Вычисляем его скорость.
        // Скорость расширения газа (сумма p.expansion == 1 )
        // Расширение газа посчитано в calcCharacteristicParticle()
        const float2 vExpansionNear = pNear.expansion * nearForce( k );
        // Скорость материи в ячейке (под действием внешних сил)
        const float2 velocityNear = (float2)(pNear.velocityX, pNear.velocityZ);

        // Результирующая масса частицы
        // В ячейку не может "перетечь" массы больше, чем собирается отдать соседняя ячейка
        // @todo optimize Убрать 'clamp': масса всегда положительная.
        const float maxIncomingMassNear = pNear.mass * transfer;
        const float incomingMassNear = clamp(
            length( velocityNear + vExpansionNear * 0/* @debug */ ) * transfer * dt,
            0.0f, maxIncomingMassNear
        );

        incomingMass += incomingMassNear;

        // (incomingTransfer == 1) == "приняли ровно ту массу, которую и хотел передать сосед"
        // @todo optimize Можно убрать 'select zero()'?
        const float incomingTransfer = select(
            incomingMassNear / maxIncomingMassNear, 1.0f, zero( maxIncomingMassNear )
        );
        setNB( &p, k, incomingTransfer );

    } // for (size_t k = 1; k <= 8; ++k)

    
    // Проверяем: возможно, ячейка не содержит такой материи. Тогда
    // создаём в этой ячейке новую материю.
    if ( pVacuum && gZero( incomingMass ) ) {
        addNewMatter = true;
        p.matter = MATTER_WATER;  // pNear.matter;
        // Массу посчитаем ниже
        p.mass = 0.0f;
        p.velocityX = 50.0f;
        p.velocityZ = 0.0f;
        p.temperature = -111; //pNear.temperature;
    }
    

    // Ячейка могла получить новую материю.
    // Посчитаем её изменение при след. пульсе.
    if ( !pVacuum ) {
        // Из ячейки не может "перетечь" массы больше, чем в ней находится.
        // Не учитываем свою матрицу переноса, т.к. в любом случае передаём соседям
        // массу, зависящую только от скорости (сумма значений в матрице всегда 1.0 или 0.0).
        // @todo optimize Убрать 'transfer'.
        const float transfer = 1.0f;
        const float2 velocity = (float2)(p.velocityX, p.velocityZ);
        // @todo optimize Заменить 'clamp': масса всегда положительная.
        const float outcomingMass = clamp(
            ( length( velocity ) + p.expansion * 0/* @debug */ ) * transfer * dt,
            0.0f, p.mass
        );
        p.mass += incomingMass - outcomingMass;

        // Ячейка может опустеть
        if ( leZero( p.mass ) ) {
            p.matter = MATTER_VACUUM;
            p.mass = 0.0f;
            p.velocityX = p.velocityZ = 0.0f;
            p.temperature = -222.0f;
        }
        
        // @test
        p.t1 = incomingMass;
        p.t2 = outcomingMass;
        
    } // if ( !vacuum( p ) )
    
    
    p.velocityX += p.accelerationX * dt;
    p.velocityZ += p.accelerationZ * dt;


    // Сохраняем результат
    wm[i].pe[n] = p;

}
