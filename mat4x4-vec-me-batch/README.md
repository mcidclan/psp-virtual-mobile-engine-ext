## VME, Mat4x4 Multiplied by Vector

A sample code demonstrating how to multiply a batch of vectors by a 4x4 matrix using the Virtual Mobile Engine and its host CPU (Media Engine). Which gives us a hybrid processing of vector batches against a matrix. The data is distributed over the 4 PEs and the VMAC operator is then used on each line, which allows us to use a fresh accumulator for each line of the matrix.

The context is built once at initialization and we still have one vector processed per VME context execution. However, a full set of 16 vectors is loaded once at initialization into the VME scratchpad. Then, in the main ME loop, the datapath is dynamically updated from by and from the ME at each iteration in order to switch to the next vector to be processed (This only updates the offset rather than reloading the data).

The loop processes 4 vectors at a time and Triangle/Cross are used to switch between the batches of vectors. This new sample code also includes other improvements regarding data movement between the VME and RAM.

## Usage

Use Triangle or Cross to change the current batch of vectors.  
Use Home to exit  

### Special Note

As this is new work that has required time and effort, and which could be useful to other people and projects, please consider at least leaving a reference to the repository in your projects. This would allow people to trace its history and refer back to the original sources and sample code for a better understanding of the work related to the VME.

## Disclamer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
