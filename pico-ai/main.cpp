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

#include "main.h"

PSP_MODULE_INFO("vme-pico-ai", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();
VME_DEBUG_SET_BUFFER_WORD_COUNT(4);

#define MIN_LOCK      90
#define SAMPLE_PERIOD 16667

#define VME_DATA_GAP 256
#define VME_OUTPUT_LAYER_BUFFER_COUNT 4
#define VME_SAMPLE_BUFFER_OFFSET (VME_BASE_BUFF0_WOFF + VME_DATA_GAP)

#define VME_SAMPLE_BUFFER_COUNT 128
#define VME_SAMPLE_BUFFER_SIZE (VME_SAMPLE_BUFFER_COUNT * 4)
static volatile int samples[VME_SAMPLE_BUFFER_COUNT] __attribute__((aligned(64))) = {0};

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[0])
#define sampleReady  (meLibSharedMemory[1])
#define capturing    (meLibSharedMemory[2])

static volatile u32* weights __attribute__((aligned(64))) = NULL;

VME_LIB_CONTEXT_BUILDER(setupHiddenLayer, param, {
  
  vme_set(ENABLE, FU_1, 0x0f << 28);

  vme_icn(AGU_TOP, 0);
  vme_icn(AGU_BASE, 0x4440);
  vme_icn(AGU_WRITE, 0);
  
  const int rShift = 7;
  const u32 max = 127;
  const u32 min = 0;
  
  const u32 VMAC = fu_op(MAC, INNER_PRODUCT_BIAS);
  const u32 CLAMP = fu_op(CLAMP, BACK);

  // note: ReLu and sat are managed with max(0, min(127, n) on secondary FUs

  // first neuron 
  {
    {
      const u32 mux = vme_mux(TOP_0, BASE_0);
      vme_pe0(vme_fu(PRIMARY), VMAC, mux, rShift);
    }
    {
      const u32 mux = vme_mux(NONE, STAGING_0);
      vme_pe0(vme_fu(SECONDARY), mux, CLAMP);
      vme_pe0(fu_reg(SECONDARY, A), max);
      vme_pe0(fu_reg(SECONDARY, B), min);
    }
  }

  const int count = VME_SAMPLE_BUFFER_COUNT;
  
  // weights for first layer
  vme_pe0(agu_top(MODE), VME_DEF_MODE);
  vme_pe0(agu_top(COUNT), VME_DEF_STEP, count);
  
   // dynamic input from joystick reader
  vme_pe0(agu_base(MODE), VME_DEF_MODE, VME_DATA_GAP);
  vme_pe0(agu_base(COUNT), VME_DEF_STEP, count);
  
  vme_pe0(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_9);
  vme_pe0(agu_write(COUNT), VME_DEF_STEP, count);
  
  // second neuron
  {
    const u32 mux = vme_mux(TOP_1, BASE_0);
    vme_pe1(vme_fu(PRIMARY), VMAC, mux, rShift);
    {
      const u32 mux = vme_mux(NONE, STAGING_1);
      vme_pe1(vme_fu(SECONDARY), mux, CLAMP);
      vme_pe1(fu_reg(SECONDARY, A), max);
      vme_pe1(fu_reg(SECONDARY, B), min);
    }
  }
  
  // third neuron
  {
    const u32 mux = vme_mux(TOP_2, BASE_0);
    vme_pe2(vme_fu(PRIMARY), VMAC, mux, rShift);
    {
      const u32 mux = vme_mux(NONE, STAGING_2);
      vme_pe2(vme_fu(SECONDARY), mux, CLAMP);
      vme_pe2(fu_reg(SECONDARY, A), max);
      vme_pe2(fu_reg(SECONDARY, B), min);
    }
  }
  
  // forth neuron
  {
    const u32 mux = vme_mux(TOP_3, BASE_0);
    vme_pe3(vme_fu(PRIMARY), VMAC, mux, rShift);
    {
      const u32 mux = vme_mux(NONE, STAGING_3);
      vme_pe3(vme_fu(SECONDARY), mux, CLAMP);
      vme_pe3(fu_reg(SECONDARY, A), max);
      vme_pe3(fu_reg(SECONDARY, B), min);
    }
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

  const int count = VME_OUTPUT_LAYER_BUFFER_COUNT;
  
  // weights for second layer
  vme_pe0(agu_top(MODE), VME_DEF_MODE, VME_DATA_GAP);
  vme_pe0(agu_top(COUNT), VME_DEF_STEP, count);
  
   // dynamic data from previous hidden layer output
  vme_pe0(agu_base(MODE), VME_DEF_MODE, VME_DATA_GAP);
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

void meLibOnProcess(void) {

  meCoreDcacheWritebackInvalidateAll();
  meLibExceptionHandlerInit(0);

  // setup scratchpad data
  vmeLibEnable();
  vmeLibWipe();
  
  // weight for layer 1
  {
    const int count = VME_SAMPLE_BUFFER_COUNT;
    vmeLibMemoryToRingBuffer((void*)&weights[count*0], VME_TOP_BUFF0_WOFF, count);
    vmeLibMemoryToRingBuffer((void*)&weights[count*1], VME_TOP_BUFF1_WOFF, count);
    vmeLibMemoryToRingBuffer((void*)&weights[count*2], VME_TOP_BUFF2_WOFF, count);
    vmeLibMemoryToRingBuffer((void*)&weights[count*3], VME_TOP_BUFF3_WOFF, count);
  }
  
  // weight for layer 2
  {
    const int gap = VME_DATA_GAP;
    const u32 offset = VME_SAMPLE_BUFFER_COUNT * 4;
    const int count = VME_OUTPUT_LAYER_BUFFER_COUNT;
    vmeLibMemoryToRingBuffer((void*)&weights[offset + 0x00], VME_TOP_BUFF0_WOFF + gap, count);
    vmeLibMemoryToRingBuffer((void*)&weights[offset + 0x04], VME_TOP_BUFF1_WOFF + gap, count);
    vmeLibMemoryToRingBuffer((void*)&weights[offset + 0x08], VME_TOP_BUFF2_WOFF + gap, count);
    vmeLibMemoryToRingBuffer((void*)&weights[offset + 0x0c], VME_TOP_BUFF3_WOFF + gap, count);
  }
  
  vmeLibDisable();

  // setup VME contexts: neuron layers
  void* const hiddenLayerContext = setupHiddenLayer(nullptr);
  void* const outputLayerContext = setupOutputLayer(nullptr);
  
  while (1) {

    if (sampleReady) {
      
      vmeLibEnable();
      
      meCoreDcacheInvalidateRange((void*)samples, VME_SAMPLE_BUFFER_SIZE);
      vmeLibMemoryToRingBuffer((void*)samples, VME_SAMPLE_BUFFER_OFFSET, VME_SAMPLE_BUFFER_COUNT);
      
      // todo: review context switch
      {
        vmeLibStart();
        vmeLibLoadCustomContext(hiddenLayerContext);
        vmeLibFinish();
        
        // copy previous result to BASE_0 + VME_DATA_GAP words
        const u32 dstBase = VME_BASE_BUFFER_0 + VME_DATA_GAP * 4;
        const u32 srcOff = VME_SAMPLE_BUFFER_SIZE - 4;

        hw(dstBase + 0x00) = hw(VME_BASE_BUFFER_0 + srcOff);
        hw(dstBase + 0x04) = hw(VME_BASE_BUFFER_1 + srcOff);
        hw(dstBase + 0x08) = hw(VME_BASE_BUFFER_2 + srcOff);
        hw(dstBase + 0x0c) = hw(VME_BASE_BUFFER_3 + srcOff);
        
        vmeLibStart();
        vmeLibLoadCustomContext(outputLayerContext);
        vmeLibFinish();
        
        vmeDebugFillWith(VME_BASE_BUFFERS);
      }
      
      vmeLibDisable();
      sampleReady = 0;
    }
    
    meCounter += 1;
  }
}

#define JOYSTICK_ADJUSTER(v) ((8 * (int)(v / 8)) - 128)

int locked = 0;
int reader(SceSize args, void *argp) {
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
  const int THRESHOLD = 64;
  
  //int locked = 0;
  int preHead = 0;
  int capIndex = 0;

  signed char preX[8] = {0};
  signed char preY[8] = {0};
  
  u32 lastTick = sceKernelGetSystemTimeLow();
  while (!*ended) {

    u32 now = sceKernelGetSystemTimeLow();
    if (now - lastTick < SAMPLE_PERIOD) {
      sceKernelDelayThread(500);
      continue;
    }
    lastTick = now;

    if (sampleReady) {
      // lock until the VME has finished with the previous sample
      sceKernelDelayThread(500);
      continue;
    }

    sceCtrlPeekBufferPositive(&pad, 1);
    signed char x = (signed char)JOYSTICK_ADJUSTER(pad.Lx);
    signed char y = (signed char)JOYSTICK_ADJUSTER(pad.Ly);

    if (!capturing) {
      
      if (!locked) {
        
        preX[preHead % PRETRIG] = x;
        preY[preHead % PRETRIG] = y;
        preHead++;
        
        if (preHead >= PRETRIG &&
           (x > THRESHOLD || x < -THRESHOLD || y > THRESHOLD || y < -THRESHOLD)) {
          
          for (int i = 0; i < PRETRIG; i++) {
            int idx = (preHead + i) % PRETRIG;
            samples[i] = preX[idx];
            samples[i + 64] = preY[idx];
          }
          capIndex = PRETRIG;
          capturing = 1;
        }
      }
    } else {
      
      samples[capIndex] = x;
      samples[capIndex + 64] = y;
      capIndex++;
      
      if (capIndex >= 64) {

        capturing = 0;
        locked = 1;

        sceKernelDcacheWritebackRange((void*)samples, VME_SAMPLE_BUFFER_SIZE);
        sampleReady = 1;

        preHead = 0;
        for (int i = 0; i < PRETRIG; i++) { preX[i] = 0; preY[i] = 0; }

        {
          u32 lockTick = sceKernelGetSystemTimeLow();
          int lockFrames = 1;
          while (lockFrames && !*ended) {

            u32 lockNow = sceKernelGetSystemTimeLow();
            if (lockNow - lockTick < SAMPLE_PERIOD) {
              sceKernelDelayThread(500);
              continue;
            }
            lockTick = lockNow;

            sceCtrlPeekBufferPositive(&pad, 1);
            signed char lx = (signed char)JOYSTICK_ADJUSTER(pad.Lx);
            signed char ly = (signed char)JOYSTICK_ADJUSTER(pad.Ly);

            if (lx > THRESHOLD || lx < -THRESHOLD || ly > THRESHOLD || ly < -THRESHOLD) {
              preX[preHead % PRETRIG] = lx;
              preY[preHead % PRETRIG] = ly;
              preHead++;
              lastTick = lockTick;
              lockFrames = 0;
            }
          }
        }

        locked = 0;
        capIndex = 0;
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
  Uncached32* const uvar = loadBinary("./model.bin", &weights);
  
  vmeDebugSetupBuffers();
  pspDebugScreenInit();
  meLibDefaultInit();
  
  if (!uvar->base) {
    pspDebugScreenSetXY(1, 1);
    pspDebugScreenPrintf("Model not load");
    sceKernelExitGame();
  }
  
  int ended;
  int thid = startThread(reader, &ended);
  
  SceCtrlData ctl;
  do {
    pspDebugScreenSetTextColor(0xffffffff);

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
      
      pspDebugScreenSetXY(x, y + 5);
      pspDebugScreenPrintf("BASE_2:");
      vmeDebugDisplayBuffer(VME_DEBUG_DIGIT_4, VME_DBG_IDX_BASE_2, x, y + 6);
      
      pspDebugScreenSetXY(x + 34, y + 5);
      pspDebugScreenPrintf("BASE_3:");
      vmeDebugDisplayBuffer(VME_DEBUG_DIGIT_4, VME_DBG_IDX_BASE_3, x + 34, y + 6);
    }

    {
      const signed char b[] = {
        (signed char)(vmeDebugGetValue(VME_DBG_IDX_BASE_0, 3) & 0xFF),
        (signed char)(vmeDebugGetValue(VME_DBG_IDX_BASE_1, 3) & 0xFF),
        (signed char)(vmeDebugGetValue(VME_DBG_IDX_BASE_2, 3) & 0xFF),
        (signed char)(vmeDebugGetValue(VME_DBG_IDX_BASE_3, 3) & 0xFF),
      };
      
      int index = 0;
      for (int i = 1; i < 4; i++) {
        if (b[i] > b[index]) {
          index = i;
        }
      }
      pspDebugScreenSetXY(1, 10);
      pspDebugScreenPrintf("Label/Class: %i   ", index);
    }
    
    pspDebugScreenSetXY(1, 11);
    if (!locked) {
      pspDebugScreenSetTextColor(0xff00ff00);
      pspDebugScreenPrintf("capturing...                 ");
      pspDebugScreenSetTextColor(0xffffffff);
    } else {
      pspDebugScreenPrintf("waiting for motion...");
    }
    
    pspDebugScreenSetXY(46, 32);
    pspDebugScreenPrintf("ME Counter: 0x%lx", meCounter);
    
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
