// (!) Все определения из этого файла должны быть согласованы со struct.h
// @see Примечания в struct.h

enum PState {
    VACUUM = 0,
    SOLID = 1,
    LIQUID = 2,
    GAS = 3,
    PLASMA = 4
};




typedef struct {
    uint nb14;
    uint nb58;
} NB;




// (!) Структуры на HOST и GPU должны быть выровнены.
// @see http://khronos.org/message_boards/viewtopic.php?f=37&t=3098
typedef struct __attribute__ ((packed)) {
    uint matter;

    float mass;
    float velocityX;
    float velocityZ;
    float temperature;

    uint state;
    float accelerationX;
    float accelerationZ;
    float density;
    float pressure;
    float expansion;

    NB nb;

    float t1;
    float t2;

} ParticleEntity;




// MAX_WMAP_CELL определяется при вызове
typedef struct {
    // Частицы, разделяющие одну ячейку
    ParticleEntity pe[ MAX_WMAP_CELL ];
} WMap;




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
    PREPARE_INTERACTION,
    GO_INTERACTION_I,
    GO_INTERACTION_II,
    GO_INTERACTION_III,
    COMMIT_INTERACTION
};
