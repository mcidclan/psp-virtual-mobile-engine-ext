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
    print(f"\nReading {path}")
    with open(path, 'rb') as f:
      index = 0
      while True:
        offset = f.tell()
        raw = f.read(4)
        if len(raw) < 4:
          break
        label = struct.unpack('<i', raw)[0]
        print(f"record {index:4d}  offset={offset:6d}  label={label}")
        raw = f.read(INPUT_SIZE)
        if len(raw) < INPUT_SIZE:
          raise ValueError(f"truncated file: {path}")
        values = struct.unpack('128b', raw)
        samples.append(values)
        labels.append(label)
        index += 1
        
  X = np.array(samples, dtype=np.float32) / 128.0
  Y = np.array(labels, dtype=np.int32)
  return X, Y

#def relu(x):
#  return np.maximum(0, x)

#def relu_grad(x):
#  return (x > 0).astype(np.float32)

def relu(x):
  return np.where(x > 0, x, 0.01 * x)

def relu_grad(x):
  return np.where(x > 0, 1.0, 0.01).astype(np.float32)
  
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

def confusion_matrix(preds, Y, n_classes):
  cm = np.zeros((n_classes, n_classes), dtype=int)
  for p, y in zip(preds, Y):
    cm[y, p] += 1
  return cm

if __name__ == '__main__':
  dataset_paths = sys.argv[1:]
  print(f"loading {len(dataset_paths)} file(s)...")
  X, Y = load_dataset(dataset_paths)
  N = X.shape[0]
  print("\nSummary:")
  print(f"{N} samples loaded")
  print(f"labels found: {np.unique(Y)}")
  
  for c in np.unique(Y):
    print(f"  class {c}: {np.sum(Y == c)} samples")

  print("\nClass mean differences:")
  for c1 in range(4):
    for c2 in range(c1 + 1, 4):
      a = X[Y == c1].mean(axis=0)
      b = X[Y == c2].mean(axis=0)
      diff = np.mean(np.abs(a - b))
      print(f"class {c1} vs class {c2}: {diff:.6f}")
    
  print("\nChecking for identical samples across different labels...")
  dup_found = False

  for i in range(N):
    for j in range(i + 1, N):
      if Y[i] != Y[j] and np.array_equal(X[i], X[j]):
        print(
          f"  IDENTICAL samples at index {i} "
          f"(label {Y[i]}) and {j} (label {Y[j]})"
        )
        dup_found = True

  if not dup_found:
    print("  none found")

  rng = np.random.RandomState(42)
  idx = rng.permutation(N)
  split = int(N * 0.75)
  train_idx = idx[:split]
  test_idx  = idx[split:]
  
  X_train = X[train_idx]
  Y_train = Y[train_idx]
  X_test = X[test_idx]
  Y_test = Y[test_idx]

  print(
    f"\ntrain: {len(train_idx)} samples, "
    f"test: {len(test_idx)} samples"
  )

  #X_train = X
  #Y_train = Y
  #X_test = X
  #Y_test = Y
  #print(
  #  f"\ntrain: {len(X_train)} samples, "
  #  f"test: {len(X_test)} samples"
  #)

  np.random.seed(42)

  W1 = np.random.randn(
    INPUT_SIZE,
    HIDDEN_SIZE
  ).astype(np.float32) * 0.01

  W2 = np.random.randn(
    HIDDEN_SIZE,
    OUTPUT_SIZE
  ).astype(np.float32) * 0.01

  N_train = X_train.shape[0]

  for epoch in range(EPOCHS):
    Z1, A1, Z2, A2 = forward(
      X_train,
      W1,
      W2
    )

    if epoch % 100 == 0:
      print("hidden activation mean:", np.mean(A1))
      print("hidden zeros:", np.mean(A1 == 0) * 100, "%")
    
    loss = cross_entropy(
      A2,
      Y_train,
      N_train
    )

    dW1, dW2 = backward(
      X_train,
      Z1,
      A1,
      A2,
      Y_train,
      W2,
      N_train
    )

    if epoch % 100 == 0:
      print("dW2:")
      print(dW2)

    W1 -= LR * dW1
    W2 -= LR * dW2

    if epoch % 100 == 0:
      preds = np.argmax(A2, axis=1)
      acc = np.mean(
        preds == Y_train
      ) * 100
      print(
        f"epoch {epoch:4d} "
        f"loss={loss:.4f} "
        f"train_acc={acc:.1f}%"
      )

  print("\n=== Final evaluation ===")
  _, _, _, A2_train = forward(
    X_train,
    W1,
    W2
  )
  preds_train = np.argmax(
    A2_train,
    axis=1
  )

  print(
    f"train accuracy: "
    f"{np.mean(preds_train == Y_train)*100:.1f}%"
  )
  _, _, _, A2_test = forward(
    X_test,
    W1,
    W2
  )
  preds_test = np.argmax(
    A2_test,
    axis=1
  )

  print(
    f"test accuracy: "
    f"{np.mean(preds_test == Y_test)*100:.1f}%"
  )

  n_classes = len(np.unique(Y))
  print("\nConfusion matrix (test set)")

  cm = confusion_matrix(
    preds_test,
    Y_test,
    n_classes
  )

  print(cm)
  print("\nConfusion matrix (full dataset)")
  _, _, _, A2_full = forward(
    X,
    W1,
    W2
  )

  preds_full = np.argmax(
    A2_full,
    axis=1
  )

  cm_full = confusion_matrix(
    preds_full,
    Y,
    n_classes
  )
  
  print(cm_full)

  #print("\nHidden representation:")
  #Z1_full, A1_full, _, _ = forward(
  #  X,
  #  W1,
  #  W2
  #)
  #for i in range(N):
  #  print(
  #    "label",
  #    Y[i],
  #    "->",
  #    A1_full[i]
  #  )
  #print("\n\n")
  #for i in range(N):
  #  print(
  #    "label",
  #    Y[i],
  #    "->",
  #    Z1_full[i]
  #  )
