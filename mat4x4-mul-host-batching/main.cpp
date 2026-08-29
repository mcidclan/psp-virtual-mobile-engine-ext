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

PSP_MODULE_INFO("vme-vm-batch", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();

#define BATCH_COUNT 4
#define VECTOR_COUNT 4

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[1])
#define sharedIdx    (meLibSharedMemory[2])

volatile u32 __attribute__((aligned(64))) sharedMat[16] = {
  0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0e, 0x0f,
};

volatile u32 __attribute__((aligned(64))) sharedVec[16 * 4] = {
  0x01, 0x00, 0x00, 0x00,
  0x01, 0x01, 0x01, 0x01,
  0x01, 0x02, 0x04, 0x08,
  0x00, 0x00, 0x00, 0x01,
  
  0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0e, 0x0f,
  
  0x20, 0x21, 0x22, 0x23,
  0x30, 0x31, 0x32, 0x33,
  0x40, 0x42, 0x42, 0x43,
  0x50, 0x51, 0x52, 0x53,
  
  0x02, 0x02, 0x03, 0x03,
  0x04, 0x04, 0x05, 0x05,
  0x08, 0x08, 0x09, 0x09,
  0x10, 0x10, 0x11, 0x11,
};

volatile u32 __attribute__((aligned(64))) sharedRes[16] = {0};

/*
 * 4x4 matrix-by-vector multiplication using PE row distribution
 */
VME_LIB_CONTEXT_BUILDER(setupMulMatCtx, param, {
  
  vme_icn(AGU_TOP, 0);
  vme_icn(AGU_BASE, VME_DEF_MAPPER);
  vme_icn(AGU_WRITE, 0);

  const int count = 4 - 1;
  const u32 op = 0x00240000; // VME_FU_OPCODE_MAC_INNER_PRODUCT_BIAS
  
  {
    const u32 mux = vme_mux(TOP_0, BASE_0);
    vme_pe0(vme_fu(PRIMARY), mux, op);
    
    vme_pe0(agu_top(MODE), VME_DEF_MODE);
    vme_pe0(agu_top(COUNT), VME_DEF_STEP, count);
    
    vme_pe0(agu_base(MODE), VME_DEF_MODE, 0x100);
    vme_pe0(agu_base(COUNT), VME_DEF_STEP, count);
  
    vme_pe0(agu_write(MODE), VME_DEF_MODE, vme_cyc(0x06));
    vme_pe0(agu_write(COUNT), VME_DEF_STEP, count);
  }

  { 
    const u32 mux = vme_mux(TOP_1, BASE_0);
    vme_pe1(vme_fu(PRIMARY), mux, op);
  }
  
  {
    const u32 mux = vme_mux(TOP_2, BASE_0);
    vme_pe2(vme_fu(PRIMARY), mux, op);
  }
  
  { 
    const u32 mux = vme_mux(TOP_3, BASE_0);
    vme_pe3(vme_fu(PRIMARY), mux, op);
  }
  
});

void meLibOnProcess(void) {

  static u32 lastIdx = (BATCH_COUNT - 1);

  meCoreDcacheWritebackInvalidateAll();
  meLibExceptionHandlerInit(0);
  
  bool preloadDone = false;
  void* const mulMatCtx = setupMulMatCtx(nullptr);
  
  vmeLibEnable();
  vmeLibWipe();
  
  //const int size = ((sizeof(sharedVec) + 63) & ~63);
  //meCoreDcacheWritebackRange((void*)sharedVec, size);
  vmeLibMemoryToRingBuffer((void*)sharedVec, 0x100, sizeof(sharedVec) / 4);
  vmeSetDistributed4x4((void*)sharedMat);
  
  vmeLibDisable();
  
  while (1) {
    
    meCounter += 1;
    
    if (lastIdx != sharedIdx) {
      
      vmeLibEnable();
      
      for (int i=0; i < (VECTOR_COUNT + 1); i++) {
        
        if (!preloadDone) {
          
          vmeLibPreloadCustomContext(mulMatCtx);
          preloadDone = true;
        }

        const int vIndex = i * 4; // vector index
        const int bOffset = sharedIdx * 16; // batch offset

        vmeLibStart();
        vme_pe0(agu_base(MODE), VME_DEF_MODE, 0x100 + vIndex + bOffset);
        vmeLibFinish();
        
        vmeGetVectorFromDistributedVMac((void*)&(sharedRes[vIndex]));
      }

      meCoreDcacheWritebackRange((void*)sharedRes, sizeof(sharedRes) / 4);
      
      vmeLibDisable();
      
      lastIdx = sharedIdx;
      meLibSync();
    }
  }
}

void updateVector(SceCtrlData* const ctl) {
  
  static bool up = false;
  if (!(ctl->Buttons & PSP_CTRL_TRIANGLE) && !(ctl->Buttons & PSP_CTRL_CROSS)) {
    up = true;
  } else if (up) {
    
    if (ctl->Buttons & PSP_CTRL_TRIANGLE) {
      sharedIdx = (sharedIdx + 1) & (BATCH_COUNT - 1);
      up = false;
    }
    else if (ctl->Buttons & PSP_CTRL_CROSS) {
      sharedIdx = (sharedIdx - 1) & (BATCH_COUNT - 1);
      up = false;
    }
  }
}

