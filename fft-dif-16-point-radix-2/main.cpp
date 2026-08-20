/*
 * Copyright mcidclan, m-c/d 2026
 */
#include "main.h"

PSP_MODULE_INFO("vme-fft", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();
VME_DEBUG_SET_BUFFER_WORD_COUNT(16);

#define FFT_ADD_RSHIFT ADD_IBUF_RSHIFT
#define FFT_SUB_RSHIFT SUB_BACK_FROM_FRONT_RSHIFT

#define FFT_STAGE_0_STRIDE (FFT_VALUE_COUNT / 2)
#define FFT_STAGE_1_STRIDE (FFT_STAGE_0_STRIDE / 2)
#define FFT_STAGE_2_STRIDE (FFT_STAGE_1_STRIDE / 2)
#define FFT_STAGE_3_STRIDE (FFT_STAGE_2_STRIDE / 2)

#define FFT_TWIDDLE_0_OFFSET (0)
#define FFT_TWIDDLE_1_OFFSET (FFT_TWIDDLE_0_OFFSET + FFT_STAGE_0_STRIDE)
#define FFT_TWIDDLE_2_OFFSET (FFT_TWIDDLE_1_OFFSET + FFT_STAGE_1_STRIDE)
#define FFT_TWIDDLE_3_OFFSET (FFT_TWIDDLE_2_OFFSET + FFT_STAGE_2_STRIDE)

#define FFT_TWIDDLE_BASE_OFFSET (128)
#define FFT_ROUND (1 << 6)

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[0])
#define meFFTIndex   (meLibSharedMemory[1])

/*
 * Context 1: Handles the additions and subtractions for both legs of the
 * butterfly (upper and lower legs). This is done for real and imaginary parts
 */
VME_LIB_CONTEXT_GENERATOR(lowerLegGenerator, param, {
  
  const int tOffset[] = {
    
    FFT_TWIDDLE_0_OFFSET, FFT_TWIDDLE_1_OFFSET,
    FFT_TWIDDLE_2_OFFSET, FFT_TWIDDLE_3_OFFSET,
  };
  
  vme_set(ENABLE, FU_1, 0b0101 << 28);

  vme_icn(AGU_BASE, 0x1010);
  vme_icn(AGU_WRITE, 0x1010);

  const int count = FFT_STAGE_0_STRIDE;
  
  const int stageId = *((int*)param);
  const int offset = tOffset[stageId];
  const int tStride = (FFT_VALUE_COUNT >> (stageId + 1));
  const int shift = Q_FORMAT;
  const int round = FFT_ROUND;
  
  {
    const u32 op = fu_op(MUL_VEC_RSHIFT);
    
    vme_pe0(vme_fu(PRIMARY), vme_mux(BASE_1, TOP_0), op, shift, round); // real (LowerLeg) vs real (Twiddles)
    vme_pe1(vme_fu(PRIMARY), vme_mux(BASE_3, TOP_2), op, shift, round); // imag (LowerLeg) vs imag (Twiddles)
    vme_pe2(vme_fu(PRIMARY), vme_mux(BASE_1, TOP_2), op, shift, round); // real (LowerLeg) vs imag (Twiddles)
    vme_pe3(vme_fu(PRIMARY), vme_mux(BASE_3, TOP_0), op, shift, round); // imag (LowerLeg) vs real (Twiddles)
    
    vme_pe0(agu_top(MODE), agu_mode(2), FFT_TWIDDLE_BASE_OFFSET + offset);
    vme_pe0(agu_top(COUNT), VME_DEF_STEP, (count - 1));
    vme_pe0(agu_top(INNER_0),  0x00010000, tStride - 1);
    vme_pe0(agu_top(FORMAT_0), 0x00020000);
    
    vme_pe1(agu_base(MODE), VME_DEF_MODE);
    vme_pe1(agu_base(COUNT), VME_DEF_STEP, (count - 1));
    
    vme_pe0(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_6);
    vme_pe0(agu_write(COUNT), VME_DEF_STEP, (count - 1));
  }
  
  {
    // >>
    vme_pe1(vme_fu(SECONDARY), vme_mux(STAGING_0, STAGING_1), fu_op(FFT_SUB_RSHIFT), 1, round);
    vme_pe3(vme_fu(SECONDARY), vme_mux(STAGING_3, STAGING_2), fu_op(FFT_ADD_RSHIFT), 1, round);
    
    vme_pe1(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_9, 0);
    vme_pe1(agu_write(COUNT), VME_DEF_STEP, (count - 1));
  }
});

