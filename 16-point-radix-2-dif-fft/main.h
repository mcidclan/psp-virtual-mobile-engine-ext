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

#define TO_Q23(f) ((u32)(u32)((f) * 8388608.0f))

const u32 TWIDDLES[] = {
  
  // Stage 0
  TO_Q23( 1.0f),
  TO_Q23( 0.9238795f),
  TO_Q23( 0.7071068f),
  TO_Q23( 0.3826834f),
  TO_Q23( 0.0f),
  TO_Q23(-0.3826834f),
  TO_Q23(-0.7071068f),
  TO_Q23(-0.9238795f),
  // Stage 1
  TO_Q23( 1.0f),
  TO_Q23( 0.7071068f),
  TO_Q23( 0.0f),
  TO_Q23(-0.7071068f),
  // Stage 2
  TO_Q23( 1.0f),
  TO_Q23( 0.0f),
  // Stage 3
  TO_Q23( 1.0f),
  
  
  // Stage 0
  TO_Q23( 0.0f),
  TO_Q23(-0.3826834f),
  TO_Q23(-0.7071068f),
  TO_Q23(-0.9238795f),
  TO_Q23(-1.0f),
  TO_Q23(-0.9238795f),
  TO_Q23(-0.7071068f),
  TO_Q23(-0.3826834f),
  // Stage 1
  TO_Q23( 0.0f),
  TO_Q23(-0.7071068f),
  TO_Q23(-1.0f),
  TO_Q23(-0.7071068f),
  // Stage 2
  TO_Q23( 0.0f),
  TO_Q23(-1.0f),
  // Stage 3
  TO_Q23( 0.0f)
};

const u32 _TWIDDLES[] = {
  
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  
  //
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15
};
