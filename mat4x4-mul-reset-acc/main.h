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

#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
#include <me-core-mapper/me-core.h>
#include <vme-ext.h>
#include <debug.h>

static inline void vfpuMat4x4MulVec(void* const dst,
  const void* const mat, const void* const vec) {
  
  asm volatile (
    ".set push;"
    ".set noreorder;"
    ".set volatile;"
    
    "move    $t0, %1;"
    "lv.q    C000, 0($t0);"
    "lv.q    C010, 16($t0);"
    "lv.q    C020, 32($t0);"
    "lv.q    C030, 48($t0);"
    "lv.q    C100, 0(%2);"
    "vtfm4.q C200, M000, C100;"
    "sv.q    C200, %0;"
    
    ".set pop;"
    
    : "+m"(*(ScePspFVector4*)dst)
    : "r"(mat), "r"(vec)
    : "$t0", "memory"
  );
}
