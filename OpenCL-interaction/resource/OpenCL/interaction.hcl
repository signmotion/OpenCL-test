/**
* �������������� ������.
* ���������� �������������� ����� PREPARE_INTERACTION ������.
*/



/**
* ������� "������� ���� ������� �������".
* wm -> wmT
*/
inline void interactionI(
    __global WMap* wmT,
    __global WMap* wm,
    const float dt,
    const uint x,
    const uint z,
    // = x + z * N
    const uint i
) {
    /* @test
    wmT[i].pe[0] = wm[i].pe[0];
    return;
    */
    
    // �������������� ������ ������� �������
    // @todo optimize ����� ������.
    if ( zero( dt ) ) {
        return;
    }


    // ������ ����� ��������� �� MAX_WMAP_CELL ������ �� ������ �������.
    // ��������������� � ������ �� ���.
    /* - ���. ���� �������� � ����� �������.
    for (size_t n = 0; n < MAX_WMAP_CELL; ++n) {
    */
    const size_t n = 0;

    ParticleEntity p = wm[i].pe[n];

    // ������ ����� ��� �� ����������
    if ( vacuum( p ) ) {
        wmT[i].pe[n] = p;
        return;
    }
    
    
    // ��������� ��� ��������� ���������
    // �������� ��������� �� �������� �������� ����� ������� (������
    // ���������� ������ �������� �����)
    const float2 velocity = (float2)(p.velocityX, p.velocityZ);
    
    
    // �������� � ����������� ��������� ����� ���� �������
    float2 massDt = (float2)0.0f;
    
    
    // ������� ������� �������
    for (size_t k = 1; k <= 8; ++k) {
        const int iNear = nearCell( x, z, k );
        // ������� ����� ���� �� ������� ����
        if (iNear == -1) {
            continue;
        }

        const ParticleEntity pNear = wm[iNear].pe[n];
        if ( vacuum( pNear ) ) {
            // ������ �� ������
            continue;
        }

        const float2 velocityNear = (float2)(pNear.velocityX, pNear.velocityZ);
        const float2 vij = velocity - velocityNear;
        massDt += pNear.mass * vij;

    } // for (size_t k = 1; k <= 8; ++k)


    // �������������� �������� ��������� ����� �������
    // ���� ���������� �������
    const float2 force = (
        (float2)(DEFAULT_FORCE_X, DEFAULT_FORCE_Z)
    );
    /* - ��� ��������� � calcCharacteristicParticle().
    const float2 acceleration = 
        zero( p.mass ) ? 0.0f : ( force / p.mass );
    */
    const float2 acceleration = (float2)(p.accelerationX, p.accelerationZ);
    const float2 vForce = velocity + acceleration * dt;
    massDt += vForce;
    
    // ������ ������� ��������
    //transferGas( &p );
    transferSolid( &p, massDt );
    
    // ��������� - ��. interactionII()

    /* @test
    const size_t k = 1;
    setNB( &p, k, 0.01f );
    const float v = getNB( &p, k );
    setNB( &p, k, v );
    */


    // ��������� ���������
    //p.velocityX = massDt.x;
    //p.velocityZ = massDt.y;
    wmT[i].pe[n] = p;

}








