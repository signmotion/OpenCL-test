#include "StdAfx.h"


// Constants, defines, typedefs and global declarations
//*****************************************************************************

// Uncomment this #define to enable CL/GL Interop
//#define GL_INTEROP    

struct RGBA {
	float r;
	float g;
	float b;
	float a;
};




// Взаимодействия элементов мира делаем через OpenCL/image2d.
// @see (!) http://developer.apple.com/library/mac/#documentation/Performance/Conceptual/OpenCL_MacProgGuide/ImageProcessingwithOpenCL/ImageProcessingwithOpenCL.html#//apple_ref/doc/uid/TP40008312-CH103-SW2
// @see http://www.cmsoft.com.br/index.php?option=com_content&view=category&layout=blog&id=115&Itemid=172
// @see http://www.khronos.org/message_boards/viewtopic.php?f=28&t=2358
// Для работы с изображением
//   > http://www.imagemagick.org/Magick++/Image.html
//   > http://labs.qt.nokia.com/2010/04/07/using-opencl-with-qt


// Шаг времени в секундах, за которое будут рассчитываться взаимодействия
const float TIME = 1.0f;

// Текущее время в мире
float time = 0.0f;


// Размер мира
const size_t N = 512;
const size_t M = 512;
const size_t NM = N * M;
// 1 метр содержит 10 ячеек
const float scale = 1.0f / 10.0f;


// Параметры сетки
const size_t GRID_WIDTH = N;
const size_t GRID_HEIGHT = M;
const size_t GRID_WORK_DIM = 2;
const size_t GRID_GLOBAL_WORK_SIZE[] = { GRID_WIDTH, GRID_HEIGHT };
const size_t GRID_LOCAL_WORK_SIZE[] = { 16, 16 };


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
void loadWorld();
void createImageEntity();


// OpenCL functions
void render();

// OpenGL functionality
void initGL(int* argc, char** argv);
void displayGL();
void reshape(int w, int h);
void idle();
void keyboardGL(unsigned char key, int x, int y);
void initGLBuffers();




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
    const string kernelName = "renderEntity";
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
    status = clBuildProgram(
        program, 0, NULL,
        // @todo optimize Параметры > "-cl-fast-relaxed-math"
        "",
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
    //loadWorld();

    // Создаём образ мира
    createImageEntity();

    // Запускаем основной цикл рендеринга
    glutMainLoop();


    //TestNoGL();
    
    // Завершение работы: убираем за собой
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
    // Определяем размер буфера отображения.
    //@todo optimize? Реализовать через текстуру http://www.songho.ca/opengl/gl_pbo.html#unpack
    // 1 пиксель = 4 канала: RGBA, float
    const size_t V = 4 * sizeof(cl_float) * NM;

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
        V,
        // const void* data - Данные для копирования
        NULL,
        // GLenum usage
        // @todo optimize? http://www.songho.ca/opengl/gl_vbo.html
        GL_STREAM_DRAW_ARB
    );
    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, 0 );
    
    // И здесь же - резервируем место в памяти видеокарты
    cl_int status;
    pbo_cl = clCreateBuffer(
        context,
        CL_MEM_WRITE_ONLY,
        V,
        NULL,
        &status
    );
    assert(status == CL_SUCCESS);

    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&pbo_cl);
    status |= clSetKernelArg(kernel, 1, sizeof(size_t), &N);
    status |= clSetKernelArg(kernel, 2, sizeof(size_t), &M);
    assert(status == CL_SUCCESS);
}







/**
* Создаёт описание мира.
*/
void createWorld() {
#if 0
    /**
    * Структуры хранения информации о мире.
    * Описание мира хранятся в виде 2D-изображений. Каждое изображение содержит
    * в себе какую-то часть информации.
    *
    * @see OpenCL/interaction.cl
    * @see http://www.khronos.org/registry/cl/sdk/1.0/docs/man/xhtml/restrictions.html
    */
    typedef struct {
        cl_uchar s0;
        cl_uchar s1;
        cl_uchar s2;
        cl_uchar s3;
    } DataUChar;


    const size_t SIZE = volumeSize[0] * volumeSize[1] * volumeSize[2];

    DataUChar* w1 = new DataUChar[SIZE];



    // Мир храним в памяти устройства
    delete[] w1;

#endif
}