void displayData() {

  const u32* const mat = (u32*)sharedMat;
  const u32* const in = (u32*)&(sharedVec[sharedIdx * 16]);
  
  sceKernelDcacheInvalidateRange((void*)sharedRes, 64);
  const u32* const out = (u32*)sharedRes;

  pspDebugScreenPrintf("\nInput 4x4 Matrix:\n");
  pspDebugScreenPrintf("%lu, %lu, %lu, %lu\n", mat[0], mat[1], mat[2], mat[3]);
  pspDebugScreenPrintf("%lu, %lu, %lu, %lu\n", mat[4], mat[5], mat[6], mat[7]);
  pspDebugScreenPrintf("%lu, %lu, %lu, %lu\n", mat[8], mat[9], mat[10], mat[11]);
  pspDebugScreenPrintf("%lu, %lu, %lu, %lu\n", mat[12], mat[13], mat[14], mat[15]);
  
  pspDebugScreenPrintf("\nInput Vectors:\n");
  pspDebugScreenPrintf("%lu, %lu, %lu, %lu\n", in[0], in[1], in[2], in[3]);
  pspDebugScreenPrintf("%lu, %lu, %lu, %lu\n", in[4], in[5], in[6], in[7]);
  pspDebugScreenPrintf("%lu, %lu, %lu, %lu\n", in[8], in[9], in[10], in[11]);
  pspDebugScreenPrintf("%lu, %lu, %lu, %lu\n", in[12], in[13], in[14], in[15]);

  pspDebugScreenPrintf("\nVME Output Vectors:\n");
  pspDebugScreenPrintf("%lu, %lu, %lu, %lu\n", out[0], out[1], out[2], out[3]);
  pspDebugScreenPrintf("%lu, %lu, %lu, %lu\n", out[4], out[5], out[6], out[7]);
  pspDebugScreenPrintf("%lu, %lu, %lu, %lu\n", out[8], out[9], out[10], out[11]);
  pspDebugScreenPrintf("%lu, %lu, %lu, %lu\n", out[12], out[13], out[14], out[15]);

  {
    ScePspFMatrix4 __attribute__((aligned(16))) vfpuMat;
    ScePspFVector4 __attribute__((aligned(16))) vfpuVec[4];
    ScePspFVector4 __attribute__((aligned(16))) vfpuOut[4];

    // convert integer matrix and vector to float to compare VFPU and VME results
    for (int i = 0; i < 16; i++) {
      ((float*)&vfpuMat)[i] = (float)((u32*)mat)[i];
    }

    for (int i = 0; i < 4; i++) {
      const u32* const iBase = &(in[i * 4]);
      float* const vBase = (float*)&(vfpuVec[i]);
      for(int j = 0; j < 4; j++) {
        vBase[j] = (float)(iBase[j]);
      }
    }
      
    vfpuMat4x4MulVec((void*)&(vfpuOut[0]), (void*)&vfpuMat, (void*)&(vfpuVec[0]));
    vfpuMat4x4MulVec((void*)&(vfpuOut[1]), (void*)&vfpuMat, (void*)&(vfpuVec[1]));
    vfpuMat4x4MulVec((void*)&(vfpuOut[2]), (void*)&vfpuMat, (void*)&(vfpuVec[2]));
    vfpuMat4x4MulVec((void*)&(vfpuOut[3]), (void*)&vfpuMat, (void*)&(vfpuVec[3]));

    pspDebugScreenPrintf("\nVFPU reference Vector:\n");
    pspDebugScreenPrintf("%.0f, %.0f, %.0f, %.0f\n", vfpuOut[0].x, vfpuOut[0].y, vfpuOut[0].z, vfpuOut[0].w);
    pspDebugScreenPrintf("%.0f, %.0f, %.0f, %.0f\n", vfpuOut[1].x, vfpuOut[1].y, vfpuOut[1].z, vfpuOut[1].w);
    pspDebugScreenPrintf("%.0f, %.0f, %.0f, %.0f\n", vfpuOut[2].x, vfpuOut[2].y, vfpuOut[2].z, vfpuOut[2].w);
    pspDebugScreenPrintf("%.0f, %.0f, %.0f, %.0f\n", vfpuOut[3].x, vfpuOut[3].y, vfpuOut[3].z, vfpuOut[3].w);
  }
  
  pspDebugScreenPrintf("\nUse Triangle/Cross to change the Vector values\n");
}


int main() {
  
  meLibDefaultInit();
  pspDebugScreenInit();
  
  SceCtrlData ctl;
  do {
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    updateVector(&ctl);
    
    pspDebugScreenSetXY(0, 0);
    pspDebugScreenPrintf("meCounter: 0x%lx\n", meCounter);
    
    displayData();
    
    sceDisplayWaitVblank();
    
  } while (!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceKernelDelayThread(100000);
  
  sceKernelExitGame();
  return 0;
}
