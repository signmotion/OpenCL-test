#include "StdAfx.h"


// Constants, defines, typedefs and global declarations
//*****************************************************************************

// Uncomment this #define to enable CL/GL Interop
//#define GL_INTEROP    

#define MAX_EPSILON_ERROR 5.0f
#define THRESHOLD         0.15f

const string sSDKsample = "oclSimpleTexture3D";

typedef unsigned int uint;
typedef unsigned char uchar;

const string volumeFilename = "Bucky.raw";
const size_t volumeSize[] = { 32, 32, 32 };
const uint width = 512, height = 512;
const size_t localWorkSize[] = { 16, 16 };
const size_t globalWorkSize[] = { width, height };

// OpenCL vars
cl_platform_id cpPlatform;
cl_uint uiNumDevices;
cl_device_id* cdDevices;
cl_context cxGPUContext;
cl_device_id cdDevice;
cl_command_queue cqCommandQueue;
cl_program cpProgram;
cl_kernel ckKernel;
cl_int status;
cl_mem pbo_cl;
cl_mem d_volume;
cl_sampler volumeSamplerLinear;
cl_sampler volumeSamplerNearest;
char* cPathAndName = NULL;          // var for full paths to data, src, etc.
char* cSourceCL = NULL;             // Buffer to hold source for compilation 

// Sim app config parameters
int iFrameCount = 0;                // FPS count for averaging
int iFrameTrigger = 90;             // FPS trigger for sampling
int iFramesPerSec = 0;              // frames per second
int iTestSets = 3;
float w = 0.5;                      // initial texture coordinate in z
int g_Index = 0;
//shrBOOL bQATest = shrFALSE;
//shrBOOL bNoPrompt = shrFALSE;
bool linearFiltering = true;
bool animate = true;

int iGLUTWindowHandle;              // handle to the GLUT window
GLuint pbo;                         // OpenGL pixel buffer object



// Forward Function declarations
//*****************************************************************************

// OpenCL functions
cl_mem initTexture3D(uchar *h_volume, const size_t volumeSize[3]);
void loadVolumeData(const string& pathAndName);
void render();
cl_mem initTexture3D(uchar *h_volume, const size_t volumeSize[3]);

// OpenGL functionality
void InitGL(int* argc, char** argv);
void DisplayGL();
void Reshape(int w, int h);
void Idle();
void KeyboardGL(unsigned char key, int x, int y);
void initGLBuffers();

// Helpers
void Cleanup(int iExitCode);
void (*pCleanup)(int) = &Cleanup;
void TestNoGL();



// Main program
//*****************************************************************************
int main(int argc, char** argv) {
    InitGL(&argc, argv); 

    // Получаем платформу
    status = u::ocl::getPlatformID( &cpPlatform, "NVIDIA" );
    assert(status == CL_SUCCESS);

    // Получаем устройства
    status = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 0, NULL, &uiNumDevices);
    assert(status == CL_SUCCESS);
    cdDevices = (cl_device_id *)malloc(uiNumDevices * sizeof(cl_device_id) );
    assert(cdDevices != nullptr);
    status = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, uiNumDevices, cdDevices, NULL);
    assert(status == CL_SUCCESS);

    //cout << getDeviceInfo( cdDevice );

    // Создаём контекст
    cxGPUContext = clCreateContext(0, uiNumDevices, cdDevices, NULL, NULL, &status);
    assert(status == CL_SUCCESS);

    // Выбираем самое мощное устройство
    cdDevice = u::ocl::getMaxFlopsDevice( cxGPUContext );

    // Строим очередь команд
    cqCommandQueue = clCreateCommandQueue(cxGPUContext, cdDevice, 0, &status);
    assert(status == CL_SUCCESS);

    // Получаем текст программы
    size_t program_length;
    const string pathAndName =
        "D:/Projects/workspace/OpenCL-test/OpenCL-test/resource/OpenCL/" + sSDKsample + "_kernel.cl";
    cSourceCL = u::ocl::loadProgramSource(pathAndName.c_str(), "", &program_length);
    assert(cSourceCL != nullptr);

    // Создаём программу
    cpProgram = clCreateProgramWithSource(
        cxGPUContext,
        1,
		(const char **) &cSourceCL,
        &program_length,
        &status
    );
    assert(status == CL_SUCCESS);
    
    // Компилируем программу
    status = clBuildProgram(
        cpProgram, 0, NULL, "-cl-fast-relaxed-math", NULL, NULL
    );
    if (status != CL_SUCCESS) {
        cerr << "clBuildProgram()" << endl;
        const auto firstDevice = u::ocl::getFirstDevice(cxGPUContext);
        u::ocl::logBuildInfo( cpProgram, firstDevice );
        u::ocl::logPtx( cpProgram, firstDevice, (sSDKsample + ".ptx").c_str() );
    }
    assert(status == CL_SUCCESS);

    // Создаём исполняемое ядро
    ckKernel = clCreateKernel(cpProgram, "render", &status);
    assert(status == CL_SUCCESS);


    // Загружаем данные
    const string pvFileName = 
        "D:/Projects/workspace/OpenCL-test/OpenCL-test/resource/data/" + volumeFilename;
    loadVolumeData( pvFileName );

    // Создаём буферы и текстуры и связываем их с GLUT
    initGLBuffers();

    // Запускаем основной цикл рендеринга
    glutMainLoop();

    //TestNoGL();
    
    // Завершение работы: убираем за собой
    Cleanup(EXIT_FAILURE);
}






