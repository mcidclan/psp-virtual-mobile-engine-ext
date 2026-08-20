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

### Provided Data

You can find default recorded samples for the first group of combos, and its related model in the recorder/generated folder. Noting that to get better results, you'll have to train your own model and record your own samples by performing your own gestures on your device.

## Special Note

As this is new work that has required time and effort, and which could be useful to other people and projects, please consider at least leaving a reference to the repository in your projects. This would allow people to trace its history and refer back to the original sources and sample code for a better understanding of the work related to the VME.

## Pipelines and Architecture

See [Pipelines and Architecture](pipeline-and-architecture.md)

## Disclamer

This project and code are provided as-is without warranty. Users assume full responsibility for any implementation or consequences. Use at your own discretion and risk

*m-c/d*
