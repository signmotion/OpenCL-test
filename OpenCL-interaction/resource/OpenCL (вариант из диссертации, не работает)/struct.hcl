// (!) Все определения из этого файла должны быть согласованы со struct.h
// @see Примечания в struct.h


// (!) Структуры на HOST и GPU должны быть выровнены.
// @see http://khronos.org/message_boards/viewtopic.php?f=37&t=3098
typedef struct __attribute__ ((packed)) {
    uint entity;

    float coordX;
    float coordZ;

    uint matter;

    float mass;
    float forceX;
    float forceZ;
    float velocityX;
    float velocityZ;
    float temperature;
    float viscosity;

    float state;
    float accelerationX;
    float accelerationZ;
    float density;
    float pressure;

} ParticleEntity;




// MAX_WMAP_CELL определяется при вызове
typedef struct __attribute__ ((packed)) {
    int uidOrSign[ MAX_WMAP_CELL ];
} WMap;




enum PState {
    VACUUM = 0,
    SOLID = 1,
    LIQUID = 2,
    GAS = 3,
    PLASMA = 4
};



enum UIDMatter {
    MATTER_VACUUM = 0,
    MATTER_WATER = LIQUID * 10 + 1
};



typedef struct __attribute__ ((packed)) {
    uint color;
    float density;
    float tt;
} MatterSolid;



typedef struct __attribute__ ((packed)) {
    uint color;
    float density;
    float tt;
} MatterLiquid;


typedef struct __attribute__ ((packed)) {
    uint color;
    float density;
    float tt;
    float molarMass;
} MatterGas;



typedef struct __attribute__ ((packed)) {
    uint color;
    float density;
    float molarMass;
} MatterPlasma;




enum RenderWhat {
    IMAGE_WMAP = 1,
    CLEAR_MATRIX,
    PREPARE_INTERACTION,
    GO_INTERACTION,
    COMMIT_INTERACTION
};
