#ifndef VME_EXT_H
#define VME_EXT_H

// SPDX-License-Identifier: MIT
// Copyright (c) 2026 m-c/d, mcidclan

#ifdef __cplusplus
extern "C" {
#endif

static inline void setPadded4x4(void* const dst, const unsigned int* const data) {
  
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
    : "r"(dst), "r"(mat)
    : "$t0","$t1","$t2","$t3","$t4","$t5","$t6","$t7","$t8","$t9", "memory"
  );
}

#ifdef __cplusplus
}
#endif

#endif