// Initialize GL
//*****************************************************************************
void InitGL(int* argc, char **argv ) {
    // init GLUT 
    glutInit(argc, (char**)argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowPosition (
        glutGet(GLUT_SCREEN_WIDTH)/2 - width/2, 
        glutGet(GLUT_SCREEN_HEIGHT)/2 - height/2
    );
    glutInitWindowSize(width, height);
    iGLUTWindowHandle = glutCreateWindow("Interaction");

    // register GLUT callback functions
    glutDisplayFunc(DisplayGL);
    glutKeyboardFunc(KeyboardGL);
    glutReshapeFunc(Reshape);
    glutIdleFunc(Idle);

    // init GLEW
    glewInit();
    GLboolean bGLEW = glewIsSupported(
        "GL_VERSION_2_0 GL_ARB_pixel_buffer_object"
    );
    assert( bGLEW );
}






// render image using OpenCL    
//*****************************************************************************
void render() {
    status = CL_SUCCESS;

    // Transfer ownership of buffer from GL to CL
#ifdef GL_INTEROP
    // Acquire PBO for OpenCL writing
    status |= clEnqueueAcquireGLObjects(cqCommandQueue, 1, &pbo_cl, 0,0,0);
    assert(status == CL_SUCCESS);
#endif    

    // Аргументы ядра
    status |= clSetKernelArg(ckKernel, 5, sizeof(float), &w);
    assert(status == CL_SUCCESS);

    // Запускаем ядро, записываем результат в Pixel Buffer Object (PBO)
    // @see http://www.songho.ca/opengl/gl_pbo.html
    status |= clEnqueueNDRangeKernel(
        cqCommandQueue, ckKernel,
        2, NULL,
        globalWorkSize, localWorkSize,
        0, 0, 0
    );
    assert(status == CL_SUCCESS);

#ifdef GL_INTEROP
    // Transfer ownership of buffer back from CL to GL    
    status |= clEnqueueReleaseGLObjects(cqCommandQueue, 1, &pbo_cl, 0, 0, 0);
    assert(status == CL_SUCCESS);
#else

    // Прямое копирование
    // Карта PBO копируется из OpenCL буфера через хост
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);    

    // Карта буфера объекта - в память клиента
    GLubyte* ptr = (GLubyte*)glMapBufferARB(
        GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB
    );
    status |= clEnqueueReadBuffer(
        cqCommandQueue,
        pbo_cl,
        CL_TRUE,
        0,
        sizeof(unsigned int) * height * width,
        ptr,
        0, NULL, NULL
    );
    assert(status == CL_SUCCESS);

    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
#endif
}





