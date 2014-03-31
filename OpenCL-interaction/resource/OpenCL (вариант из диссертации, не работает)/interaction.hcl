/**
* �������������� ������.
* ������� 'peOut' � ������������� ������ �������� ����� ���������.
* ���������� �������������� ����� PREPARE_INTERACTION ������.
*/
inline void interaction(
    __global ParticleEntity* peOut,
    __global WMap* wm,
    const float dt,
    // ������ ��������� ������ ���������� ������, � �� uid �������
    const uint xCell,
    const uint zCell,
    // = xCell + zCell * N
    const uint coordCell
) {
    // �������, ��������� ������� ����� ��������.
    // �������� ��������, ����� ��������� ��������� ������ ��� �������
    // ������� ���� �� �����.
    ParticleEntity peOutT[ MAX_WMAP_CELL ];
    // ���� UID ������� ������ ��������.
    int uidOrSignT[ MAX_WMAP_CELL ];

    // ������, ������� � ������� ���� ���������
    const WMap cell = wm[coordCell];
    
    // ��������� �������� � ������
    float densityCell = 0.0f;
    
    // ���� �� ����������� �������� � ������.
    for (size_t n = 0; n < MAX_WMAP_CELL; n++) {
        const int uidOrSign = cell.uidOrSign[n];
        uidOrSignT[n] = uidOrSign;
        // ��� ������� ������������ � ������ ��������������� => ������� '-1'
        // ��������, ��� ������ ������ � ������ ���
        if (uidOrSign == -1) {
            break;
        }
        
        const uint uid = uidOrSign;
        
        // ������������� �������: ������ � ��� ����� ���������.
        // �������� ��������� � 'peOutT' ���� (��� ��������� ����).
        ParticleEntity p = peOut[uid];
        
        // ���������� ���������� ��� ��������� �������� ����.
        const float2 coord = (float2)( p.coordX, p.coordZ );
        const float2 acceleration = (float2)( p.accelerationX, p.accelerationZ );
        const float2 velocity = (float2)( p.velocityX, p.velocityZ );
        const float pressureRel = p.pressure - PRESSURE_0;
        const float density2 = p.density * p.density;


        // ���������� ��� ���������� ����������.
        // a - ����. �� Accumulation
        float aDensity = 0.0;
        float aPressure = 0.0;
        float2 aVelocity = (float2)( 0.0f, 0.0f );
        float2 aCoord = (float2)( 0.0f, 0.0f );
        float aViscosity = 0.0f;

        // ������� �������
        size_t neighbor = 0;


        // ������� ������� �������.
        // ������������� ����������� ������ (������ ����, � ��� ����� ����
        // ������ �������) � ������ �����.
        for (size_t q = 0; q <= 8; q++) {
            const int coordCellNear = nearCell(xCell, zCell, q);
            // ������ ����� ��� �������������� ���� (�������), ����������
            if (coordCellNear == -1) {
                continue;
            }

            const WMap cellNear = wm[coordCellNear];

            // ������������� ��� ������� � ���� ������.
            // ������� ������, � ������� ����� �����������������.
            for (size_t k = 0; k < MAX_WMAP_CELL; k++) {
                // ������ ����������� ������ �������� �����. �����������
                // ����������� �������.
                
                const int uidOrSign = cellNear.uidOrSign[k];
                if (uidOrSign == -1) {
                    break;
                }

                const uint uidNear = uidOrSign;

                // ���������, ��� ����� �� �������� ������������� ��������
                if (uidNear == uid) {
                    continue;
                }

                // �����
                const ParticleEntity pNear = peOut[uidNear];
                neighbor++;
                
                // ������� ������ �� �������.
                // ��. ���������� ���������� ����.
                
                // ���������� ���������� ��� ��������� �������� ����.
                const float2 coordNear = (float2)( pNear.coordX, pNear.coordZ );
                // @todo optimize ��������� �� ����������� 'w'.
                const float2 velocityNear = (float2)( pNear.velocityX, pNear.velocityZ );
                const float pressureRelNear = pNear.pressure - PRESSURE_0;
                
                // ���������� ����� ��������� � ������������ ����.
                // @todo optimize distance(x, y) = sqrt( dot(x, y) )
                //       ���. ������ � ���������� ���� � ���� �����.
                const float r = distance( coord, coordNear ) +
                    // ��������� �� ������� �� '0'.
                    1e-4;
                const float r2 = r * r;
                const float2 rij = coord - coordNear;
                // @todo optimize ������ � ����, ����������� �� ������� �������
                const float w = sphW( r, SPH_H );
                if ( zero( w ) ) {
                    // ����� ������� ������, ����� ������ �� �������
                    break;
                }
                
                // ������� �������
                
                // �������� ������������� ���� ������������ ����� ��������������
                // ����������� ������������� ����
                // @see SPH / ���. 34 (1.9)
                const float2 gradW = rij / r;

                // ������������� ��������
                const float2 vij = velocity - velocityNear;

                // ���������
                /* - ��������� ������� ����� ��������� �������������. ��. ����.
                // @see SPH / ���. 40 (2.5)
                aDensity += pNear.mass * w;
                */
                
                // ��������� ����� ��������� �������������
                // @see SPH / ���. 42 (2.13)
                aDensity += pNear.mass * dot( vij, gradW );

                // ��������
                // @see SPH / ���. 40 (2.6)
                const float density2Near = pNear.density * pNear.density;
                aPressure += p.density * pNear.mass * (
                    pressureRel / density2 + pressureRelNear / density2Near
                ) * w;
                // ��� @see SPH / ���. 39 (2.2)
                //aPressure += pNear.mass / pNear.density * pressureRelNear * w;
                
                // ��������
                // @see SPH / ���. 40 (2.7) ��� SPH / ���. 55 (2.23)
                // @todo �������� ��������.
                // @todo optimize ������������ ��������� �� ��������.
                aVelocity += - pNear.mass * (
                    p.pressure / density2 + pNear.pressure / density2Near
                ) * gradW;
                
                // ������ ����
                /* - ������ ���������� �������. ��. ����.
                // @see SPH / ���. 48 (2.20)
                const float vrC = dot( vij, rij );
                const float dijC = p.density * pNear.density;
                const float vdijC = (p.viscosity + pNear.viscosity) / dij;
                // @see SPH / ���. 45 (2.18)
                const float vijC = SPH_H * vr / ( dot( rij, rij ) + 0.01 * SPH_H * SPH_H );
                const float halfDensity = (p.density + pNear.density) / 2;
                // @todo ? ��������� �������� �������?
                aViscosity += pNear.mass * (
                    isless( vrC, 0 )
                      ? (
                            dot( vdijC * coord / r2, vij ) +
                            SPH_BETA * vijC * vijC / halfDensity * w
                        )
                      : (
                            dot ( vdijC, vij)
                        )
                );
                */
                const float vrC = dot( vij, rij );
                aViscosity += pNear.mass * w * (
                    isless( vrC, 0 )
                      ? (
                            - SPH_ALPHA * 
                            SPH_BETA * vijC * vijC / halfDensity * w
                        )
                      : (
                            dot ( vdijC, vij)
                        )
                );
                
                // �����������
                // ���. ������������ ��������
                // @see SPH / ���. 55 (2.26)
                const float2 vji = - vij;
                aCoord += velocity + SPH_EPSILON * ( pNear.mass / halfDensity * vji * w );
                

                densityCell += pNear.mass * w;
                
            } // for (size_t k = 0; k < MAX_WMAP_CELL; k++)

        } // for (size_t q = 0; q <= 8; q++)
        
        
        // ��������� �������.
        // ��������� � ����, ����������� �� �������.
        // ��������� ��������� � calcCharacteristicParticle().
        p.density += aDensity * dt;
        p.pressure += aPressure * dt;

        p.velocityX += aVelocity.x * dt + p.accelerationX * dt;
        p.velocityZ += aVelocity.y * dt + p.accelerationZ * dt;
        
        p.coordX += aCoord.x * dt + p.accelerationX * dt * dt / 2;
        p.coordZ += aCoord.y * dt + p.accelerationZ * dt * dt / 2;

        // @test
        //p.coordX += -40.0f;
        //p.coordZ += -20.0f;

        
        // ������� �� ������� ���� ���������.
        // ������ ������������... � ����������.
        peOutT[n] = cropCoordToBorder( p );
        
    } // for (size_t n = 0; n < MAX_WMAP_CELL; n++)
    
    
    
    // ��������� ���������� � ��������, ������� ���������� � ������.
    // ����� �� ��������� ����� ������������ �������.
    for (size_t n = 0; n < MAX_WMAP_CELL; n++) {
        if (uidOrSignT[n] == -1) {
            break;
        }
        
        const ParticleEntity p = peOutT[n];
        const uint uid = uidOrSignT[n];
        peOut[uid] = p;
    }
    
}
