/*
 * Copyright mcidclan, m-c/d 2026
 */
#include "main.h"

PSP_MODULE_INFO("vme-pico-ai", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();
VME_DEBUG_SET_BUFFER_WORD_COUNT(32);

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[1])


VME_LIB_CONTEXT_BUILDER(setupHiddenLayer, param, {
  
  vme_icn(AGU_TOP, 0);
  vme_icn(AGU_BASE, 0);
  vme_icn(AGU_WRITE, 0);
  
  // first neurone 
  vme_pe0(vme_fu(PRIMARY), 0);

  vme_pe0(agu_top(MODE), VME_DEF_MODE);
  vme_pe0(agu_top(COUNT), VME_DEF_STEP, count);
  vme_pe0(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_6);
  vme_pe0(agu_write(COUNT), VME_DEF_STEP, count);
  
  // second neurone 
  vme_pe1(vme_fu(PRIMARY), 0);
  
  // third neurone 
  vme_pe2(vme_fu(PRIMARY), 0);
  
  // forth neurone 
  vme_pe3(vme_fu(PRIMARY), 0);
});

VME_LIB_CONTEXT_BUILDER(setupOutputLayer, param, {
  
  vme_icn(AGU_TOP, 0);
  vme_icn(AGU_BASE, 0);
  vme_icn(AGU_WRITE, 0);
  
  // first neurone 
  vme_pe0(vme_fu(PRIMARY), 0);

  vme_pe0(agu_top(MODE), VME_DEF_MODE);
  vme_pe0(agu_top(COUNT), VME_DEF_STEP, count);
  vme_pe0(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_6);
  vme_pe0(agu_write(COUNT), VME_DEF_STEP, count);
  
  // second neurone 
  vme_pe1(vme_fu(PRIMARY), 0);
});

void runContext() {
  
  vmeLibStart();
  //
  vmeLibFinish();
}

void meLibOnProcess(void) {

  meCoreDcacheWritebackInvalidateAll();
  meLibExceptionHandlerInit(0);
  
  void* const hiddenLayerContext = setupHiddenLayer(nullptr);
  void* const outputLayerContext = setupOutputLayer(nullptr);
  
  
  while (1) {
    
    if () {
      vmeLibEnable();
      
      {
        vmeLibWipe();
        
        vmeLibStart();
        {
          vmeLibLoadCustomContext(addContext);
          vmeLibProcessAsync();
          
          vmeLibLoadCustomContext(subContext);
          vmeLibProcessAsync();
          
          vmeLibLoadCustomContext(mulContext);
          vmeLibProcessAsync();
        }
        vmeLibFinishAsync();
      }
      
      vmeDebugFillWith(VME_BASE_BUFFERS);
      vmeLibDisable();
    }
  
    meCounter += 1;
  }
}

int sampleRecorder(SceSize args, void *argp) {
  
  int* const stopped = (int*)*((int*)argp);
  
  int index = 0;
  signed char samples[128] = {0};
  
  sceCtrlSetSamplingCycle(0);
  sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
  
  SceCtrlData pad;
  while (!*stopped) {
    
    sceCtrlPeekBufferPositive(&pad, 1);

    samples[index] = (signed char)(pad.Lx - 128);
    samples[index + 64] = (signed char)(pad.Ly - 128);
    index += 1;
    
    if (index >= 64) {
      // todo process vme
      index = 0;
    }
    
    sceKernelDelayThread(1);
  }
  *stopped = 1;
  return sceKernelExitDeleteThread(0);
}

int startThread(const void* const thread, int* ended) {
  
  *ended = 0;
  
  int thid = sceKernelCreateThread("thread",
    thread, 0x18, 0x2000, PSP_THREAD_ATTR_VFPU, 0);
  
  if (thid >= 0) {
    
    int* param[1] = {ended};
    sceKernelStartThread(thid, sizeof(int), &param);
  }
  return thid;
}

void endThread(int* ended, const int thid) {
  
  *ended = 1;
  SceUInt timeout = 500000;
  sceKernelWaitThreadEnd(thid, &timeout);
  sceKernelTerminateDeleteThread(thid);
}

int main() {
  
  scePowerSetClockFrequency(333, 333, 166);
  Uncached32* const uvar = loadBinary("./model.bin");
  
  vmeDebugSetupBuffers();
  pspDebugScreenInit();
  meLibDefaultInit();
  
  startThread();
  
  SceCtrlData ctl;
  do {
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    
    {
      const int x = 1;
      const int y = 1;
      
      pspDebugScreenSetXY(x, y);
      pspDebugScreenPrintf("Result of the 4 Processing Elements:");
      
      pspDebugScreenSetXY(x, y + 2);
      pspDebugScreenPrintf("BASE_0:");
      vmeDebugDisplayBuffer(VME_DEBUG_DIGIT_4, VME_DBG_IDX_BASE_0, x, y + 3);

      pspDebugScreenSetXY(x + 34, y + 2);
      pspDebugScreenPrintf("BASE_1:");
      vmeDebugDisplayBuffer(VME_DEBUG_DIGIT_4, VME_DBG_IDX_BASE_1, x + 34, y + 3);
      
      pspDebugScreenSetXY(x, y + 12);
      pspDebugScreenPrintf("BASE_2:");
      vmeDebugDisplayBuffer(VME_DEBUG_DIGIT_4, VME_DBG_IDX_BASE_2, x, y + 13);
      
      pspDebugScreenSetXY(x + 34, y + 12);
      pspDebugScreenPrintf("BASE_3:");
      vmeDebugDisplayBuffer(VME_DEBUG_DIGIT_4, VME_DBG_IDX_BASE_3, x + 34, y + 13);
    }
  
    pspDebugScreenSetXY(1, 24);
    pspDebugScreenPrintf("meCounter: 0x%lx", meCounter);
    
    
    sceDisplayWaitVblank();
    
  } while (!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceKernelDelayThread(100000);
  
  vmeDebugTouch();
  vmeDebugDumpBuffers();
  vmeDebugFreeBuffers();
  
  unloadBinary(uvar);
  sceKernelExitGame();
  return 0;
}
