#pragma once

#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
#include <me-core-mapper/me-core.h>

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

static inline void vmeGetVectorFromDistributedVMac(void* const dst) {
  
  vme_dma(MEMORY, ADDR, useg_mem((u32)dst));
  vme_dma(SPAD, OFFSET, 3);
  vme_dma(ITERATION, DIMS, (4 - 1), (0x800 - 1));
  vme_dma(ITERATION, STEP, 4);
  vme_dma(GROUP, SIZE, 0);
  vme_dma(GROUP, STEPS, 0);
  vme_dma(CTRL, VALUE, 0x58);
  
  meCoreDMACPrimWaitTransferFinish();
}

static inline void vmeSetDistributed4x4(void* const src) {

  vme_dma(MEMORY, ADDR, useg_mem((u32)src));
  vme_dma(ITERATION, DIMS, (16 - 1), 0);
  vme_dma(ITERATION, STEP, 4);
  vme_dma(SPAD, OFFSET, 0x8000);
  vme_dma(GROUP, SIZE, (4 - 1));
  vme_dma(GROUP, STEPS, 0x800, 1);
  vme_dma(CTRL, VALUE, 0x50);
  
  meCoreDMACPrimWaitTransferFinish();
}