/**
* Образ мира.
* Создаётся путём обработки данных о мире через OpenCL.
*/
void createImageEntity() {

    // @see http://www.songho.ca/opengl/gl_pbo.html
    cl_int status = clEnqueueNDRangeKernel(
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






// render image using OpenCL    
//*****************************************************************************
void render() {
    
#if 0
    // Если хотим получать новый образ постоянно.
    createImageEntity();
#endif


#if 0

    // Передаём данные из буфера GL в CL
#ifdef GL_INTEROP
    // Acquire PBO for OpenCL writing
    status = clEnqueueAcquireGLObjects(commandQueue, 1, &pbo_cl, 0,0,0);
    assert(status == CL_SUCCESS);
#endif

    // Аргументы ядра - 'глубина'
    status = clSetKernelArg( kernel, 5, sizeof(float), &w );
    assert(status == CL_SUCCESS);

    // Запускаем ядро, записываем результат в Pixel Buffer Object (PBO)
    // @see http://www.songho.ca/opengl/gl_pbo.html
    status = clEnqueueNDRangeKernel(
        commandQueue, kernel,
        workDim,
        NULL,
        globalWorkSize, localWorkSize,
        0, 0, 0
    );
    assert(status == CL_SUCCESS);

#ifdef GL_INTEROP
    // Transfer ownership of buffer back from CL to GL    
    status = clEnqueueReleaseGLObjects(commandQueue, 1, &pbo_cl, 0, 0, 0);
    assert(status == CL_SUCCESS);
#else

    // Чтение в память компьютера (показ изображения)
    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pbo );    
    GLubyte* ptr = (GLubyte*)glMapBufferARB(
        GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB
    );
    status = clEnqueueReadBuffer(
        commandQueue,
        pbo_cl,
        CL_TRUE,
        0,
        sizeof(unsigned char) * 4 * width * height,
        ptr,
        0, NULL, NULL
    );
    assert(status == CL_SUCCESS);

    glUnmapBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB );

#endif

#endif
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

    
    render();


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

    glFlush();

    //glutReportErrors();


    // Что-то можем написать в заголовок
    //glutSetWindowTitle( s );

}





//*****************************************************************************
void idle() {
    if ( !pause ) {
        glutPostRedisplay();
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
    switch(key) {
        case '=':
        case '+':
            time += 0.05;
            break;
        case '-':
        case '_':
            time -= 0.05;
            break;
        case ' ':
            pause = !pause;
            break;
        case '\033': // escape quits
        case '\015':// Enter quits    
        case 'Q':    // Q quits
        case 'q':    // q (or escape) quits
            exit( EXIT_SUCCESS );
            break;
    }
    glutPostRedisplay();
}







// Run a test sequence without any GL 
//*****************************************************************************
void TestNoGL() {
    // execute OpenCL kernel without GL interaction
    /* - Оставлено на закуску.

    pbo_cl = clCreateBuffer(context, CL_MEM_WRITE_ONLY, width * height * sizeof(GLubyte) * 4, NULL, &status);
    status |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&pbo_cl);
    status |= clSetKernelArg(kernel, 3, sizeof(unsigned int), &width);
    status |= clSetKernelArg(kernel, 4, sizeof(unsigned int), &height);   
    status |= clSetKernelArg(kernel, 5, sizeof(float), &w);

    // warm up
    int iCycles = 20;
    for (int i = 0; i < iCycles; i++)
    {
        status |= clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL, globalWorkSize, localWorkSize, 0,0,0 );
        oclCheckErrorEX(status, CL_SUCCESS, pcleanup);	
    }
    clFinish(commandQueue);
    
	// Start timer 0 and process n loops on the GPU 
	shrDeltaT(0); 
    for (int i = 0; i < iCycles; i++)
    {
        status |= clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL, globalWorkSize, localWorkSize, 0,0,0 );
        oclCheckErrorEX(status, CL_SUCCESS, pcleanup);	
    }
    clFinish(commandQueue);
    
    // Get elapsed time and throughput, then log to sample and master logs
    double dAvgTime = shrDeltaT(0)/(double)iCycles;
    shrLogEx(LOGBOTH | MASTER, 0, "oclSimpleTexture3D, Throughput = %.4f, Time = %.5f, Size = %u, NumDevsUsed = %u, Workgroup = %u\n", 
           (1.0e-6 * width * height)/dAvgTime, dAvgTime, (width * height), 1, (localWorkSize[0] * localWorkSize[1])); 

    // cleanup and exit
    cleanup(EXIT_SUCCESS);
    */
}
