## PSP Virtual Mobile Engine Extension.

This repository contains various code examples demonstrating how to use the Virtual Mobile Engine (VME) and take advantage of this reconfigurable CGRA. It also contains an extension for the 'me-custom-core' library to ease the use and debug/log of the VME in homebrew.

## Usage

As the work on this project and around the VME in general has required time and effort and is still in a WIP state, and as it could be useful to other people and projects, please consider at least leaving a reference to this repository in your projects. This would allow people to trace its history and refer back to the original sources and sample code for a better understanding of the work related to the VME.  

Make sure you have `psp-media-engine-custom-core` installed and up to date before building the sample code, see below.

## Sample Code

[fft-dif-16-point-radix-2](https://github.com/mcidclan/psp-virtual-mobile-engine-ext/blob/main/fft-dif-16-point-radix-2/README.md)  
[mat4x4-mul-distributed](https://github.com/mcidclan/psp-virtual-mobile-engine-ext/tree/main/mat4x4-mul-distributed/README.md)  
[mat4x4-mul-host-batching](https://github.com/mcidclan/psp-virtual-mobile-engine-ext/blob/main/mat4x4-mul-host-batching/README.md)  
[mat4x4-mul-reset-acc](https://github.com/mcidclan/psp-virtual-mobile-engine-ext/blob/main/mat4x4-mul-reset-acc/README.md)  
[mat4x4-mul-batch](https://github.com/mcidclan/psp-virtual-mobile-engine-ext/blob/main/mat4x4-mul-batch/README.md)  
[mat3x3-mul-batch-q8](https://github.com/mcidclan/psp-virtual-mobile-engine-ext/blob/main/mat3x3-mul-batch-q8/README.md)  
[pico-ai](https://github.com/mcidclan/psp-virtual-mobile-engine-ext/blob/main/pico-ai/README.md)  
[poc-from-custom-core](https://github.com/mcidclan/psp-virtual-mobile-engine-ext/blob/main/poc-from-custom-core/README.md)  
[saturate-clamp-second-fu](https://github.com/mcidclan/psp-virtual-mobile-engine-ext/blob/main/saturate-clamp-second-fu/README.md)  
[switching-ram-context](https://github.com/mcidclan/psp-virtual-mobile-engine-ext/tree/main/switching-ram-context/README.md)  
[texture-processing](https://github.com/mcidclan/psp-virtual-mobile-engine-ext/tree/main/texture-processin/README.md)  
[32bits-precision-output](https://github.com/mcidclan/psp-virtual-mobile-engine-ext/tree/main/2bits-precision-output/README.md)  

## Contribution Guidelines

### AI-assisted development

AI tools may be used as development aids. However, the following rules apply strictly:

* All commits must be authored by a human contributor (pseudonyms are perfectly acceptable).
* The commit history must not contain any AI attribution as author or co-author.
* Contributors must fully review, understand, and validate all submitted code before opening a pull request.
* Contributors are expected to be able to explain and justify their changes during code review.
* The contributor is responsible for ensuring their code does not break existing functionality, including dependencies and the overall library behavior.

*In short: AI can assist, but humans must retain full ownership of the work.*

Pull requests that include AI attribution in commits, or that are not clearly understood and validated by the contributor, will be rejected.

### License compatibility

All code submitted to this repository must be compatible with the MIT License. Dependencies or code snippets under more restrictive licenses (e.g. GPL, LGPL, proprietary) are not accepted. Contributors are responsible for verifying that any third-party code they include is under a permissive license granting at least the same level of freedom as MIT.

## Disclaimer
This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

## Related work
[PSP Media Engine Cracking The Unknown](https://github.com/mcidclan/psp-media-engine-cracking-the-unknown)  
[PSP Media Engine Custom Core](https://github.com/mcidclan/psp-media-engine-custom-core)  
[PSP Media Engine Safe Task](https://github.com/mcidclan/psp-media-engine-safe-task)  
[PSP Media Engine Reload](https://github.com/mcidclan/psp-media-engine-reload)  

*m-c/d*
