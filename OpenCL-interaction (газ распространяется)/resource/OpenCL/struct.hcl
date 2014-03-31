// @see Примечания в struct.h

enum PState {
    VACUUM = 0,
    SOLID = 1,
    LIQUID = 2,
    GAS = 3,
    PLASMA = 4
};


/**
* Структура ячейки материи.
*/
/* - Используется только в программе.
typedef struct {
    uint color;
    float tt;
    float density;
} Matter;
*/


typedef struct {
    // = R
    uchar matter;
    uchar undefined1;
    uchar undefined2;
    uchar undefined3;
    // = G
    uchar undefined4;
    uchar undefined5;
    uchar undefined6;
    uchar undefined7;
    // = B
    uchar undefined8;
    uchar undefined9;
    uchar undefined10;
    uchar undefined11;
    // = A
    uchar undefined12;
    uchar undefined13;
    uchar undefined14;
    uchar undefined15;
} DataS1U;


typedef struct {
    // = R
    uint color;
    // = G
    uint undefined1;
    // = B
    uint undefined2;
    // = A
    uint undefined3;
} DataS4U;


typedef struct {
    // = R
    float density;
    // = G
    float temperature;
    // = B
    float pressure;
    // = A
    float undefined3;
} DataS4F;


typedef struct {
    // = R, G
    float2 force;
    // = B, A
    float2 speed;
} DataW8F;



typedef struct {
    // = R
    uint s0;
    // = G
    uint s1;
    // = B
    uint s2;
    // = A
    uint s3;
} DataTempSU;


typedef struct {
    // = R
    float s0;
    // = G
    float s1;
    // = B
    float s2;
    // = A
    float s3;
} DataTempSF;


typedef struct {
    // = R, G
    float2 s0;
    // = B, A
    float2 s1;
} DataTempWU;

typedef struct {
    // = R, G
    float2 s0;
    // = B, A
    float2 s1;
} DataTempWF;
