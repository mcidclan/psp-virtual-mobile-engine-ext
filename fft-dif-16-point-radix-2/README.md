# Custom 16 Point Radix 2 DIF FFT over the VME

An experimental technical demonstration of a custom radix 2, 16 point DIF FFT implementation over the VME.

## Overview

This sample code demonstrates a custom implementation of a 16 point radix 2 Decimation In Frequency (DIF) FFT built on top of the VME. The DIF process is handled in several steps, split across index reorganization (performed by the DMAC at the input of each stage) and the arithmetic itself (performed across two VME contexts per stage).

## Index Reorganization per Stage

At the input of each stage, the DMAC reorganizes the sample indexes according to a stride that is halved at every stage. This reorganization defines the upper leg and lower leg used for that stage's butterfly operations.

### Stage 0 (stride 8)

```
0, 8, 1, 9, 2, A, 3, B, 4, C, 5, D, 6, E, 7, F
```

Upper leg: `0, 1, 2, 3, 4, 5, 6, 7`  
Lower leg: `8, 9, A, B, C, D, E, F`  

### Stage 1 (stride 4)

Applied on the output indexes of Stage 0:

```
0, 4, 1, 5, 2, 6, 3, 7, 8, C, 9, D, A, E, B, F
```

Upper leg: `0, 1, 2, 3, 8, 9, A, B`  
Lower leg: `4, 5, 6, 7, C, D, E, F`  

### Stage 2 (stride 2)

Applied on the output indexes of Stage 1:

```
0, 2, 1, 3, 4, 6, 5, 7, 8, A, 9, B, C, E, D, F
```

Upper leg: `0, 1, 4, 5, 8, 9, C, D`  
Lower leg: `2, 3, 6, 7, A, B, E, F`  

### Stage 3 (stride 1)

Applied on the output indexes of Stage 2:

```
0, 2, 4, 6, 8, A, C, E, 1, 3, 5, 7, 9, B, D, F
```

Upper leg: `0, 2, 4, 6, 8, A, C, E`  
Lower leg: `1, 3, 5, 7, 9, B, D, F`  

## Two Context Processing per Stage

Each stage is built from two sub steps, executed as two distinct VME contexts.

### Context 1: Add / Sub between legs

For each pass over the upper leg:

```
upper[n] = upperLeg[n] + lowerLeg[n]
```

For each pass over the lower leg:

```
lower[n] = upperLeg[n] - lowerLeg[n]
```

### Context 2: Twiddle multiplication and final lower leg output

```
M1[n] = Sreal[n] * Treal[n]
M2[n] = Simg[n]  * Timag[n]
M3[n] = Sreal[n] * Timag[n]
M4[n] = Simg[n]  * Treal[n]

LowerReal[n] = M1[n] - M2[n]
LowerImag[n] = M3[n] + M4[n]
```

Where `S` represents the result of the subtraction produced by the previous stage's context and `T` the twiddle factors data.

## Additional Adjustments

A few adjustments were necessary to keep the pipeline numerically stable, including the shift required for Q.20 format arithmetic and an additional divide by two shift at the output of each leg to prevent saturation. A hardware rounding step is also applied to avoid parasitic overflow.

## Usage

Use `TRIANGLE` and `CROSS` to switch between the stimulus patterns.
> *See main.h for more information about the DATA*

### Special Note

As this work has required time and effort and is still in a WIP state, and as it could be useful to other people and projects, please consider at least leaving a reference to the repository in your projects. This would allow people to trace its history and refer back to the original sources and sample code for a better understanding of the work related to the VME.  

## Notes, DIT vs DIF

While the hardware could in principle be better suited to DIF, the combined use of its DMAC to prepare and reorganize the indexes at the input of each stage, along with the use of its CGRA type pipeline to perform the calculations, made this current sample an interesting challenge.

## Disclamer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
