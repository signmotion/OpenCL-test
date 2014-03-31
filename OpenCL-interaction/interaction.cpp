// @see След. проект > 'phygen'

#include "StdAfx.h"
#include "struct-local.h"
#include "struct.h"
#include "const.h"

// Взаимодействия элементов мира делаем через OpenCL/image2d.
// @see (!) http://developer.apple.com/library/mac/#documentation/Performance/Conceptual/OpenCL_MacProgGuide/ImageProcessingwithOpenCL/ImageProcessingwithOpenCL.html#//apple_ref/doc/uid/TP40008312-CH103-SW2
// @see http://www.cmsoft.com.br/index.php?option=com_content&view=category&layout=blog&id=115&Itemid=172
// @see http://www.khronos.org/message_boards/viewtopic.php?f=28&t=2358
// Для работы с изображением
//   > http://www.imagemagick.org/Magick++/Image.html
//   > http://labs.qt.nokia.com/2010/04/07/using-opencl-with-qt


// Говорить в консоли
#define DEBUG_VERBOSE 0


// Шаг времени в секундах, за которое будут рассчитываться взаимодействия
const float dt = 1.0f / 10;

// Текущее время в мире
float time = 0.0f;

// Пауза между переводом мира в след. состояние
const unsigned long TICK_PAUSE = 1;
// Вспомогательная переменная для отработки паузы
unsigned long  tickPause = 0;


// Размер мира
const size_t N = 512;
const size_t M = 512 / 2;
const size_t NM = N * M;
// Размер одной ячейки
// 1.0f / 5.0f == 1 метр содержит 5 неделимых ячеек (по 20 см)
const float SCALE = 1.0f / 5.0f;
// Объём частицы (частица представлена в форме куба)
const float V_PARTICLE = SCALE * SCALE * SCALE;

// Максимальное количество уникальных единиц материи в мире
const size_t MAX_COUNT_MATTER = 256;


// Ко всем частицам мира может быть приложена сила (например, сила тяжести)
const cl_float DEFAULT_FORCE_X = 0.0f;
const cl_float DEFAULT_FORCE_Z = -9.81f * 0;

// Постоянное внешнее давление
const float DEFAULT_PRESSURE = 0.0f;


// Что визуализируем
RenderWhat viewMode = IMAGE_WMAP;

// Что делает щелчок правой кнопкой мыши
ClickWhat clickRightMode = HOT_1000;


// Параметры сетки
const size_t GRID_WIDTH = N;
const size_t GRID_HEIGHT = M;
const size_t GRID_WORK_DIM = 2;
const size_t GRID_GLOBAL_WORK_SIZE[] = { GRID_WIDTH, GRID_HEIGHT };
const size_t GRID_LWS = 16;
const size_t GRID_LOCAL_WORK_SIZE[] = { GRID_LWS, GRID_LWS / 2 };


cl_context context;
cl_command_queue commandQueue;
cl_kernel kernel;


// handle to the GLUT window
int glutWindowHandle;

// OpenGL pixel buffer object
GLuint pbo;
cl_mem pbo_cl;


// Признак: время в мире замерло
bool pause = true;




// Forward Function declarations
//*****************************************************************************

// Методы для работы с миром
void createWorld();
void interaction(float dt);
void createImage();
void snapshotWorld();
void cellInfo(size_t a, size_t b);
void clickAction(size_t a, size_t b);


// OpenCL functions
void render();

// OpenGL functionality
void initGL(int* argc, char** argv);
void displayGL();
void mouseGL(int button, int state, int x, int y);
void reshape(int w, int h);
void idle();
void keyboardGL(unsigned char key, int x, int y);
void initGLBuffers();




// Материя
// @see createWorld()
MatterSolid* mSolid = nullptr;
MatterLiquid* mLiquid = nullptr;
MatterGas* mGas = nullptr;
MatterPlasma* mPlasma = nullptr;


// Мир N x M
WMap* wm = nullptr;
cl_mem oclWM = nullptr;





