#include <iostream>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <thread>
#include <mpi.h>
#include <CL/cl.h>
#include <chrono>
#include <iomanip>

using namespace std::chrono;
using namespace std;

#define HEAD 0

//Declare the size, matrices and the buffers outside main so they can be accessed anywhere
int size = 100;
int *M1, *M2, *M3, num_rows;
cl_mem bufM1, bufM2, bufM3;

//Declare the variables needed for OpenCL outside main
cl_device_id device_id;
cl_context context;
cl_program program;
cl_kernel kernel;
cl_command_queue queue;
cl_event event = NULL;
int err;

//Declare all the functions for OpenCL prior to running them
cl_device_id create_device();
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname);
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename);
void setup_kernel_memory(int num_rows);
void copy_kernel_args();
void free_memory();
void runOpenCL(int num_rows);

//Declare all the functions for MPI and calculation prior to running them
void init(int* &matrix, int rows, int cols, bool initialise, bool resultant);
void print( int* matrix, int rows, int cols);
void head(int num_processes);
void node(int process_rank, int num_processes);

int main(int argc, char** argv) {

    int num_processes, process_rank;

    //Initializing MPI environment and getting processes and rank
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

    //The head and the nodes have different functions
    if(process_rank == HEAD)
        head(num_processes);
    else
        node(process_rank, num_processes);
    
    //Close out the MPI environment
    MPI_Finalize();
}

void head(int num_processes)
{
    //Initiliaze and allocate memory for matrices
    init(M1, size, size, true, false), init(M2, size, size, true, false), init(M3, size, size, false, true);

    //calcualted sizes for num of rows per processer, size to M2 to broadcast and size to scatter
    num_rows = size / num_processes;
    int broadcast_size = (size * size);
    int scatter_gather_size = (size * size) / num_processes;

    //Start the clock prior to scattering data, taking into account this time
    auto start = high_resolution_clock::now();

    //Scatter the data in M1 across all processes evenly, broadcast the whole of M2 to all processes
    MPI_Scatter(&M1[0], scatter_gather_size ,  MPI_INT , &M1 , 0, MPI_INT, HEAD, MPI_COMM_WORLD);
    MPI_Bcast(&M2[0], broadcast_size , MPI_INT , HEAD , MPI_COMM_WORLD);

    runOpenCL(num_rows);

    //Gather the different MPI processes back together after the OpenCL code
    MPI_Gather(MPI_IN_PLACE, scatter_gather_size , MPI_INT, &M3[0] , scatter_gather_size , MPI_INT, HEAD , MPI_COMM_WORLD);

    //Stop the clock and print the result
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);

    cout << "Time taken by function: " << fixed << setprecision(3) << duration.count()/1000.0 << " seconds" << endl;

    //Free the memory that was assigned for buffers and matrices
    free_memory();
}


void node(int process_rank, int num_processes)
{
    //Calculated sizes for num of rows per processer, size of M2 to broadcast and size to scatter, done each node as the values are not global
    num_rows = size / num_processes;
    int broadcast_size = (size * size);
    int scatter_gather_size = (size * size) / num_processes;

    //Initiliaize empty matrices for M1, M2 and M3, are filled when they are recieved in broadcast
    init(M1, num_rows, size, false, false), init(M2, size, size, false, false), init(M3, num_rows, size, false, true);

    //Receive M1 and M2 from the head broadcasting/scattering them out
    MPI_Scatter(NULL, scatter_gather_size , MPI_INT , &M1[0], scatter_gather_size, MPI_INT, HEAD , MPI_COMM_WORLD);
    MPI_Bcast(&M2[0], broadcast_size , MPI_INT , HEAD , MPI_COMM_WORLD);
    
    runOpenCL(num_rows);

    MPI_Gather(&M3[0], scatter_gather_size , MPI_INT, NULL, scatter_gather_size , MPI_INT, HEAD , MPI_COMM_WORLD);

    free_memory();
}

