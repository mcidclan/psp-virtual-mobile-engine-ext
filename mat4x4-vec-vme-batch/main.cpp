/*
 * Copyright mcidclan, m-c/d 2026
 */
#include "main.h"
#include <vme-ext.h>

PSP_MODULE_INFO("vme-vm-batch-2", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();

#define BATCH_COUNT 4
#define VECTOR_COUNT 8

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[1])
#define sharedIdx    (meLibSharedMemory[2])

volatile u32 __attribute__((aligned(64))) sharedMat[16] = {
  
  0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0e, 0x0f,
};

volatile u32 __attribute__((aligned(64))) sharedVec[VECTOR_COUNT * 4] = {

  0x01, 0x02, 0x03, 0x04,
  0x05, 0x06, 0x07, 0x08,
  0x09, 0x0a, 0x0b, 0x0c,
  0x0d, 0x0e, 0x0f, 0x10,
  
  0x11, 0x12, 0x13, 0x14,
  0x15, 0x16, 0x17, 0x18,
  0x19, 0x1a, 0x1b, 0x1c,
  0x1d, 0x1e, 0x1f, 0x20,
};

volatile u32 __attribute__((aligned(64))) sharedRes[VECTOR_COUNT * 16] = {0};

/*
 * 
 */
VME_LIB_CONTEXT_BUILDER(setupMulMatCtx, param, {
  
  
});

static void uploadBatchOfVectors(u32 src, u32 dst, int count) {

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

void meLibOnProcess(void) {

  meCoreDcacheWritebackInvalidateAll();
  meLibExceptionHandlerInit(0);
  
  //void* const mulMatCtx = setupMulMatCtx(nullptr);
  
  vmeLibEnable();
  vmeLibWipe();
  
  const int size = ((sizeof(sharedVec) + 63) & ~63);
  meCoreDcacheWritebackRange((void*)sharedVec, size); // todo: use uncached instead
  
  uploadBatchOfVectors((u32)sharedVec, VME_BASE_BUFF0_WOFF, (sizeof(sharedRes) / 4));
 
  // tmp debug
  meCoreMemcpy((void*)sharedRes, (void*)VME_BASE_BUFFER_0, sizeof(sharedRes));
  meCoreDcacheWritebackRange((void*)sharedRes, sizeof(sharedRes));
  vmeLibDisable();
  
  while (1) {
    
    meCounter += 1;
  }
}

void displayVectors() {

  sceKernelDcacheInvalidateRange((void*)sharedRes, sizeof(sharedRes));
  const u32* const out = (u32*)sharedRes;

  pspDebugScreenSetXY(0, 0);
  pspDebugScreenPrintf("VME Output Vectors:\n");
  
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[0], out[1], out[2], out[3]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[4], out[5], out[6], out[7]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[8], out[9], out[10], out[11]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[12], out[13], out[14], out[15]);
  
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[16], out[17], out[18], out[19]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[20], out[21], out[22], out[23]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[24], out[25], out[26], out[27]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[28], out[29], out[30], out[31]);
  
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[32], out[33], out[34], out[35]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[36], out[37], out[38], out[39]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[40], out[41], out[42], out[43]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[44], out[45], out[46], out[47]);
  
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[48], out[49], out[50], out[51]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[52], out[53], out[54], out[55]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[56], out[57], out[58], out[59]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[60], out[61], out[62], out[63]);
  
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[64], out[65], out[66], out[67]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[68], out[69], out[70], out[71]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[72], out[73], out[74], out[75]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[76], out[77], out[78], out[79]);
  
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[80], out[81], out[82], out[83]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[84], out[85], out[86], out[87]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[88], out[89], out[90], out[91]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[92], out[93], out[94], out[95]);

  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[96], out[97], out[98], out[99]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[100], out[101], out[102], out[103]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[104], out[105], out[106], out[107]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[108], out[109], out[110], out[111]);

  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[112], out[113], out[114], out[115]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[116], out[117], out[118], out[119]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[120], out[121], out[122], out[123]);
  pspDebugScreenPrintf("%lx, %lx, %lx, %lx\n", out[124], out[125], out[126], out[127]);
}


int main() {
  
  meLibDefaultInit();
  pspDebugScreenInit();
  
  SceCtrlData ctl;
  do {
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    
    pspDebugScreenSetXY(46, 0);
    pspDebugScreenPrintf("ME Counter: 0x%08lx", meCounter);
    displayVectors();
    
    sceDisplayWaitVblank();
    
  } while (!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceKernelDelayThread(100000);
  
  sceKernelExitGame();
  return 0;
}