// Main program
//*****************************************************************************
int main(int argc, char** argv) {

	setlocale(LC_ALL, "Russian");
    // Для разделителя '.' вместо ','
    setlocale(LC_NUMERIC, "C");


    initGL(&argc, argv);


    cl_int status = CL_SUCCESS;

    // Получаем платформу
    cl_platform_id platform;
    status = u::ocl::getPlatformID( &platform, "NVIDIA" );
    assert(status == CL_SUCCESS);

    // Получаем устройства
    cl_uint numDevices;
    status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
    assert(status == CL_SUCCESS);
    //cl_device_id* devices = (cl_device_id *)malloc(numDevices * sizeof(cl_device_id) );
    cl_device_id* devices = new cl_device_id[ numDevices * sizeof(cl_device_id) ];
    assert(devices != nullptr);
    status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
    assert(status == CL_SUCCESS);

    //cout << getDeviceInfo( device );

    // Создаём контекст
    context = clCreateContext(
        0, numDevices, devices, NULL, NULL, &status
    );
    assert(status == CL_SUCCESS);

    // Выбираем самое мощное устройство
    cl_device_id device = u::ocl::getMaxFlopsDevice( context );

    // Строим очередь команд
    commandQueue = clCreateCommandQueue(context, device, 0, &status);
    assert(status == CL_SUCCESS);

    // Получаем текст программы
    const string kernelName = "render";
    const string sourcePath =
        "D:/Projects/workspace/OpenCL-test/OpenCL-interaction/resource/OpenCL/"
      + kernelName + ".cl";
    size_t program_length;
    char* sourceCL = u::ocl::loadProgramSource(
        sourcePath.c_str(), "", &program_length
    );
    assert(sourceCL != nullptr);

    // Создаём программу
    cl_program program = clCreateProgramWithSource(
        context,
        1,
		(const char **) &sourceCL,
        &program_length,
        &status
    );
    assert(status == CL_SUCCESS);
    
    // Компилируем программу
    ostringstream ss;
    ss << " -D N=" << N;
    ss << " -D M=" << M;
    ss << " -D NM=" << NM;
    ss << " -D V_PARTICLE=" << V_PARTICLE;
    ss << " -D MAX_COUNT_MATTER=" << MAX_COUNT_MATTER;
    ss << " -D MAX_WMAP_CELL=" << MAX_WMAP_CELL;
    ss << " -D DEFAULT_FORCE_X=" << DEFAULT_FORCE_X;
    ss << " -D DEFAULT_FORCE_Z=" << DEFAULT_FORCE_Z;
    ss << " -D DEFAULT_PRESSURE=" << DEFAULT_PRESSURE;
    string options = " \
        -I D:/Projects/workspace/OpenCL-test/OpenCL-interaction/resource/OpenCL \
        -cl-opt-disable \
        -Werror \
    " + ss.str();
    //options = "-g";
    status = clBuildProgram(
        program, 0, NULL,
        // @todo optimize Параметры > "-cl-fast-relaxed-math"
        // @see OpenCL spec. 5.4.3
        options.c_str(),
        NULL, NULL
    );
    if (status != CL_SUCCESS) {
        cerr << "clBuildProgram()" << endl;
        const auto firstDevice = u::ocl::getFirstDevice( context );
        u::ocl::logBuildInfo( program, firstDevice );
        const string dumpFile = "interaction-dump-error.ptx";
        u::ocl::logPtx( program, firstDevice, dumpFile.c_str() );
    }
    assert(status == CL_SUCCESS);

    // Создаём исполняемое ядро.
    // Принято: Ядро программы всегда называется по имени файла.
    kernel = clCreateKernel(program, kernelName.c_str(), &status);
    assert(status == CL_SUCCESS);


    // Создаём буферы отображения и связываем их с GLUT
    initGLBuffers();

    // Загружаем мир
    createWorld();

    // Создаём образ мира. Визуализация требует инициализации.
    interaction( 0.0f );
    createImage();

    // Снимок мира
    snapshotWorld();

    // Запускаем основной цикл рендеринга
    glutMainLoop();


    // Завершение работы: убираем за собой
    delete[] wm;
    delete[] mSolid;
    delete[] mLiquid;
    delete[] mGas;
    delete[] mPlasma;

    delete[] devices;
    clReleaseKernel( kernel );
    clReleaseProgram( program );
    clReleaseCommandQueue( commandQueue );
    clReleaseContext( context );
 

    #ifdef WIN32
    //    getchar();
    #endif
}







