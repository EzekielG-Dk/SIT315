
int partition(__global int *array, int min, int max) 
{
    int left = array[max];
    int pivot = min;
    int temp;

    for(int i = min; i < max; i++) 
    {
        if(array[i] < left) 
        {
            temp = array[i];
            array[i] = array[pivot];
            array[pivot]=temp;
            pivot++;
        }
    }

    temp = array[pivot];
    array[pivot]=array[max];
    array[max]=temp;
    //printf("Max: %i, %i, Min: %i, %i \n", array[max], max, array[pivot], pivot);
    return pivot;
}
__kernel void quicksortpart(__global int* array,__global int* stack, int min, int max) 
{
    int top = -1; 

    stack[++top] = min; 
    stack[++top] = max; 
  
    if(min <= max)
    {
        while (top >= 0) 
        { 
            max = stack[top--]; 
            min = stack[top--]; 

            int split = partition(array, min, max); 

            if (split - 1 > min) 
            { 
                stack[++top] = min; 
                stack[++top] = split - 1; 
            } 

            if (split + 1 < max) 
            { 
                stack[++top] = split + 1; 
                stack[++top] = max; 
            } 
        }
    }
}

__kernel void quicksort(__global int* inArray, __global int* outArray) 
{
    const int quantity = get_global_id(0);

    quicksortpart(inArray, outArray, 0, quantity);

    for(int i = 0; i <= quantity; i++)
        outArray[i] = inArray[i];
}
