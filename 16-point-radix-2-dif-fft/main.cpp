/*
 * Copyright mcidclan, m-c/d 2026
 */
#include "main.h"

PSP_MODULE_INFO("vme-fft", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();
VME_DEBUG_SET_BUFFER_WORD_COUNT(64);

#define FFT_POINT_COUNT 16
#define FFT_VALUE_COUNT FFT_POINT_COUNT

#define FFT_STAGE_0_STRIDE (FFT_VALUE_COUNT / 2)
#define FFT_STAGE_1_STRIDE (FFT_STAGE_0_STRIDE / 2)
#define FFT_STAGE_2_STRIDE (FFT_STAGE_1_STRIDE / 2)
#define FFT_STAGE_3_STRIDE (FFT_STAGE_2_STRIDE / 2)

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[0])

VME_LIB_CONTEXT_GENERATOR(lowerLegGenerator, param, {

  
});

/*
  {
    // lower leg (twiddle factor, first layer)
    const u32 op = fu_op(MUL_DELAYED_BACK);
    //const u32 op = 0x00004000;
    vme_pe1(vme_fu(SECONDARY), vme_mux(TWIDDLES, STAGING_1), op);
    
    vme_pe2(agu_top(MODE), VME_DEF_MODE);
    vme_pe2(agu_top(COUNT), VME_DEF_STEP, count - 1);
  }
  */
VME_LIB_CONTEXT_GENERATOR(butterflyGenerator, param, {
  
  //vme_set(ENABLE, FU_1, 0b0100 << 28);
  vme_icn(AGU_TOP, 0x1010);
  //vme_icn(AGU_BASE, 0);
  vme_icn(AGU_WRITE, 0x1010);

  const int count = FFT_STAGE_0_STRIDE;
  const int stride = *((int*)param);

  //#define TWIDDLES TOP_2
  {
    // upper leg (real and imaginary parts)
    //const u32 op = fu_op(ADD_VEC);
    const u32 op = 0x00004000;
    
    vme_pe0(vme_fu(PRIMARY), vme_mux(TOP_1, TOP_0), op); // real parts
    vme_pe2(vme_fu(PRIMARY), vme_mux(TOP_3, TOP_2), op); // imaginary parts

    vme_pe0(agu_top(MODE), VME_DEF_MODE, 0x800 - count);
    vme_pe0(agu_top(COUNT), VME_DEF_STEP, count - 1);
    
    vme_pe0(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_6, 0x800 - count);
    vme_pe0(agu_write(COUNT), VME_DEF_STEP, count - 1);
  }

  {
    // lower leg (real and imaginary parts)
    //const u32 op = fu_op(SUB_BACK_FROM_FRONT);
    const u32 op = 0x00004000;
    
    vme_pe1(vme_fu(PRIMARY), vme_mux(TOP_0, TOP_1), op); // real parts
    vme_pe3(vme_fu(PRIMARY), vme_mux(TOP_2, TOP_3), op); // imaginary parts
    
    vme_pe1(agu_top(MODE), VME_DEF_MODE, stride);
    vme_pe1(agu_top(COUNT), VME_DEF_STEP, count - 1);
    
    vme_pe1(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_6);
    vme_pe1(agu_write(COUNT), VME_DEF_STEP, count - 1);
  }
});


