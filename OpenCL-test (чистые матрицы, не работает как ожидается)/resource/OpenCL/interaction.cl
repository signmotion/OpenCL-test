/**
* Взаимодействие сущностей.
*/


/**
* Структура для расчёта взаимодействия сущностей.
* Передаётся как матрица одного типа, по 32 разряда,
* т.к. есть ограничения на запись в память >
* http://www.khronos.org/registry/cl/sdk/1.0/docs/man/xhtml/restrictions.html
*/
typedef struct {
    uint entity;
    float forceX;
    float forceZ;
    float speedX;
    float speedZ;
    float density;
} WData;


/**
* Информация о конкретных сущностях мира.
*/
typedef struct {
    uint id;
    float density;
} EData;


__kernel void interaction(
    // Результат взаимодействия сущностей.
    __global WData* wOut,
    // Входящие данные о мире.
    __global WData* wIn,
    // Входящие данные о конкретных сущностях.
    __global EData* eIn,
    // Размер неделимой ячейки мира, в метрах.
    // Мир - плоский, ячейки считаем квадратными.
    const float scale,
    // Размер мира.
    const uint2 size,
    // Время в секундах, за которое рассчитываются взаимодействия.
    const float time
) {
    const unsigned int i = get_global_id(0);

    // test
    wOut[i].entity = 7;

    const WData thisWIn = wIn[i];

    const float mass = thisWIn.density * pow(scale, 3);
    if ( isless(mass, 1e-4) ) {
        // Сущность с нулевой массой считаем неподвижной
        return;
    }

    // Считаем новые скорости сущности
    float2 thisForce = (float2)( thisWIn.forceX, thisWIn.forceZ );
    const float2 a = thisForce / mass;
    float2 thisSpeed =
        (float2)( thisWIn.speedX, thisWIn.speedZ ) + time * a;


    // Дожидаемся завершения других потоков
    //barrier( CLK_GLOBAL_MEM_FENCE );

    
    // Рассчитываем столкновения и силы, пересчитываем
    // вычисленные выше значения.

    // Просматриваем сущности, с которыми может быть пересечение этой сущности.
    // Смотрим не весь мир, а только сущности, встречающиеся на пути этой сущности.
    // Для этого проходим по вектору скорости сущности, предварительно вычислив
    // оптимальное значение dt, чтобы не пропустить и одной ячейки, которую
    // пересекает сущность за время 'time'.
    //const uint N = size.s0;
    //const uint M = size.s1;

    //const float dtX = speedNew[i].s0


    /* @todo ...
    // Work items with x ID at right workgroup edge will read Left apron pixel
    if (get_local_id(0) == (get_local_size(0) - 1))
    {
        // set local offset to read data from the next region over
        iLocalPixOffset = mul24((int)get_local_id(1), iLocalPixPitch);

        // If source offset is within the image boundaries and not at the leftmost workgroup
        if ((iDevYPrime > -1) && (iDevYPrime < iDevImageHeight) && (get_group_id(0) > 0))
        {
            // Read data into the LMEM apron from the GMEM at the left edge of the next block region over
            uc4LocalData[iLocalPixOffset] = uc4Source[mul24(iDevYPrime, (int)get_global_size(0)) + mul24(get_group_id(0), get_local_size(0)) - 1];
        }
        else 
        {
            uc4LocalData[iLocalPixOffset] = (uchar4)0; 
        }

        // If in the bottom 2 rows of workgroup block 
        if (get_local_id(1) < 2)
        {
            // Increase local offset by 1 workgroup LMEM block height
            // to read in top rows from the next block region down
            iLocalPixOffset += mul24((int)get_local_size(1), iLocalPixPitch);

            // If source offset in the next block down isn't off the image and not at the leftmost workgroup
            if (((iDevYPrime + get_local_size(1)) < iDevImageHeight) && (get_group_id(0) > 0))
            {
                // read in from GMEM (reaching down 1 workgroup LMEM block height and left 1 pixel)
                uc4LocalData[iLocalPixOffset] = uc4Source[mul24((iDevYPrime + (int)get_local_size(1)), (int)get_global_size(0)) + mul24(get_group_id(0), get_local_size(0)) - 1];
            }
            else 
            {
                uc4LocalData[iLocalPixOffset] = (uchar4)0; 
            }
        }
    } 
    else if (get_local_id(0) == 0) // Work items with x ID at left workgroup edge will read right apron pixel
    {
        // set local offset 
        iLocalPixOffset = mul24(((int)get_local_id(1) + 1), iLocalPixPitch) - 1;

        if ((iDevYPrime > -1) && (iDevYPrime < iDevImageHeight) && (mul24(((int)get_group_id(0) + 1), (int)get_local_size(0)) < iImageWidth))
        {
            // read in from GMEM (reaching left 1 pixel) if source offset is within image boundaries
            uc4LocalData[iLocalPixOffset] = uc4Source[mul24(iDevYPrime, (int)get_global_size(0)) + mul24((get_group_id(0) + 1), get_local_size(0))];
        }
        else 
        {
            uc4LocalData[iLocalPixOffset] = (uchar4)0; 
        }

        // Read bottom 2 rows of workgroup LMEM block
        if (get_local_id(1) < 2)
        {
            // increase local offset by 1 workgroup LMEM block height
            iLocalPixOffset += (mul24((int)get_local_size(1), iLocalPixPitch));

            if (((iDevYPrime + get_local_size(1)) < iDevImageHeight) && (mul24((get_group_id(0) + 1), get_local_size(0)) < iImageWidth) )
            {
                // read in from GMEM (reaching down 1 workgroup LMEM block height and left 1 pixel) if source offset is within image boundaries
                uc4LocalData[iLocalPixOffset] = uc4Source[mul24((iDevYPrime + (int)get_local_size(1)), (int)get_global_size(0)) + mul24((get_group_id(0) + 1), get_local_size(0))];
            }
            else 
            {
                uc4LocalData[iLocalPixOffset] = (uchar4)0; 
            }
        }
    }
*/



    // Перемещаем сущности и их характеристики


}
