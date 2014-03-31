// (!) Все определения из этого файла должны быть согласованы со struct.h

// @see http://khronos.org/message_boards/viewtopic.php?f=37&t=3098
#pragma pack (push, 16)


/**
* Все сущности состоят из материи.
* Материя может находиться в 4 агрегатных состояниях (по порядку):
*/
enum PState {
    // Вакуум введён для удобства.
    // Совпадает с ID материи, представляющей 'пустоту'.
    VACUUM = 0,
    // Агрегатные состояния материи (вещества)
    SOLID = 1,
    LIQUID = 2,
    GAS = 3,
    PLASMA = 4
};
// Состояние '0' означает, что агр. сост. материи не определено.
// Для каждого состояния декларируются СВОИ характеристики.




/**
* Структура соседних частиц.
* nb - сокр. от Neightbors.
* Используется для расчёта взаимодействия.
* Расположение соседей - см. helper.hcl / near()
* Информация о центральной частице не хранится.
* Точность хранимых значений = 1 / 255 (~ 2 знака).
*/
typedef struct {
    // Информация о 8-ми соседях зашита в два 4-байтных числа
    cl_uint nb14;
    cl_uint nb58;
} NB;





/**
* Частица сущности.
* Значения, отмеченные префиксом '=', рассчитываются на основе других значений.
*/
typedef struct {
    // Материя, из которой состоит частица.
    // Частица может состоять только из одного вида материи.
    // Не декларировано в виде uchar, т.к. при этом не удалось
    // выровнять структуры C++ и OpenCL.
    cl_uint matter;

    // Масса частицы.
    // В зависимости от агрегатного состояния по массе рассчитывается
    // плотность, давление.
    cl_float mass;

    // Скорость
    cl_float velocityX;
    cl_float velocityZ;

    // Температура
    cl_float temperature;


    // = Агрег. состояние
    cl_uint state;

    // = Ускорение
    cl_float accelerationX;
    cl_float accelerationZ;

    // = Плотность
    cl_float density;

    // = Давление
    cl_float pressure;

    // = Скорость расширения материи в газовом состоянии
    cl_float expansion;


    // Структура соседних частиц для расчёта их взаимодействия
    NB nb;


    // Значения используются исключитеьно для отладки
    // @test
    cl_float t1;
    cl_float t2;

} ParticleEntity;





/**
* Ячейка карты мира.
* Карта мира представляет собой матрицу N x M, в которую
* помещены частицы ParticleEntity.
* Одна ячейка может содержать данные (вмещать) максимум о
* MAX_WMAP_CELL частицах. Если частиц в ячейке несколько, значит
* они обязательно должны представлять РАЗНУЮ материю. Исключением
* является особая частица материи - MATTER_VACUUM.
*/
const size_t MAX_WMAP_CELL = 5;
typedef struct {
    // Частицы, разделяющие одну ячейку
    ParticleEntity pe[ MAX_WMAP_CELL ];
} WMap;





/**
* Идентификаторы материй.
*/
enum UIDMatter {
    MATTER_VACUUM = 0,
    MATTER_WATER = LIQUID * 10 + 1
};




/**
* Характеристика ячейки материи для разных агрегатных состояний.
*/
typedef struct {
    // ID материи
    // cl_uchar id; - Храним в общем списке.

    // Цвет материи
    cl_uint color;

    // Агрегатное состояние
    // cl_uchar state; - Храним в общем списке.

    // Характеристики согласно этому агр. состоянию.
    // Плотность
    cl_float density;

    // Условия нахождения в этом агрегатном состоянии.
    // transition temperature
    // Температура перехода всегда упорядочена по возрастанию состояний.
    //   S -> L -> G -> P
    // Состояние PLASMA не имеет температуры перехода.
    cl_float tt;

} MatterSolid;



typedef struct {
    cl_uint color;
    cl_float density;
    cl_float tt;

} MatterLiquid;


typedef struct {
    cl_uint color;
    cl_float density;
    cl_float tt;

    // Молярная масса
    cl_float molarMass;

} MatterGas;



typedef struct {
    cl_uint color;
    cl_float density;

    cl_float molarMass;

} MatterPlasma;




/**
* Что будем рендерить?
*/
enum RenderWhat {
    // Сформировать визуальный образ
    // Образ - карта частиц
    IMAGE_WMAP = 1,
    // Шаги, переводящие мир в другое состояние q(time)
    PREPARE_INTERACTION,
    GO_INTERACTION_I,
    GO_INTERACTION_II,
    GO_INTERACTION_III,
    COMMIT_INTERACTION
};
