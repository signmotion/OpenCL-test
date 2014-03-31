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




// �������������� ��������� ���� ������ ����� OpenCL/image2d.
// @see (!) http://developer.apple.com/library/mac/#documentation/Performance/Conceptual/OpenCL_MacProgGuide/ImageProcessingwithOpenCL/ImageProcessingwithOpenCL.html#//apple_ref/doc/uid/TP40008312-CH103-SW2
// @see http://www.cmsoft.com.br/index.php?option=com_content&view=category&layout=blog&id=115&Itemid=172
// @see http://www.khronos.org/message_boards/viewtopic.php?f=28&t=2358
// ��� ������ � ������������
//   > http://www.imagemagick.org/Magick++/Image.html
//   > http://labs.qt.nokia.com/2010/04/07/using-opencl-with-qt


// ��� ������� � ��������, �� ������� ����� �������������� ��������������
const float TIME = 1.0f;

// ������� ����� � ����
float time = 0.0f;


// ������ ����
const size_t N = 512;
const size_t M = 512;
const size_t NM = N * M;
// 1 ���� �������� 10 �����
const float scale = 1.0f / 10.0f;


// ��������� �����
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


// �������: ����� � ���� �������
bool pause = true;




// Forward Function declarations
//*****************************************************************************

// ������ ��� ������ � �����
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
    // ��� ����������� '.' ������ ','
    setlocale(LC_NUMERIC, "C");


    initGL(&argc, argv);


    cl_int status = CL_SUCCESS;

    // �������� ���������
    cl_platform_id platform;
    status = u::ocl::getPlatformID( &platform, "NVIDIA" );
    assert(status == CL_SUCCESS);

    // �������� ����������
    cl_uint numDevices;
    status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
    assert(status == CL_SUCCESS);
    //cl_device_id* devices = (cl_device_id *)malloc(numDevices * sizeof(cl_device_id) );
    cl_device_id* devices = new cl_device_id[ numDevices * sizeof(cl_device_id) ];
    assert(devices != nullptr);
    status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
    assert(status == CL_SUCCESS);

    //cout << getDeviceInfo( device );

    // ������ ��������
    context = clCreateContext(
        0, numDevices, devices, NULL, NULL, &status
    );
    assert(status == CL_SUCCESS);

    // �������� ����� ������ ����������
    cl_device_id device = u::ocl::getMaxFlopsDevice( context );

    // ������ ������� ������
    commandQueue = clCreateCommandQueue(context, device, 0, &status);
    assert(status == CL_SUCCESS);

    // �������� ����� ���������
    const string kernelName = "renderEntity";
    const string sourcePath =
        "D:/Projects/workspace/OpenCL-test/OpenCL-interaction/resource/OpenCL/"
      + kernelName + ".cl";
    size_t program_length;
    char* sourceCL = u::ocl::loadProgramSource(
        sourcePath.c_str(), "", &program_length
    );
    assert(sourceCL != nullptr);

    // ������ ���������
    cl_program program = clCreateProgramWithSource(
        context,
        1,
		(const char **) &sourceCL,
        &program_length,
        &status
    );
    assert(status == CL_SUCCESS);
    
    // ����������� ���������
    status = clBuildProgram(
        program, 0, NULL,
        // @todo optimize ��������� > "-cl-fast-relaxed-math"
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

    // ������ ����������� ����.
    // �������: ���� ��������� ������ ���������� �� ����� �����.
    kernel = clCreateKernel(program, kernelName.c_str(), &status);
    assert(status == CL_SUCCESS);


    // ������ ������ ����������� � ��������� �� � GLUT
    initGLBuffers();

    // ��������� ���
    //loadWorld();

    // ������ ����� ����
    createImageEntity();

    // ��������� �������� ���� ����������
    glutMainLoop();


    //TestNoGL();
    
    // ���������� ������: ������� �� �����
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
* ������ ������ ��� ������ � ������������.
*/
void initGLBuffers() {
    // ���������� ������ ������ �����������.
    //@todo optimize? ����������� ����� �������� http://www.songho.ca/opengl/gl_pbo.html#unpack
    // 1 ������� = 4 ������: RGBA, float
    const size_t V = 4 * sizeof(cl_float) * NM;

    // ������ Pixel Buffer Object (PBO)
    // @see http://www.songho.ca/opengl/gl_pbo.html
    // @see http://www.songho.ca/opengl/gl_vbo.html#create
    glGenBuffersARB(
        // ���������� �������
        1,
        // ������������� �������
        &pbo
    );
    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pbo );
    glBufferDataARB(
        // GLenum target
        GL_PIXEL_UNPACK_BUFFER_ARB,
        // GLsizei size, � ������
        V,
        // const void* data - ������ ��� �����������
        NULL,
        // GLenum usage
        // @todo optimize? http://www.songho.ca/opengl/gl_vbo.html
        GL_STREAM_DRAW_ARB
    );
    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, 0 );
    
    // � ����� �� - ����������� ����� � ������ ����������
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
* ������ �������� ����.
*/
void createWorld() {
#if 0
    /**
    * ��������� �������� ���������� � ����.
    * �������� ���� �������� � ���� 2D-�����������. ������ ����������� ��������
    * � ���� �����-�� ����� ����������.
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



    // ��� ������ � ������ ����������
    delete[] w1;

#endif
}





/**
* ����� ����.
* �������� ���� ��������� ������ � ���� ����� OpenCL.
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

    // ������ � ������ ���������� (����� �����������)
    // 1 ������� = 4 ������: RGBA, float
    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pbo );    
    // ����� ������ ������� - � ������ �������
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
    // ���� ����� �������� ����� ����� ���������.
    createImageEntity();
#endif


#if 0

    // ������� ������ �� ������ GL � CL
#ifdef GL_INTEROP
    // Acquire PBO for OpenCL writing
    status = clEnqueueAcquireGLObjects(commandQueue, 1, &pbo_cl, 0,0,0);
    assert(status == CL_SUCCESS);
#endif

    // ��������� ���� - '�������'
    status = clSetKernelArg( kernel, 5, sizeof(float), &w );
    assert(status == CL_SUCCESS);

    // ��������� ����, ���������� ��������� � Pixel Buffer Object (PBO)
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

    // ������ � ������ ���������� (����� �����������)
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


    // ���������� ���������
    
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

    // ����������� ����� �� �����
    // @todo ������������ ������������� � ���� ��������.
    glutSwapBuffers();
    glutError = glGetError();
    assert(glutError == 0);

    glFlush();

    //glutReportErrors();


    // ���-�� ����� �������� � ���������
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
    /* - ��������� �� �������.

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
