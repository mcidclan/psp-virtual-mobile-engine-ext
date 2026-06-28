#pragma once

#include <stdio.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
#include <me-core-mapper/me-core.h>
#include <vme-ext.h>
#include <debug.h>

static inline Uncached32* loadBinary(const char* const path) {

  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    return {NULL, NULL};
  }
  
  fseek(file, 0, SEEK_END);
  const int bytes = ftell(file);
  rewind(file);

  static volatile u32* var __attribute__((aligned(64))) = NULL;
  Uncached32 uvar = {&(var), NULL};
  meLibAllocUncached32(&uvar, bytes / 4);
  
  int read = fread(dst, 1, bytes, file);
  fclose(file);
  
  if (read != bytes) {
    return {NULL, NULL};
  }
  return &uvar;
}

static inline void unloadBinary(Uncached32* const uvar) {
  
  meLibAllocUncached32(uvar, 0);
}

