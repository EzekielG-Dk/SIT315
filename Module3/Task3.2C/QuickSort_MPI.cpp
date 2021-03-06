#include <iostream>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <thread>
#include <mpi.h>
#include <chrono>
#include <CL/cl.h>
#include <iomanip>
#include <algorithm>

using namespace std::chrono;
using namespace std;

#define HEAD 0

//Declare size
int size = 4000;
int *A, length, *out;
cl_mem bufIn, bufOut;

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
void setup_kernel_memory(int min, int partition_size, int* out);
void copy_kernel_args();
void free_memory();
void openCL_quickSort(int min, int max);

//Declare each of the functions before use
void init(int* &A, int init_size, bool initialise);
void head(int num_processes);
void node(int process_rank, int num_processes);
int partition(int min, int max);
int randPartition(int min, int max);
void quickSort(int min, int max);
void quickSort_MPI(int min, int max, int process_rank, int num_processes, int rank_depth);
void printArray(int* array, int length);

int main(int argc, char** argv) 
{
    int num_processes, process_rank;

    //Setup the MPI environment
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

    //The head and the nodes have different functions, the head kicks off the initial quickSort_MPI recursion
    if(process_rank == HEAD) 
        head(num_processes);
    else
        node(process_rank, num_processes);
    
    //Close out the MPI environment
    MPI_Finalize();

    free_memory();
}

void head(int num_processes)
{
    //Initiliaze and allocate memory for array
    init(A, size, true);

    //Start the clock prior to scattering data, taking into account this time
    auto start = high_resolution_clock::now();

    //Start by calling an initial call of quickSort MPI on the main process
    quickSort_MPI(0, size, HEAD, num_processes, 0);

    //Stop the clock and print the result
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);

    cout << "Time taken by function: " << fixed << setprecision(5) << duration.count()/1000.0 << " seconds" << endl;
}

void node(int process_rank, int num_processes)
{
    //nodes have to get a partition size, their parent and the data sent to them, declared here
    int parent;
    MPI_Status msg;

    int depth;
    //Get the depth this process first appears in using a left shift of one from the current depth
    //Depth will increase by 1 every 2, 4, 8, 16... total ranks indicating the depths
    while (1 << depth <= process_rank)
    {
        depth++;
    }

    //Probe for a message before collecting it so we know its size
    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &msg);
    //Get the size and parent of the message
    MPI_Get_count(&msg, MPI_INT, &length);
    parent = msg.MPI_SOURCE;

    //Allocate space for the section of array this node gets
    A = (int*)malloc(sizeof(int) * length);

    //Receive the data from the node into arary_part
    MPI_Recv(A, length, MPI_INT, parent, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //Start a quickSort_MPI recursive call on this node
    quickSort_MPI(0, length, process_rank, num_processes, depth);
    //Once the quickSort has finished return the array part to its parent
    MPI_Send(A, length, MPI_INT, parent, 1, MPI_COMM_WORLD);

}

//Function for creating and initializing the array
void init(int* &array, int init_size, bool initialise) { 
    array = (int *) malloc(sizeof(int) * init_size); 
  
    if(!initialise) return;

    for(long i = 0 ; i < init_size; i++) 
        array[i] = rand() % 1000;

}

//Takes in two integer pointers and switches them
void swap(int *x, int *y)
{
    int temp = *x;
    *x = *y;
    *y = temp;
}

//Partiion function that takes a min and max and puts all less than min to the left of it and all items
//greater than it to the right of it
int partition(int min, int max)
{
    int right = A[min];
    int pivot = min;

    for(int i = min + 1; i < max; i++)
    {
        if(A[i] <= right)
        {
            pivot++;
            swap(A[pivot], A[i]);
        }
    }
    swap(A[min], A[pivot]);

    //Returns the center point of this pivot, so that recursive quicksort calls are run on each side of it
    return pivot;
}

//randPartition function run by the quicksort algorithm that takes a random item from the array and swaps it to
//the end to be partitioned. This is done because a normal quicksort has an expected has runtime of O(n^2)
//whilst randomized has an expected runtime of O(nlog(n)), much faster
int randPartition(int min, int max)
{
    int random = min + rand() % (max-min);

    swap(A[random], A[min]);

    return partition(min, max);
}

