/*
 * Copyright mcidclan, m-c/d 2026
 */
#include "main.h"

PSP_MODULE_INFO("vme-fft", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();
VME_DEBUG_SET_BUFFER_WORD_COUNT(32);

#define POINT_COUNT 16

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[0])


VME_LIB_CONTEXT_GENERATOR(butterflyGenerator, param, {
  
  //vme_set(ENABLE, FU_1, 0);
  //vme_icn(AGU_TOP, 0);
  //vme_icn(AGU_BASE, 0);
  //vme_icn(AGU_WRITE, 0);

  const int count = 8;
  const int stride = *((int*)param);
  
  vme_pe0(vme_fu(PRIMARY), vme_mux(TOP_0, TOP_0), 0x00004000);
  
  vme_pe0(agu_top(MODE), VME_DEF_MODE, stride ? (count + stride) : 0);
  vme_pe0(agu_top(COUNT), VME_DEF_STEP, count - 1);
  
  vme_pe0(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_6, stride ? count : 0);
  vme_pe0(agu_write(COUNT), VME_DEF_STEP, count - 1);
});

static void stageToVme(u32 src, u16 stride, u16 points, u8 slice) {
  
  vme_dma(MEMORY, ADDR, useg_mem(src));
  vme_dma(SPAD, OFFSET, VME_TOP_BUFF0_WOFF - slice * (points/2 + stride));
  vme_dma(ITERATION, STEP, 0);
  vme_dma(ITERATION, DIMS, 1, (points - 1) + stride);
  vme_dma(GROUP, SIZE, (stride * 2) - 1);
  vme_dma(GROUP, STEPS, stride, 1);
  vme_dma(CTRL, VALUE, 0x50);

  meCoreDMACPrimWaitTransferFinish();
}

void processStage(void* upperLeg, void* lowerLeg) {
  
  vmeLibStart();
  
  vmeLibLoadCustomContext(upperLeg);
  vmeLibProcessAsync();
  
  vmeLibLoadCustomContext(lowerLeg);
  vmeLibProcessAsync();
  
  vmeLibFinishAsync();
}

#define VME_CONTEXT_BYTE_COUNT (VME_CONTEXT_WORD_COUNT * 4)

void meLibOnProcess(void) {
    
  meLibExceptionHandlerInit(0);
  
  const int strides[] = {0, 8, 4, 2, 1};
  vmeLibGenContext(butterflyGenerator, butterflyUpperLeg,  (void*)&(strides[0]));
  vmeLibGenContext(butterflyGenerator, butterflyLowerLeg0, (void*)&(strides[1]));
  vmeLibGenContext(butterflyGenerator, butterflyLowerLeg1, (void*)&(strides[2]));
  vmeLibGenContext(butterflyGenerator, butterflyLowerLeg2, (void*)&(strides[3]));
  vmeLibGenContext(butterflyGenerator, butterflyLowerLeg3, (void*)&(strides[4]));
  
  const u32 sample[32] __attribute__((aligned(16))) = {

    0x00000000, 0x00000001, 0x00000002, 0x00000003,
    0x00000004, 0x00000005, 0x00000006, 0x00000007,
    
    0x00000008, 0x00000009, 0x0000000a, 0x0000000b,
    0x0000000c, 0x0000000d, 0x0000000e, 0x0000000f,
  };
  
  meCoreDcacheWritebackRange((void*)sample, sizeof(sample));
  
  vmeLibEnable();
  vmeLibWipe();

  // stage 1
  stageToVme((u32)sample, 8, POINT_COUNT, 0);
  processStage(vme_ctx(butterflyUpperLeg), vme_ctx(butterflyLowerLeg0));

/*
  // stage 2
  stageToVme((u32)sample, 4, POINT_COUNT, 0);
  processStage(vme_ctx(butterflyUpperLeg), vme_ctx(butterflyLowerLeg1));

  // stage 3
  stageToVme((u32)sample, 2, POINT_COUNT, 0);
  processStage(vme_ctx(butterflyUpperLeg), vme_ctx(butterflyLowerLeg2));

  // stage 4
  stageToVme((u32)sample, 1, POINT_COUNT, 0);
  processStage(vme_ctx(butterflyUpperLeg), vme_ctx(butterflyLowerLeg3));
*/

  // debug
  vmeDebugFillWith(VME_BASE_BUFFERS);

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
