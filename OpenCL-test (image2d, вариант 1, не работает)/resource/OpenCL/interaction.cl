/**
* �������������� ���������. ��. interaction() � ����� ����.
* ���������� ����� image2d.
* @see http://www.cmsoft.com.br/index.php?option=com_content&view=category&layout=blog&id=115&Itemid=172
*/

#define TEST



/**
* ���������� � ���������� ��������� ����.
*/
typedef struct {
    uchar id;
    // ��������� ������������ �������� � ���� ����� ����������
    float density;
} EData;


/**
* ��������� ��� ������� �������������� ���������.
* ������� ��� ����� ���������� ������� � ���������, �������� � ���� �������.
* ��������� ��� �������� imge2d_t, � �������� ������� ���������� ����������
* � ������� ����.
*
* @see convert()
*/
typedef struct {
    // @todo optimize �������� �� ��, ��� ID �������� ����� � ��������� [1..255],
    // ��� �������� �������� � uint?
    uchar entity;
    float density;
    float2 force;
    float2 speed;
} WData;


/**
* @return ������ � ������ ����, ���������� �� �������� ��������.
*
* ������� �������� -> WData:
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
* ����������, ��� ������ ��������������.
*/
__kernel void interaction(
    // ��������� �������������� ���������.
    __write_only image2d_t wOutVisual
    // �������� ������ � ����.
    //__read_only image2d_t wIn1,
    //__read_only image2d_t* wIn2,
    //__read_only image2d_t* wIn3,
    // �������� ������ � ���������� ���������.
    //__global EData* entity,
    // ������ ��������� ������ ����, � ������.
    // ��� - �������, ������ ������� �����������.
    //const float scale,
    // ����� � ��������, �� ������� �������������� ��������������.
    //const float time
) {
    const uint x = get_global_id(0);
    //const uint z = get_global_id(1);
    //uint2 coord = (uint2)(x, z);

    /* - �����?
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

    /* - ������ ���������/������ �������� �� �����������.
    int2 coord = (int2)(get_global_id(0), get_global_id(1));
    float4 val = read_imagef(img1, smp, coord);
    val.w = 0;
    write_imagef(img2, coord, val);
    */

    // �������� ����� ������� ����.
    // ������� ���� ������ ���� ����������� �������.
    //const uint N = get_image_width( wIn1 );
    //const uint M = get_image_height( wIn1 );


    // �������������� image2d ��� ������� � ������� � ����
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
        // �������� � ������� ������ ������� �����������
        return;
    }
    
    // ������� ����� �������� ��������
    float2 thisForce = (float2)( thisWIn.forceX, thisWIn.forceZ );
    const float2 a = thisForce / mass;
    float2 thisSpeed =
        (float2)( thisWIn.speedX, thisWIn.speedZ ) + time * a;


    // ���������� ���������� ������ �������
    //barrier( CLK_GLOBAL_MEM_FENCE );

    
    // ������������ ������������ � ����, �������������
    // ����������� ���� ��������.

    // ������������� ��������, � �������� ����� ���� ����������� ���� ��������.
    // ������� �� ���� ���, � ������ ��������, ������������� �� ���� ���� ��������.
    // ��� ����� �������� �� ������� �������� ��������, �������������� ��������
    // ����������� �������� dt, ����� �� ���������� � ����� ������, �������
    // ���������� �������� �� ����� 'time'.
    //const uint N = size.s0;
    //const uint M = size.s1;

    //const float dtX = speedNew[i].s0


    // ���������� �������� � �� ��������������
    */


}