/**
* ������� "����� ����� ������� ������ �������".
* ��������� ����.
* wmT -> wm
*
* @see interactionI()
*/
inline void interactionII(
    __global WMap* wmT,
    __global WMap* wm,
    const float dt,
    const uint x,
    const uint z,
    // = x + z * N
    const uint i
) {
    /* @test
    wm[i].pe[0] = wmT[i].pe[0];
    return;
    */


    // �������������� ������ ������� �������
    // @todo optimize ����� ������.
    if ( zero( dt ) ) {
        return;
    }
    

    // ������ ����� ��������� �� MAX_WMAP_CELL ������ �� ������ �������.
    // ��������������� � ������ �� ���.
    /* - ���. ���� ���������� � ����� �������.
    for (size_t n = 0; n < MAX_WMAP_CELL; ++n) {
    */
    const size_t n = 0;

    // �������� � ���������� �������
    ParticleEntity p = wmT[i].pe[n];

    // ������ ���� ��������� � ��������.
    const bool pVacuum = vacuum( p );

    // ����� ����� ��������� ���� ������� �� �������
    float incomingMass = 0.0f;

    // �������, ��� ������ �������� � �������� �������� ����� �������
    bool addNewMatter = false;

    // ������� ������� ��������� (== �����) �������
    for (size_t k = 1; k <= 8; ++k) {
        const int iNear = nearCell( x, z, k );
        // ������� ����� ���� �� ������� ����
        if (iNear == -1) {
            // 0 == "����� �� ������ �� �������"
            setNB( &p, k, 0.0f );
            continue;
        }

        // �������� � ���������� �������
        ParticleEntity pNear = wmT[iNear].pe[n];
        const bool pNearVacuum = vacuum( pNear );

        // �������� �������� �� ������ ������� ��������, � ������� �������������
        // @see struct.h / struct NB
        const float transfer = getNBNear( pNear, k );
        if ( zero( transfer ) ) {
            // ��� ��������
            // 0 == �����: "����� ������ �� �������"
            setNB( &p, k, 0.0f );
            continue;
        }

        if ( pNearVacuum ) {
            // ������ �� �����������
            // @todo optimize ����� �� �������� setNB(), �.�. �� ����� ��������
            //       ������ ��� ���������. �.�. �������� ��������� transfer ����.
            // 1 == "������� ��, ��� ����� ����� ��������; ����� - ������"
            setNB( &p, k, 1.0f );
            continue;
        }

        // �������, ������� ����� �������� ������� ����� ������� � ���� � ������.
        // �������� ������ � ��������� ����� �������. ������ �� ��������� ��.
        
        /*
        // @todo ���������: ������� ���������� ������, ������� ������ �������.
        if (p.matter != pNear.matter) {
            // 0 == "����� �� ������ �� �������"
            setNB( &p, k, 0 );
            continue;
        }
        */

        // @todo ���������: ������� ��������.

        // ��� - �����������. ��������� ��� ��������.
        // �������� ���������� ���� (����� p.expansion == 1 )
        // ���������� ���� ��������� � calcCharacteristicParticle()
        const float2 vExpansionNear = pNear.expansion * nearForce( k );
        // �������� ������� � ������ (��� ��������� ������� ���)
        const float2 velocityNear = (float2)(pNear.velocityX, pNear.velocityZ);

        // �������������� ����� �������
        // � ������ �� ����� "��������" ����� ������, ��� ���������� ������ �������� ������
        // @todo optimize ������ 'clamp': ����� ������ �������������.
        const float maxIncomingMassNear = pNear.mass * transfer;
        const float incomingMassNear = clamp(
            length( velocityNear + vExpansionNear * 0/* @debug */ ) * transfer * dt,
            0.0f, maxIncomingMassNear
        );

        incomingMass += incomingMassNear;

        // (incomingTransfer == 1) == "������� ����� �� �����, ������� � ����� �������� �����"
        // @todo optimize ����� ������ 'select zero()'?
        const float incomingTransfer = select(
            incomingMassNear / maxIncomingMassNear, 1.0f, zero( maxIncomingMassNear )
        );
        setNB( &p, k, incomingTransfer );

    } // for (size_t k = 1; k <= 8; ++k)

    
    // ���������: ��������, ������ �� �������� ����� �������. �����
    // ������ � ���� ������ ����� �������.
    if ( pVacuum && gZero( incomingMass ) ) {
        addNewMatter = true;
        p.matter = MATTER_WATER;  // pNear.matter;
        // ����� ��������� ����
        p.mass = 0.0f;
        p.velocityX = 50.0f;
        p.velocityZ = 0.0f;
        p.temperature = -111; //pNear.temperature;
    }
    

    // ������ ����� �������� ����� �������.
    // ��������� � ��������� ��� ����. ������.
    if ( !pVacuum ) {
        // �� ������ �� ����� "��������" ����� ������, ��� � ��� ���������.
        // �� ��������� ���� ������� ��������, �.�. � ����� ������ ������� �������
        // �����, ��������� ������ �� �������� (����� �������� � ������� ������ 1.0 ��� 0.0).
        // @todo optimize ������ 'transfer'.
        const float transfer = 1.0f;
        const float2 velocity = (float2)(p.velocityX, p.velocityZ);
        // @todo optimize �������� 'clamp': ����� ������ �������������.
        const float outcomingMass = clamp(
            ( length( velocity ) + p.expansion * 0/* @debug */ ) * transfer * dt,
            0.0f, p.mass
        );
        p.mass += incomingMass - outcomingMass;

        // ������ ����� ��������
        if ( leZero( p.mass ) ) {
            p.matter = MATTER_VACUUM;
            p.mass = 0.0f;
            p.velocityX = p.velocityZ = 0.0f;
            p.temperature = -222.0f;
        }
        
        // @test
        p.t1 = incomingMass;
        p.t2 = outcomingMass;
        
    } // if ( !vacuum( p ) )
    
    
    p.velocityX += p.accelerationX * dt;
    p.velocityZ += p.accelerationZ * dt;


    // ��������� ���������
    wm[i].pe[n] = p;

}
