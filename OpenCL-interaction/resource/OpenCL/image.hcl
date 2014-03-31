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
    __global WMap* wm,
    const uint i
) {
    // Ячейка может не содержать частиц.
    // Работаем только с первой частицей.
    const ParticleEntity p = wm[i].pe[0];

    const uint color = colorMatter(
        mSolid, mLiquid, mGas, mPlasma, p
    );

    // Прозрачность в зависимости от плотности
    //const float maxDensity = 1500.0f;
    //const float alpha = select( p.density / maxDensity, 1.0f, isgreater( p.density, maxDensity ) );

    const float alpha = 1.0f;

    // Цвет переводим в RGBA
    // @source http://rclermont.blogspot.com/2010/02/convert-long-into-byte-array-in-cc.html
    const uchar cr = (int)((color >> 24) & 0xFF);
    const uchar cg = (int)((color >> 16) & 0xFF);
    const uchar cb = (int)((color >> 8) & 0xFF);
    const uchar ca = (int)((color) & 0xFF);

    // Нормализируем каналы
    const float colorDiff = alpha;
    const float r = (float)cr / 255.0f * colorDiff;
    const float g = (float)cg / 255.0f * colorDiff;
    const float b = (float)cb / 255.0f * colorDiff;
    const float a = (float)ca / 255.0f;

    imageOut[i] = (float4)( r, g, b, a );

    // @test
    wm[i].pe[0].pressure = r;

}