// Initialize GL
//*****************************************************************************
void initGL(int* argc, char **argv ) {
    // init GLUT 
    glutInit( argc, (char**)argv );
    glutInitDisplayMode( GLUT_RGB | GLUT_ALPHA | GLUT_SINGLE );
    glutInitWindowSize(N, M);
    glutInitWindowPosition(
        glutGet( GLUT_SCREEN_WIDTH ) / 2 - N / 2, 
        glutGet( GLUT_SCREEN_HEIGHT ) / 2 - M / 2
    );
    glutWindowHandle = glutCreateWindow("Interaction");

    // OpenGL properties
    // Background color
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    // @see other in reshape()

    // register GLUT callback functions
    glutDisplayFunc( displayGL );
    glutMouseFunc( mouseGL );
    glutKeyboardFunc( keyboardGL );
    glutReshapeFunc( reshape );
    glutIdleFunc( idle );

    // init GLEW
    glewInit();
    GLboolean bGLEW = glewIsSupported(
        "GL_VERSION_2_0 GL_ARB_pixel_buffer_object"
    );
    assert( bGLEW );

}







//*****************************************************************************
/**
* Создаёт буферы для работы с изображением.
*/
void initGLBuffers() {

    cout << "initGLBuffers()" << endl;

    // Определяем размер буфера отображения.
    // 1 пиксель = 4 канала: RGBA, float
    const size_t V_IMAGE = 4 * sizeof(cl_float) * NM;

    //@todo optimize? Сравнить скорость при реализации через
    //      текстуру image2d_t > http://www.songho.ca/opengl/gl_pbo.html#unpack

    // Создаём Pixel Buffer Object (PBO)
    // @see http://www.songho.ca/opengl/gl_pbo.html
    // @see http://www.songho.ca/opengl/gl_vbo.html#create
    glGenBuffersARB(
        // Количество буферов
        1,
        // Идентификатор буферов
        &pbo
    );
    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pbo );
    glBufferDataARB(
        // GLenum target
        GL_PIXEL_UNPACK_BUFFER_ARB,
        // GLsizei size, в байтах
        V_IMAGE,
        // const void* data - Данные для копирования
        NULL,
        // GLenum usage
        // @todo optimize? http://www.songho.ca/opengl/gl_vbo.html
        GL_STREAM_DRAW_ARB
    );
    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, 0 );
    
    /* test
    auto imageOut = new cl_float4[NM];
    for (size_t z = 0; z < M; z++) {
        for (size_t x = 0; x < N; x++) {
            const size_t i = x + z * N;
            imageOut[i].s[0] = (float)z / (float)M;
            imageOut[i].s[1] = (float)x / (float)M;
            imageOut[i].s[2] = (float)(z + x) / (float)(M + N);
            imageOut[i].s[3] = 1.0f;
        }
    }
    */

    // И здесь же резервируем место в памяти OpenCL
    cl_int status;
    pbo_cl = clCreateBuffer(
        context,
        CL_MEM_WRITE_ONLY,
        //CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR,
        V_IMAGE,
        NULL,
        //imageOut,
        &status
    );
    assert(status == CL_SUCCESS);

    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&pbo_cl);
    assert(status == CL_SUCCESS);


    // Буферы для хранения промежуточных данных создаются в createWorld()

}







