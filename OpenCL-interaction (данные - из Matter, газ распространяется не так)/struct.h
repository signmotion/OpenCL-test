/**
* ��������� ������ �������.
* ��������������� ��������.
*/
typedef struct {
    // ID �������
    // cl_uchar id; - ������ � ����� ������.
        
    // �������� �����
    cl_float molarMass;

    // ���� �������
    cl_uint color;

    // ���������� ���������
    // cl_uchar state; - ������ � ����� ������.

    // ������� ���������� � ���� ���������� ���������.
    // transition temperature
    // ����������� �������� ������� ��������� ����� ����������� [1; 3]:
    // S | L | G | P
    // ������ (�������� 4) �� ����� 'tt'.
    cl_float tt;

    // �������������� �������� ����� ���. ���������.
    cl_float density;

} Matter;



/**
* ��� �������� ������� �� �������.
* ������� ����� ���������� � 4 ���������� ���������� (�� �������):
*/
enum PState {
    // ������ ����� ��� ��������.
    // ��������� � ID �������, �������������� '�������'.
    VACUUM = 0,
    // ���������� ��������� ������� (��������)
    SOLID = 1,
    LIQUID = 2,
    GAS = 3,
    PLASMA = 4
};
// ��������� '0' ��������, ��� ���. ����. ������� �� ����������.
// ��� ������� ��������� ������������� ���� ��������������.




/**
* ��������� �������� ���������� � ����.
* �������� ���� �������� � ���� 2D-������ (�� ����������� ����).
* ������ ij ������� �������� ���������� � ��������, ����������� � ����
* ������ ����. � ����� ����������� ������� ������������� �� �������
* �������� � ��� ������ � ��������� �� 'RGBA float'.
*
* ����������� � ������ ������:
*   - ����� = ���������� ����
*   - �����
*     = S = ��������� ��������
*     = W = ��������� ������
*     = U = ����������� ���
*     = F = ��� ��������� ��������
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
* ��� �������� �������� ������ ���������� ��������.
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
