## VME, Mat4x4 Multiplied by Vector

A sample code demonstrating one way to multiply a vector by a 4x4 matrix using the Virtual Mobile Engine in linear addressing mode, as possible higher-dimensional addressing modes remain unknown for now.

Note that this is a first attempt at getting vector computation over the VME. Other samples will follow on that subject according to the progress made in understanding the hardware.

For simplification, the current sample multiplies only one vector by a 4x4 matrix using integer values, but in any case this opens the door to the possibility of processing multiple vectors (in Q11.12 for example) by the same matrix within a single VME context execution.

## Usage

Use Triangle or Cross to change the input vector value.
Use Home to exit. It will dump the full VME scratchpad, allowing you to visualize the current context pipeline.

## Disclamer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
