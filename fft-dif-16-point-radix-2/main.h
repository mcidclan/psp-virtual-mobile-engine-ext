#pragma once

#include <psptypes.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
#include <pspgu.h>
#include <me-core-mapper/me-core.h>

#include <vme-ext.h>
#include <debug.h>

//#define TO_Q23(f) ((u32)(u32)((f) * 8388608.0f))

#define Q_FORMAT 23
#define F2Q(v) ((int)((v) * (1u << Q_FORMAT)))
#define Q2F(v) ((float)(int)(v) / (1u << Q_FORMAT))

const int REAL_TWIDDLES[15] = {
  
  // Stage 0
  F2Q( 1.0f),          // 0
  F2Q( 0.9238795f),
  F2Q( 0.7071068f),
  F2Q( 0.3826834f),
  F2Q( 0.0f),
  F2Q(-0.3826834f),
  F2Q(-0.7071068f),
  F2Q(-0.9238795f),
  
  // Stage 1
  F2Q( 1.0f),          // 8
  F2Q( 0.7071068f),
  F2Q( 0.0f),
  F2Q(-0.7071068f),
  // Stage 2
  F2Q( 1.0f),          // 12
  F2Q( 0.0f),
  // Stage 3
  F2Q( 1.0f),          // 14
};

const int IMAG_TWIDDLES[15] = {
  
  // Stage 0
  F2Q( 0.0f),
  F2Q(-0.3826834f),
  F2Q(-0.7071068f),
  F2Q(-0.9238795f),
  F2Q(-1.0f),
  F2Q(-0.9238795f),
  F2Q(-0.7071068f),
  F2Q(-0.3826834f),
  // Stage 1
  F2Q( 0.0f),
  F2Q(-0.7071068f),
  F2Q(-1.0f),
  F2Q(-0.7071068f),
  // Stage 2
  F2Q( 0.0f),
  F2Q(-1.0f),
  // Stage 3
  F2Q( 0.0f)
};

const u32 _REAL_TWIDDLES[] = {  
  1 | 0x00300000,
  2 | 0x00300000,
  3 | 0x00300000,
  4 | 0x00300000,
  5 | 0x00300000,
  6 | 0x00300000,
  7 | 0x00300000,
  8 | 0x00300000,
  
  9,
  10,
  11,
  12,
  
  13,
  14,
  
  15,
};

const u32 _IMAG_TWIDDLES[] = {  
  1 | 0x00400000,
  2 | 0x00400000,
  3 | 0x00400000,
  4 | 0x00400000,
  5 | 0x00400000,
  6 | 0x00400000,
  7 | 0x00400000,
  8 | 0x00400000,
  
  9,
  10,
  11,
  12,
  
  13,
  14,
  
  15
};