// Display callback for GLUT main loop
//*****************************************************************************
void DisplayGL() {
    // Запускаем ядро OpenCL
    render();

    // Показываем результат
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glRasterPos2i(0, 0);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
    glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

    // Переключаем буфер на экран
    glutSwapBuffers();
    glutReportErrors();

    // Что-то можем написать
    //glutSetWindowTitle( s ); 
}





//*****************************************************************************
void Idle() {
    if (animate) {
        w += 0.001f;
		if (w > 1.0f) {
			w = 0.0f;
        }
        glutPostRedisplay();
    }
}





// Window resize handler callback
//*****************************************************************************
void Reshape(int x, int y) {
    glViewport(0, 0, x, y);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, 1.0); 
}





// Keyboard event handler callback
//*****************************************************************************
void KeyboardGL(unsigned char key, int x, int y) {
    switch(key) {
        case '=':
        case '+':
            w += 0.01;
            break;
        case '-':
        case '_':
            w -= 0.01;
            break;
        case 'F':
        case 'f':
            linearFiltering = !linearFiltering;
            status = clSetKernelArg(
                ckKernel, 1, sizeof(cl_sampler),
                linearFiltering ? &volumeSamplerLinear : &volumeSamplerNearest
            );
            printf("\nLinear Filtering Toggled %s...\n", linearFiltering ? "ON" : "OFF");
            assert(status == CL_SUCCESS);
            break;
        case ' ':
            animate = !animate;
            break;
        case '\033': // escape quits
        case '\015':// Enter quits    
        case 'Q':    // Q quits
        case 'q':    // q (or escape) quits
            // Cleanup up and quit
            Cleanup(EXIT_SUCCESS);
            break;
    }
    glutPostRedisplay();
}





//*****************************************************************************
void initGLBuffers() {
    // Создаём Pixel Buffer Object (PBO)
    // @see http://www.songho.ca/opengl/gl_pbo.html
    glGenBuffersARB(1, &pbo);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
    glBufferDataARB(
        GL_PIXEL_UNPACK_BUFFER_ARB,
        width*height*sizeof(GLubyte)*4,
        0,
        GL_STREAM_DRAW_ARB
    );
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

#ifdef GL_INTEROP
    // create OpenCL buffer from GL PBO
    pbo_cl = clCreateFromGLBuffer(cxGPUContext,CL_MEM_WRITE_ONLY, pbo, &status);
#else            
    pbo_cl = clCreateBuffer(
        cxGPUContext, 
        CL_MEM_WRITE_ONLY,  
        width*height*sizeof(GLubyte)*4, 
        NULL, 
        &status
    );
#endif
    assert(status == CL_SUCCESS);

    status |= clSetKernelArg(ckKernel, 2, sizeof(cl_mem), (void*) &pbo_cl);
    status |= clSetKernelArg(ckKernel, 3, sizeof(unsigned int), &width);
    status |= clSetKernelArg(ckKernel, 4, sizeof(unsigned int), &height);
    assert(status == CL_SUCCESS);
}





//*****************************************************************************
cl_mem initTexture3D(uchar *h_volume, const size_t volumeSize[3]) {
    cl_int status;

    // create 3D array and copy data to device
	cl_image_format volume_format;
    volume_format.image_channel_order = CL_RGBA;
    volume_format.image_channel_data_type = CL_UNORM_INT8;
    uchar* h_tempVolume =(uchar*)malloc(volumeSize[0] * volumeSize[1] * volumeSize[2] * 4);
    for(unsigned int i = 0; i < (volumeSize[0] * volumeSize[1] * volumeSize[2]); i++) {
        h_tempVolume[4 * i] = h_volume[i];
    }
    cl_mem d_volume = clCreateImage3D(
        cxGPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &volume_format, 
        volumeSize[0], volumeSize[1], volumeSize[2],
        (volumeSize[0] * 4), (volumeSize[0] * volumeSize[1] * 4),
        h_tempVolume, &status
    );
    assert(status == CL_SUCCESS);
    free (h_tempVolume);

    return d_volume;
}





