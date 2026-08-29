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

#include <psptypes.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
#include <pspgu.h>
#include <me-core-mapper/me-core.h>

#include <vme-ext.h>
#include <debug.h>

#define Q_FORMAT 20

#define F2Q(v) (((int)((v) * (1 << Q_FORMAT))) /*& ((1 << (Q_FORMAT + 1)) - 1)*/)
#define Q2F(v) ((float)(int)(v) / (1u << Q_FORMAT))

/*
 *  Twiddles 
 */
 
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

#define FFT_POINT_COUNT (16)
#define FFT_VALUE_COUNT FFT_POINT_COUNT
#define SAMPLE_SIZE (FFT_VALUE_COUNT * 2)
#define SAMPLE_BYTE_COUNT (FFT_VALUE_COUNT * 2 * 4)

/*
 *  Samples 
 */
 
// Multi-tone signal (Harmonic 1 + Harmonic 3), amplitude max 2.0f
const int sampleReal7[16] __attribute__((aligned(16))) = {
    
    F2Q( 2.000000f), F2Q( 1.306563f), F2Q( 0.000000f), F2Q(-0.541196f),
    F2Q( 0.000000f), F2Q( 0.541196f), F2Q( 0.000000f), F2Q(-1.306563f),
    F2Q(-2.000000f), F2Q(-1.306563f), F2Q( 0.000000f), F2Q( 0.541196f),
    F2Q( 0.000000f), F2Q(-0.541196f), F2Q( 0.000000f), F2Q( 1.306563f)
};

// Pure sine wave (first harmonic)
const int sampleReal6[SAMPLE_SIZE] __attribute__((aligned(16))) = { // ok
  
  F2Q( 0.000000f), F2Q( 0.382683f), F2Q( 0.707107f), F2Q( 0.923880f),
  F2Q( 1.000000f), F2Q( 0.923880f), F2Q( 0.707107f), F2Q( 0.382683f),
  F2Q( 0.000000f), F2Q(-0.382683f), F2Q(-0.707107f), F2Q(-0.923880f),
  F2Q(-1.000000f), F2Q(-0.923880f), F2Q(-0.707107f), F2Q(-0.382683f),
  
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// Nyquist signal
const int sampleReal5[SAMPLE_SIZE] __attribute__((aligned(16))) = { // ok
  
    F2Q( 1.0f), F2Q(-1.0f), F2Q( 1.0f), F2Q(-1.0f),
    F2Q( 1.0f), F2Q(-1.0f), F2Q( 1.0f), F2Q(-1.0f),
    F2Q( 1.0f), F2Q(-1.0f), F2Q( 1.0f), F2Q(-1.0f),
    F2Q( 1.0f), F2Q(-1.0f), F2Q( 1.0f), F2Q(-1.0f),

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// Second harmonic cos wave
const int sampleReal4[SAMPLE_SIZE] __attribute__((aligned(16))) = { // ok

  F2Q( 1.0f), F2Q( 0.7071f), F2Q( 0.0f), F2Q(-0.7071f),
  F2Q(-1.0f), F2Q(-0.7071f), F2Q( 0.0f), F2Q( 0.7071f),
  F2Q( 1.0f), F2Q( 0.7071f), F2Q( 0.0f), F2Q(-0.7071f),
  F2Q(-1.0f), F2Q(-0.7071f), F2Q( 0.0f), F2Q( 0.7071f),
  
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// First harmonic cos wave
const int sampleReal3[SAMPLE_SIZE] __attribute__((aligned(16))) = { // ok
  
  F2Q( 1.000000f), F2Q( 0.923880f), F2Q( 0.707107f), F2Q( 0.382683f),
  F2Q( 0.000000f), F2Q(-0.382683f), F2Q(-0.707107f), F2Q(-0.923880f),
  F2Q(-1.000000f), F2Q(-0.923880f), F2Q(-0.707107f), F2Q(-0.382683f),
  F2Q( 0.000000f), F2Q( 0.382683f), F2Q( 0.707107f), F2Q( 0.923880f),
  
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// Shifted impulse
const int sampleReal2[SAMPLE_SIZE] __attribute__((aligned(16))) = { // ok
  
  F2Q(0.0f), F2Q(0.0f), F2Q(0.0f), F2Q(0.0f),
  F2Q(1.0f), F2Q(0.0f), F2Q(0.0f), F2Q(0.0f),
  F2Q(0.0f), F2Q(0.0f), F2Q(0.0f), F2Q(0.0f),
  F2Q(0.0f), F2Q(0.0f), F2Q(0.0f), F2Q(0.0f),
  
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// Unit impulse
const int sampleReal1[SAMPLE_SIZE] __attribute__((aligned(16))) = { // ok
  
  F2Q(1.0f), F2Q(0.0f), F2Q(0.0f), F2Q(0.0f),
  F2Q(0.0f), F2Q(0.0f), F2Q(0.0f), F2Q(0.0f),
  F2Q(0.0f), F2Q(0.0f), F2Q(0.0f), F2Q(0.0f),
  F2Q(0.0f), F2Q(0.0f), F2Q(0.0f), F2Q(0.0f),
  
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// DC signal
const int sampleReal0[SAMPLE_SIZE] __attribute__((aligned(16))) = { // ok
  
  F2Q(1.0f), F2Q(1.0f), F2Q(1.0f), F2Q(1.0f),
  F2Q(1.0f), F2Q(1.0f), F2Q(1.0f), F2Q(1.0f),
  F2Q(1.0f), F2Q(1.0f), F2Q(1.0f), F2Q(1.0f),
  F2Q(1.0f), F2Q(1.0f), F2Q(1.0f), F2Q(1.0f),
  
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

const int* _sampleReal[SAMPLE_SIZE] __attribute__((aligned(16))) = {
  
  sampleReal0, sampleReal1, sampleReal2, sampleReal3,
  sampleReal4, sampleReal5, sampleReal6, sampleReal7,
};

// imaginary
const int sampleImag[SAMPLE_SIZE] __attribute__((aligned(16))) = {
  
  F2Q(0.0f), F2Q(0.0f), F2Q(0.0f), F2Q(0.0f),
  F2Q(0.0f), F2Q(0.0f), F2Q(0.0f), F2Q(0.0f),
  F2Q(0.0f), F2Q(0.0f), F2Q(0.0f), F2Q(0.0f),
  F2Q(0.0f), F2Q(0.0f), F2Q(0.0f), F2Q(0.0f),
  
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/*
const int sampleReal[SAMPLE_SIZE] __attribute__((aligned(16))) = {
  
  0x0, 0x1, 0x2, 0x3,
  0x4, 0x5, 0x6, 0x7,
  0x8, 0x9, 0xa, 0xb,
  0xc, 0xd, 0xe, 0xf,
};
*/
