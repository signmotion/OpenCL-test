// (!) ��� ����������� �� ����� ����� ������ ���� ����������� �� struct.h

// @see http://khronos.org/message_boards/viewtopic.php?f=37&t=3098
#pragma pack (push, 16)


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
* ��������� �������� ������.
* nb - ����. �� Neightbors.
* ������������ ��� ������� ��������������.
* ������������ ������� - ��. helper.hcl / near()
* ���������� � ����������� ������� �� ��������.
* �������� �������� �������� = 1 / 255 (~ 2 �����).
*/
typedef struct {
    // ���������� � 8-�� ������� ������ � ��� 4-������� �����
    cl_uint nb14;
    cl_uint nb58;
} NB;





/**
* ������� ��������.
* ��������, ���������� ��������� '=', �������������� �� ������ ������ ��������.
*/
typedef struct {
    // �������, �� ������� ������� �������.
    // ������� ����� �������� ������ �� ������ ���� �������.
    // �� ������������� � ���� uchar, �.�. ��� ���� �� �������
    // ��������� ��������� C++ � OpenCL.
    cl_uint matter;

    // ����� �������.
    // � ����������� �� ����������� ��������� �� ����� ��������������
    // ���������, ��������.
    cl_float mass;

    // ��������
    cl_float velocityX;
    cl_float velocityZ;

    // �����������
    cl_float temperature;


    // = �����. ���������
    cl_uint state;

    // = ���������
    cl_float accelerationX;
    cl_float accelerationZ;

    // = ���������
    cl_float density;

    // = ��������
    cl_float pressure;

    // = �������� ���������� ������� � ������� ���������
    cl_float expansion;


    // ��������� �������� ������ ��� ������� �� ��������������
    NB nb;


    // �������� ������������ ������������ ��� �������
    // @test
    cl_float t1;
    cl_float t2;

} ParticleEntity;





/**
* ������ ����� ����.
* ����� ���� ������������ ����� ������� N x M, � �������
* �������� ������� ParticleEntity.
* ���� ������ ����� ��������� ������ (�������) �������� �
* MAX_WMAP_CELL ��������. ���� ������ � ������ ���������, ������
* ��� ����������� ������ ������������ ������ �������. �����������
* �������� ������ ������� ������� - MATTER_VACUUM.
*/
const size_t MAX_WMAP_CELL = 5;
typedef struct {
    // �������, ����������� ���� ������
    ParticleEntity pe[ MAX_WMAP_CELL ];
} WMap;





/**
* �������������� �������.
*/
enum UIDMatter {
    MATTER_VACUUM = 0,
    MATTER_WATER = LIQUID * 10 + 1
};




/**
* �������������� ������ ������� ��� ������ ���������� ���������.
*/
typedef struct {
    // ID �������
    // cl_uchar id; - ������ � ����� ������.

    // ���� �������
    cl_uint color;

    // ���������� ���������
    // cl_uchar state; - ������ � ����� ������.

    // �������������� �������� ����� ���. ���������.
    // ���������
    cl_float density;

    // ������� ���������� � ���� ���������� ���������.
    // transition temperature
    // ����������� �������� ������ ����������� �� ����������� ���������.
    //   S -> L -> G -> P
    // ��������� PLASMA �� ����� ����������� ��������.
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

    // �������� �����
    cl_float molarMass;

} MatterGas;



typedef struct {
    cl_uint color;
    cl_float density;

    cl_float molarMass;

} MatterPlasma;




/**
* ��� ����� ���������?
*/
enum RenderWhat {
    // ������������ ���������� �����
    // ����� - ����� ������
    IMAGE_WMAP = 1,
    // ����, ����������� ��� � ������ ��������� q(time)
    PREPARE_INTERACTION,
    GO_INTERACTION_I,
    GO_INTERACTION_II,
    GO_INTERACTION_III,
    COMMIT_INTERACTION
};
