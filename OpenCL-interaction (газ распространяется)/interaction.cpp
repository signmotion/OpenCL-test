#include "StdAfx.h"
#include "struct.h"
#include "const.h"

// �������������� ��������� ���� ������ ����� OpenCL/image2d.
// @see (!) http://developer.apple.com/library/mac/#documentation/Performance/Conceptual/OpenCL_MacProgGuide/ImageProcessingwithOpenCL/ImageProcessingwithOpenCL.html#//apple_ref/doc/uid/TP40008312-CH103-SW2
// @see http://www.cmsoft.com.br/index.php?option=com_content&view=category&layout=blog&id=115&Itemid=172
// @see http://www.khronos.org/message_boards/viewtopic.php?f=28&t=2358
// ��� ������ � ������������
//   > http://www.imagemagick.org/Magick++/Image.html
//   > http://labs.qt.nokia.com/2010/04/07/using-opencl-with-qt


// ��� ������� � ��������, �� ������� ����� �������������� ��������������
const float dt = 1.0f;

// ������� ����� � ����
float time = 0.0f;

// ����� ����� ��������� ���� � ����. ���������
const unsigned long TICK_PAUSE = 1000;
// ��������������� ���������� ��� ��������� �����
unsigned long  tickPause = 0;


// ������ ����.
const size_t N = 512;
const size_t M = 512 / 2;
const size_t NM = N * M;
// 1 ���� �������� 10 �����
const float scale = 1.0f / 10.0f;


// ��������� �����
const size_t GRID_WIDTH = N;
const size_t GRID_HEIGHT = M;
const size_t GRID_WORK_DIM = 2;
const size_t GRID_GLOBAL_WORK_SIZE[] = { GRID_WIDTH, GRID_HEIGHT };
const size_t GRID_LOCAL_WORK_SIZE[] = { 16, 16 / 2 };


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
void createWorld();
void createImageMatter();


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
    const string kernelName = "render";
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
    ostringstream ss;
    ss << " -D N=" << N;
    ss << " -D M=" << M;
    string options = " \
        -I D:/Projects/workspace/OpenCL-test/OpenCL-interaction/resource/OpenCL \
        -cl-opt-disable \
        -Werror \
    " + ss.str();
    //options = "-g";
    status = clBuildProgram(
        program, 0, NULL,
        // @todo optimize ��������� > "-cl-fast-relaxed-math"
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

    // ������ ����������� ����.
    // �������: ���� ��������� ������ ���������� �� ����� �����.
    kernel = clCreateKernel(program, kernelName.c_str(), &status);
    assert(status == CL_SUCCESS);


    // ������ ������ ����������� � ��������� �� � GLUT
    initGLBuffers();

    // ��������� ���
    createWorld();

    // ������ ����� ����
    createImageMatter();

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

    cout << "initGLBuffers()" << endl;

    // ���������� ������ ������ �����������.
    // 1 ������� = 4 ������: RGBA, float
    const size_t V = 4 * sizeof(cl_float) * NM;

    //@todo optimize? �������� �������� ��� ���������� �����
    //      �������� image2d_t > http://www.songho.ca/opengl/gl_pbo.html#unpack

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
    
    // � ����� �� - ����������� ����� � ������ OpenCL
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
    assert(status == CL_SUCCESS);


    // ������ ������ ��� �������� ������������� ������
    cl_mem oclS1uOut = clCreateBuffer(
        context, CL_MEM_WRITE_ONLY,
        sizeof(DataS1U) * NM, NULL,
        &status
    );
    assert(status == CL_SUCCESS);

    cl_mem oclS4uOut = clCreateBuffer(
        context, CL_MEM_WRITE_ONLY,
        sizeof(DataS4U) * NM, NULL,
        &status
    );
    assert(status == CL_SUCCESS);

    cl_mem oclS4fOut = clCreateBuffer(
        context, CL_MEM_WRITE_ONLY,
        sizeof(DataS4F) * NM, NULL,
        &status
    );
    assert(status == CL_SUCCESS);

    cl_mem oclW8fOut = clCreateBuffer(
        context, CL_MEM_WRITE_ONLY,
        sizeof(DataW8F) * NM, NULL,
        &status
    );
    assert(status == CL_SUCCESS);

    /* - ��������� ��� ��������� �������.
    cl_mem oclTempSU = clCreateBuffer(
        context, CL_MEM_WRITE_ONLY,
        sizeof(DataTempSU) * NM, NULL,
        &status
    );
    assert(status == CL_SUCCESS);

    cl_mem oclTempSF = clCreateBuffer(
        context, CL_MEM_WRITE_ONLY,
        sizeof(DataTempSF) * NM, NULL,
        &status
    );
    assert(status == CL_SUCCESS);

    cl_mem oclTempWU = clCreateBuffer(
        context, CL_MEM_WRITE_ONLY,
        sizeof(DataTempWU) * NM, NULL,
        &status
    );
    assert(status == CL_SUCCESS);

    cl_mem oclTempWF = clCreateBuffer(
        context, CL_MEM_WRITE_ONLY,
        sizeof(DataTempWF) * NM, NULL,
        &status
    );
    assert(status == CL_SUCCESS);
    */

    status |= clSetKernelArg( kernel, 7, sizeof(cl_mem), (void*)&oclS1uOut );
    status |= clSetKernelArg( kernel, 8, sizeof(cl_mem), (void*)&oclS4uOut );
    status |= clSetKernelArg( kernel, 9, sizeof(cl_mem), (void*)&oclS4fOut );
    status |= clSetKernelArg( kernel, 10, sizeof(cl_mem), (void*)&oclW8fOut );
    /* - ���������.
    status |= clSetKernelArg( kernel, 11, sizeof(cl_mem), (void*)&oclTempSU );
    status |= clSetKernelArg( kernel, 12, sizeof(cl_mem), (void*)&oclTempSF );
    status |= clSetKernelArg( kernel, 13, sizeof(cl_mem), (void*)&oclTempWU );
    status |= clSetKernelArg( kernel, 14, sizeof(cl_mem), (void*)&oclTempWF );
    */
    assert(status == CL_SUCCESS);

}