/*
 * Context 2: Handles the complex multiplications between the twiddle factors
 * and the lower leg, leveraging the 64-bit accumulator to perform the entire set
 * of operations within a single pipeline. This is done for real and imaginary parts
 */
VME_LIB_CONTEXT_GENERATOR(butterflyGenerator, param, {

  vme_icn(AGU_TOP, 0x1010);
  vme_icn(AGU_WRITE, 0x1010);

  const int count = FFT_STAGE_0_STRIDE;
  const int stride = *((int*)param);
  const int round = FFT_ROUND;

  // TOP_0: real upper leg
  // TOP_1: real lower leg
  // TOP_2: imaginary upper leg
  // TOP_3: imaginary lower leg
  {
    // upper leg (real and imaginary parts)
    const u32 op = fu_op(FFT_ADD_RSHIFT);
    
    vme_pe0(vme_fu(PRIMARY), vme_mux(TOP_1, TOP_0), op, 1, round); // real parts
    vme_pe2(vme_fu(PRIMARY), vme_mux(TOP_3, TOP_2), op, 1, round); // imaginary parts

    vme_pe0(agu_top(MODE), VME_DEF_MODE, 0x800 - count);
    vme_pe0(agu_top(COUNT), VME_DEF_STEP, count - 1);
    
    vme_pe0(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_6, 0x800 - count);
    vme_pe0(agu_write(COUNT), VME_DEF_STEP, count - 1);
  }

  {
    // lower leg (real and imaginary parts)
    const u32 op = fu_op(FFT_SUB_RSHIFT);
    
    // >>
    vme_pe1(vme_fu(PRIMARY), vme_mux(TOP_0, TOP_1), op, round); // real parts
    vme_pe3(vme_fu(PRIMARY), vme_mux(TOP_2, TOP_3), op, round); // imaginary parts
    
    vme_pe1(agu_top(MODE), VME_DEF_MODE, stride);
    vme_pe1(agu_top(COUNT), VME_DEF_STEP, count - 1);
    
    vme_pe1(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_6);
    vme_pe1(agu_write(COUNT), VME_DEF_STEP, count - 1);
  }
});

// transferts
static void memToVme(u32 src, u32 dst, u16 size) {
  
  vme_dma(MEMORY, ADDR, useg_mem(src));
  vme_dma(SPAD, OFFSET, dst);
  vme_dma(ITERATION, DIMS, size - 1);
  vme_dma(CTRL, VALUE, 0x40);
  
  meCoreDMACPrimWaitTransferFinish();
}

static void vmeToMem(u32 src, u32 dst, u16 size) {
  
  vme_dma(MEMORY, ADDR, useg_mem(dst));
  vme_dma(SPAD, OFFSET, src);
  vme_dma(ITERATION, DIMS, size - 1);
  vme_dma(CTRL, VALUE, 0x48);
  
  meCoreDMACPrimWaitTransferFinish();
}

static void stageToVme(u32 src, u32 dst, u16 stride, u16 points, u8 slice) {
  
  const int half = points/2;
  vme_dma(MEMORY, ADDR, useg_mem(src));
  vme_dma(SPAD, OFFSET, dst - (slice * (half + stride)) - half);
  vme_dma(ITERATION, STEP, 0);
  vme_dma(ITERATION, DIMS, 1, (points - 1) + stride);
  vme_dma(GROUP, SIZE, (stride * 2) - 1);
  vme_dma(GROUP, STEPS, stride, 1);
  vme_dma(CTRL, VALUE, 0x50);

  meCoreDMACPrimWaitTransferFinish();
}

