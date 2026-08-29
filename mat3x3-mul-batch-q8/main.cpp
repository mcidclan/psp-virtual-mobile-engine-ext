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

#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
#include <me-core-mapper/me-core.h>
#include <me-core-mapper/vme-fu-opcodes.h>

PSP_MODULE_INFO("vme-vm3x3-batch", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();

#define VECTOR_COUNT 16
#define VECTOR_BATCH_WORD_COUNT (VECTOR_COUNT * 3)
#define VECTOR_MAC_WORD_COUNT 9

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[1])
#define sharedIdx    (meLibSharedMemory[2])

#define Q_FORMAT 8
#define ROUND_CONST (1 << (Q_FORMAT - 1))
#define F2Q(v) ((u32)(((int)((v) * (1u << Q_FORMAT)))))
#define Q2F(v) ((float)(int)(v) / (1u << Q_FORMAT))

volatile const u32 __attribute__((aligned(64))) cancelMask[3] = {
  0, 0, 0xffffffff,
};

// Matrix 3x3 Data
volatile u32 __attribute__((aligned(64))) sharedMat[16] = {
  
  F2Q( 0.866028f), F2Q(-0.500000f), F2Q( 0.000000f),
  F2Q( 0.500000f), F2Q( 0.866028f), F2Q( 0.000000f),
  F2Q( 0.000000f), F2Q( 0.000000f), F2Q( 0.999969f),
  
  /*
  F2Q(1.000000f), F2Q(0.000000f), F2Q(0.000000f),
  F2Q(0.000000f), F2Q(1.000000f), F2Q(0.000000f),
  F2Q(0.000000f), F2Q(0.000000f), F2Q(1.000000f),
  */
};

// Vectors Data
volatile u32 __attribute__((aligned(64))) sharedVec[VECTOR_BATCH_WORD_COUNT] = {

  F2Q(1.0f),   F2Q(2.0f),   F2Q(3.0f),
  F2Q(5.0f),   F2Q(0.0f),   F2Q(0.0f),
  F2Q(0.0f),   F2Q(5.0f),   F2Q(0.0f),
  F2Q(10.0f),  F2Q(10.0f),  F2Q(1.0f),
  F2Q(-3.0f),  F2Q(4.0f),   F2Q(2.0f),
  F2Q(7.0f),   F2Q(-2.0f),  F2Q(5.0f),
  F2Q(-1.0f),  F2Q(-1.0f),  F2Q(-1.0f),
  F2Q(63.0f), F2Q(0.0f),   F2Q(0.0f),

  F2Q(0.0f),    F2Q(-50.0f),  F2Q(0.0f),
  F2Q(8.0f),    F2Q(8.0f),    F2Q(8.0f),
  F2Q(-20.0f),  F2Q(15.0f),   F2Q(3.0f),
  F2Q(2.0f),    F2Q(2.0f),    F2Q(2.0f),
  F2Q(0.0f),    F2Q(0.0f),    F2Q(0.0f),
  F2Q(-63.0f), F2Q(-63.0f), F2Q(10.0f),
  F2Q(50.0f),   F2Q(-25.0f),  F2Q(7.0f),
  F2Q(1.0f),    F2Q(2.0f),    F2Q(3.0f),
};

volatile u32 __attribute__((aligned(64))) sharedRes[VECTOR_BATCH_WORD_COUNT] = {0};

/*
 * mat3x3 vs batch of vectors
 */