/**
* Создаёт описание мира.
*/
void createWorld() {

    cout << "createWorld()" << endl;

    cout << "Размер матрицы: " << N << " x " << M << " = " << NM << " ячеек" << endl;

    // Материя.
    // Храним в виде списка, чтобы уменьшить количество условий
    // при написании кода. Список - одномерный, чтобы не возникало
    // вопросов при передаче его OpenCL.
    // Элемент '0' резервируем как "пустой" для более наглядного
    // обращения к элементам и хранении информации о материи "вакуум".
    mSolid = new MatterSolid[ MAX_COUNT_MATTER ];
    mLiquid = new MatterLiquid[ MAX_COUNT_MATTER ];
    mGas = new MatterGas[ MAX_COUNT_MATTER ];
    mPlasma = new MatterPlasma[ MAX_COUNT_MATTER ];

    cout << "Память для MatterSolid: " << sizeof(MatterSolid) * MAX_COUNT_MATTER / 1024 << " КБ" << endl;
    cout << "Память для MatterLiquid: " << sizeof(MatterLiquid) * MAX_COUNT_MATTER / 1024 << " КБ" << endl;
    cout << "Память для MatterGas: " << sizeof(MatterGas) * MAX_COUNT_MATTER / 1024 << " КБ" << endl;
    cout << "Память для MatterPlasma: " << sizeof(MatterPlasma) * MAX_COUNT_MATTER / 1024 << " КБ" << endl;

    // Обнуляем все состояния всех материй
    for (size_t id = 0; id < MAX_COUNT_MATTER; id++) {
        mSolid[id].color =
            mLiquid[id].color =
                mGas[id].color =
                    mPlasma[id].color =
                        0x00000000;
        mSolid[id].density =
            mLiquid[id].density =
                mGas[id].density =
                    mPlasma[id].density =
                        0.0f;
        // Декларируем температуру и для вакуума
        mSolid[id].tt =
            mLiquid[id].tt =
                mGas[id].tt =
                    (id == MATTER_VACUUM) ? C0 : 0.0f;

        mGas[id].molarMass = 0.0f;
    }


    size_t id = 0;

    // Вакуум
    // Для вакуума тоже прописываем все состояния, чтобы уменьшить
    // количество условий при моделировании законов мира.
    id = MATTER_VACUUM;
    // : Сделано при инициализации, выше.

    // Воздух
    /* - Позже. Работаем с водой.
    // @todo (!) Жёстко зашито в helper.hcs / calcState(). Исправить.
    id = GAS * 10 + 1;

    molarMass = 29.0f * 1000.0f;

    state = SOLID;
    i = id * MAX_COUNT_PSTATE + state;
    md[i].molarMass = molarMass;
    md[i].color = 0x1e90ff80;
    md[i].tt = -214.0f;
    md[i].density = 1200.0f;
    
    state = LIQUID;
    i = id * MAX_COUNT_PSTATE + state;
    md[i].molarMass = molarMass;
    md[i].color = 0x42aaff80;
    md[i].tt = -192.0f;
    md[i].density = 960.0f;

    state = GAS;
    i = id * MAX_COUNT_PSTATE + state;
    md[i].molarMass = molarMass;
    md[i].color = 0x7fc7ff80;
    md[i].tt = 100000.0f;
    md[i].density = 1.293f;

    state = PLASMA;
    i = id * MAX_COUNT_PSTATE + state;
    md[i].molarMass = molarMass;
    md[i].color = 0xe3263680;
    // Крайнюю температуру для плазмы логично обозначить как NAN.
    // Но нельзя: OpenCL тогда не сможет включать "быструю математику"
    // (поддерживается большинством GPU).
    md[i].tt = 0.0f;
    md[i].density = md[ id * MAX_COUNT_PSTATE + GAS ].density / 1e6;
    */


    // Вода
    // @source http://ru.wikipedia.org/wiki/%D0%92%D0%BE%D0%B4%D0%B0
    id = MATTER_WATER;
    mSolid[id].color = 0xfff5eeff;
    mSolid[id].density = 917.0f;
    mSolid[id].tt = 0.0f;

    mLiquid[id].color = 0x42aaff80;
    mLiquid[id].density = 998.0f;
    mLiquid[id].tt = 100.0f;

    mGas[id].color = 0x7fc7ff80;
    mGas[id].density = 598.0f;
    mGas[id].tt = mLiquid[id].tt * 1000.0f;
    mGas[id].molarMass = 18.0f * 1000.0f;

    mPlasma[id].color = 0xe3263680;
    mPlasma[id].density = mGas[id].density / 1e6;
    mPlasma[id].molarMass = mGas[id].molarMass;


    // Заполняем мир.
    // Мир задан матрицей N x M: 1 ячейка = 1 единица материи.
    // В мире воплощены сущности, состоящие из материи.
    // Сущности разбиты на частицы. Каждая частица получает свой UID, который
    // совпадает с постоянным адресом частицы в матрице мира (см. ниже).

    // Выделяем для мира участок.
    // Количество частиц в мире никогда не превышает N x M.
    wm = new WMap[ NM ];
    cout << "Память для карты мира WMap: " <<
        sizeof(WMap) * NM / 1024 / 1024 << " МБ" <<
    endl;


    // Очищаем мир
    for (size_t z = 0; z < M; ++z) {
        for (size_t x = 0; x < N; ++x) {
            const size_t i = x + z * N;
            for (size_t n = 0; n < MAX_WMAP_CELL; ++n) {
                wm[i].pe[n].matter = MATTER_VACUUM;
                wm[i].pe[n].velocityX = wm[i].pe[n].velocityZ = 0.0f;
                wm[i].pe[n].temperature = C0;
                wm[i].pe[n].mass = 0.0f;

                wm[i].pe[n].state = VACUUM;
                wm[i].pe[n].accelerationX = wm[i].pe[n].accelerationZ = 0.0f;
                wm[i].pe[n].density = 0.0f;
                wm[i].pe[n].pressure = 0.0f;
                wm[i].pe[n].expansion = 0.0f;

                wm[i].pe[n].nb.nb14 = wm[i].pe[n].nb.nb58 = 0x00000000;

                wm[i].pe[n].t1 = wm[i].pe[n].t2 = 0.0f;
            }
        }
    }


    // Создаём блок воды
    const size_t WATER_BLOCK_WIDTH = 200;
    const size_t WATER_BLOCK_HEIGHT = 100;
    const size_t SHIFT_X = N / 4;
    const size_t SHIFT_Z = M / 4;
    id = MATTER_WATER;
    for (size_t z = SHIFT_Z; z < SHIFT_Z + WATER_BLOCK_HEIGHT; z++) {
        for (size_t x = SHIFT_X; x < SHIFT_X + WATER_BLOCK_WIDTH; x++) {
            // Координата ячейки в матрице мира
            const size_t i = x + z * N;
            wm[i].pe[0].matter = id;
            wm[i].pe[0].velocityX = 50.0f;
            wm[i].pe[0].velocityZ = 0.0f;
            wm[i].pe[0].temperature = -10.0f;  // лёд
            wm[i].pe[0].mass = mLiquid[id].density * V_PARTICLE;
        }
    }


    // Переносим информацию о материи и декларацию мира в память OpenCL.
    // Резервируем место для результатов.

    cl_int status = CL_SUCCESS;
    
    cl_mem oclMSolid = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(MatterSolid) * MAX_COUNT_MATTER,
        mSolid,
        &status
    );
    assert(status == CL_SUCCESS);

    cl_mem oclMLiquid = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(MatterLiquid) * MAX_COUNT_MATTER,
        mLiquid,
        &status
    );
    assert(status == CL_SUCCESS);

    cl_mem oclMGas = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(MatterGas) * MAX_COUNT_MATTER,
        mGas,
        &status
    );
    assert(status == CL_SUCCESS);

    cl_mem oclMPlasma = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(MatterPlasma) * MAX_COUNT_MATTER,
        mPlasma,
        &status
    );
    assert(status == CL_SUCCESS);


    oclWM = clCreateBuffer(
        context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        sizeof(WMap) * NM,
        wm,
        &status
    );


    cout << "Память для временной карты мира WMap: " <<
        sizeof(WMap) * NM / 1024 / 1024 << " МБ" <<
    endl;
    cl_mem oclWMT = clCreateBuffer(
        context, CL_MEM_READ_WRITE,
        sizeof(WMap) * NM,
        nullptr,
        &status
    );

    // Мир храним в памяти устройства OpenCL
    status |= clSetKernelArg( kernel, 1, sizeof(cl_mem), (void*)&oclMSolid );
    status |= clSetKernelArg( kernel, 2, sizeof(cl_mem), (void*)&oclMLiquid );
    status |= clSetKernelArg( kernel, 3, sizeof(cl_mem), (void*)&oclMGas );
    status |= clSetKernelArg( kernel, 4, sizeof(cl_mem), (void*)&oclMPlasma );
    status |= clSetKernelArg( kernel, 5, sizeof(cl_mem), (void*)&oclWM );
    // ... (устанавливаются перед вызовом kernel)
    status |= clSetKernelArg( kernel, 9, sizeof(cl_mem), (void*)&oclWMT );
    assert(status == CL_SUCCESS);

}





