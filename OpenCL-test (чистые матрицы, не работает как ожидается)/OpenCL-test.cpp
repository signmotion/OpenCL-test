//#define _CRT_SECURE_NO_DEPRECATE

#include <iostream>
#include <fstream>
#include <sstream>
#include "opencl.h"
#include <oclUtils.h>

//#include "halffloat.h"


// Максимальный размер файла OpenCL
#define MAX_SOURCE_SIZE_FILE_CL (0x100000)


using namespace std;


/**
* Игра с точностью вычислений.
*/
void forces();


/**
* Сложение векторов.
*/
void vectorAdd();




// Main function  
// ********************************************************************* 
int main(int argc, char **argv)
{ 
	setlocale(LC_ALL, "Russian");
    // Для разделителя '.' вместо ','
    setlocale(LC_NUMERIC, "C");


    //vectorAdd();

    forces();

    return 0;
}










void forces() {

    // Шаг времени в секундах, за которое будут рассчитываться взаимодействия
    const float TIME = 1.0;

    // Размер мира
    const cl_uint N = 5 * 10;
    const cl_uint M = 3 * 10;
    const cl_uint NM = N * M;
    const cl_uint2 SIZE = { N, M };
    // 1 метр содержит 10 ячеек
    const cl_float scale = 1.0f / 10.0f;

    // Сущности системы
    typedef struct {
        cl_uint id;
        cl_float density;
    } EData;
    cout << "sizeof(EData) " << sizeof(EData) << endl;

    // Песок
    EData sand = { 4, 1500.0f };

    const int ENTITY_COUNT = 1;
    EData* ed = new EData[ENTITY_COUNT];
    ed[0] = sand;


    /**
    * Структура для расчёта взаимодействия сущностей.
    * @see OpenCL/interaction.cl
    * @see http://www.khronos.org/registry/cl/sdk/1.0/docs/man/xhtml/restrictions.html
    */
    typedef struct {
        // @todo optimize Для entity достаточно uchar. uint - чтобы можно было записывать.
        cl_uint entity;
        // @todo optimize Не получилось реализовать через cl_float2.
        //       OpenCL ожидает типы одинаковой длины, кратные 32?
        //cl_float2 force;
        cl_float forceX;
        cl_float forceZ;
        //cl_float2 speed;
        cl_float speedX;
        cl_float speedZ;
        cl_float density;
    } WData;
    cout << "sizeof(WData) " << sizeof(WData) << endl;

    WData* wd = new WData[NM];


    // На все ячейки системы всегда действует сила тяжести
    const float defaultForce[] = { 0.0f, -9.81f };

    // Сущности системы заданы матрицей N x M. Но хранятся в
    // одномерной матрице, чтобы уверенно передавать их в OpenCL.
    // Сущности хранятся в ячейках: 1 ячейка = 1 сущность.
    // Там же храним некоторые характеристики сущностей.
    /*
    cl_uchar entity[NM] = {};
    cl_float2 force[NM] = {};
    cl_float density[NM] = {};
    */
    for(int z = 0; z < M; z++) {
        for(int x = 0; x < N; x++) {
            const int i = x + z * M;
            wd[i].entity = (cl_uint)sand.id;
            //cout << wd[i].entity << "\t";
            wd[i].forceX = defaultForce[0];
            wd[i].forceZ = defaultForce[1];
            wd[i].speedX = wd[i].speedZ = 0.0f;
            wd[i].density = sand.density;
        }
        //cout << endl;
    }



    cl_int status = CL_SUCCESS;

    cl_uint numPlatforms;
    cl_platform_id platform = NULL;
    status = clGetPlatformIDs(0, NULL, &numPlatforms);
    cout << "Количество платформ: " << numPlatforms << endl;
    if (numPlatforms < 1) {
        cout << "Не найдено доступных платформ." << endl;
    }
    assert(status == CL_SUCCESS);

    cl_platform_id* platforms = new cl_platform_id[numPlatforms];
    status = clGetPlatformIDs(numPlatforms, platforms, NULL);
    if (status != CL_SUCCESS) {
        cout << "clGetPlatformIDs() failed." << endl;
    }
    assert(status == CL_SUCCESS);

    // Проходимся по всем обнаруженным платформам и выбираем подходящую
    for (unsigned i = 0; i < numPlatforms; ++i) {
        cout << "Платформа ID: " << platforms[i] << endl;
        char pbuf[100];
        status = clGetPlatformInfo(
            platforms[i],
            CL_PLATFORM_VENDOR,
            sizeof(pbuf),
            pbuf,
            NULL
        );
        assert(status == CL_SUCCESS);
        cout << "Платформа info: " << pbuf << endl;

        if ( !strcmp(pbuf, "NVIDIA Corporation") ) {
            cout << "Выбираем платформу '" << pbuf << "'" << endl;
            platform = platforms[i];
            break;
        }
    }
    delete[] platforms;

    if (platform == nullptr) {
        cout << "Подходящая платформа не найдена." << endl;
    }
    assert(platform != nullptr);


    // Работаем с найденной выше платформой
    const cl_device_type dType = CL_DEVICE_TYPE_GPU;
    cl_context_properties cps[3] = {
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)platform, 
        0
    };
    cl_context context = clCreateContextFromType(
        cps, dType, NULL, NULL, &status
    );

    if (platform == NULL) {
        cout << "Подходящая платформа не найдена." << endl;
    }
    assert(platform != NULL);


    // Get the list of GPU devices associated with this context 
    size_t ParmDataBytes;
    clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &ParmDataBytes);
    cl_device_id* GPUDevices = (cl_device_id*)malloc(ParmDataBytes);
    clGetContextInfo(context, CL_CONTEXT_DEVICES, ParmDataBytes, GPUDevices, NULL);

    // Create a command-queue on the first GPU device
    cl_command_queue GPUCommandQueue = clCreateCommandQueue(
        context, GPUDevices[0], 0, NULL
    );

 
    // Выделяем память в устройстве, инициируем её значениями
    cout << "Размер матрицы: " << N << " x " << M << " = " << NM << " ячеек" << endl;
    cout << "Память для WData: " << sizeof(WData) * NM << " байт" << endl;
    cl_mem gpuWData = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(WData) * NM, wd, NULL
    );
    cout << "Память для EData: " << sizeof(EData) * NM << " байт" << endl;
    cl_mem gpuEData = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(EData) * ENTITY_COUNT, ed, NULL
    );
 
    // Место для формирования нового снимка мира
    cl_mem gpuWDataNew = clCreateBuffer(
        context, CL_MEM_WRITE_ONLY,
        sizeof(WData) * NM, NULL, NULL
    );
  

    const char* sourcePath =
        "D:/Projects/workspace/OpenCL-test/OpenCL-test/resource/OpenCL/interaction.cl";
    FILE* fp = nullptr;
    errno_t error = fopen_s(&fp, sourcePath, "r");
    assert(error == 0);
    assert( fp );
    char* source_str = (char*)malloc(MAX_SOURCE_SIZE_FILE_CL);
    size_t source_size = fread( source_str, 1, MAX_SOURCE_SIZE_FILE_CL, fp );
    fclose( fp );

    cl_program OpenCLProgram = clCreateProgramWithSource(
        context,
        1,
        (const char **)&source_str,
        (const size_t *)&source_size,
        NULL
    );
    assert(OpenCLProgram != nullptr);
    assert(status == CL_SUCCESS);


    // Build the program (OpenCL JIT compilation)
    clBuildProgram(OpenCLProgram, 0, NULL, NULL, NULL, NULL); 
 
    // Create a handle to the compiled OpenCL function (Kernel) 
    cl_kernel openCLInteraction = clCreateKernel(OpenCLProgram, "interaction", &status);
    assert(status == CL_SUCCESS);
 
    // Передаём устройству аргументы (см. interaction.cl)
    clSetKernelArg(openCLInteraction, 0, sizeof(cl_mem), (void*)&gpuWDataNew); 
    clSetKernelArg(openCLInteraction, 1, sizeof(cl_mem), (void*)&gpuWData); 
    clSetKernelArg(openCLInteraction, 2, sizeof(cl_mem), (void*)&gpuEData); 
    clSetKernelArg(openCLInteraction, 3, sizeof(cl_float), (void*)&scale); 
    clSetKernelArg(openCLInteraction, 4, sizeof(cl_uint2), (void*)&SIZE); 
    clSetKernelArg(openCLInteraction, 5, sizeof(cl_float), (void*)&TIME); 
 
    // Запускаем ядро на устройстве
    size_t globalWorkSize[1] = { NM };
    //size_t localWorkSize[2] = { 16, 16 };
    //size_t globalWorkSize[2] = { 1024, 1024 };
    clEnqueueNDRangeKernel(
        // commanr_queue
        GPUCommandQueue,
        // kernel
        openCLInteraction,
        // work_dim
        1,
        // global_work_offset
        NULL,
        // global_work_size
        globalWorkSize,
        // local_work_size
        NULL,
        // num_events_in_wait_list
        0,
        // event_wait_list
        NULL,
        // event
        NULL
    ); 
  
    // Возвращаем результат хосту
    clEnqueueReadBuffer(
        GPUCommandQueue, gpuWDataNew, CL_TRUE, 0, 
        sizeof(WData) * NM, wd, 0, NULL, NULL
    );


    // Смотрим
    cout << "Готово." << endl << endl;
    for(int z = 0; z < M; z++) {
        for(int x = 0; x < N; x++) {
            const int i = x + z * M;
            const auto w = wd[i];

            //stringstream ss;
            //ss << w.entity;
            //cout << ss << "\t";
            cout << w.entity << "\t";
            //cout << w.force.s[0] << ", " << w.force.s[0] << "\t";
        }
        cout << endl;
    }


    // Cleanup
    free(GPUDevices);
    clReleaseKernel(openCLInteraction);
    clReleaseProgram(OpenCLProgram);
    clReleaseCommandQueue(GPUCommandQueue);
    clReleaseContext(context);
    clReleaseMemObject(gpuWDataNew);
    clReleaseMemObject(gpuEData);
    clReleaseMemObject(gpuWData);
 
    delete[] ed;
    delete[] wd;

}










