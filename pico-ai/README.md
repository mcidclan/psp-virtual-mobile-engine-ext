## VME Pico AI PoC

This project uses 2 VME contexts to process a 2-layer neural network. The hidden layer contains 4 neurons, each performing a running MAC operation followed by ReLU activation using 128 weights applied to the dynamic input data. The output layer also contains 4 neurons, which compute the hidden layer outputs against their corresponding weights to produce the final classification scores.  

More precisely, this pico neural network is designed to detect joystick movements by training a model using recorded input data. Users can train their own models and experiment with different configurations through the tools provided by this project.  

## usage

### recording
Use the recorder app on PSP to generate sample.NN.bin files, either copied from the memory stick or written directly to PC via psplink.

### training
```bash
python train.py model.bin sample.*.bin
```

### debug training and output model
```bash
python train-debug.py sample.*.bin
```

```bash
python model-debug.py model.bin sample.*.bin
```

**The ideal confusion matrix should look like this:  **
```text
[[8 0 0 0]
 [0 8 0 0]
 [0 0 8 0]
 [0 0 0 8]]
```
*(With 8 recordings per class, this means that every sample is correctly classified into its corresponding class index.)*

### using the model
Use the main PSP app to experiment/test the model.


## Combo Group Examples
See [Combo Groups](recorder/combo-groups.md)

## Pipelines and Architecture

See [Pipelines and Architecture](pipeline-and-architecture.md)

## Disclamer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