/**
* Образ мира.
* Создаётся путём обработки данных о мире через OpenCL.
*/
void createImage() {

    //cout << "createImage() " << endl;

    cl_int status = CL_SUCCESS;

    const cl_uint what = viewMode;
    status = clSetKernelArg( kernel, 6, sizeof(cl_uint), &what );
    const cl_float dt0 = 0.0f;
    status |= clSetKernelArg( kernel, 7, sizeof(cl_float), &dt0 );
    const cl_uint r0 = rand();
    status |= clSetKernelArg( kernel, 8, sizeof(cl_uint), &r0 );
    assert(status == CL_SUCCESS);

    // @see http://www.songho.ca/opengl/gl_pbo.html
    status = clEnqueueNDRangeKernel(
        commandQueue,
        kernel,
        GRID_WORK_DIM,
        NULL,
        GRID_GLOBAL_WORK_SIZE, GRID_LOCAL_WORK_SIZE,
        0, 0, 0
    );
    status |= clFinish( commandQueue );
    assert(status == CL_SUCCESS);


    // Чтение в память компьютера (показ изображения)
    // 1 пиксель = 4 канала: RGBA, float
    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pbo );    
    // Карта буфера объекта - в память клиента
    GLfloat* ptr = (GLfloat*)glMapBufferARB(
        GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB
    );
    status = clEnqueueReadBuffer(
        commandQueue,
        pbo_cl,
        CL_TRUE,
        0,
        4 * sizeof(cl_float) * NM,
        ptr,
        0, NULL, NULL
    );
    assert(status == CL_SUCCESS);

    glUnmapBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB );
}







