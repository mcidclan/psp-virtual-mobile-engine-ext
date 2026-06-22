## VME, Mat4x4 Multiplied by Vector: Resetting the Accumulator

A sample code demonstrating one way to multiply a vector by a 4x4 matrix using the Virtual Mobile Engine in linear addressing mode, as possible higher-dimensional addressing modes remain unknown for now.

Note that this is a first attempt at getting vector computation over the VME. Other samples will follow on that subject according to the progress made in understanding the hardware.

For simplification, the current sample multiplies only one vector by a 4x4 matrix using integer values, but in any case this opens the door to the possibility of processing multiple vectors (in Q11.12 for example) by the same matrix within a single VME context execution.

## Technical aspects and purpose

I did the following across 4 stages (same pipeline).

**With k = 0:**

1. opcode `(back[n] * front[n]) >> k` on FU0
2. opcode `(-(back[n] * front[n])) >> k` on FU1
3. then I add the two together by applying a 4-word shift on one of them
4. opcode `(back[n] + front[n]) >> k` on FU2
5. then I apply a running sum `(i == 0 ? (b >> k) : out[n-1]) + (back[n] >> k)`, where `b` equals zero and where `out[n-1]` is actually the current value of the accumulator of the functional unit being processed

I did it this way so that the accumulation cancels out every 8 steps. It works and ultimately amounts to a 2D MAC in terms of result. It should be possible to use this scheme to process batches of vectors on a single pipeline/context.

You can check the mat4x4 multiplied by a single vector example to see how to take advantage of the VMAC across the 4 PEs to achieve the same result. However, it is not yet possible to process batches of vectors with it, since it is not yet known how to reset the accumulator at a given stream position.

## Usage

Use Triangle or Cross to change the input vector value.
Use Home to exit. It will dump the full VME scratchpad, allowing you to visualize the current context pipeline.

## Disclamer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
