## VME Pico AI PoC

## test
```bash
python fake_gen.py
python train.py dataset.bin model.bin
```

## Pipelines and Architecture 
```c
// pico-ai-psp, trainer (PC side):
// load dataset from binary file
// each entry is a 32-bit label followed by 128 x 32-bit words
// input values are signed char centered on 0 (PSP joystick 0..255 minus 128), packed in 32-bit words
// initialize W1 (128 x 4) and W2 (4 x 4) with small random values
// for each epoch :
//   forward pass : MAC over 128 inputs -> ReLU -> MAC over 4 features -> softmax
//   compute cross entropy loss between predicted scores and true labels
//   backward pass : compute gradients for W1 and W2
//   update W1 and W2 by gradient descent
// export model to binary file :
//   4 buffers of 128 weights (W1, hidden layer), signed 24-bit packed in 32-bit words
//   4 buffers of 4 weights (W2, output layer), signed 24-bit packed in 32-bit words

// pico-ai-psp, recorder (PSP side):
// thread samples joystick via sceCtrlPeekBufferPositive
// fills buffer : 64 x values then 64 y values, signed char centered on 0, resets on full
// sets ready flag/signal to vme or write to file when buffer is complete (128 values total)

// pico-ai-psp, detector (PSP side, CGRA):
// scratchpad layout :
//   128 input values (64 x + 64 y, signed char centered on 0, packed in 32-bit words)
//   512 weights W1 (4 x 128, signed 24-bit packed in 32-bit words)
//   16 weights W2 (4 x 4, signed 24-bit packed in 32-bit words)
//   4 intermediate values (hidden layer output)
//   4 final scores (output layer)
// 4 PE available, 64-bit accumulator per PE
// Hidden layer (pipeline 1) :
//   4 neurons, each with 128 weights (W1)
//   AGU READ 1 sweeps 128 weights, AGU READ 2 sweeps 128 inputs
//   MAC over 128 values, accumulates in 64-bit acc
//   shifter requantizes output to int8, ReLU kills negative scores
//   outputs 4 feature scores to scratchpad intermediate buffer
//   note : a 5th neuron could be added as a "no combo" detector
// Output layer (pipeline 2) :
//   4 neurons, each with 4 weights (W2)
//   AGU READ 1 sweeps 4 weights, AGU READ 2 reads 4 feature scores
//   MAC over 4 values, no ReLU on this layer
//   outputs 4 combo scores to scratchpad output buffer
//   pick the highest score index as the detected combo (0=combo1, 1=combo2, 2=combo3, 3=combo4)
```

## Disclamer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