// transferts
static void twiddlesToVme(u32 src, u16 size) {
  
  vme_dma(MEMORY, ADDR, useg_mem(src));
  vme_dma(SPAD, OFFSET, VME_TOP_BUFF2_WOFF);
  vme_dma(ITERATION, DIMS, 1, size - 1);
  vme_dma(CTRL, VALUE, 0x40);
  
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

void processStage(void* butterfly/*void* upperLeg, void* lowerLeg*/) {
  
  vmeLibStart();

  vmeLibLoadCustomContext(butterfly);
  vmeLibProcessAsync();
    
  /*
  vmeLibLoadCustomContext(upperLeg);
  vmeLibProcessAsync();
  
  vmeLibLoadCustomContext(lowerLeg);
  vmeLibProcessAsync();
  */
  vmeLibFinishAsync();
}

#define VME_CONTEXT_BYTE_COUNT (VME_CONTEXT_WORD_COUNT * 4)

void meLibOnProcess(void) {
    
  meLibExceptionHandlerInit(0);
  const int strides[] = {
    FFT_STAGE_0_STRIDE, FFT_STAGE_1_STRIDE,
    FFT_STAGE_2_STRIDE, FFT_STAGE_3_STRIDE,
  };
  vmeLibGenContext(butterflyGenerator, butterfly0, (void*)&(strides[0]));
  
  //vmeLibGenContext(butterflyGenerator, butterfly1, (void*)&(strides[1]));
  //vmeLibGenContext(butterflyGenerator, butterfly2, (void*)&(strides[2]));
  //vmeLibGenContext(butterflyGenerator, butterfly3, (void*)&(strides[3]));

  vmeLibGenContext(lowerLegGenerator, lowerLeg0, (void*)&(strides[0]));
  
  #define twiddles _TWIDDLES
  meCoreDcacheWritebackRange((void*)twiddles, sizeof(twiddles));
  /*
  const u32 sample[FFT_VALUE_COUNT * 2] __attribute__((aligned(16))) = {

    0x00000000, 0x00000000, 0x00000001, 0x00000001,
    0x00000002, 0x00000002, 0x00000003, 0x00000003,
    0x00000004, 0x00000004, 0x00000005, 0x00000005,
    0x00000006, 0x00000006, 0x00000007, 0x00000007,
    
    0x00000008, 0x00000008, 0x00000009, 0x00000009,
    0x0000000a, 0x0000000a, 0x0000000b, 0x0000000b,
    0x0000000c, 0x0000000c, 0x0000000d, 0x0000000d,
    0x0000000e, 0x0000000e, 0x0000000f, 0x0000000f,
  };
  */
  
  const u32 sampleReal[FFT_VALUE_COUNT * 2] __attribute__((aligned(16))) = {

    0x00000000, 0x00000001, 0x00000002, 0x00000003,
    0x00000004, 0x00000005, 0x00000006, 0x00000007,
    0x00000008, 0x00000009, 0x0000000a, 0x0000000b,
    0x0000000c, 0x0000000d, 0x0000000e, 0x0000000f,
  };
  
  const u32 sampleImag[FFT_VALUE_COUNT * 2] __attribute__((aligned(16))) = {

    0x00100000, 0x00100001, 0x00100002, 0x00100003,
    0x00100004, 0x00100005, 0x00100006, 0x00100007,
    0x00100008, 0x00100009, 0x0010000a, 0x0010000b,
    0x0010000c, 0x0010000d, 0x0010000e, 0x0010000f,
  };
  
  meCoreDcacheWritebackRange((void*)sampleReal, sizeof(sampleReal));
  meCoreDcacheWritebackRange((void*)sampleImag, sizeof(sampleImag));
  
  vmeLibEnable();
  vmeLibWipe();

  twiddlesToVme((u32)twiddles, sizeof(twiddles) / 4);
  
  // stage 1
  stageToVme((u32)sampleReal, VME_TOP_BUFF1_WOFF, FFT_STAGE_0_STRIDE, FFT_VALUE_COUNT, 0);
  stageToVme((u32)sampleImag, VME_TOP_BUFF3_WOFF, FFT_STAGE_0_STRIDE, FFT_VALUE_COUNT, 0);
  processStage(vme_ctx(butterfly0));
  //processStage(vme_ctx(lowerLeg0));


/*
  // stage 2
  stageToVme((u32)sample, 4, POINT_COUNT, 0);
  processStage(vme_ctx(butterfly1));

  // stage 3
  stageToVme((u32)sample, 2, POINT_COUNT, 0);
  processStage(vme_ctx(butterfly2));

  // stage 4
  stageToVme((u32)sample, 1, POINT_COUNT, 0);
  processStage(vme_ctx(butterfly3));
*/

  // debug
  // vmeDebugFillWith(VME_BASE_BUFFERS);
  vmeDebugFillAtWith(0, VME_BASE_BUFF1_WOFF - FFT_STAGE_0_STRIDE);
  vmeDebugFillAtWith(1, VME_BASE_BUFF3_WOFF - FFT_STAGE_0_STRIDE);
  
  vmeLibDisable();

  while (1) {
    /*
    vmeLibEnable();
    vmeLibDisable();
    */
    meCounter += 1;
  }
  
}

int main() {
  
  vmeDebugSetupBuffers();

  meLibDefaultInit();
  pspDebugScreenInit();
  pspDebugScreenPrintf("16-point-radix-2");

  SceCtrlData ctl;
  do {
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    
    pspDebugScreenSetXY(1, 1);
    pspDebugScreenPrintf("meCounter: 0x%lx08", meCounter);

    sceDisplayWaitVblankStart();
  } while (!(ctl.Buttons & PSP_CTRL_HOME) && 0);
  
  vmeDebugTouch();
  vmeDebugDumpBuffers();
  vmeDebugFreeBuffers();
  
  sceKernelExitGame();
  return 0;
}