/**
* Расчёт взаимодействия.
* Обрабатывается через OpenCL.
* После завершения работы метода, матрицы ParticleEntity и WMap согласованы.
*/
void interaction(float dt) {

    //cout << "interaction() " << endl;

    // Подготавливаем данные
    cl_uint what = PREPARE_INTERACTION;
    cl_int status = clSetKernelArg( kernel, 6, sizeof(cl_uint), &what );
    status |= clSetKernelArg( kernel, 7, sizeof(cl_float), &dt );
    const cl_uint r = rand();
    status |= clSetKernelArg( kernel, 8, sizeof(cl_uint), &r );
    assert(status == CL_SUCCESS);
    status = clEnqueueNDRangeKernel(
        commandQueue,
        kernel,
        GRID_WORK_DIM,
        NULL,
        GRID_GLOBAL_WORK_SIZE, GRID_LOCAL_WORK_SIZE,
        0, 0, 0
    );
    status |= clFinish( commandQueue );
    assert(status == CL_SUCCESS);


    // Рассчитываем состояние мира по прошествии 'dt'

    what = GO_INTERACTION_I;
    status = clSetKernelArg( kernel, 6, sizeof(cl_uint), &what );
    assert(status == CL_SUCCESS);
    status = clEnqueueNDRangeKernel(
        commandQueue,
        kernel,
        GRID_WORK_DIM,
        NULL,
        GRID_GLOBAL_WORK_SIZE, GRID_LOCAL_WORK_SIZE,
        0, 0, 0
    );
    status |= clFinish( commandQueue );
    assert(status == CL_SUCCESS);

    what = GO_INTERACTION_II;
    status = clSetKernelArg( kernel, 6, sizeof(cl_uint), &what );
    assert(status == CL_SUCCESS);
    status = clEnqueueNDRangeKernel(
        commandQueue,
        kernel,
        GRID_WORK_DIM,
        NULL,
        GRID_GLOBAL_WORK_SIZE, GRID_LOCAL_WORK_SIZE,
        0, 0, 0
    );
    status |= clFinish( commandQueue );
    assert(status == CL_SUCCESS);

    what = GO_INTERACTION_III;
    status = clSetKernelArg( kernel, 6, sizeof(cl_uint), &what );
    assert(status == CL_SUCCESS);
    status = clEnqueueNDRangeKernel(
        commandQueue,
        kernel,
        GRID_WORK_DIM,
        NULL,
        GRID_GLOBAL_WORK_SIZE, GRID_LOCAL_WORK_SIZE,
        0, 0, 0
    );
    status |= clFinish( commandQueue );
    assert(status == CL_SUCCESS);


    // Фиксируем расчёты взаимодействия в новом состоянии мира
    // @todo optimize Подготовку и фиксирование перенести внутрь GO_INTERACTION.
    what = COMMIT_INTERACTION;
    status = clSetKernelArg( kernel, 6, sizeof(cl_uint), &what );
    assert(status == CL_SUCCESS);
    status = clEnqueueNDRangeKernel(
        commandQueue,
        kernel,
        GRID_WORK_DIM,
        NULL,
        GRID_GLOBAL_WORK_SIZE, GRID_LOCAL_WORK_SIZE,
        0, 0, 0
    );
    status |= clFinish( commandQueue );
    assert(status == CL_SUCCESS);

}