VME_LIB_CONTEXT_GENERATOR(mulMatCtxGenerator, param, {

  vme_icn(AGU_TOP, 0x4210);
  vme_icn(AGU_BASE, 0x4440);
  vme_icn(AGU_WRITE, 0x3440);
  vme_icn(SWEN, fu_on(0b0111));

  vme_pe0(vme_fu(PRIMARY), vme_mux(TOP_0, TOP_1), fu_code(0x240));

  const int count = VECTOR_COUNT * 9;
  {
    const int count = 9;
    vme_pe0(agu_top(MODE), agu_mode(4));
    vme_pe0(agu_top(COUNT), agu_step(1), count - 1);
    vme_pe0(agu_top(INNER_0), count << 16,  1);
    vme_pe0(agu_top(FORMAT_0), VME_RING_TOKEN);
  }
  
  {
    vme_pe1(agu_top(MODE), agu_mode(4));
    vme_pe1(agu_top(COUNT), agu_step(1), count - 1);
  }
  
  {
    const int count = 3;
    vme_pe2(agu_top(MODE), agu_mode(4));
    vme_pe2(agu_top(COUNT), agu_step(1), count - 1);
    vme_pe2(agu_top(INNER_0), count << 16, 1);
    vme_pe2(agu_top(FORMAT_0), VME_RING_TOKEN);
  }
  
  {
    const int prologue = 5;
    const int lostCycles = prologue + 6;
    vme_pe0(agu_write(MODE), VME_DEF_MODE);
    vme_pe0(agu_write(COUNT), VME_DEF_STEP, (count - 1) + lostCycles);
    vme_pe0(agu_write(FORMAT_0), prologue);
    vme_pe0(agu_write(FORMAT_1), VME_END_TOKEN);
    
    vme_pe0(agu_base(MODE), VME_DEF_MODE);
    vme_pe0(agu_base(COUNT),  VME_DEF_STEP, (count - 1) + lostCycles);
  }
  
  vme_pe1(vme_fu(PRIMARY), vme_mux(TOP_2, STAGING_0), fu_code(OP_AND));
  vme_pe2(vme_fu(PRIMARY), vme_mux(NONE, STAGING_1), fu_code(0x250));
  vme_pe2(vme_fu(SECONDARY), vme_mux(NONE, STAGING_2), fu_code(OP_BAK));
  vme_pe1(vme_fu(SECONDARY), vme_mux(STAGING_2, STAGING_6), fu_code(OP_SUB_OFB));
  
  vme_pe3(vme_fu(PRIMARY), vme_mux(BASE_0, STAGING_5), fu_code(OP_SUB_OFB));
  vme_pe3(vme_fu(SECONDARY), vme_mux(NONE, STAGING_3), fu_code(OP_ADD_CST), Q_FORMAT);
  vme_pe3(fu_reg(SECONDARY, B), ROUND_CONST);
  
  vme_pe3(agu_write(MODE), agu_mode(4), vme_cyc(20));
  vme_pe3(agu_write(COUNT),  agu_step(1), (count - 1));
});

static void uploadBatchOfVectors(void* const src, u32 dst, int count) {

  vme_dma(MEMORY, ADDR, useg_mem((u32)src));
  
  vme_dma(ITERATION, DIMS, (count - 1), 0);
  vme_dma(ITERATION, STEP, 4);
  vme_dma(GROUP, SIZE, (3 - 1));
  vme_dma(GROUP, STEPS, 9, 1);
  
  vme_dma(SPAD, OFFSET, dst + 0x00);
  vme_dma(CTRL, VALUE, 0x50);
  vme_dma(SPAD, OFFSET, dst + 0x03);
  vme_dma(CTRL, VALUE, 0x50);
  meCoreDMACPrimWaitTransferFinish();
  
  vme_dma(SPAD, OFFSET, dst + 0x06);
  vme_dma(CTRL, VALUE, 0x50);
  meCoreDMACPrimWaitTransferFinish();
}

void downloadBatchOfVectors(void* const dst, int count) {
  
  vme_dma(MEMORY, ADDR, useg_mem((u32)dst));

  const u32 src = VME_BASE_BUFF3_WOFF;
  vme_dma(SPAD, OFFSET, src + 2);
  vme_dma(ITERATION, DIMS, (count - 1), (3 - 1));
  vme_dma(ITERATION, STEP, 4);
  vme_dma(GROUP, SIZE, 0);
  vme_dma(GROUP, STEPS, 0);
  
  vme_dma(CTRL, VALUE, 0x58);
  meCoreDMACPrimWaitTransferFinish();
}

