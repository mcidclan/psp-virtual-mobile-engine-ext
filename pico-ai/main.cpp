/*
 * Copyright mcidclan, m-c/d 2026
 */
#include "main.h"

PSP_MODULE_INFO("vme-pico-ai", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();
VME_DEBUG_SET_BUFFER_WORD_COUNT(32);

#define SAMPLE_PERIOD 16667
signed char samples[128] __attribute__((aligned(64))) = {0};

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[0])
#define sampleReady  (meLibSharedMemory[1])
#define capturing    (meLibSharedMemory[2])

VME_LIB_CONTEXT_BUILDER(setupHiddenLayer, param, {
  
  // todo: add ReLu and sat using max(0, min(127, n) on secondary FUs
  
  vme_icn(AGU_TOP, 0);
  vme_icn(AGU_BASE, 0x4440);
  vme_icn(AGU_WRITE, 0);
  
  const int rShift = 7;
  const u32 VMAC = 0x00240000;
  
  // first neuron 
  {
    const u32 mux = vme_mux(TOP_0, BASE_0);
    vme_pe0(vme_fu(PRIMARY), VMAC, mux, rShift);
  }

  const int count = 128;
  
  // weights for first layer
  vme_pe0(agu_top(MODE), VME_DEF_MODE);
  vme_pe0(agu_top(COUNT), VME_DEF_STEP, count);
  
   // dynamic input from joystick recorder
  vme_pe0(agu_base(MODE), VME_DEF_MODE, 1024);
  vme_pe0(agu_base(COUNT), VME_DEF_STEP, count);
  
  vme_pe0(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_6);
  vme_pe0(agu_write(COUNT), VME_DEF_STEP, count);
  
  // second neuron
  {
    const u32 mux = vme_mux(TOP_1, BASE_0);
    vme_pe1(vme_fu(PRIMARY), VMAC, mux, rShift);
  }
  
  // third neuron
  {
    const u32 mux = vme_mux(TOP_2, BASE_0);
    vme_pe2(vme_fu(PRIMARY), VMAC, mux, rShift);
  }
  
  // forth neuron
  {
    const u32 mux = vme_mux(TOP_3, BASE_0);
    vme_pe3(vme_fu(PRIMARY), VMAC, mux, rShift);
  }
});

VME_LIB_CONTEXT_BUILDER(setupOutputLayer, param, {
    
  vme_icn(AGU_TOP, 0);
  vme_icn(AGU_BASE, 0x4440);
  vme_icn(AGU_WRITE, 0);
  
  const int sat = 8 << 7; // n << 7: [-2^(n-1), 2^(n-1) - 1]
  const int rShift = 7;
  const u32 VMAC = 0x00240000;
  
  {
    const u32 mux = vme_mux(TOP_0, BASE_0);
    vme_pe0(vme_fu(PRIMARY), VMAC, mux, sat, rShift);
  }

  const int count = 4;
  
  // weights for second layer
  vme_pe0(agu_top(MODE), VME_DEF_MODE, 1024);
  vme_pe0(agu_top(COUNT), VME_DEF_STEP, count);
  
   // dynamic data from previous layer output
  vme_pe0(agu_base(MODE), VME_DEF_MODE, 1024);
  vme_pe0(agu_base(COUNT), VME_DEF_STEP, count);
  
  vme_pe0(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_6);
  vme_pe0(agu_write(COUNT), VME_DEF_STEP, count);
  
  // second neuron
  {
    const u32 mux = vme_mux(TOP_1, BASE_0);
    vme_pe1(vme_fu(PRIMARY), VMAC, mux, sat, rShift);
  }
  
  // third neuron
  {
    const u32 mux = vme_mux(TOP_2, BASE_0);
    vme_pe2(vme_fu(PRIMARY), VMAC, mux, sat, rShift);
  }
  
  // forth neuron
  {
    const u32 mux = vme_mux(TOP_3, BASE_0);
    vme_pe3(vme_fu(PRIMARY), VMAC, mux, sat, rShift);
  }
});

void runContext() {
  
  vmeLibStart();
  //
  vmeLibFinish();
}

#define VME_SAMPLE_BUFFER_SIZE   (512)
#define VME_SAMPLE_BUFFER_OFFSET (VME_TOP_BUFFERS + 1024)

