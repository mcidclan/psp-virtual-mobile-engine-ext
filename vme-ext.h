#ifndef VME_EXT_H
#define VME_EXT_H

// SPDX-License-Identifier: MIT
// Copyright (c) 2026 m-c/d, mcidclan

#define VME_TOP_BUFFER_0 (VME_TOP_BUFFERS + 8192*0)
#define VME_TOP_BUFFER_1 (VME_TOP_BUFFERS + 8192*1)
#define VME_TOP_BUFFER_2 (VME_TOP_BUFFERS + 8192*2)
#define VME_TOP_BUFFER_3 (VME_TOP_BUFFERS + 8192*3)

#define VME_BASE_BUFFER_0 (VME_BASE_BUFFERS + 8192*0)
#define VME_BASE_BUFFER_1 (VME_BASE_BUFFERS + 8192*1)
#define VME_BASE_BUFFER_2 (VME_BASE_BUFFERS + 8192*2)
#define VME_BASE_BUFFER_3 (VME_BASE_BUFFERS + 8192*3)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  u32 x, y, z, w;
} __attribute__((aligned(16))) vmeExtVec4u32;

typedef struct {
  vmeExtVec4u32 x, y, z, w;
} __attribute__((aligned(16))) vmeExtMat4x4u32;

typedef struct {
  vmeExtVec4u32 row;
} __attribute__((aligned(16))) vmeExtRow4u32;


static inline void vmeExtSetPadded4x4(void* const dst, const void* const data) {
  
  asm volatile (
    ".set push;"
    ".set noreorder;"
    
    "move $t8, %0; move $t9, %1;"
    "lw $t0, 0($t9);  lw $t1, 4($t9);  lw $t2, 8($t9);   lw $t3, 12($t9);"
    "lw $t4, 16($t9); lw $t5, 20($t9); lw $t6, 24($t9);  lw $t7, 28($t9);"
    "sw $t0, 0($t8);  sw $t1, 4($t8);  sw $t2, 8($t8);   sw $t3, 12($t8);"
    "sw $t4, 32($t8); sw $t5, 36($t8); sw $t6, 40($t8);  sw $t7, 44($t8);"
    "lw $t0, 32($t9); lw $t1, 36($t9); lw $t2, 40($t9);  lw $t3, 44($t9);"
    "lw $t4, 48($t9); lw $t5, 52($t9); lw $t6, 56($t9);  lw $t7, 60($t9);"
    "sw $t0, 64($t8); sw $t1, 68($t8); sw $t2, 72($t8);  sw $t3, 76($t8);"
    "sw $t4, 96($t8); sw $t5, 100($t8); sw $t6, 104($t8); sw $t7, 108($t8);"
    
    ".set pop;"
    :
    : "r"(dst), "r"(data)
    : "$t0","$t1","$t2","$t3","$t4","$t5","$t6","$t7","$t8","$t9", "memory"
  );
}

static inline void vmeExtSetWord4(void* const dst, const void* const data) {
  
  asm volatile (
    ".set push;"
    ".set noreorder;"
    
    "move $t8, %0; move $t9, %1;"
    "lw $t0, 0($t9); lw $t1, 4($t9); lw $t2, 8($t9); lw $t3, 12($t9);"
    "sw $t0, 0($t8); sw $t1, 4($t8); sw $t2, 8($t8); sw $t3, 12($t8);"
    
    ".set pop;"
    :
    : "r"(dst), "r"(data)
    : "$t0","$t1","$t2","$t3","$t8","$t9", "memory"
  );
}

static inline void vmeExtGetWord4FromPaddedMac(const void* const data, void* const dst) {
  
  asm volatile (
    ".set push;"
    ".set noreorder;"
    
    "move $t8, %0; move $t9, %1;"
    "lw $t0, 28($t9); lw $t1, 60($t9); lw $t2, 92($t9); lw $t3, 124($t9);"
    "sw $t0, 0($t8); sw $t1, 4($t8); sw $t2, 8($t8); sw $t3, 12($t8);"
    
    ".set pop;"
    :
    : "r"(dst), "r"(data)
    : "$t0","$t1","$t2","$t3","$t8","$t9", "memory"
  );
}

#ifdef __cplusplus
}
#endif

#endif

