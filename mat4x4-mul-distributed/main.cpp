/*
 * Copyright mcidclan, m-c/d 2026
 */
#include "main.h"

PSP_MODULE_INFO("vme-mat", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();
VME_DEBUG_SET_BUFFER_WORD_COUNT(32);

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[1])
#define sharedIdx    (meLibSharedMemory[2])


volatile u32 __attribute__((aligned(64))) sharedMat[16] = {
  0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0e, 0x0f,
};

volatile u32 __attribute__((aligned(64))) sharedVec[16] = {
  0x01, 0x00, 0x00, 0x00,
  0x01, 0x01, 0x01, 0x01,
  0x01, 0x02, 0x04, 0x08,
  0x00, 0x00, 0x00, 0x01,
};

volatile u32 __attribute__((aligned(64))) sharedRes[16] = {0};

/*
 * 4x4 matrix-by-vector multiplication using PE row distribution
 */
void runContext() {
  
  vmeLibStart();
  
  // configure the interconnect
  vme_icn(AGU_TOP, 0);
  vme_icn(AGU_BASE, VME_DEF_MAPPER);
  vme_icn(AGU_WRITE, 0);

  const int count = 4 - 1;
  
  /*
   * Pipeline stage 0 
   */
  {
    const u32 mux = vme_mux(TOP_0, BASE_0);
    vme_pe0(vme_fu(PRIMARY), mux, 0x00240000);
    
    vme_pe0(agu_top(MODE), VME_DEF_MODE);
    vme_pe0(agu_top(COUNT), VME_DEF_STEP, count);
    
    vme_pe0(agu_base(MODE), VME_DEF_MODE, 4);
    vme_pe0(agu_base(COUNT), VME_DEF_STEP, count);
  
    vme_pe0(agu_write(MODE), VME_DEF_MODE, vme_cyc(0x06));
    vme_pe0(agu_write(COUNT), VME_DEF_STEP, count);
  }

  /*
   * Pipeline stage 1
   */
  { 
    const u32 mux = vme_mux(TOP_1, BASE_0);
    vme_pe1(vme_fu(PRIMARY), mux, 0x00240000);
  }
  
  /*
   * Pipeline stage 2
   */
  {
    const u32 mux = vme_mux(TOP_2, BASE_0);
    vme_pe2(vme_fu(PRIMARY), mux, 0x00240000);
  }
  
  /*
   * Pipeline stage 3
   */
  { 
    const u32 mux = vme_mux(TOP_3, BASE_0);
    vme_pe3(vme_fu(PRIMARY), mux, 0x00240000);
  }
  
  vmeLibFinish();
}

void meLibOnProcess(void) {

  static u32 lastIdx = 0;

  meCoreDcacheWritebackInvalidateAll();
  meLibExceptionHandlerInit(0);
  
  vmeLibEnable();
  vmeLibWipe();
  
  vmExtSetDistributed4x4((void*)VME_TOP_BUFFERS, (void*)sharedMat);
  vmeExtSetWord4((void*)(VME_BASE_BUFFER_0 + 16), (void*)sharedVec);
  
  runContext();
  
  vmeExtGetWord4FromDistributedVMac((void*)VME_BASE_BUFFERS, (void*)sharedRes);
  meCoreDcacheWritebackRange((void*)sharedRes, 64);
  
  // debug
  vmeDebugFillWith(VME_BASE_BUFFERS);
  vmeLibDisable();
  
  lastIdx = sharedIdx;
  
  while (1) {
    
    meCounter += 1;
    
    if (lastIdx != sharedIdx) {
      
      vmeLibEnable();
    
      void* const vector = (void*)(&sharedVec[sharedIdx * 4]);
      vmeExtSetWord4((void*)(VME_BASE_BUFFER_0 + 16), vector);

      vmeLibStart();
      vmeLibRefresh();
      
      vmeExtGetWord4FromDistributedVMac((void*)VME_BASE_BUFFERS, (void*)sharedRes);
      meCoreDcacheWritebackRange((void*)sharedRes, 64);
      
      // debug
      vmeDebugFillWith(VME_BASE_BUFFERS);
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
      sharedIdx = (sharedIdx + 1) & 3;
      up = false;
    }
    else if (ctl->Buttons & PSP_CTRL_CROSS) {
      sharedIdx = (sharedIdx - 1) & 3;
      up = false;
    }
  }
}

void displayData(const int x, const int y) {

  const u32* const mat = (u32*)sharedMat;
  const u32* const in = (u32*)&(sharedVec[sharedIdx * 4]);
  
  sceKernelDcacheInvalidateRange((void*)sharedRes, 64);
  const u32* const out = (u32*)sharedRes;

  pspDebugScreenSetXY(x, y);
  pspDebugScreenPrintf("Input 4x4 Matrix:");
  
  for(int i = 0; i < 4; i++) {
    
    const int off = i * 4;
    pspDebugScreenSetXY(x, i+1+y);
    pspDebugScreenPrintf("%lu, %lu, %lu, %lu",
      mat[off+0], mat[off+1], mat[off+2], mat[off+3]);
  }
  
  pspDebugScreenSetXY(x, 6+y);
  pspDebugScreenPrintf("Input Vector:");
  pspDebugScreenSetXY(x, 7+y);
  pspDebugScreenPrintf("%lu, %lu, %lu, %lu", in[0], in[1], in[2], in[3]);

  pspDebugScreenSetXY(x, 9+y);
  pspDebugScreenPrintf("VME Output Vector:");
  pspDebugScreenSetXY(x, 10+y);
  pspDebugScreenPrintf("%lu, %lu, %lu, %lu        ", out[0], out[1], out[2], out[3]);

  {
    ScePspFMatrix4 __attribute__((aligned(16))) vfpuMat;
    ScePspFVector4 __attribute__((aligned(16))) vfpuVec;
    ScePspFVector4 __attribute__((aligned(16))) vfpuOut;

    // convert integer matrix and vector to float to compare VFPU and VME results
    for(int i = 0; i < 16; i++) {
      
      ((float*)&vfpuMat)[i] = (float)((u32*)mat)[i];
      if (i < 4) {
        ((float*)&vfpuVec)[i] = (float)((u32*)in)[i];
      }
    }
    
    vfpuMat4x4MulVec((void*)&vfpuOut, (void*)&vfpuMat, (void*)&vfpuVec);

    pspDebugScreenSetXY(x, 12+y);
    pspDebugScreenPrintf("VFPU reference Vector:");
    pspDebugScreenSetXY(x, 13+y);
    pspDebugScreenPrintf("%.0f, %.0f, %.0f, %.0f        ", vfpuOut.x, vfpuOut.y, vfpuOut.z, vfpuOut.w);
  }
  
  pspDebugScreenSetXY(x, 15+y);
  pspDebugScreenPrintf("Use Triangle/Cross to change the Vector values");
}


int main() {
  
  scePowerSetClockFrequency(333, 333, 166);
  vmeDebugSetupBuffers();

  meLibDefaultInit();
  
  pspDebugScreenInit();
  SceCtrlData ctl;
  do {
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    updateVector(&ctl);
    
    pspDebugScreenSetXY(1, 1);
    pspDebugScreenPrintf("meCounter: 0x%lx", meCounter);
    
    displayData(1, 3);
    
    sceDisplayWaitVblank();
    
  } while (!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceKernelDelayThread(100000);
  
  vmeDebugTouch();
  vmeDebugDumpBuffers();
  vmeDebugFreeBuffers();
  
  sceKernelExitGame();
  return 0;
}