/**
* ������ �������� ����.
*/
void createWorld() {

    cout << "createWorld()" << endl;

    cout << "������ �������: " << N << " x " << M << " = " << NM << " �����" << endl;

    /**
    * ��������� ������ �������.
    * ��������������� ��������.
    */
    typedef struct {
        // ID �������
        // cl_uchar id; - ������ � ����� ������.
        
        // ���� �������
        cl_uint color;

        // ���������� ���������
        // cl_uchar state; - ������ � ����� ������.

        // ������� ���������� � ���� ���������� ���������.
        // transition temperature
        // ����������� �������� ������� ��������� ����� ����������� [1; 3]:
        // S | L | G | P
        // ������ (�������� 4) �� ����� 'tt'.
        cl_float tt;

        // �������������� �������� ����� ���. ���������.
        cl_float density;

    } Matter;

    // �������.
    // ������ � ���� ������, ����� ��������� ���������� �������
    // ��� ��������� ����. ������ - ����������, ����� �� ���������
    // �������� ��� �������� ��� OpenCL.
    const int MAX_MATTER_ID = 255;
    const int COUNT_PSTATE = 4;
    auto md = new Matter[ (MAX_MATTER_ID + 1) * COUNT_PSTATE ];

    cout << "������ ��� Matter: " << sizeof(Matter) * (MAX_MATTER_ID + 1) * COUNT_PSTATE << " ����" << endl;

    cl_uchar id = 0;
    cl_uchar state = 0;
    cl_uchar i = 0;

    // ������
    id = 1;

    // solid
    state = 1;
    i = (id - 1) * COUNT_PSTATE + (state - 1);
    md[i].color = 0x1e90ff80;
    md[i].tt = -214.0f;
    md[i].density = 1200.0f;
    
    // liquid
    state = 2;
    i = (id - 1) * COUNT_PSTATE + (state - 1);
    md[i].color = 0x42aaff80;
    md[i].tt = -192.0f;
    md[i].density = 960.0f;

    // gas
    state = 3;
    i = (id - 1) * COUNT_PSTATE + (state - 1);
    md[i].color = 0x7fc7ff80;
    md[i].tt = 100000.0f;
    md[i].density = 1.293f;

    // plasma
    state = 4;
    i = (id - 1) * COUNT_PSTATE + (state - 1);
    md[i].color = 0xe3263680;
    // NAN ������ ������������, �.�. OpenCL ����� �� ������ ��������
    // "������� ����������" (�������������� ������������ GPU).
    md[i].tt = 0.0f;
    md[i].density = md[ (id - 2) * COUNT_PSTATE + (state - 1) ].density / 1e6;


    //EData water = { 2, 0.0f };
    //EData soil = { 3, 0.0f };
    //EData sand = { 4, 1500.0f };
    //EData rock = { 5, 0.0f };



    
    // ��������� ���.
    // ��� ����� �������� N x M: 1 ������ = 1 ������� �������.
    // ���� �� ������� ��������; �������� � ������ ��������.

    // ���������� ������������ ��� ������������ ��������/�������
    auto s1u = new DataS1U[NM];
    auto s4u = new DataS4U[NM];
    auto s4f = new DataS4F[NM];
    auto w8f = new DataW8F[NM];

    cout << "������ ��� DataS1U: " << sizeof(DataS1U) * NM / 1024.0f / 1024.0f << " ��" << endl;
    cout << "������ ��� DataS4U: " << sizeof(DataS4U) * NM / 1024.0f / 1024.0f << " ��" << endl;
    cout << "������ ��� DataS4F: " << sizeof(DataS4F) * NM / 1024.0f / 1024.0f << " ��" << endl;
    cout << "������ ��� DataW8F: " << sizeof(DataW8F) * NM / 1024.0f / 1024.0f << " ��" << endl;

    // �� ��� ������ ������� ������ ��������� ���� �������
    const float DEFAULT_FORCE[] = { 0.0f, -9.81f };

    // �������������� ���� ��� ��������.
    // ���������� ��� �������� ���������, ����� ���� ���������� � �� ���������.
    id = 0;
    for (size_t z = 0; z < M; z++) {
        for (size_t x = 0; x < N; x++) {
            // ���������� ������ � ���������� ������������
            const size_t i = x + z * N;

            // ���������� ������ ��������
            s1u[i].matter = id;
            
            s4u[i].color = 0x00000000;

            s4f[i].temperature = 0.0f;
            s4f[i].density = 0.0f;
            s4f[i].pressure = 0.0f;

            // ���� ���������� ��������� �� ������ ������
            w8f[i].force.s[0] = DEFAULT_FORCE[0];
            w8f[i].force.s[1] = DEFAULT_FORCE[1];

            w8f[i].speed.s[0] = 0.0f;
            w8f[i].speed.s[1] = 0.0f;
        }
    }


    // ������ ���� �������
    const size_t AIR_BLOCK_WIDTH = 200;
    const size_t AIR_BLOCK_HEIGHT = 100;
    id = 1;
    const float molarMass = 29.0f / 1000.0f;
    for (size_t z = M / 4; z < M / 4 + AIR_BLOCK_HEIGHT; z++) {
        for (size_t x = N / 4; x < N / 4 + AIR_BLOCK_WIDTH; x++) {
            // ���������� ������ � ���������� ������������
            const size_t i = x + z * N;
            // ��������� �� ���������� �� "������������� ���������"
            const size_t iMatter = (id - 1) * COUNT_PSTATE + (LIQUID - 1);            

            // ���������� ������ ���������� �� ���������� �������
            s1u[i].matter = id;
            
            // ������ color �����, ����� �������������� ������ � ����� �������
            // �� ������� (��������) ����������� ����� � ��� Kernel.
            s4u[i].color = md[iMatter].color;

            s4f[i].temperature = 0.0f;
            s4f[i].density = md[iMatter].density;
            // p = D / M * R * T
            // ��� � - �������� ����� �������� (������ = 29/1000 ��/����)
            // R - ������� ����������
            // D - ���������
            s4f[i].pressure = md[iMatter].density / molarMass * R * (-C0);

            /* - ��������� - �� ��������� (���������� ����).
            w8f[i].force.s[0] = DEFAULT_FORCE[0];
            w8f[i].force.s[1] = DEFAULT_FORCE[1];
            .....
            */
        }
    }

    
    // ��������� ���������� � ������� � ���������� ���� � ������ OpenCL.
    // ����������� ����� � ������ OpenCL.

    cl_int status;

    cl_mem oclS1u = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(DataS1U) * NM, s1u,
        &status
    );
    assert(status == CL_SUCCESS);

    cl_mem oclS4u = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(DataS4U) * NM, s4u,
        &status
    );
    assert(status == CL_SUCCESS);

    cl_mem oclS4f = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(DataS4F) * NM, s4f,
        &status
    );
    assert(status == CL_SUCCESS);

    cl_mem oclW8f = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(DataW8F) * NM, w8f,
        &status
    );
    assert(status == CL_SUCCESS);

    status |= clSetKernelArg( kernel, 1, sizeof(cl_mem), (void*)&oclS1u );
    status |= clSetKernelArg( kernel, 2, sizeof(cl_mem), (void*)&oclS4u );
    status |= clSetKernelArg( kernel, 3, sizeof(cl_mem), (void*)&oclS4f );
    status |= clSetKernelArg( kernel, 4, sizeof(cl_mem), (void*)&oclW8f );
    assert(status == CL_SUCCESS);




    // ��� ������ � ������ ���������� OpenCL
    delete[] s1u;
    delete[] s4u;
    delete[] s4f;
    delete[] w8f;

    delete[] md;

}






