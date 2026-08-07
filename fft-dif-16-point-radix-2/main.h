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

#define Q_FORMAT 21
#define F2Q(v) ((int)((v) * (1u << Q_FORMAT)))
#define Q2F(v) ((float)(int)(v) / (1u << Q_FORMAT))

const int REAL_TWIDDLES[] = {
  // Stage 0
  F2Q(1.0f), F2Q(0.9238795f), F2Q(0.7071068f), F2Q(0.3826834f),
  F2Q(0.0f), F2Q(-0.3826834f), F2Q(-0.7071068f), F2Q(-0.9238795f),
  // Stage 1
  F2Q(1.0f), F2Q(0.7071068f), F2Q(0.0f), F2Q(-0.7071068f),
  // Stage 2
  F2Q(1.0f), F2Q(0.0f),
  // Stage 3
  F2Q(1.0f)
};

const int IMAG_TWIDDLES[] = {
  // Stage 0
  F2Q(0.0f), F2Q(-0.3826834f), F2Q(-0.7071068f), F2Q(-0.9238795f),
  F2Q(-1.0f), F2Q(-0.9238795f), F2Q(-0.7071068f), F2Q(-0.3826834f),
  // Stage 1
  F2Q(0.0f), F2Q(-0.7071068f), F2Q(-1.0f), F2Q(-0.7071068f),
  // Stage 2
  F2Q(0.0f), F2Q(-1.0f),
  // Stage 3
  F2Q(0.0f)
};