void processStage(void* butterfly) {
  
  // todo: merge butterfly pre and post context transfers
  vmeLibStart();
  vmeLibLoadCustomContext(butterfly);
  vmeLibFinish();
  //vmeLibProcessAsync();
  //vmeLibFinishAsync();
}

// tmp
typedef struct Complex {
  
  u32 real;
  u32 imag;
} Complex;

Complex scratchpadToEdram() {
  
  Complex complex {
    0x40000000,
    0x40000000 | (FFT_VALUE_COUNT * 4 * 2)
  };
  
  const u32 scratchpadReal = VME_BASE_BUFF1_WOFF - FFT_STAGE_0_STRIDE;
  const u32 scratchpadImag = VME_BASE_BUFF3_WOFF - FFT_STAGE_0_STRIDE;
  vmeToMem(scratchpadReal, complex.real, FFT_VALUE_COUNT);
  vmeToMem(scratchpadImag, complex.imag, FFT_VALUE_COUNT);
  
  return complex;
}

void debug(void* data) {
}
void meLibOnProcess(void) {
    
  meLibExceptionHandlerInit(0);
  
  /*
   * Init FFT data
   */
  const int bStrides[] = {
    FFT_STAGE_0_STRIDE, FFT_STAGE_1_STRIDE,
    FFT_STAGE_2_STRIDE, FFT_STAGE_3_STRIDE,
  };
  vmeLibGenContext(butterflyGenerator, butterfly0, (void*)&(bStrides[0]));
  vmeLibGenContext(butterflyGenerator, butterfly1, (void*)&(bStrides[1]));
  vmeLibGenContext(butterflyGenerator, butterfly2, (void*)&(bStrides[2]));
  vmeLibGenContext(butterflyGenerator, butterfly3, (void*)&(bStrides[3]));

  const int sIndex[] = { 0, 1, 2, 3 };
  vmeLibGenContext(lowerLegGenerator, lowerLeg0, (void*)&(sIndex[0]));
  vmeLibGenContext(lowerLegGenerator, lowerLeg1, (void*)&(sIndex[1]));
  vmeLibGenContext(lowerLegGenerator, lowerLeg2, (void*)&(sIndex[2]));
  vmeLibGenContext(lowerLegGenerator, lowerLeg3, (void*)&(sIndex[3]));
  
  #define rTwiddles REAL_TWIDDLES
  #define iTwiddles IMAG_TWIDDLES
  meCoreDcacheWritebackRange((void*)rTwiddles, sizeof(rTwiddles)); // todo: remove
  meCoreDcacheWritebackRange((void*)iTwiddles, sizeof(iTwiddles)); // todo: remove

  meCoreDcacheWritebackRange((void*)sampleImag, SAMPLE_BYTE_COUNT);
  
  vmeLibEnable();
  vmeLibWipe();
  
  memToVme((u32)rTwiddles, VME_TOP_BUFF0_WOFF + FFT_TWIDDLE_BASE_OFFSET, sizeof(rTwiddles) / 4);
  memToVme((u32)iTwiddles, VME_TOP_BUFF2_WOFF + FFT_TWIDDLE_BASE_OFFSET, sizeof(iTwiddles) / 4);
  vmeLibDisable();

  /*
   * Dynamic
   */
  while (1) {
    
    vmeLibEnable();
  
    const int* sampleReal = _sampleReal[meFFTIndex];
    meCoreDcacheWritebackRange((void*)sampleReal, SAMPLE_BYTE_COUNT);
  
    // stage 0
    {
      stageToVme((u32)sampleReal, VME_TOP_BUFF1_WOFF, FFT_STAGE_0_STRIDE, FFT_VALUE_COUNT, 0);
      stageToVme((u32)sampleImag, VME_TOP_BUFF3_WOFF, FFT_STAGE_0_STRIDE, FFT_VALUE_COUNT, 0);
      processStage(vme_ctx(butterfly0));
      processStage(vme_ctx(lowerLeg0));
    }

    // stage 1  
    {
      Complex complex = scratchpadToEdram();
      stageToVme((u32)complex.real, VME_TOP_BUFF1_WOFF, FFT_STAGE_1_STRIDE, FFT_VALUE_COUNT, 0);
      stageToVme((u32)complex.imag, VME_TOP_BUFF3_WOFF, FFT_STAGE_1_STRIDE, FFT_VALUE_COUNT, 0);
      processStage(vme_ctx(butterfly1));
      processStage(vme_ctx(lowerLeg1));
    }

    // stage 2
    {
      Complex complex = scratchpadToEdram();
      stageToVme((u32)complex.real, VME_TOP_BUFF1_WOFF, FFT_STAGE_2_STRIDE, FFT_VALUE_COUNT, 0);
      stageToVme((u32)complex.imag, VME_TOP_BUFF3_WOFF, FFT_STAGE_2_STRIDE, FFT_VALUE_COUNT, 0);
      processStage(vme_ctx(butterfly2));
      processStage(vme_ctx(lowerLeg2));
    }
    
    // stage 3
    {
      Complex complex = scratchpadToEdram();
      stageToVme((u32)complex.real, VME_TOP_BUFF1_WOFF, FFT_STAGE_3_STRIDE, FFT_VALUE_COUNT, 0);
      stageToVme((u32)complex.imag, VME_TOP_BUFF3_WOFF, FFT_STAGE_3_STRIDE, FFT_VALUE_COUNT, 0);
      processStage(vme_ctx(butterfly3));
      processStage(vme_ctx(lowerLeg3));
    }

    vmeDebugFillAtWith(0, VME_BASE_BUFF1_WOFF - FFT_STAGE_0_STRIDE);
    vmeDebugFillAtWith(1, VME_BASE_BUFF3_WOFF - FFT_STAGE_0_STRIDE);

    vmeLibDisable();

    meCounter += 1;
  }
  
}