//*****************************************************************************
void loadVolumeData(const string& pathAndName) {
    size_t size = volumeSize[0] * volumeSize[1] * volumeSize[2];
    unsigned char* h_volume = u::loadRawFile(pathAndName, size);
    assert(h_volume != nullptr);

    // Устанавливаем 3D-изображение
    d_volume = initTexture3D(h_volume, volumeSize);
    status = clSetKernelArg(ckKernel, 0, sizeof(cl_mem), &d_volume);
    assert(status == CL_SUCCESS);

    // Создаём образцы изображения для линейной и ближайшей интерполяции
    volumeSamplerLinear = clCreateSampler(
        cxGPUContext, true, CL_ADDRESS_REPEAT, CL_FILTER_LINEAR, &status
    );
    assert(status == CL_SUCCESS);

    volumeSamplerNearest = clCreateSampler(
        cxGPUContext, true, CL_ADDRESS_REPEAT, CL_FILTER_NEAREST, &status
    );
    assert(status == CL_SUCCESS);

    status = clSetKernelArg(
        ckKernel, 1, sizeof(cl_sampler), &volumeSamplerLinear
    );        
    assert(status == CL_SUCCESS);

    free(h_volume);
}






// Run a test sequence without any GL 
//*****************************************************************************
void TestNoGL() {
    // execute OpenCL kernel without GL interaction
    /* - Оставлено на закуску.

    pbo_cl = clCreateBuffer(cxGPUContext, CL_MEM_WRITE_ONLY, width * height * sizeof(GLubyte) * 4, NULL, &status);
    status |= clSetKernelArg(ckKernel, 2, sizeof(cl_mem), (void *)&pbo_cl);
    status |= clSetKernelArg(ckKernel, 3, sizeof(unsigned int), &width);
    status |= clSetKernelArg(ckKernel, 4, sizeof(unsigned int), &height);   
    status |= clSetKernelArg(ckKernel, 5, sizeof(float), &w);

    // warm up
    int iCycles = 20;
    for (int i = 0; i < iCycles; i++)
    {
        status |= clEnqueueNDRangeKernel(cqCommandQueue, ckKernel, 2, NULL, globalWorkSize, localWorkSize, 0,0,0 );
        oclCheckErrorEX(status, CL_SUCCESS, pCleanup);	
    }
    clFinish(cqCommandQueue);
    
	// Start timer 0 and process n loops on the GPU 
	shrDeltaT(0); 
    for (int i = 0; i < iCycles; i++)
    {
        status |= clEnqueueNDRangeKernel(cqCommandQueue, ckKernel, 2, NULL, globalWorkSize, localWorkSize, 0,0,0 );
        oclCheckErrorEX(status, CL_SUCCESS, pCleanup);	
    }
    clFinish(cqCommandQueue);
    
    // Get elapsed time and throughput, then log to sample and master logs
    double dAvgTime = shrDeltaT(0)/(double)iCycles;
    shrLogEx(LOGBOTH | MASTER, 0, "oclSimpleTexture3D, Throughput = %.4f, Time = %.5f, Size = %u, NumDevsUsed = %u, Workgroup = %u\n", 
           (1.0e-6 * width * height)/dAvgTime, dAvgTime, (width * height), 1, (localWorkSize[0] * localWorkSize[1])); 

    // Cleanup and exit
    Cleanup(EXIT_SUCCESS);
    */
}





// Helper to clean up
//*****************************************************************************
void Cleanup(int iExitCode) {
    // Cleanup allocated objects
    if(cPathAndName)free(cPathAndName);
    if(cSourceCL)free(cSourceCL);
	if(ckKernel)clReleaseKernel(ckKernel); 
    if(cpProgram)clReleaseProgram(cpProgram);
    if(cqCommandQueue)clReleaseCommandQueue(cqCommandQueue);
    if(cxGPUContext)clReleaseContext(cxGPUContext);
    if(volumeSamplerLinear)clReleaseSampler(volumeSamplerLinear);
    if(volumeSamplerNearest)clReleaseSampler(volumeSamplerNearest);
    if(pbo_cl)clReleaseMemObject(pbo_cl);
    if(iGLUTWindowHandle)glutDestroyWindow(iGLUTWindowHandle);

    #ifdef WIN32
        getchar();
    #endif

    exit( iExitCode );
}