void init(int* &matrix, int rows, int cols, bool initialise, bool resultant) 
{
    //Create an array of size rows*cols, this the a matrix stored in a ray.
    //Whenever referencing specific items within this array maths must be done
    //to determine its position
    matrix = (int *)malloc(sizeof(int*) * rows * cols); 

    if(resultant)
    {
        for(long i = 0 ; i < rows*cols; i++) 
        {
            matrix[i] = 0; 
        }
    }

    if(!initialise) return;

    for(long i = 0 ; i < rows*cols; i++) 
    {
        matrix[i] = rand() % 100; 
    }
}

//Function that prints out a matrix. If the matrices size is greater than 15 it only prints the
//first and last 5 items of the first and last 5 rows
void print( int* matrix, int rows, int cols) 
{
    if (size > 15)
    {
        for(long i = 0; i < 5; i++)
        {
            for (long j = 0; j < 5; j++)
            {                       
                printf("%d ",  matrix[i*cols+j]); 
            }
            printf(" ..... ");
            for (long j = size - 5; j < size; j++)
            {                       
                printf("%d ",  matrix[i*cols+j]); // print the cell value
            }
            printf("\n");
        }
        printf(".\n");
        printf(".\n");
        printf(".\n");
        printf(".\n");
        printf(".\n");
        for(long i = size - 5; i < size; i++)
        {
            for (long j = 0; j < 5; j++)
            {                       
                printf("%d ",  matrix[i*cols+j]); 
            }
            printf(" ..... ");
            for (long j = size - 5; j < size; j++)
            {                       
                printf("%d ",  matrix[i*cols+j]); // print the cell value
            }
            printf("\n");
        }
    }
    else
    {
        for (long i = 0; i < size; i++)
        {                       
            for(long j = 0 ; j < cols; j++) {  
                printf("%d ",  matrix[i*cols+j]); // print the cell value
            }
            printf("\n"); //at the end of the row, print a new line
        }
    }
    printf("\n----------------------------\n");
}

//OpenCL Functions
void free_memory()
{
    //free the buffers
    clReleaseMemObject(bufM1);
    clReleaseMemObject(bufM2);
    clReleaseMemObject(bufM3);

    //free opencl objects
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);

    free(M1);
    free(M2);
    free(M3);
}

void copy_kernel_args()
{
    //sets the kernel arguments for the kernel and takes the arguments: Kernel, argument index, argument size, argument value
    clSetKernelArg(kernel, 0, sizeof(int), (void *)&size);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&bufM1);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&bufM2);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&bufM3);

    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

