/**
* Формирует визуальный образ мира.
* Материя.
*
* @see render()
*/
#if 0
// - Достаточно imageWMap()
inline void imageMatter(
    __global float4* imageOut,
    __global MatterSolid* mSolid,
    __global MatterLiquid* mLiquid,
    __global MatterGas* mGas,
    __global MatterPlasma* mPlasma,
    __global ParticleEntity* pe,
    // UID частицы
    const uint uid,
    // == p[uid]
    const ParticleEntity p,
    // == calcState()
    const enum PState state
) {

    // Не трогаем частицы, которые вышли за границы мира
    /* - Сделано в __kernel.
    if (
          (p.coordX < 0) || (p.coordX >= N)
       || (p.coordZ < 0) || (p.coordZ >= M)
    ) {
        return;
    };
    */

    // Каждая частица имеет свои координаты.
    // В общем случае координаты не совпадают с UID частицы.
    const int iCoord = convert_int( p.coordX ) + convert_int( p.coordZ ) * N;

    const uint color = colorMatter(
        mSolid, mLiquid, mGas, mPlasma,
        p.matter, state
    );
    
    //const float density = pe[i].mass / V_PARTICLE;

    // Цвет переводим в RGBA
    // @source http://rclermont.blogspot.com/2010/02/convert-long-into-byte-array-in-cc.html
    const uchar cr = (int)((color >> 24) & 0xFF);
    const uchar cg = (int)((color >> 16) & 0xFF);
    const uchar cb = (int)((color >> 8) & 0xFF);

#if 1
    // Нормализируем каналы
    float colorDiff = 1.0f;
    const float r = (float)cr / 255.0f * colorDiff;
    const float g = (float)cg / 255.0f * colorDiff;
    const float b = (float)cb / 255.0f * colorDiff;
    const float a = 1.0f;  //colorDiff;  // * 1.0f;

#else
    // Цвет "воздуха"
    const float r = (p.matter == 0) ? 0.0f : 0x7f / 255.0f ;
    const float g = (p.matter == 0) ? 0.0f : 0xc7 / 255.0f;
    const float b = (p.matter == 0) ? 0.0f : 0xff / 255.0f;
    const float a = (p.matter == 0) ? 0.0f : 0x80 / 255.0f;
#endif

    imageOut[iCoord] = (float4)( r, g, b, a );

}
#endif





/**
* Формирует визуальный образ мира.
* Карта мира.
* Если в ParticleEntity[] зашит UID частицы, то в WMap[]
* зашиты координаты частицы.
*
* @see imageMatter()
*/
inline void imageWMap(
    __global float4* imageOut,
    __global MatterSolid* mSolid,
    __global MatterLiquid* mLiquid,
    __global MatterGas* mGas,
    __global MatterPlasma* mPlasma,
    __global ParticleEntity* pe,
    const WMap wm,
    const uint coordCell
) {
    // Отмечаем установленную частицу цветом.
    // Ячейка может не содержать частиц.
    // Проверяем только по первой частице.
    
    const int uidOrSign = wm.uidOrSign[0];

    if (uidOrSign != -1) {
        imageOut[coordCell] = (float4)( 1.0f, 1.0f, 1.0f, 1.0f );
    } else {
        imageOut[coordCell] = (float4)( 1.0f, 0.0f, 0.0f, 1.0f );
    }

    const int uidOrSign1 = wm.uidOrSign[1];
    if (uidOrSign1 != -1) {
        imageOut[coordCell] = (float4)( 0.0f, 0.0f, 1.0f, 1.0f );
    }

    return;
    
    const uint uid = uidOrSign;
    
    const ParticleEntity p = pe[uid];
    const uint color = colorMatter(
        mSolid, mLiquid, mGas, mPlasma,
        p.matter, p.state
    );
    
    // Цвет переводим в RGBA
    // @source http://rclermont.blogspot.com/2010/02/convert-long-into-byte-array-in-cc.html
    const uchar cr = (int)((color >> 24) & 0xFF);
    const uchar cg = (int)((color >> 16) & 0xFF);
    const uchar cb = (int)((color >> 8) & 0xFF);
    const uchar ca = (int)((color) & 0xFF);

    // Нормализируем каналы
    float colorDiff = 1.0f;
    const float r = (float)cr / 255.0f * colorDiff;
    const float g = (float)cg / 255.0f * colorDiff;
    const float b = (float)cb / 255.0f * colorDiff;
    const float a = (float)ca / 255.0f * colorDiff;

    imageOut[coordCell] = (float4)( r, g, b, a );

}