/**
* Обновляет WMap (GPU -> CPU).
*/
void updateWMapToCPU() {
    cl_int status = clEnqueueReadBuffer(
        commandQueue,
        oclWM,
        CL_TRUE,
        0,
        sizeof(WMap) * NM,
        wm,
        0, NULL, NULL
    );
    assert(status == CL_SUCCESS);
}




/**
* Обновляет WMap (CPU -> GPU).
*/
void updateWMapToGPU() {
    cl_int status = clEnqueueWriteBuffer(
        commandQueue,
        oclWM,
        CL_TRUE,
        0,
        sizeof(WMap) * NM,
        wm,
        0, NULL, NULL
    );
    assert(status == CL_SUCCESS);
}






/**
* Снимок мира.
*/
void snapshotWorld() {
    // Обновляем из GPU перед показом
    updateWMapToCPU();

    size_t countP[5];
    countP[VACUUM] =
        countP[SOLID] =
            countP[LIQUID] =
                countP[GAS] =
                    countP[PLASMA] = 0;
    float massP[5];
    massP[VACUUM] =
        massP[SOLID] =
            massP[LIQUID] =
                massP[GAS] =
                    massP[PLASMA] = 0.0f;
    for (size_t iz = 0; iz < M; ++iz) {
        for (size_t ix = 0; ix < N; ++ix) {
            const size_t i = ix + iz * N;
            // @todo Работать с несколькими слоями
            const size_t n = 0;
            const ParticleEntity p = wm[i].pe[n];
            countP[ p.state ]++;
            massP[ p.state ] += p.mass;
        }
    }

    cout << dec;
    cout << "\tVac \tSol \tLiq \tGas \tPla" << endl;
    const size_t count = countP[SOLID] + countP[LIQUID] + countP[GAS] + countP[PLASMA];
    cout << "count \t" <<
        countP[VACUUM] << " \t" << 
        countP[SOLID] << " \t" << 
        countP[LIQUID] << " \t" << 
        countP[GAS] << " \t" << 
        countP[PLASMA] <<
        " \t= " << count << " / " << countP[VACUUM] <<
    endl;
    const float mass = massP[SOLID] + massP[LIQUID] + massP[GAS] + massP[PLASMA];
    cout << "mass \t" <<
        massP[VACUUM] << " \t" << 
        massP[SOLID] << " \t" << 
        massP[LIQUID] << " \t" << 
        massP[GAS] << " \t" << 
        massP[PLASMA] <<
        " \t= " << mass << " / " << massP[VACUUM] <<
    endl;
    cout << endl;
}



/**
* Информация о ячейке.
*/
void cellInfo(size_t x, size_t z) {
    // Обновляем из GPU перед показом
    updateWMapToCPU();

    if ( (x > N) || (z > M) ) {
        cout << dec;
        cout << "Ячейка " << x << " " << z << " лежит за границами мира" << endl;
        return;
    }

    const size_t i = x + z * N;
    cout << dec;
    cout << "coord " << x << " " << z << endl;
    cout << "<particle 1>" << endl;

    // ParticleEntity
    const ParticleEntity p = wm[i].pe[0];

    cout << "matter " << p.matter << endl;
    cout << "state " << p.state << endl;

    cout << "mass " << p.mass << endl;
    cout << "acceleration " << p.accelerationX << " " << p.accelerationZ << endl;
    cout << "velocity " << p.velocityX << " " << p.velocityZ << endl;
    cout << "temperature " << p.temperature << endl;
    cout << "density " << p.density << endl;
    cout << "pressure " << p.pressure << endl;

    cout << "t " << p.t1 << " " << p.t2 << endl;

    cout << hex;
    cout << "transfer in/out " << p.nb.nb14 << " " << p.nb.nb58 << endl;

    cout << endl;
}




/**
* Делает над ячейкой действие согласно установленному 'clickRightMode'.
*/
void clickAction(size_t x, size_t z) {
    // Обновляем из GPU перед показом
    updateWMapToCPU();

    
    // ClickWhat::HOT_1000
    const size_t i = x + z * N;
    wm[i].pe[0].temperature = 1000.0f;


    // Записываем изменения в GPU
    updateWMapToGPU();

    // Обновляем картинку
    //interaction( 0.0f );
    createImage();
    glutPostRedisplay();

    // Делаем снимок мира
    snapshotWorld();
}









