import struct
import numpy as np

OUTPUT = "dataset.bin"
SAMPLES_PER_CLASS = 50

with open(OUTPUT, 'wb') as f:
  for label in range(4):
    for _ in range(SAMPLES_PER_CLASS):
      f.write(struct.pack('<I', label))
      for i in range(128):
        val = int(np.clip(np.random.randint(-128, 128), -128, 127)) & 0xFF
        f.write(struct.pack('<I', val))

print(f"generated {4 * SAMPLES_PER_CLASS} samples in {OUTPUT}")

