import numpy as np
import struct
import sys

INPUT_SIZE  = 128
HIDDEN_SIZE = 4
OUTPUT_SIZE = 4
EPOCHS      = 1000
LR          = 0.01

def load_dataset(paths):
  samples = []
  labels  = []
  for path in paths:
    with open(path, 'rb') as f:
      while True:
        raw = f.read(4)
        if len(raw) < 4:
          break
        label = struct.unpack('<i', raw)[0]
        raw = f.read(INPUT_SIZE)
        if len(raw) < INPUT_SIZE:
          raise ValueError(f"truncated file: {path}")
        values = struct.unpack('128b', raw)
        samples.append(values)
        labels.append(label)
  X = np.array(samples, dtype=np.float32) / 128.0
  Y = np.array(labels,  dtype=np.int32)
  return X, Y

def relu(x):
  return np.maximum(0, x)

def relu_grad(x):
  return (x > 0).astype(np.float32)

def softmax(x):
  e = np.exp(x - np.max(x, axis=1, keepdims=True))
  return e / e.sum(axis=1, keepdims=True)

def forward(X, W1, W2):
  Z1 = X @ W1
  A1 = relu(Z1)
  Z2 = A1 @ W2
  A2 = softmax(Z2)
  return Z1, A1, Z2, A2

def cross_entropy(A2, Y, N):
  return -np.sum(np.log(A2[np.arange(N), Y] + 1e-9)) / N

def backward(X, Z1, A1, A2, Y, W2, N):
  dZ2 = A2.copy()
  dZ2[np.arange(N), Y] -= 1
  dZ2 /= N
  dW2 = A1.T @ dZ2
  dA1 = dZ2 @ W2.T
  dZ1 = dA1 * relu_grad(Z1)
  dW1 = X.T @ dZ1
  return dW1, dW2

def save_model(path, W1, W2):
  with open(path, 'wb') as f:
    for neuron in range(HIDDEN_SIZE):
      for w in W1[:, neuron]:
        val = int(np.clip(round(w * 128), -128, 127)) & 0xFFFFFF
        f.write(struct.pack('<I', val))
    for neuron in range(OUTPUT_SIZE):
      for w in W2[:, neuron]:
        val = int(np.clip(round(w * 128), -128, 127)) & 0xFFFFFF
        f.write(struct.pack('<I', val))

if __name__ == '__main__':
  if len(sys.argv) < 3:
    print("usage: train.py <model.bin> <sample.00.bin> [sample.01.bin ...]")
    sys.exit(1)
  model_path    = sys.argv[1]
  dataset_paths = sys.argv[2:]
  print(f"loading {len(dataset_paths)} file(s)...")
  X, Y = load_dataset(dataset_paths)
  N    = X.shape[0]
  print(f"{N} samples loaded, classes {np.unique(Y)}")
  np.random.seed(42)
  W1 = np.random.randn(INPUT_SIZE,  HIDDEN_SIZE).astype(np.float32) * 0.01
  W2 = np.random.randn(HIDDEN_SIZE, OUTPUT_SIZE).astype(np.float32) * 0.01
  for epoch in range(EPOCHS):
    Z1, A1, Z2, A2 = forward(X, W1, W2)
    loss           = cross_entropy(A2, Y, N)
    dW1, dW2       = backward(X, Z1, A1, A2, Y, W2, N)
    W1 -= LR * dW1
    W2 -= LR * dW2
    if epoch % 100 == 0:
      preds    = np.argmax(A2, axis=1)
      accuracy = np.mean(preds == Y) * 100
      print(f"epoch {epoch:4d}  loss={loss:.4f}  acc={accuracy:.1f}%")
  print("saving model...")
  save_model(model_path, W1, W2)
  print(f"model saved to {model_path}")
