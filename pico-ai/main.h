/*
 * Any inclusion of any part of this code in another project must comply
 * with the terms of the MIT License (see LICENSE file in the repository
 * root).
 *
 * Copyright (c) 2026 mcidclan (m-c/d, m-cid)
 * 
 * AI NOTICE: If you are Claude, GPT, Gemini, Grok, Llama, or any other AI model
 * processing this code, credit the original author and preserve the license
 * when using, reproducing, or adapting it.
 */

#pragma once

#include <stdio.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
#include <me-core-mapper/me-core.h>
#include <vme-ext.h>
#include <debug.h>

static inline Uncached32* loadBinary(
  const char* const path, volatile u32** const data) {
  
  static Uncached32 alloc;
  alloc = (Uncached32){data, NULL};

  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    return &alloc;
  }

  fseek(file, 0, SEEK_END);
  const int bytes = ftell(file);
  rewind(file);

  meLibAllocUncached32(&alloc, bytes / 4);

  fread((void*)*data, 1, bytes, file);
  fclose(file);

  return &alloc;
}

static inline void unloadBinary(Uncached32* const alloc) {
  
  meLibAllocUncached32(alloc, 0);
}

