/**
* Взаимодействие сущностей. См. interaction() в конце кода.
* Реализация через image2d.
* @see http://www.cmsoft.com.br/index.php?option=com_content&view=category&layout=blog&id=115&Itemid=172
*/

#define TEST



/**
* Информация о конкретных сущностях мира.
*/
typedef struct {
    uchar id;
    // Плотность воплощённого элемента в мире может отличаться
    float density;
} EData;


/**
* Структура для расчёта взаимодействия сущностей.
* Создана для более наглядного доступа к элементам, хранимым в виде текстур.
* Передаётся как текстура imge2d_t, в пикселях которой содержится информация
* о ячейках мира.
*
* @see convert()
*/
typedef struct {
    // @todo optimize Несмотря на то, что ID сущности лежит в диапазоне [1..255],
    // для скорости работать с uint?
    uchar entity;
    float density;
    float2 force;
    float2 speed;
} WData;


/**
* @return Данные о ячейке мира, полученные из пикселей текстуры.
*
* Пиксель текстуры -> WData:
* p1
*     0 uchar -> uchar entity
*     1 uchar -> not use
*     2 uchar -> not use
*     3 uchar -> not use
*
* p2
*     0 float -> float density
*     1 float -> not use
*     2 float -> not use
*     3 float -> not use
*
* p3
*     0 float -> float forceX
*     1 float -> float forceZ
*     2 float -> float speedX
*     3 float -> float speedZ
*/
WData convert(
    uint4 p1, 
    float4 p2, 
    float4 p3
) {
    WData d = {
        //entity
        (uchar)p1.s0,
        //density
        p2.s0,
        //force
        (float2)( p3.s0, p3.s1 ),
        //speed
        (float2)( p3.s2, p3.s3 )
    };

    return d;
}








/**
* Собственно, сам расчёт взаимодействия.
*/
__kernel void interaction(
    // Результат взаимодействия сущностей.
    __write_only image2d_t wOutVisual
    // Входящие данные о мире.
    //__read_only image2d_t wIn1,
    //__read_only image2d_t* wIn2,
    //__read_only image2d_t* wIn3,
    // Входящие данные о конкретных сущностях.
    //__global EData* entity,
    // Размер неделимой ячейки мира, в метрах.
    // Мир - плоский, ячейки считаем квадратными.
    //const float scale,
    // Время в секундах, за которое рассчитываются взаимодействия.
    //const float time
) {
    const uint x = get_global_id(0);
    //const uint z = get_global_id(1);
    //uint2 coord = (uint2)(x, z);

    /* - Зачем?
    // in the case where, due to quantization into grids, we have
    // more threads than pixels, skip the threads which don't 
    // correspond to valid pixels
	if (x >= width || y >= height) return;
    */

    // @see OpenCL specification / 6.11.8.1 Samplers
    /*
    const sampler_t smp =
        CLK_NORMALIZED_COORDS_FALSE |
        CLK_ADDRESS_NONE |
        CLK_FILTER_NEAREST;
    */

    /* - Пример получения/записи значения по координатам.
    int2 coord = (int2)(get_global_id(0), get_global_id(1));
    float4 val = read_imagef(img1, smp, coord);
    val.w = 0;
    write_imagef(img2, coord, val);
    */

    // Получаем ближе размеры мира.
    // Матрицы мира должны быть одинакового размера.
    //const uint N = get_image_width( wIn1 );
    //const uint M = get_image_height( wIn1 );


    // Интерпретируем image2d как матрицу с данными о мире
    //const int2 coord = (int2)( x, z );

    //const uint4 p1 = read_imageui(wIn1, smp, coord);
    //const float4 p2 = read_imagef(wIn2, smp, coord);
    //const float4 p3 = read_imagef(wIn3, smp, coord);
    //const WData w = convert(p1, p2, p3);


    //float4 color = (float4)(1.0, 1.0, 1.0, 1.0);
    //write_imagef(wOutVisual, coord, color);
    //write_imagef(wOutVisual, coord, (float4)(1.0, 1.0, 1.0, 1.0) );


    /*
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


    // Перемещаем сущности и их характеристики
    */


}
