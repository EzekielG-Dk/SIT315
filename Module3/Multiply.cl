__kernel void multiply(const int size, const __global int* input1, const __global int* input2, __global int* result)
{
    //Get the i, j and k for indexing from global ids
    const int i = get_global_id(0);
    const int j = get_global_id(1);
    const int k = get_global_id(2);

    //Convert the i, j and k into the indexes for matrices
    const int resultIndex = (i * size) + j;
    const int input1Index = (i * size) + k;
    const int input2Index = (k * size) + j;

    //Add the multiplication of the two inputs to the result index
    result[resultIndex] += (input1[input1Index] * input2[input2Index]);
}
