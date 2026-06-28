## VME Pico AI PoC

## test
```bash
python fake_gen.py
python train.py dataset.bin model.bin
```

## Pipelines and Architecture 

## Recorder (PSP side)

- [x] thread samples joystick via `sceCtrlPeekBufferPositive`, signed char centered on 0
- [x] sampling rate : one sample per frame (~60fps, 16667µs delay)
- [x] pretrigger ring buffer (8 samples) running continuously
- [x] trigger fires when x or y exceeds threshold on any sample
  - [x] copy pretrigger ring buffer to start of capture window
  - [x] record the 64 samples that follow (total window = 64)
- [x] after capture completes, set a lock flag
  - [x] display lock state on screen so user knows a record was captured
  - [x] lock clears only after MIN_LOCK frames with joystick continuously at rest (resets if stick moves during pause)
- [x] store captures in an in-memory buffer with a fixed max count (8 slots)
  - [x] display remaining slots on screen
  - [x] display current recording number (from config) on screen
  - [x] block new captures when buffer is full
  - [x] allow full reset of buffer at any time (Square), restarts from slot 0
- [x] write buffer to file only when all slots are filled
  - [x] filename : `./sample.NN.bin` where NN is read from config file at save time
  - [x] config file updated (NN incremented) after each successful write
  - [x] triggered by Circle button only
  - [x] if buffer not full, button press is ignored (warning displayed instead)
  - [x] buffer resets automatically after successful save for next 8 recordings
- [x] each entry written : 32-bit label + 128 signed char (64 x + 64 y)

## Trainer (PC side)

- [ ] load dataset from binary file
  - [ ] each entry is a 32-bit label followed by 128 x 32-bit words
  - [ ] input values are signed char centered on 0 (PSP joystick 0..255 minus 128), packed in 32-bit words
- [ ] initialize W1 (128 x 4) and W2 (4 x 4) with small random values
- [ ] for each epoch :
  - [ ] forward pass : MAC over 128 inputs -> ReLU -> MAC over 4 features -> softmax
  - [ ] compute cross entropy loss between predicted scores and true labels
  - [ ] backward pass : compute gradients for W1 and W2
  - [ ] update W1 and W2 by gradient descent
- [ ] export model to binary file :
  - [ ] 4 buffers of 128 weights (W1, hidden layer), signed 24-bit packed in 32-bit words
  - [ ] 4 buffers of 4 weights (W2, output layer), signed 24-bit packed in 32-bit words


## Detector (PSP side, CGRA)

### Scratchpad layout

- [ ] 128 input values (64 x + 64 y, signed char centered on 0, packed in 32-bit words)
- [ ] 512 weights W1 (4 x 128, signed 24-bit packed in 32-bit words)
- [ ] 16 weights W2 (4 x 4, signed 24-bit packed in 32-bit words)
- [ ] 4 intermediate values (hidden layer output)
- [ ] 4 final scores (output layer)

> 4 PE available, 64-bit accumulator per PE

### Hidden layer (pipeline 1)

- [ ] 4 neurons, each with 128 weights (W1)
- [ ] AGU READ 1 sweeps 128 weights, AGU READ 2 sweeps 128 inputs
- [ ] MAC over 128 values, accumulates in 64-bit acc
- [ ] shifter requantizes output to int8, ReLU kills negative scores
- [ ] outputs 4 feature scores to scratchpad intermediate buffer
- [ ] *(note)* a 5th neuron could be added as a "no combo" detector

### Output layer (pipeline 2)

- [ ] 4 neurons, each with 4 weights (W2)
- [ ] AGU READ 1 sweeps 4 weights, AGU READ 2 reads 4 feature scores
- [ ] MAC over 4 values, no ReLU on this layer
- [ ] outputs 4 combo scores to scratchpad output buffer
- [ ] pick the highest score index as the detected combo (0=combo1, 1=combo2, 2=combo3, 3=combo4)


## Disclamer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
