/*
 * Copyright mcidclan, m-c/d 2026
 */
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
#include <me-core-mapper/me-core.h>

PSP_MODULE_INFO("vme-vm-batch-2", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();

#define VECTOR_COUNT 16
#define VECTOR_BATCH_WORD_COUNT (VECTOR_COUNT * 4)
#define VECTOR_MAC_WORD_COUNT 16

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[1])

volatile const u32 __attribute__((aligned(64))) cancelMask[4] = {
  0, 0, 0xffffffff, 0
};

volatile u32 __attribute__((aligned(64))) sharedMat[16] = {
  
  0x00006EDA, 0xFFFFC000, 0x00000000, 0x00000000,
  0x00004000, 0x00006EDA, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00007FFF, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00007FFF,
  
  /*
  0x01, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x01,
  */
};

volatile u32 __attribute__((aligned(64))) sharedVec[VECTOR_BATCH_WORD_COUNT] = {

  0x00000001, 0x00000002, 0x00000003, 0x00000004,
  0x00000005, 0x00000000, 0x00000000, 0x00000001,
  0x00000000, 0x00000005, 0x00000000, 0x00000001,
  0x0000000A, 0x0000000A, 0x00000001, 0x00000001,
  0xFFFFFFFD, 0x00000004, 0x00000002, 0x00000001,
  0x00000007, 0xFFFFFFFE, 0x00000005, 0x00000001,
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000001,
  0x00000064, 0x00000000, 0x00000000, 0x00000001,
  
  0x00000000, 0xFFFFFFCE, 0x00000000, 0x00000001,
  0x00000008, 0x00000008, 0x00000008, 0x00000001,
  0xFFFFFFEC, 0x0000000F, 0x00000003, 0x00000001,
  0x00000002, 0x00000002, 0x00000002, 0x00000002,
  0x00000000, 0x00000000, 0x00000000, 0x00000001,
  0xFFFFFF9C, 0xFFFFFF9C, 0x0000000A, 0x00000001,
  0x00000032, 0xFFFFFFE7, 0x00000007, 0x00000001,
  0x00000001, 0x00000002, 0x00000003, 0x00000004,
};

volatile u32 __attribute__((aligned(64))) sharedRes[VECTOR_BATCH_WORD_COUNT] = {0};

/*
 * Q1.15 mat4x4 vs batch of vectors
 */
VME_LIB_CONTEXT_BUILDER(setupMulMatCtx, param, {
  
  vme_icn(AGU_TOP, 0x4210);
  vme_icn(AGU_BASE, 0x4040);
  vme_icn(AGU_WRITE, 0x3240);
  
  vme_set(ENABLE, FU_1, 0b0001 << 28);

  // VMAC between 4x4 matrix and vectors
  vme_pe0(vme_fu(PRIMARY), vme_mux(TOP_0, TOP_1), 0x240 << 12); // staging 0

  {
    // AGU 'Read' for the input matrix
    const int count = (16 - 1);
    vme_pe0(agu_top(MODE), VME_DEF_MODE);
    vme_pe0(agu_top(COUNT), VME_DEF_STEP, count);
    vme_pe0(agu_top(INNER_0), count << 16, 1);
    vme_pe0(agu_top(FORMAT_0), VME_RING_TOKEN);
  }
 
  {
    // AGU 'Read' for the 4 word accumulator cancel mask selector
    const int count = (4 - 1);
    vme_pe2(agu_top(MODE), VME_DEF_MODE);
    vme_pe2(agu_top(COUNT), VME_DEF_STEP, count);
    vme_pe2(agu_top(INNER_0), count << 16, 1);
    vme_pe2(agu_top(FORMAT_0), VME_RING_TOKEN);
  }
  
  const int lostVector = 1;
  const int count = ((lostVector + VECTOR_COUNT) * 16 - 1);
  
  // Note: the lost vector is not necessary when using ICN invalidation. However,
  // it does not require the VME to clean the interconnect each time, so use one
  // or the other according to your needs
  
  {
    // AGU 'Read' for the batch of vectors
    const int lostCycles = 16 * lostVector;
    const int offset = (0x10000 - lostCycles) & 0xfffff;
    vme_pe1(agu_top(MODE), VME_DEF_MODE, offset);
    vme_pe1(agu_top(COUNT), VME_DEF_STEP, count);
  }
  
  const int lostCycles = 12;
  
  // AGUs 'Write'
  vme_pe2(agu_write(MODE), VME_DEF_MODE);
  vme_pe2(agu_write(COUNT),  VME_DEF_STEP, count + lostCycles);
  vme_pe2(agu_write(FORMAT_0), 1);
  vme_pe2(agu_write(FORMAT_1), VME_END_TOKEN);
  
  vme_pe0(agu_write(MODE), VME_DEF_MODE);
  vme_pe0(agu_write(COUNT),  VME_DEF_STEP, count + lostCycles);
  vme_pe0(agu_write(FORMAT_0), 5);
  vme_pe0(agu_write(FORMAT_1), VME_END_TOKEN);
  
  // AGUs 'Read' for intermediate buffers
  vme_pe0(agu_base(MODE), VME_DEF_MODE);
  vme_pe0(agu_base(COUNT),  VME_DEF_STEP, count + lostCycles);
  
  // Build the sparse canceler by applying a logical AND between
  // the cancel mask selector and the full VMAC result
  vme_pe1(vme_fu(PRIMARY), vme_mux(TOP_2, STAGING_0), 0x0c0 << 12); // staging 1

  // Fill the gaps of the accumulator canceler using a cumulative sum
  // followed by a delayed self-subtraction
  vme_pe2(vme_fu(PRIMARY), vme_mux(NONE, STAGING_1), 0x250 << 12); // staging 2
  vme_pe3(vme_fu(PRIMARY), vme_mux(STAGING_2, BASE_2), 0x48 << 12); // staging 3
  
  // Output, subtract to cancel accumulation excess
  vme_pe3(vme_fu(SECONDARY), vme_mux(BASE_0, STAGING_3), 0x048 << 12);
  vme_pe3(agu_write(MODE), VME_DEF_MODE, vme_cyc((0x11)));
  vme_pe3(agu_write(COUNT),  VME_DEF_STEP, count);
});

