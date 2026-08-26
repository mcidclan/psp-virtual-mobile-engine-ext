#!/bin/bash

set -e

echo "Starting global scan and build"

find . -name "CMakeLists.txt" | while read -r cmake_path; do
  dir=$(dirname "$cmake_path")
  build_dir="$dir/build"
  if [ "$dir" = "." ]; then
    continue
  fi
  echo "Building: $dir"
  mkdir -p "$build_dir"
  find "$build_dir" -maxdepth 1 -type f \
    ! -name "usbhostfs_pc" \
    ! -name "*.png" \
    -delete

  cd "$build_dir"
  cmake ..
  make -j"$(nproc)"
  cd - > /dev/null
done

echo "All projects built successfully"

