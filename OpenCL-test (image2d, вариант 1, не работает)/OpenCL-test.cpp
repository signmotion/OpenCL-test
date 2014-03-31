//#define _CRT_SECURE_NO_DEPRECATE

#include <iostream>
#include <fstream>
#include <sstream>
#include "opencl.h"
#include <oclUtils.h>


using namespace std;


/**
* Максимальный размер загружаемого *.cl файла в байтах.
*/
const unsigned int MAX_SOURCE_SIZE_FILE_CL = 1000 * 1024;


/**
* Игра с точностью вычислений.
*/
void forces();





// Main function  
// ********************************************************************* 
int main(int argc, char **argv)
{ 
	setlocale(LC_ALL, "Russian");
    // Для разделителя '.' вместо ','
    setlocale(LC_NUMERIC, "C");


    forces();

    return 0;
}










void forces() {

    // Взаимодействия элементов мира делаем через OpenCL/image2d.
    // @see (!) http://developer.apple.com/library/mac/#documentation/Performance/Conceptual/OpenCL_MacProgGuide/ImageProcessingwithOpenCL/ImageProcessingwithOpenCL.html#//apple_ref/doc/uid/TP40008312-CH103-SW2
    // @see http://www.cmsoft.com.br/index.php?option=com_content&view=category&layout=blog&id=115&Itemid=172
    // @see http://www.khronos.org/message_boards/viewtopic.php?f=28&t=2358
    // Для работы с изображением
    //   > http://www.imagemagick.org/Magick++/Image.html
    //   > http://labs.qt.nokia.com/2010/04/07/using-opencl-with-qt


    // Шаг времени в секундах, за которое будут рассчитываться взаимодействия
    const float TIME = 1.0;

    // Размер мира
    const cl_uint N = 16;
    const cl_uint M = 8;
    const cl_uint NM = N * M;
    const cl_uint2 SIZE = { N, M };
    // 1 метр содержит 10 ячеек
    const cl_float scale = 1.0f / 10.0f;


    /**
    * Структуры хранения информации о мире.
    * @see OpenCL/interaction.cl
    * @see http://www.khronos.org/registry/cl/sdk/1.0/docs/man/xhtml/restrictions.html
    */
    typedef struct {
        cl_uchar s0;
        cl_uchar s1;
        cl_uchar s2;
        cl_uchar s3;
    } DataUChar;

    typedef struct {
        cl_float s0;
        cl_float s1;
        cl_float s2;
        cl_float s3;
    } DataFloat;


    // Здесь обновляем визуальный образ мира
    DataFloat* wOutVisual = new DataFloat[NM];


    // Сущности системы
    typedef struct {
        cl_uchar id;
        cl_float density;
    } EData;
    cout << "sizeof(EData) " << sizeof(EData) << endl;

    // Песок
    EData sand = { 4, 1500.0f };

    const int ENTITY_COUNT = 1;
    EData* ed = new EData[ENTITY_COUNT];
    ed[0] = sand;


    // Для оптимального быстродействия большая часть мира хранится
    // в виде 2D-текстур.
    DataUChar* wIn1 = new DataUChar[NM];
    DataFloat* wIn2 = new DataFloat[NM];
    DataFloat* wIn3 = new DataFloat[NM];


    // На все ячейки системы всегда действует сила тяжести
    const float defaultForce[] = { 0.0f, -9.81f };

    // Сущности системы заданы матрицей N x M.
    // Мир разбит на ячейки: 1 ячейка = 1 бесформенная сущность.
    for(unsigned int z = 0; z < M; z++) {
        for(unsigned int x = 0; x < N; x++) {
            const unsigned int i = x + z * M;

            wOutVisual[i].s0 = wOutVisual[i].s1 =
                wOutVisual[i].s2 = wOutVisual[i].s3 = (cl_float)i;

            wIn1[i].s0 = sand.id;
            wIn1[i].s1 = 0;
            wIn1[i].s2 = 0;
            wIn1[i].s3 = 0;

            wIn2[i].s0 = sand.density;
            wIn2[i].s1 = 0;
            wIn2[i].s2 = 0;
            wIn2[i].s3 = 0;

            wIn3[i].s0 = defaultForce[0];
            wIn3[i].s1 = defaultForce[1];
            wIn3[i].s2 = 0.0f;
            wIn3[i].s3 = 0.0f;
        }
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
    size_t parmDataBytes;
    clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &parmDataBytes);
    cl_device_id* gpuDevices = (cl_device_id*)malloc(parmDataBytes);
    clGetContextInfo(context, CL_CONTEXT_DEVICES, parmDataBytes, gpuDevices, NULL);

    // Create a command-queue on the first GPU device
    cl_command_queue gpuCommandQueue = clCreateCommandQueue(
        context, gpuDevices[0], 0, NULL
    );


    // Выделяем память в устройстве, инициируем её значениями
    cout << "Размер матрицы: " << N << " x " << M << " = " << NM << " ячеек" << endl;
    
    // DataUChar
    cout << "Память для DataUChar: " << sizeof( DataUChar ) * NM << " байт" << endl;

    // Calculating the row pitch for an image in 8-bit RGBA format
    // assume that each channel is represented with in CL_RGBA / CL_UNORM_INT8 format
    const unsigned int numChannelsPerPixel = 4;
    const unsigned int imageWidth = N;
    unsigned int channelSize = sizeof( cl_uint8 );
    unsigned int pixelSize = channelSize * numChannelsPerPixel;
    unsigned int rowPitch = imageWidth * pixelSize;

    // set the image format properties
    cl_image_format imageFormat;
    imageFormat.image_channel_order = CL_RGBA;
    imageFormat.image_channel_data_type = CL_UNSIGNED_INT8;

    /*
    cl_mem gpuWIn1 = clCreateImage2D(
        context,
        // option flags
        CL_MEM_READ_ONLY,
        // image format properties
        &imageFormat,
        // width of the image in pixels
        N,
        // height of the image in pixels
        M,
        // scan-line pitch in bytes
        rowPitch,
        // pointer to the image data
        wIn1,
        &status
    );
    assert(status == CL_SUCCESS);
    assert(gpuWIn1 != nullptr);
    */
 

    // DataFloat
    cout << "Память для DataFloat: " << sizeof( DataFloat ) * NM << " байт" << endl;

    // .....


    // Other

    cout << "Память для EData: " << sizeof(EData) * NM << " байт" << endl;
    /*
    cl_mem gpuEntity = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(EData) * ENTITY_COUNT, ed, NULL
    );
    */
 
    // Место для формирования визуального образа мира
    channelSize = sizeof( cl_float );
    pixelSize = channelSize * numChannelsPerPixel;
    rowPitch = imageWidth * pixelSize;

    // set the image format properties
    imageFormat.image_channel_order = CL_RGBA;
    imageFormat.image_channel_data_type = CL_FLOAT;

    cl_mem gpuWOutVisual = clCreateImage2D(
        context,
        // option flags
        CL_MEM_WRITE_ONLY,
        // image format properties
        &imageFormat,
        // width of the image in pixels
        N,
        // height of the image in pixels
        M,
        // scan-line pitch in bytes
        rowPitch,
        // empty image data
        wOutVisual,
        &status
    );
    assert(status == CL_SUCCESS);


    const char* sourcePath =
        "D:/Projects/workspace/OpenCL-test/OpenCL-test/resource/OpenCL/interaction.cl";
    FILE* fp = nullptr;
    errno_t error = fopen_s(&fp, sourcePath, "r");
    assert(error == 0);
    assert( fp );
    char* source_str = (char*)malloc(MAX_SOURCE_SIZE_FILE_CL);
    size_t source_size = fread( source_str, 1, MAX_SOURCE_SIZE_FILE_CL, fp );
    fclose( fp );

    cl_program openCLProgram = clCreateProgramWithSource(
        context,
        1,
        (const char **)&source_str,
        (const size_t *)&source_size,
        NULL
    );
    assert(status == CL_SUCCESS);
    assert(openCLProgram != nullptr);


    // Build the program (OpenCL JIT compilation)
    // Option '-g' > 'debbuging with GDB'
    status = clBuildProgram(openCLProgram, 0, NULL, "-g", NULL, NULL); 
    if (status != CL_SUCCESS) {
        printf("Error building program\n");
        char buffer[4096];
        size_t length;
        clGetProgramBuildInfo(
            openCLProgram,  // valid program object
            gpuDevices[0],  // valid device_id that executable was built
            CL_PROGRAM_BUILD_LOG,  // indicate to retrieve build log
            sizeof(buffer),  // size of the buffer to write log to
            buffer,  // the actual buffer to write log to
            &length
        );   // the actual size in bytes of data copied to buffer
        printf("%s\n", buffer);
    }
    assert(status == CL_SUCCESS);
 
    // Create a handle to the compiled OpenCL function (Kernel) 
    cl_kernel openCLInteraction = 
        clCreateKernel(openCLProgram, "interaction", &status);
    assert(status == CL_SUCCESS);
    assert(openCLInteraction != nullptr);
 
    // Передаём устройству аргументы (см. interaction.cl)
    status = clSetKernelArg(openCLInteraction, 0, sizeof(cl_mem), (void*)&gpuWOutVisual);
    assert(status == CL_SUCCESS);
    //status = clSetKernelArg(openCLInteraction, 1, sizeof(cl_mem), (void*)&gpuWIn1);
    assert(status == CL_SUCCESS);
    //status = clSetKernelArg(openCLInteraction, 2, sizeof(cl_mem), (void*)&gpuEntity);
    assert(status == CL_SUCCESS);
    //status = clSetKernelArg(openCLInteraction, 3, sizeof(cl_float), (void*)&scale);
    assert(status == CL_SUCCESS);
    //status = clSetKernelArg(openCLInteraction, 4, sizeof(cl_float), (void*)&TIME);
    assert(status == CL_SUCCESS);
 
    // Запускаем ядро на устройстве
    const size_t globalWorkSize[2] = { N, M };
    //size_t localWorkSize[2] = { 16, 16 };
    clEnqueueNDRangeKernel(
        gpuCommandQueue,
        // kernel
        openCLInteraction,
        // work_dim
        2,
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

    // sync host
    //clFinish(gpuCommandQueue);


    // Возвращаем результат хосту
    channelSize = sizeof( cl_float4 );
    pixelSize = channelSize * numChannelsPerPixel;
    rowPitch = imageWidth * pixelSize;

    const unsigned int origin[] = { 0, 0, 0 };
    const unsigned int region[] = { N, M, 1 };
    status = clEnqueueReadImage(
        gpuCommandQueue,
        gpuWOutVisual,
        // blocking_read
        CL_TRUE,
        // origin
        origin,
        // region
        region,
        rowPitch,
        //slicePitch
        0,
        wOutVisual,
        0, NULL, NULL
    );
    assert(status == CL_SUCCESS);


    // Смотрим
    cout << "Готово." << endl << endl;
    /*
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
    */


    // Cleanup
    free(gpuDevices);
    clReleaseKernel(openCLInteraction);
    clReleaseProgram(openCLProgram);
    clReleaseCommandQueue(gpuCommandQueue);
    clReleaseContext(context);
    clReleaseMemObject(gpuWOutVisual);
    //clReleaseMemObject(gpuEntity);
    //clReleaseMemObject(gpuWIn1);
 
    delete[] ed;
    delete[] wIn1;
    delete[] wIn2;
    delete[] wIn3;

}
