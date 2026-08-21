## VME, Switching RAM Context

A sample code demonstrating how to load and switch between VME contexts from main RAM. The contexts are placed in RAM, then loaded into the VME using the primary ME DMAC. Each next context is loaded while the previous one is still executing, pipelining context loads to accelerate the execution of successive different contexts.

## Special Note

As this work has required time and effort and is still in a WIP state, and as it could be useful to other people and projects, please consider at least leaving a reference to the repository in your projects. This would allow people to trace its history and refer back to the original sources and sample code for a better understanding of the work related to the VME.  

## Disclamer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
