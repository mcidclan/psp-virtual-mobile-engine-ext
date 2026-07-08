import numpy as np
import struct
import sys

INPUT_SIZE  = 128
HIDDEN_SIZE = 4
OUTPUT_SIZE = 4

def load_dataset(paths):
  samples = []
  labels = []
  
  for path in paths:
    with open(path, 'rb') as f:
      while True:
        raw = f.read(4)
        if len(raw) < 4:
          break
        label = struct.unpack('<i', raw)[0]
        raw = f.read(INPUT_SIZE)
        if len(raw) < INPUT_SIZE:
          raise ValueError("truncated file")
        values = struct.unpack('128b', raw)
        samples.append(values)
        labels.append(label)

  X = np.array(samples, dtype=np.float32) / 128.0
  Y = np.array(labels, dtype=np.int32)
  return X, Y

def load_model(path):
  weights = []

  with open(path, 'rb') as f:
    while True:
      raw = f.read(4)
      if len(raw) < 4:
        break
      value = struct.unpack('<I', raw)[0]
      if value & 0x800000:
        value -= 0x1000000
      weights.append(value / 128.0)
      
  weights = np.array(weights, dtype=np.float32)
  w1_size = INPUT_SIZE * HIDDEN_SIZE
  W1 = weights[:w1_size]
  W2 = weights[w1_size:]
  W1 = W1.reshape(
    HIDDEN_SIZE,
    INPUT_SIZE
  ).T
  W2 = W2.reshape(
    OUTPUT_SIZE,
    HIDDEN_SIZE
  ).T
  return W1, W2

def relu(x):
  return np.where(x > 0, x, 0.01 * x)

def softmax(x):
  e = np.exp(
    x - np.max(x, axis=1, keepdims=True)
  )
  return e / e.sum(
    axis=1,
    keepdims=True
  )

def forward(X, W1, W2):
  Z1 = X @ W1
  A1 = relu(Z1)
  Z2 = A1 @ W2
  A2 = softmax(Z2)
  return A2

def confusion_matrix(preds, Y):
  n = len(np.unique(Y))
  cm = np.zeros(
    (n, n),
    dtype=int
  )
  for p, y in zip(preds, Y):
    cm[y, p] += 1
  return cm

if __name__ == "__main__":
  if len(sys.argv) < 3:
    print("usage: test_model.py model.bin sample.00.bin ...")
    sys.exit(1)

  model_path = sys.argv[1]
  files = sys.argv[2:]

  X, Y = load_dataset(files)
  print(f"{len(Y)} samples loaded")

  W1, W2 = load_model(model_path)

  print("W1:")
  print(W1)
  print("W2:")
  print(W2)

  A2 = forward(X, W1, W2)

  preds = np.argmax(A2, axis=1)
  acc = np.mean(preds == Y) * 100

  print(f"accuracy: {acc:.1f}%")
  print("Predictions:")

  for i in range(len(Y)):
    print(f"{i:3d} true={Y[i]} pred={preds[i]} score={A2[i]}")

  print("Confusion matrix:")
  print(confusion_matrix(preds, Y))