int main() {
  
  vmeDebugSetupBuffers();

  meLibDefaultInit();
  pspDebugScreenInit();
  pspDebugScreenPrintf("16-point-radix-2");

  int up = 1;
  SceCtrlData ctl;
  do {
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    
    if (up && (ctl.Buttons & PSP_CTRL_TRIANGLE)) {
      
      meFFTIndex += meFFTIndex < 7 ? 1 : 0;
      up = 0;
    }
    
    if (up && (ctl.Buttons & PSP_CTRL_CROSS)) {
      
      meFFTIndex -= meFFTIndex > 0 ? 1 : 0;
      up = 0;
    }
    
    if (!(ctl.Buttons)) {
      up = 1;
    }
    
    pspDebugScreenSetXY(1, 2);
    pspDebugScreenPrintf("meCounter: 0x%08lx    ", meCounter);

    pspDebugScreenSetXY(1, 4);
    pspDebugScreenPrintf("Real parts: ");
    vmeDebugDisplayBufferWithSpace('8', 0, 1, 5);
    
    pspDebugScreenSetXY(1, 10);
    pspDebugScreenPrintf("Imaginary parts: ");
    vmeDebugDisplayBufferWithSpace('8', 1, 1, 11);
    
    pspDebugScreenSetXY(1, 18);
    pspDebugScreenPrintf("Stimulus index: %lx", meFFTIndex);

    sceDisplayWaitVblankStart();
  } while (!(ctl.Buttons & PSP_CTRL_HOME));
  
  vmeDebugTouch();
  vmeDebugDumpBuffers();
  vmeDebugFreeBuffers();
  
  sceKernelExitGame();
  return 0;
}