void setup_kernel_memory(int num_rows)
{
    //clCreateBuffer function creates a buffer object 
    //Takes the arguements: context, flags(a bit-field that is used to specify allocation and usage information), size, host_ptr(a pointer to buffer data), error return

    //The second parameter of the clCreateBuffer is cl_mem_flags flags. Check the OpenCL documention to find out what is it's purpose and read the List of supported memory flag values 
    bufM1 = clCreateBuffer(context, CL_MEM_READ_ONLY, size * num_rows * sizeof(int), NULL, NULL);
    bufM2 = clCreateBuffer(context, CL_MEM_READ_ONLY, size * size * sizeof(int), NULL, NULL);
    bufM3 = clCreateBuffer(context, CL_MEM_WRITE_ONLY, size * size * sizeof(int), NULL, NULL);

    // Copy matrices to the GPU
    clEnqueueWriteBuffer(queue, bufM1, CL_TRUE, 0, size * num_rows * sizeof(int), &M1[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufM2, CL_TRUE, 0, size * size * sizeof(int), &M2[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufM3, CL_TRUE, 0, size * num_rows * sizeof(int), &M3[0], 0, NULL, NULL);
}

void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname)
{
    device_id = create_device();
    cl_int err;

    //clCreateContext creates context for the new device
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (err < 0)
    {
        perror("Couldn't create a context");
        exit(1);
    }

    program = build_program(context, device_id, filename);

    //Creates a host or device command-queue on a new device
    queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
    if (err < 0)
    {
        perror("Couldn't create a command queue");
        exit(1);
    };


    kernel = clCreateKernel(program, kernelname, &err);
    if (err < 0)
    {
        perror("Couldn't create a kernel");
        printf("error =%d", err);
        exit(1);
    };
}

cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename)
{

    cl_program program;
    FILE *program_handle;
    char *program_buffer, *program_log;
    size_t program_size, log_size;

    /* Read program file and place content into buffer */
    program_handle = fopen(filename, "r");
    if (program_handle == NULL)
    {
        perror("Couldn't find the program file");
        exit(1);
    }
    fseek(program_handle, 0, SEEK_END);
    program_size = ftell(program_handle);
    rewind(program_handle);
    program_buffer = (char *)malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, program_handle);
    fclose(program_handle);

    //Creates a program object with context, then loads the source code provided in the arguments in strings and string array into the program object
    //clCreateProgramWithSource takes 5 arguments: Context, Count, Source Code, Size, Error Message.
    program = clCreateProgramWithSource(ctx, 1, (const char **)&program_buffer, &program_size, &err);
    if (err < 0)
    {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    /* Build program 

   The fourth parameter accepts options that configure the compilation. 
   These are similar to the flags used by gcc. For example, you can 
   define a macro with the option -DMACRO=VALUE and turn off optimization 
   with -cl-opt-disable.
   */
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err < 0)
    {

        /* Find size of log and print to std output */
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_size);
        program_log = (char *)malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                              log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        exit(1);
    }

    return program;
}

cl_device_id create_device() {

   cl_platform_id platform;
   cl_device_id dev;
   int err;

   /* Identify a platform */
   err = clGetPlatformIDs(1, &platform, NULL);
   if(err < 0) {
      perror("Couldn't identify a platform");
      exit(1);
   } 

   // Access a device
   // GPU
   err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
   if(err == CL_DEVICE_NOT_FOUND) {
      // CPU
      printf("GPU not found\n");
      err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
   }
   if(err < 0) {
      perror("Couldn't access any devices");
      exit(1);   
   }

   return dev;
}

void runOpenCL(int num_rows)
{
    //Set the number of global work items (i, j and k), indexs of values to be calculated
    size_t global[3] = {(size_t)num_rows, (size_t)size, (size_t)size};

    //Setup the open cl device kernel using the Multiply.cl file and multiply function within
    setup_openCL_device_context_queue_kernel((char *)"./Multiply.cl", (char *)"multiply");

    //Setup the kernel memory with buffers and copy them to the GPU
    setup_kernel_memory(num_rows);
    //Set the arguments a data in the kernel
    copy_kernel_args();

    //Enqueue the data on the kernel and wait for it to runm before reading the data returned in the M3 matrix
    clEnqueueNDRangeKernel(queue, kernel, 3, NULL, global, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);
    clEnqueueReadBuffer(queue, bufM3, CL_TRUE, 0, size*num_rows*sizeof(int), &M3[0], 0, NULL, NULL);
}

//CL file conents. File name Multiply.cl
//__kernel void multiply(const int size, const __global int* input1, const __global int* input2, __global int* result)
//{
//    //Get the i, j and k for indexing from global ids
//    const int i = get_global_id(0);
//    const int j = get_global_id(1);
//    const int k = get_global_id(2);
//
//    //Convert the i, j and k into the indexes for matrices
//    const int resultIndex = (i * size) + j;
//    const int input1Index = (i * size) + k;
//    const int input2Index = (k * size) + j;
//
//    //Add the multiplication of the two inputs to the result index
//    result[resultIndex] += (input1[input1Index] * input2[input2Index]);
//}