void meLibOnProcess(void) {

  meCoreDcacheWritebackInvalidateAll();
  meLibExceptionHandlerInit(0);

  vmeLibEnable();
  vmeLibWipe();
  vmeLibDisable();

  void* const hiddenLayerContext = setupHiddenLayer(nullptr);
  void* const outputLayerContext = setupOutputLayer(nullptr);
  
  while (1) {

    if (sampleReady) {
      
      vmeLibEnable();
      
      meCoreDcacheInvalidateRange(samples, 128);
      meCoreMemcpy((void*)VME_SAMPLE_BUFFER_OFFSET, samples, VME_SAMPLE_BUFFER_SIZE);
      
      {
        vmeLibStart();
        {
          vmeLibLoadCustomContext(hiddenLayerContext);
          vmeLibProcessAsync();
          
          // todo: copy result to BASE_0 + 1024 words
          
          //vmeLibLoadCustomContext(outputLayerContext);
          //vmeLibProcessAsync();
        }
        vmeLibFinishAsync();
      }
      
      vmeDebugFillWith(VME_BASE_BUFFERS);
      
      sampleReady = 0;
      vmeLibDisable();
    }
    
    meCounter += 1;
  }
}

int recorder(SceSize args, void *argp) {

  int* const ended = (int*)*((int*)argp);
  
  SceCtrlData pad;
  sceCtrlSetSamplingCycle(0);
  sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
  
  u32 warmupStart = sceKernelGetSystemTimeLow();
  while (sceKernelGetSystemTimeLow() - warmupStart < 1000000) {
    
    sceCtrlPeekBufferPositive(&pad, 1);
    sceKernelDelayThread(500);
  }

  const int PRETRIG = 8;
  const int THRESHOLD = 20;
  int preHead = 0;
  signed char preX[8] = {0};
  signed char preY[8] = {0};

  int capIndex = 0;
  u32 lastTick = sceKernelGetSystemTimeLow();

  while (!*ended) {
    
    u32 now = sceKernelGetSystemTimeLow();
    if (now - lastTick < SAMPLE_PERIOD) {
      sceKernelDelayThread(500);
      continue;
    }
    lastTick = now;
    sceCtrlPeekBufferPositive(&pad, 1);
    signed char x = (signed char)(pad.Lx - 128);
    signed char y = (signed char)(pad.Ly - 128);

    if (!capturing) {

      preX[preHead % PRETRIG] = x;
      preY[preHead % PRETRIG] = y;
      preHead++;

      if (!sampleReady && preHead >= PRETRIG &&
         (x > THRESHOLD || x < -THRESHOLD || y > THRESHOLD || y < -THRESHOLD)) {

        for (int i = 0; i < PRETRIG; i++) {
          int idx = (preHead + i) % PRETRIG;
          samples[i] = preX[idx];
          samples[i + 64] = preY[idx];
        }
        capIndex = PRETRIG;
        capturing = 1;
      }
    } else {

      samples[capIndex] = x;
      samples[capIndex + 64] = y;
      capIndex++;

      if (capIndex >= 64) {
        
        preHead = 0;
        capIndex = 0;
        capturing = 0;
        for (int i = 0; i < PRETRIG; i++) {
          preX[i] = 0; preY[i] = 0;
        }
        
        sceKernelDcacheWritebackRange(samples, 128);
        sampleReady = 1;
      }
    }
  }
  *ended = 1;
  return sceKernelExitDeleteThread(0);
}

int startThread(SceKernelThreadEntry const thread, int* ended) {
  
  *ended = 0;
  int thid = sceKernelCreateThread("pico-ai-thread",
    thread, 0x18, 0x2000, PSP_THREAD_ATTR_VFPU, 0);
    
  if (thid >= 0) {
    int* param[1] = {ended};
    sceKernelStartThread(thid, sizeof(param), &param);
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
  
  int ended;
  int thid = startThread(recorder, &ended);
  
  SceCtrlData ctl;
  do {
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    
    {
      const int x = 1;
      const int y = 1;
      
      pspDebugScreenSetXY(x, y);
      pspDebugScreenPrintf("Result of the 4 Processing Elements");
      
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
  endThread(&ended, thid);
  sceKernelExitGame();
  return 0;
}