static void uploadBatchOfVectors(void* const src, u32 dst, int count) {

  vme_dma(MEMORY, ADDR, useg_mem((u32)src));
  
  vme_dma(ITERATION, DIMS, (count - 1), 0);
  vme_dma(ITERATION, STEP, 4);
  vme_dma(GROUP, SIZE, (4 - 1));
  vme_dma(GROUP, STEPS, 16, 1);
  
  vme_dma(SPAD, OFFSET, dst + 0x00);
  vme_dma(CTRL, VALUE, 0x50);
  vme_dma(SPAD, OFFSET, dst + 0x04);
  vme_dma(CTRL, VALUE, 0x50);
  meCoreDMACPrimWaitTransferFinish();
  
  vme_dma(SPAD, OFFSET, dst + 0x08);
  vme_dma(CTRL, VALUE, 0x50);
  vme_dma(SPAD, OFFSET, dst + 0x0c);
  vme_dma(CTRL, VALUE, 0x50);
  meCoreDMACPrimWaitTransferFinish();
}

void downloadBatchOfVectors(void* const dst, int count) {
  
  vme_dma(MEMORY, ADDR, useg_mem((u32)dst));

  const u32 src = VME_BASE_BUFF3_WOFF + VECTOR_MAC_WORD_COUNT;
  vme_dma(SPAD, OFFSET, src + 3);
  vme_dma(ITERATION, DIMS, (count - 1), (4 - 1));
  vme_dma(ITERATION, STEP, 4);
  vme_dma(GROUP, SIZE, 0);
  vme_dma(GROUP, STEPS, 0);
  
  vme_dma(CTRL, VALUE, 0x58);
  meCoreDMACPrimWaitTransferFinish();
}

void meLibOnProcess(void) {

  meLibExceptionHandlerInit(0);
  
  void* const mulMatCtx = setupMulMatCtx(nullptr);
  
  vmeLibEnable();
  vmeLibWipe();

  vmeLibSendCustomContext(mulMatCtx);
  vmeLibMemoryToRingBuffer((void*)sharedMat, VME_TOP_BUFF0_WOFF, sizeof(sharedMat) / 4);
  vmeLibMemoryToRingBuffer((void*)cancelMask, VME_TOP_BUFF2_WOFF, sizeof(cancelMask) / 4);
  
  uploadBatchOfVectors((void*)sharedVec, VME_TOP_BUFF1_WOFF, VECTOR_BATCH_WORD_COUNT);
  vmeLibDisable();

  while (1) {

    vmeLibEnable();
    vmeLibTrigger();
    meCoreDMACPrimWaitVMEFinish();
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
  pspDebugScreenPrintf("VME Output Vectors:\n");
  
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[0], out[1], out[2], out[3]);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[4], out[5], out[6], out[7]);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[8], out[9], out[10], out[11]);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[12], out[13], out[14], out[15]);
  
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[16], out[17], out[18], out[19]);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[20], out[21], out[22], out[23]);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[24], out[25], out[26], out[27]);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[28], out[29], out[30], out[31]);
  
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[32], out[33], out[34], out[35]);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[36], out[37], out[38], out[39]);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[40], out[41], out[42], out[43]);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[44], out[45], out[46], out[47]);
  
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[48], out[49], out[50], out[51]);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[52], out[53], out[54], out[55]);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[56], out[57], out[58], out[59]);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx\n", out[60], out[61], out[62], out[63]);
  
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
