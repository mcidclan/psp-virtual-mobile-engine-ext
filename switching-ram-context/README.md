## VME, Switching RAM Context

A sample code demonstrating how to load and switch between VME contexts from main RAM. The contexts are placed in RAM, then loaded into the VME using the primary ME DMAC. Each next context is loaded while the previous one is still executing, pipelining context loads to accelerate the execution of successive different contexts.

## Disclamer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