void meLibOnProcess(void) {

  meLibExceptionHandlerInit(0);
  
  vmeLibEnable();
  vmeLibWipe();
  
  vmeLibMemoryToRingBuffer((void*)sharedMat, VME_TOP_BUFF0_WOFF, sizeof(sharedMat) / 4);
  vmeLibMemoryToRingBuffer((void*)cancelMask, VME_TOP_BUFF2_WOFF, sizeof(cancelMask) / 4);
  uploadBatchOfVectors((void*)sharedVec, VME_TOP_BUFF1_WOFF, VECTOR_BATCH_WORD_COUNT);
  vmeLibDisable();
  
  vmeLibGenContext(mulMatCtxGenerator, mulMatCtx, nullptr);

  while (1) {

    vmeLibEnable();
    vmeLibICNInvalidate(VECTOR_COUNT * 9);
    
    vmeLibStart();
    vmeLibLoadCustomContext(vme_ctx(mulMatCtx));
    
    vmeLibFinish();
    vmeLibDisable();

    if (meCoreHwMutexTryLock() >= 0) {
      
      meCoreBusClockEnableDMACPrimMux();
      downloadBatchOfVectors((void*)sharedRes, VECTOR_BATCH_WORD_COUNT);
      meCoreBusClockDisableDMACPrimMux();
      meCoreHwMutexUnlock();
    }
    
    meLibDelayPipeline();
    meCounter += 1;
  }
}

void displayVectors() {
  
  while (meLibCallHwMutexTryLock() < 0) {
    sceKernelDelayThread(1);
  }
  
  const u32* const out = (u32*)(0x40000000 | (u32)sharedRes);

  pspDebugScreenSetXY(0, 2);
  pspDebugScreenPrintf(" VME Output Vectors:");
  
  pspDebugScreenSetXY(0, 4);
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f\n", Q2F(out[0]), Q2F(out[1]), Q2F(out[2]));
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f\n", Q2F(out[3]), Q2F(out[4]), Q2F(out[5]));
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f\n", Q2F(out[6]), Q2F(out[7]), Q2F(out[8]));
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f  ", Q2F(out[9]), Q2F(out[10]), Q2F(out[11]));

  pspDebugScreenSetXY(0, 9);
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f\n", Q2F(out[12]), Q2F(out[13]), Q2F(out[14]));
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f\n", Q2F(out[15]), Q2F(out[16]), Q2F(out[17]));
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f\n", Q2F(out[18]), Q2F(out[19]), Q2F(out[20]));
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f  ", Q2F(out[21]), Q2F(out[22]), Q2F(out[23]));
  
  pspDebugScreenSetXY(0, 14);
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f\n", Q2F(out[24]), Q2F(out[25]), Q2F(out[26]));
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f\n", Q2F(out[27]), Q2F(out[28]), Q2F(out[29]));
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f\n", Q2F(out[30]), Q2F(out[31]), Q2F(out[32]));
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f  ", Q2F(out[33]), Q2F(out[34]), Q2F(out[35]));
  
  pspDebugScreenSetXY(0, 19);
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f\n", Q2F(out[36]), Q2F(out[37]), Q2F(out[38]));
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f\n", Q2F(out[39]), Q2F(out[40]), Q2F(out[41]));
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f\n", Q2F(out[42]), Q2F(out[43]), Q2F(out[44]));
  pspDebugScreenPrintf(" %+11f, %+11f, %+11f ", Q2F(out[45]), Q2F(out[46]), Q2F(out[47]));
  
  meLibCallHwMutexUnlock();
}

int main() {
  
  meLibDefaultInit();
  pspDebugScreenInit();
  
  SceCtrlData ctl;
  do {
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    
    pspDebugScreenSetXY(0, 0);
    pspDebugScreenPrintf("ME Counter: 0x%08lx", meCounter);
    displayVectors();
    
    sceDisplayWaitVblank();
    
  } while (!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceKernelDelayThread(100000);
  
  sceKernelExitGame();
  return 0;
}