/**
* ����� ����.
* �������� ���� ��������� ������ � ���� ����� OpenCL.
*/
void createImageMatter() {

    // ������� ���������� ������� �������, ��� ������������ ������ �����,
    // ��� ������� ��������������.
    const float dt0 = 0.0f;
    cl_int status = clSetKernelArg( kernel, 5, sizeof(cl_float), &dt0 );
    const cl_uint r0 = 0;
    status |= clSetKernelArg( kernel, 6, sizeof(cl_float), &r0 );
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


    //cout << "createImageMatter() " << endl;
}





/**
* ������ ��������������: ����.
* �������������� ����� OpenCL.
*/
void interactionGas(float dt) {

    cl_int status = clSetKernelArg( kernel, 5, sizeof(cl_float), &dt );
    const cl_uint r = rand();
    status |= clSetKernelArg( kernel, 6, sizeof(cl_uint), &r );
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


    //cout << "interactionGas() " << endl;
}









// render image using OpenCL    
//*****************************************************************************
void render() {
    
    // �� ���������� ����� ��� ����
    const unsigned long tickCurrent = GetTickCount();
    if ( (tickCurrent - tickPause) < TICK_PAUSE ) {
        //return;
    }
    tickPause = tickCurrent;


    // ������������ �������������� �������
    interactionGas( dt );

    // ��������� ����� ����� ����
    createImageMatter();

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

    
    // ����� ����� ���-�� ���������
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

    //glScaled(2.0, 2.0, 2.0);

    glFlush();

    //glutReportErrors();


    // ���-�� ����� �������� � ���������
    //glutSetWindowTitle( s );

}





//*****************************************************************************
void idle() {
    if ( !pause ) {
        time += dt;
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
            time += dt;
            break;
        case '-':
        case '_':
            time -= dt;
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