// render image using OpenCL    
//*****************************************************************************
void render() {
    
#if 1
    // Не ускоряемся более чем надо
    const unsigned long tickCurrent = GetTickCount();
    if ( (tickCurrent - tickPause) < TICK_PAUSE ) {
        return;
    }
    tickPause = tickCurrent;
#endif

    // Рассчитываем взаимодействия
    interaction( dt );

    time += dt;
    cout << "time > " << time << endl;

    // Формируем новый образ мира
    createImage();
    glutPostRedisplay();
}






// Display callback for GLUT main loop
//*****************************************************************************
void displayGL() {

#if 0
    // test
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_POLYGON);
        glVertex2f(0.25, 0.25);
        glVertex2f(0.75, 0.25);
        glVertex2f(0.75, 0.75);
        glVertex2f(0.25, 0.01 * time);
    glEnd();
    glFlush();
    time += 1.1;

/*
    // @source http://www.gamedev.net/community/forums/topic.asp?topic_id=489283
	//glClearColor( 1, 0, 0, 0 );
	glClear( GL_COLOR_BUFFER_BIT );
    auto glutError = glGetError();
    assert(glutError == 0);

    struct RGBA {
	    float r;
	    float g;
	    float b;
	    float a;
    };
	RGBA* pixels = new RGBA[ NM ];
	for (unsigned int i = 0; i < NM/2 - 100; i++) {
		pixels[i].r = 0.0;
		pixels[i].g = 0.5;
		pixels[i].b = 0.5;
		pixels[i].a = 0.1f;
	}
	
	glDrawPixels(N, M, GL_RGBA, GL_FLOAT, pixels);
    glutError = glGetError();
    assert(glutError == 0);

    glFlush();
	//glutSwapBuffers();

    //delete[] pixels;
*/
#endif

    
    // Отображаем результат
    glClear( GL_COLOR_BUFFER_BIT );
    glDisable( GL_DEPTH_TEST );
    glRasterPos2i( 0, 0 );

    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pbo );
    auto glutError = glGetError();
    assert(glutError == 0);

    glDrawPixels( N, M, GL_RGBA, GL_FLOAT, NULL );
    glutError = glGetError();
    assert(glutError == 0);

    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, 0 );
    glutError = glGetError();
    assert(glutError == 0);

    // Переключаем буфер на экран
    // @todo Пересмотреть необходимость в этой операции.
    glutSwapBuffers();
    glutError = glGetError();
    assert(glutError == 0);

    //glScaled(2.0, 2.0, 2.0);

    glFlush();

    //glutReportErrors();


    // Что-то можем написать в заголовке
    ostringstream ss;
    ss << "viewMode: " << viewMode;
    glutSetWindowTitle( ss.str().c_str() );

}





// @see http://www.lighthouse3d.com/opengl/glut/index.php?9
void mouseGL(int button, int state, int x, int y) {
    if (state != GLUT_DOWN) {
        return;
    }

    const int zz = M - y - 1;

    if (button == GLUT_LEFT_BUTTON) {
        snapshotWorld();
        cellInfo( x, zz );
        return;
    }
    
    if (button == GLUT_RIGHT_BUTTON) {
        snapshotWorld();
        cellInfo( x, zz );
        clickAction( x, zz );
        return;
    }
    
}





//*****************************************************************************
void idle() {
    if ( !pause ) {
        // Считаем
        render();
    }
}





// Window resize handler callback
//*****************************************************************************
void reshape(int x, int y) {
    glViewport(0, 0, x, y);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, 1.0); 
}





// Keyboard event handler callback
//*****************************************************************************
void keyboardGL(unsigned char key, int x, int y) {
    switch ( key ) {
        case '1':
            viewMode = IMAGE_WMAP;
            break;

        /*
        case '2':
            viewMode =
                (viewMode == IMAGE_WMAP)
                  ? IMAGE_MATTER_NEIGHBORS_DENSITY
                  : IMAGE_WMAP;
            break;
        */

        case 'h':
            clickRightMode = HOT_1000;
            cout << "clickRightMode: HOT 1000" << endl;
            break;

        case '=':
        case '+':
            interaction( dt );
            break;

        case ' ':
            pause = !pause;
            break;

        // Выход
        case '\033':
        case '\015':
        case 'Q':
        case 'q':
            exit( EXIT_SUCCESS );
            break;
    }

    // Обновляем матрицы и визуализируем
    createImage();
    glutPostRedisplay();
}
