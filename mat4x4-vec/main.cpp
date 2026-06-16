/*
 * Copyright mcidclan, m-c/d 2026
 */
#include "main.h"

PSP_MODULE_INFO("vme-mat", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();

#define VME_DEBUG_BUFFER_WORD_COUNT 32

meLibSetSharedUncached32(10);
#define meExit       (meLibSharedMemory[0])
#define meCounter    (meLibSharedMemory[1])
#define sharedIdx    (meLibSharedMemory[2])

volatile u32* sharedMat __attribute__((aligned(64))) = nullptr;
volatile u32* sharedVec __attribute__((aligned(64))) = nullptr;
volatile u32* sharedRes __attribute__((aligned(64))) = nullptr;

void meLibOnProcess(void) {
  
  meCoreDcacheWritebackInvalidateAll();
  meLibExceptionHandlerInit(0);
  
  vmeLibEnable();
  vmeLibWipe();
  
  vmeExtSetPadded4x4((void*)VME_TOP_BUFFER_0, (void*)sharedMat);

  const void* const vector = (const void* const)(&sharedVec[sharedIdx * 4]);
  vmeExtSetWord4((void*)VME_TOP_BUFFER_1, vector);

  vmeLibStart();

  vme_icn(INPUT, 0x0010);
  vme_icn(FLOW, VME_DEF_MAPPER);
  vme_icn(ARCH, VME_DEF_MAPPER);
  vme_set(ENABLE, FU_1, 0x80000000);

  // duplicate rows
  vme_pe0(vme_fu(PRIMARY), 0x06008000);
  vme_pe0(vme_fu(SECONDARY), 0x04010000);
  vme_pe0(agu_top(MODE), VME_DEF_MODE);
  vme_pe0(agu_top(COUNT), VME_DEF_STEP, 32 - 1);
  vme_pe0(agu_write(MODE), VME_DEF_MODE, 0x60000);
  vme_pe0(agu_write(COUNT), VME_DEF_STEP, 32 - 1);
  //
  vmeLibFinish();
  
  // debug
  vmeDebugFillWith(VME_BASE_BUFFERS, VME_DEBUG_BUFFER_WORD_COUNT);
  vmeLibDisable();
  
  while (1) {
    
    meCounter += 1;
    
    vmeLibEnable();
    
    // todo: update vector
    
    vmeLibStart();
    vmeLibRefresh();
    
    // todo: update sc
    
    vmeLibDisable();
  }
}

void updateVector(SceCtrlData* const ctl) {
  
  static bool up = false;
  
  if (!(ctl->Buttons & PSP_CTRL_TRIANGLE) && !(ctl->Buttons & PSP_CTRL_CROSS)) {
    
    up = true;
  } else if (up) {

    if (ctl->Buttons & PSP_CTRL_TRIANGLE) {
      
      sharedIdx = (sharedIdx + 1) & 3;
      up = false;
    }
    else if (ctl->Buttons & PSP_CTRL_CROSS) {
      
      sharedIdx = (sharedIdx - 1) & 3;
      up = false;
    }
  }
}

int main() {
  
  vmeDebugSetupBuffers(VME_DEBUG_BUFFER_WORD_COUNT);
  
  scePowerSetClockFrequency(333, 333, 166);

  meLibGetUncached32(sharedMat, 16); // 4x4 mat
  meLibGetUncached32(sharedVec, 16); // array of 4 input vectors
  meLibGetUncached32(sharedRes, 4); // output/resulted vector

  vmeExtMat4x4u32* const mat = (vmeExtMat4x4u32*)sharedMat;
  mat->x = {0x00, 0x01, 0x02, 0x03};
  mat->y = {0x04, 0x05, 0x06, 0x07};
  mat->z = {0x08, 0x09, 0x0a, 0x0b};
  mat->w = {0x0c, 0x0d, 0x0e, 0x0f};

  vmeExtRow4u32* const vec = (vmeExtRow4u32*)sharedVec;
  vec->row = {0x01, 0x02, 0x03, 0x04};

  meLibDefaultInit();
  
  pspDebugScreenInit();
  SceCtrlData ctl;
  do {
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    updateVector(&ctl);
    
    const u32* const mat = (u32*)sharedMat;
    const u32* const in = (u32*)&(sharedVec[sharedIdx * 4]);
    const u32* const out = (u32*)sharedRes;
    
    pspDebugScreenSetXY(0, 0);
    pspDebugScreenPrintf("Current 4x4 Matrix:");
    
    for(int i = 0; i < 8; i++) {
      
      const int off = i * 4;
      pspDebugScreenSetXY(0, i+2);
      pspDebugScreenPrintf("%lx, %lx, %lx, %lx,",
        mat[off+0], mat[off+1], mat[off+2], mat[off+3]);
    }
    
    pspDebugScreenSetXY(0, 12);
    pspDebugScreenPrintf("Input Vector:");
    pspDebugScreenSetXY(0, 13);
    pspDebugScreenPrintf("%lx, %lx, %lx, %lx,", in[0], in[1], in[2], in[3]);
    pspDebugScreenSetXY(0, 14);
    pspDebugScreenPrintf("Use Triangle/Cross to change its values");

    pspDebugScreenSetXY(0, 16);
    pspDebugScreenPrintf("Output Vector:");
    pspDebugScreenSetXY(0, 17);
    pspDebugScreenPrintf("%lx, %lx, %lx, %lx,", out[0], out[1], out[2], out[3]);

    pspDebugScreenSetXY(0, 28);
    pspDebugScreenPrintf("meCounter: 0x%lx", meCounter);
    
    sceDisplayWaitVblank();
    
  } while (!(ctl.Buttons & PSP_CTRL_HOME) && 0);
  
  sceKernelDelayThread(100000);
  
  vmeDebugTouch();
  vmeDebugDumpBuffers();
  
  meLibFreeUncached32(sharedMat);
  meLibFreeUncached32(sharedVec);
  vmeDebugFreeBuffers();
  
  sceKernelExitGame();
  return 0;
}
