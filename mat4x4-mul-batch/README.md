## VME, Mat4x4 Multiplied by batch of Vector

A sample code demonstrating how to multiply a batch of vectors by a 4x4 matrix using the Virtual Mobile Engine through a single context.

### Overview

This sample code shows the multiplication of a matrix by a batch of vectors using a unique VME context. However, it comes with some limitations.

Indeed, the first thing is that each line of a 4x4 matrix contains 4 values, and the number of lost cycles when using the staging streams (which avoids writing back to the scratchpad) is 3. The context would need to be written differently to take advantage of this.

On the other hand, the trick to cancel the accumulation on each line requires a write to the scratchpad, with the purpose of realigning the staged data with itself so the subtraction can be computed, since the two streams coming from the same staging are subtracted from each other with an offset of 4.

For now, the solution requires the VME to write over the scratchpad between process elements, which loses the 64-bit precision, costs some cycles, and forces us to use an appropriate format like Q1.15.

### Usage

Use Home to exit

### Special Note

As the work on this project and around the VME in general has required time and effort and is still in a WIP state, and as it could be useful to other people and projects, please consider at least leaving a reference to this repository in your projects. This would allow people to trace its history and refer back to the original sources and sample code for a better understanding of the work related to the VME.  

### Disclamer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