//Recursive quick sort function takes and array and two endpoints, this is recursive so can be the each end of the
//array or just a small section within it
void quickSort(int min, int max)
{
    //If the provided min is still lower than the provided max perform the quicksort. This is because at the tips of
    //each branch we will reach a point where we only have one value
    if(min < max)
    {
        //Partition the array with everything greater than split on the right of it and everything less of on the left
        int split = randPartition(min, max);

        //Run the recursive quicksort algorithm of each partition of the array
        quickSort(min, split);
        quickSort(split + 1, max);
    }
}

//An MPI version of the quickSort algorithm that takes into account which node data needs to split into,
//When a node runs this it checks if the indicated child exists in the set of nodes and if it doesnt, then it
//just runs a normal quickSort. If it does exist the partitioning is done and one half is sent to the child
//while the current node works on the other half
void quickSort_MPI(int min, int max, int process_rank, int num_processes, int rank_depth)
{
    //Get rank of child using the current process rank and the current depth using left shift
    int child_rank = process_rank + (1 << rank_depth);

    //If the child doesnt exist run a normal quicksort
    if (child_rank >= num_processes)
    {
        //printf("min: %i, max: %i ", min, max);
        openCL_quickSort(min, max);
    }
    else if (min < max)
    {
        //partition the array around split
        int split = randPartition(min, max);

        //The biggest half stays in this node while the smaller half is sent the the previously calculated child node
        //where they each make another recursive quickSort_MPI call
        if(split - min > max - split - 1)
        {
            MPI_Send(&A[split+1], max-split-1, MPI_INT, child_rank, 1, MPI_COMM_WORLD);
            quickSort_MPI(min, split, process_rank, num_processes, rank_depth+1);
            MPI_Recv(&A[split+1], max-split-1, MPI_INT, child_rank, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        else
        {
            MPI_Send(&A[min], split-min, MPI_INT, child_rank, 1, MPI_COMM_WORLD);
            quickSort_MPI(split+1, max, process_rank, num_processes, rank_depth+1);
            MPI_Recv(&A[min], split-min, MPI_INT, child_rank, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
    }
}

//Function that prints an array
void printArray(int* array, int length)
{
    for (int i = 0; i < length; i++)
        printf("%d ", A[i]);
    printf("\n");
}

//OpenCL Functions
void free_memory()
{
    //free the buffers
    clReleaseMemObject(bufIn);
    clReleaseMemObject(bufOut);

    //free opencl objects
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);

    free(A);
}

void copy_kernel_args()
{
    //sets the kernel arguments for the kernel and takes the arguments: Kernel, argument index, argument size, argument value
    clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&bufIn);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&bufOut);

    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

void setup_kernel_memory(int min, int partition_size, int* out)
{
    //clCreateBuffer function creates a buffer object 
    //Takes the arguements: context, flags(a bit-field that is used to specify allocation and usage information), size, host_ptr(a pointer to buffer data), error return

    //The second parameter of the clCreateBuffer is cl_mem_flags flags. Check the OpenCL documention to find out what is it's purpose and read the List of supported memory flag values 
    bufIn = clCreateBuffer(context, CL_MEM_READ_WRITE, partition_size * sizeof(int), NULL, NULL);
    bufOut = clCreateBuffer(context, CL_MEM_READ_WRITE, partition_size * sizeof(int), NULL, NULL);

    // Copy matrices to the GPU
    clEnqueueWriteBuffer(queue, bufIn, CL_TRUE, 0, partition_size * sizeof(int), &A[min], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufOut, CL_TRUE, 0, partition_size * sizeof(int), &out, 0, NULL, NULL);
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

void openCL_quickSort(int min, int max)
{
    int partition_size = max-min;

    init(out, partition_size, false);
    size_t global[1] = {(size_t)partition_size};
    size_t local[1] = {(size_t)partition_size};

    //Setup the open cl device kernel using the Multiply.cl file and multiply function within
    setup_openCL_device_context_queue_kernel((char *)"./QuickSort.cl", (char *)"quicksort");

    //Setup the kernel memory with buffers and copy them to the GPU
    setup_kernel_memory(min, partition_size, out);
    //Set the arguments and data in the kernel
    copy_kernel_args();

    //Enqueue the data on the kernel and wait for it to run before reading the data returned in the array
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, local, 0, NULL, &event);
    clWaitForEvents(1, &event);
    clEnqueueReadBuffer(queue, bufOut, CL_TRUE, 0, partition_size*sizeof(int), &A[min], 0, NULL, NULL);
}
