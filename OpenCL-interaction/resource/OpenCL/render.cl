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
    // Карта расположения частиц
    __global WMap* wm,              // @5
    // Что будем рендерить?
    const enum RenderWhat what,     // @6
    // Промежуток времени, на который требуется рассчитать состояние мира.
    // При рендеринге визуального образа параметр игнорируется.
    const float dt,                 // @7
    // Случайное число: может быть использовано в методах
    const uint random,              // @8
    // Вспомогательные блоки.
    __global WMap* wmT              // @9
) {

    // Всегда - координаты неделимой ячейки мира
    const uint x = get_global_id( 0 );
    const uint z = get_global_id( 1 );

    // При размещении мира в сетке, может понадобиться захватить чуть больше
    // места. Проверяем здесь, чтобы не выйти за пределы.
    // @todo optimize Всегда задавать сетку в пределах мира. Убрать условие, оценить разницу.
	if ( (x >= N) || (z >= M) ) {
        return;
    };


    const uint i = x + z * N;

    
    switch ( what ) {
        case IMAGE_WMAP:
            // Перед показом картинки, считаем новые значения для частицы
            // @todo optimize Убрать?
            calcCharacteristicParticleInCell(
                mSolid, mLiquid, mGas, mPlasma, wm, i
            );
            imageWMap(
                imageOut,
                mSolid, mLiquid, mGas, mPlasma,
                // В ячейке мира может находиться несколько частиц
                wm,
                i
            );
            return;
            


        case PREPARE_INTERACTION:
            // Рассчитываем значения частицы на основании других её значений
            // без учёта взаимодействия с другими частицами.
            calcCharacteristicParticleInCell(
                mSolid, mLiquid, mGas, mPlasma, wm, i
            );
            return;


            
        case GO_INTERACTION_I:
            // Работаем с частицами, которые посчитаны в PREPARE_INTERACTION.
            // Результат запоминаем в 'wmT'.
            interactionI(
                wmT,
                wm,
                dt,
                x, z, i
            );
            return;



        case GO_INTERACTION_II:
            interactionII(
                wmT,
                wm,
                dt,
                x, z, i
            );
            return;



        case GO_INTERACTION_III:
            // @todo ...
            return;



        case COMMIT_INTERACTION:
            /* - Нет надобности: работаем прямо в GO_INTERACTION.
            for (size_t n = 0; n < MAX_WMAP_CELL; ++n) {
                wm[i].pe[n] = wmT[i].pe[n];
            }
            */
            break;



        default:
            // Неожиданное значение 'what'
            // @todo ...
            return;

    } // switch ( what )



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



}