void vectorAdd() {
    // OpenCL source code 
    /* - Перенесено в файл.
    const char* OpenCLSource[] = {  
    "__kernel void VectorAdd(__global int* c, __global int* a,__global int* b)", 
      "{", 
      " // Index of the elements to add \n", 
      " unsigned int n = get_global_id(0);", 
      " // Sum the n’th element of vectors a and b and store in c \n", 
      " c[n] = a[n] + b[n];", 
      "}" 
    };
    */


 
    // Some interesting data for the vectors 
    int InitialData1[20] = {37,50,54,50,56,0,43,43,74,71,32,36,16,43,56,100,50,25,15,17}; 
    int InitialData2[20] = {35,51,54,58,55,32,36,69,27,39,35,40,16,44,55,14,58,75,18,15}; 
 
    // Number of elements in the vectors to be added 
    const int SIZE = 2048;


    // Two integer source vectors in Host memory 
    int HostVector1[SIZE], HostVector2[SIZE]; 
 
    // Initialize with some interesting repeating data 
    for(int c = 0; c < SIZE; c++) {  
        HostVector1[c] = InitialData1[c%20]; 
        HostVector2[c] = InitialData2[c%20]; 
    } 

    
    // Инициализируем контекст
    /* - Параметр NULL (автоматический выбор платформы более не поддерживается).
       - Заменено, см. ниже.
    // Create a context to run OpenCL on our CUDA-enabled NVIDIA GPU  
    cl_context GPUContext = clCreateContextFromType(
        0, CL_DEVICE_TYPE_GPU, NULL, NULL, &error
    );
    if (error == CL_SUCCESS) {
        cout << "CL_PLATFORM_NAME: " << GPUContext << endl;
    } 
    else {
        cout << "clCreateContextFromType() error: " << error << endl;
    }
    assert(error == CL_SUCCESS);
    */

    cl_int status = CL_SUCCESS;

    cl_uint numPlatforms;
    cl_platform_id platform = NULL;
    status = clGetPlatformIDs(0, NULL, &numPlatforms);
    cout << "Количество платформ: " << numPlatforms << endl;
    if (numPlatforms < 1) {
        cout << "Не найдено доступных платформ." << endl;
    }
    assert(status == CL_SUCCESS);

    cl_platform_id* platforms = new cl_platform_id[numPlatforms];
    status = clGetPlatformIDs(numPlatforms, platforms, NULL);
    if (status != CL_SUCCESS) {
        cout << "clGetPlatformIDs() failed." << endl;
    }
    assert(status == CL_SUCCESS);

    // Проходимся по всем обнаруженным платформам и выбираем подходящую
    for (unsigned i = 0; i < numPlatforms; ++i) {
        cout << "Платформа ID: " << platforms[i] << endl;
        char pbuf[100];
        status = clGetPlatformInfo(
            platforms[i],
            CL_PLATFORM_VENDOR,
            sizeof(pbuf),
            pbuf,
            NULL
        );
        assert(status == CL_SUCCESS);
        cout << "Платформа info: " << pbuf << endl;

        if ( !strcmp(pbuf, "NVIDIA Corporation") ) {
            cout << "Выбираем платформу '" << pbuf << "'" << endl;
            platform = platforms[i];
            break;
        }
    }
    delete[] platforms;

    if (platform == nullptr) {
        cout << "Подходящая платформа не найдена." << endl;
    }
    assert(platform != nullptr);


    // Работаем с найденной выше платформой
    const cl_device_type dType = CL_DEVICE_TYPE_GPU;
    cl_context_properties cps[3] = {
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)platform, 
        0
    };
    cl_context context = clCreateContextFromType(
        cps, dType, NULL, NULL, &status
    );

    if (platform == NULL) {
        cout << "Подходящая платформа не найдена." << endl;
    }
    assert(platform != NULL);


    // Get the list of GPU devices associated with this context 
    size_t ParmDataBytes;
    clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &ParmDataBytes);
    cl_device_id* GPUDevices = (cl_device_id*)malloc(ParmDataBytes);
    clGetContextInfo(context, CL_CONTEXT_DEVICES, ParmDataBytes, GPUDevices, NULL);

    // Create a command-queue on the first GPU device
    cl_command_queue GPUCommandQueue = clCreateCommandQueue(
        context, GPUDevices[0], 0, NULL
    ); 

 
    // Allocate GPU memory for source vectors AND initialize from CPU memory 
    cl_mem GPUVector1 = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(int) * SIZE, HostVector1, NULL
    );
    cl_mem GPUVector2 = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(int) * SIZE, HostVector2, NULL
    );
 
    // Allocate output memory on GPU 
    cl_mem GPUOutputVector = clCreateBuffer(
        context, CL_MEM_WRITE_ONLY, 
        sizeof(int) * SIZE, NULL, NULL
    );
  

    // Create OpenCL program with source code 
    /* - Перенесено в файл.
    cl_program OpenCLProgram = clCreateProgramWithSource(context, 7,
        OpenCLSource, NULL, &status);
    */
    const char* sourcePath =
        "D:/Projects/workspace/OpenCL-test/OpenCL-test/resource/OpenCL/vectorAdd.cl";
    //FILE* fp = fopen(sourcePath, "r"); - deprecated
    FILE* fp = nullptr;
    errno_t error = fopen_s(&fp, sourcePath, "r");
    assert(error == 0);
    assert( fp );
    char* source_str = (char*)malloc(MAX_SOURCE_SIZE_FILE_CL);
    size_t source_size = fread( source_str, 1, MAX_SOURCE_SIZE_FILE_CL, fp);
    fclose( fp );

    cl_program OpenCLProgram = clCreateProgramWithSource(
        context,
        1,
        (const char **)&source_str,
        (const size_t *)&source_size,
        NULL
    );
    assert(OpenCLProgram != nullptr);
    assert(status == CL_SUCCESS);

    /* - Не находит нужные библиотеки.
    size_t prgLength = 0;
    char *source = oclLoadProgSource(sourcePath, "", &prgLength);
    assert(source != nullptr);
    cl_program OpenCLProgram = clCreateProgramWithSource(
        context,
        1,
        (const char **)&source,
        &prgLength,
        &status
    );
    */

    // Build the program (OpenCL JIT compilation) 
    clBuildProgram(OpenCLProgram, 0, NULL, NULL, NULL, NULL); 
 
    // Create a handle to the compiled OpenCL function (Kernel) 
    cl_kernel OpenCLVectorAdd = clCreateKernel(OpenCLProgram, "vectorAdd", &status);
    assert(status == CL_SUCCESS);
 
    // In the next step we associate the GPU memory with the Kernel arguments 
    clSetKernelArg(OpenCLVectorAdd, 0, sizeof(cl_mem),(void*)&GPUOutputVector); 
    clSetKernelArg(OpenCLVectorAdd, 1, sizeof(cl_mem), (void*)&GPUVector1); 
    clSetKernelArg(OpenCLVectorAdd, 2, sizeof(cl_mem), (void*)&GPUVector2); 
 
    // Launch the Kernel on the GPU 
    size_t WorkSize[1] = { SIZE };  // one dimensional Range 
    clEnqueueNDRangeKernel(
        GPUCommandQueue, OpenCLVectorAdd, 1, NULL,
        WorkSize, NULL, 0, NULL, NULL
    ); 
  
    // Copy the output in GPU memory back to CPU memory 
    int HostOutputVector[SIZE];
    clEnqueueReadBuffer(
        GPUCommandQueue, GPUOutputVector, CL_TRUE, 0, 
        SIZE * sizeof(int), HostOutputVector, 0, NULL, NULL
    );


    // Cleanup  
    free(GPUDevices); 
    clReleaseKernel(OpenCLVectorAdd);   
    clReleaseProgram(OpenCLProgram); 
    clReleaseCommandQueue(GPUCommandQueue); 
    clReleaseContext(context); 
    clReleaseMemObject(GPUVector1); 
    clReleaseMemObject(GPUVector2); 
    clReleaseMemObject(GPUOutputVector); 
 
    // Print out the results 
    for (int Rows = 0; Rows < (SIZE/20); Rows++, printf("\n")) {
        for(int c = 0; c <20; c++) { 
            printf("%c", (char)HostOutputVector[Rows * 20 + c]); 
        } 
    }

}
