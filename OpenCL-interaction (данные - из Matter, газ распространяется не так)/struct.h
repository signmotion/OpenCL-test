/**
* Структура ячейки материи.
* Вспомогательная сущность.
*/
typedef struct {
    // ID материи
    // cl_uchar id; - Храним в общем списке.
        
    // Молярная масса
    cl_float molarMass;

    // Цвет материи
    cl_uint color;

    // Агрегатное состояние
    // cl_uchar state; - Храним в общем списке.

    // Условия нахождения в этом агрегатном состоянии.
    // transition temperature
    // Температура перехода задаётся границами между состояниями [1; 3]:
    // S | L | G | P
    // Плазма (состояни 4) не имеет 'tt'.
    cl_float tt;

    // Характеристики согласно этому агр. состоянию.
    cl_float density;

} Matter;



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
* Структуры хранения информации о мире.
* Описание мира хранятся в виде 2D-матриц (по размерности мира).
* Ячейка ij матрицы содержит информацию о сущности, находящейся в этой
* ячейке мира. В целях оптимизации матрицы сгруппированы по размеру
* хранимых в них данных и выровнены по 'RGBA float'.
*
* Обозначения в записи данных:
*   - цифра = количество байт
*   - буква
*     = S = скалярное значение
*     = W = двумерный вектор
*     = U = беззнаковый тип
*     = F = тип одинарной точности
*/
typedef struct {
    // = R
    cl_uchar matter;
    cl_uchar undefined1;
    cl_uchar undefined2;
    cl_uchar undefined3;
    // = G
    cl_uchar undefined4;
    cl_uchar undefined5;
    cl_uchar undefined6;
    cl_uchar undefined7;
    // = B
    cl_uchar undefined8;
    cl_uchar undefined9;
    cl_uchar undefined10;
    cl_uchar undefined11;
    // = A
    cl_uchar undefined12;
    cl_uchar undefined13;
    cl_uchar undefined14;
    cl_uchar undefined15;
} DataS1U;


typedef struct {
    // = R
    cl_uint undefined0;
    // = G
    cl_uint undefined1;
    // = B
    cl_uint undefined2;
    // = A
    cl_uint undefined3;
} DataS4U;


typedef struct {
    // = R
    cl_float density;
    // = G
    cl_float temperature;
    // = B
    cl_float pressure;
    // = A
    cl_float undefined3;
} DataS4F;


typedef struct {
    // = R, G
    cl_float2 force;
    // = B, A
    cl_float2 speed;
} DataW8F;



/**
* Для временно хранения данных различными методами.
*/
typedef struct {
    // = R
    cl_uint s0;
    // = G
    cl_uint s1;
    // = B
    cl_uint s2;
    // = A
    cl_uint s3;
} DataTempSU;


typedef struct {
    // = R
    cl_float s0;
    // = G
    cl_float s1;
    // = B
    cl_float s2;
    // = A
    cl_float s3;
} DataTempSF;


typedef struct {
    // = R, G
    cl_float2 s0;
    // = B, A
    cl_float2 s1;
} DataTempWU;

typedef struct {
    // = R, G
    cl_float2 s0;
    // = B, A
    cl_float2 s1;
} DataTempWF;
