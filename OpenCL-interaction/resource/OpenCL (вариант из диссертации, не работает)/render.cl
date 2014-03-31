#include "pragma.hcl"
#include "default.hcl"
#include "const.hcl"
#include "sph.hcl"
#include "struct.hcl"
#include "helper.hcl"
#include "image.hcl"
#include "interaction.hcl"



/**
* Основной метод, вычисляющий взаимодействия сущностей и формирующий
* визуальный образ мира.
* Размер мира = N x M (см. pragma.hcl).
* Результат работы метода зависит от параметра 'what'. Причём, распараллеливание
* может вестись двумя путями:
*   1. По UID частицы. Мир не может модержать более N x M частиц. Каждая
*      частица при инициализации мира получает UID, который может быть вычислен по
*      формуле get_global_id(0) + get_global_id(1) * N
*   2. По ячейкам мира WMap. В ячейку вмещается не более MAX_WMAP_CELL частиц.
*      Формируется с помощью CPU.
* Частица может находиться одновременно только в одной ячейке.
*
* Последовательность путей 'what' в коде не соотв. действительному порядку их
* выполнения (см. interaction.cpp): задан таким для оптимизации.
*/
__kernel void render(
    // Визуальный образ мира.
    // 1 пиксель = 4 канала: RGBA, float
    __global float4* imageOut,      // @0
    // Данные о материи.
    // Количество материй мира всегда 256.
    __global MatterSolid* mSolid,   // @1
    __global MatterLiquid* mLiquid, // @2
    __global MatterGas* mGas,       // @3
    __global MatterPlasma* mPlasma, // @4
    // Описание мира. Сущности мира разбиты на частицы.
    __global ParticleEntity* pe,    // @5
    // Карта расположения частиц
    __global WMap* wm,              // @6
    // Что будем рендерить?
    const enum RenderWhat what,     // @7
    // Промежуток времени, на который требуется рассчитать состояние мира.
    // При рендеринге визуального образа параметр игнорируется.
    const float dt,                 // @8
    // Случайное число: может быть использовано в методах
    const uint random,              // @9
    // Вспомогательные блоки.
    // Описание мира после прохождения времени 'time'.
    __global ParticleEntity* peOut  // @10
) {
    // Здесь ещё не знаем, как интерпретировать 'a' и 'b'
    const uint a = get_global_id( 0 );
    const uint b = get_global_id( 1 );

    // При размещении мира в сетке, может понадобиться захватить чуть больше
    // места. Проверяем здесь, чтобы не выйти за пределы.
    // Верно и для частиц: UID не может превышать пределы мира (см. описание render() ).
    // @todo optimize Всегда задавать сетку в пределах мира. Убрать условие, оценить разницу.
	if ( (a >= N) || (b >= M) ) {
        return;
    };


    // Помним: адрес частицы также является её уникальным идентификатором.
    // Две константы - исключительно для удобства чтения кода ниже.
    const uint uid = a + b * N;
    const uint coordCell = uid;  // = a + b * N;
    
    
    switch ( what ) {
        case IMAGE_WMAP:
            imageWMap(
                imageOut,
                mSolid, mLiquid, mGas, mPlasma,
                pe,
                wm[coordCell],
                coordCell
            );
            return;


        case CLEAR_MATRIX:
            // Очищаем ячейку образа
            //imageOut[coordCell] = (float4)( 0.0f, 0.0f, 0.0f, 0.0f );

            return;
            
            
        case COMMIT_INTERACTION:
            pe[uid] = peOut[uid];
            break;
    }
    
    
            
    // Выделяем частицу
    const ParticleEntity p = pe[uid];

    // Не трогаем частицы, которые вышли за границы мира
    if ( outsideMatrix( p.coordX, p.coordZ ) ) {
        return;
    };

    
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



    // В switch нельзя декларировать переменную - декларируем здесь.
    //ParticleEntity pOut;

    switch ( what ) {
        case PREPARE_INTERACTION:
            // Подготовка данных также происходит внутри HOST/interaction()/fillWMap()

            // Рассчитываем значения частицы на основании других её значений
            // без учёта взаимодействия с другими частицами.
            peOut[uid] = calcCharacteristicParticle(
                mSolid, mLiquid, mGas, mPlasma, p
            );
            return;


        case GO_INTERACTION:
            if ( !zero( dt ) ) {
                // Передаём частицы, посчитанные в PREPARE_INTERACTION
                interaction(
                    peOut,
                    wm,
                    dt,
                    a, b, coordCell
                );
            }
            return;


        default:
            // Неожиданное значение 'what'
            // @todo ...
            return;
    }

    //barrier( CLK_GLOBAL_MEM_FENCE );

}
