## VME, Mat4x4 Multiplied by Vector

An other sample code demonstrating a light way to multiply a single vector by a 4x4 matrix using the Virtual Mobile Engine in linear addressing mode, as possible higher-dimensional addressing modes remain unknown for now.

Here, the data is distributed over the 4 PEs, and the VMAC operator is then used on each line, allowing us to use a fresh accumulator for each line of the matrix. The sample code gives the same result as the one using padded data. However, this sample code is not extensible to process batches of vectors.

## Usage

Use Triangle or Cross to change the input vector value.
Use Home to exit. It will dump the full VME scratchpad, allowing you to visualize the current context pipeline.

## Disclamer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
