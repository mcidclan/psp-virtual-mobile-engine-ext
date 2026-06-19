/*
 * Copyright mcidclan, m-c/d 2026
 */
#include "main.h"

PSP_MODULE_INFO("vme-ctx", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();

#define VME_DEBUG_BUFFER_WORD_COUNT 32

meLibSetSharedUncached32(10);
#define meExit       (meLibSharedMemory[0])
#define meCounter    (meLibSharedMemory[1])


void meLibOnProcess(void) {

  meCoreDcacheWritebackInvalidateAll();
  meLibExceptionHandlerInit(0);
  
  while (1) {
    
    meCounter += 1;
    
  }
}

int main() {
  
  scePowerSetClockFrequency(333, 333, 166);
  vmeDebugSetupBuffers(VME_DEBUG_BUFFER_WORD_COUNT);
  setupData();

  meLibDefaultInit();
  pspDebugScreenInit();
  
  SceCtrlData ctl;
  do {
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    updateVector(&ctl);
    
    pspDebugScreenSetXY(1, 1);
    pspDebugScreenPrintf("meCounter: 0x%lx", meCounter);
    
    sceDisplayWaitVblank();
    
  } while (!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceKernelDelayThread(100000);
  
  vmeDebugTouch();
  vmeDebugDumpBuffers();
  vmeDebugFreeBuffers();
  
  sceKernelExitGame();
  return 0;
}
