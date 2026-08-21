## VME Texture Processing Sample

This sample demonstrates one way to use the Virtual Mobile Engine for image and texture processing.

### Overview

The main CPU loads a 64x64 PNG image into a 16 bit buffer using the 5551 pixel format. This buffer is shared between the SC and the ME through an uncached memory segment mapped above main RAM. At ME side initialization, this image is transferred once from the shared uncached segment into the VME scratchpad. A VME context is also created at initialization, directly set through the interface exposing the datapath's memory configuration. This context is responsible for transferring 4 source buffers, each positioned at an offset of 64*16 pixels, to the VME output through the 4 available Processing Elements. Computation on the 4 blocks therefore happens in parallel, one block per PE.

### Effect

The computation applied here is a simple increase of the red channel component, implemented as a bitwise OR between a clamped input value and each pixel. Using OR instead of addition actually avoids carry propagation into the neighboring green channel bits. This clamped input value is updated on the SC side and shared with the ME. So its update rate depends on the main thread's cycle.

### Usage

Make sure to place `mcid64x64.png` (or another 64x64 png image) in the same folder as the EBOOT.

### Special Note

As the work on this project and around the VME in general has required time and effort and is still in a WIP state, and as it could be useful to other people and projects, please consider at least leaving a reference to this repository in your projects. This would allow people to trace its history and refer back to the original sources and sample code for a better understanding of the work related to the VME.  

## Disclamer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
