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

/*
 * Load a VME context with a 4-stage PE pipeline
 * for 4x4 matrix by vector multiplication using 1D computation
 */
void loadContext() {
  
  vmeLibStart();
  
  // configure the interconnect
  vme_icn(INPUT, VME_DEF_MAPPER);
  vme_icn(FLOW, VME_DEF_MAPPER);
  vme_icn(ARCH, VME_DEF_MAPPER);

  const int count = 32 - 1;
  
  /*
   * Pipeline stage 0: multiply back and front buffers with shift
   */
  {
    // read TOP_0
    vme_pe0(agu_top(MODE), VME_DEF_MODE);
    vme_pe0(agu_top(COUNT), VME_DEF_STEP, count);
    
    // READ TOP_2 (vector) and repeat the first 4 words
    vme_pe2(agu_top(MODE), VME_DEF_MODE);
    vme_pe2(agu_top(COUNT), VME_DEF_STEP, 4 - 1);
    vme_pe2(agu_top(INNER_0), 0x00030001);
    vme_pe2(agu_top(FORMAT_0), VME_RING_TOKEN);
    
    // (TOP_0 * TOP_2) `(back[n] * front[n]) >> k`
    const u32 mux = vme_mux(TOP_2, TOP_0);
    vme_pe0(vme_fu(PRIMARY), mux, 0x00200000);
    
    // write the result to BASE_0 scratchpad with a 6-cycle skew
    vme_pe0(agu_write(MODE), VME_DEF_MODE, vme_cyc(0x06));
    vme_pe0(agu_write(COUNT), VME_DEF_STEP, count);
  }

  /*
   * Pipeline stage 1: multiply-negate back and front buffers with shift
   */
  {
    // -(TOP_0 * TOP_2) `(-(back[n] * front[n])) >> k`
    const u32 mux = vme_mux(TOP_2, TOP_0);
    vme_pe1(vme_fu(PRIMARY), mux, 0x00208000); // 0x20208000
    
    // write the result to BASE_1 scratchpad with a 6-cycle skew
    vme_pe1(agu_write(MODE), VME_DEF_MODE, vme_cyc(0x06));
    vme_pe1(agu_write(COUNT), VME_DEF_STEP, count);
  }
  
  /*
   * Pipeline stage 2: add back and front buffers with shift
   */
  {
    // read BASE_0
    vme_pe0(agu_base(MODE), VME_DEF_MODE, vme_cyc(0x08));
    vme_pe0(agu_base(COUNT), VME_DEF_STEP, count);

    // read BASE_1
    vme_pe1(agu_base(MODE), VME_DEF_MODE, vme_cyc(0x0c));
    vme_pe1(agu_base(COUNT), VME_DEF_STEP, count);
      
    // (BASE_0 + BASE_1) `(back[n] + front[n]) >> k`
    const u32 mux = vme_mux(BASE_1, BASE_0);
    vme_pe2(vme_fu(PRIMARY), mux, 0x00010000); // 0x52010000
    
    // write the result to BASE_2 scratchpad with a 14-cycle skew
    vme_pe2(agu_write(MODE), VME_DEF_MODE, vme_cyc(0x0e));
    vme_pe2(agu_write(COUNT), VME_DEF_STEP, count);
  }
  
  /*
   * Pipeline stage 3: accumulate with shift and bias
   */
  {
    // read BASE_2
    vme_pe2(agu_base(MODE), VME_DEF_MODE, vme_cyc(0x0f));
    vme_pe2(agu_base(COUNT), VME_DEF_STEP, count);
      
    // (ACC + BASE_2[n]) `(i == 0 ? (b >> k) : out[n-1]) + (back[n] >> k)`
    const u32 mux = vme_mux(NONE, BASE_2);
    vme_pe3(vme_fu(PRIMARY), mux, 0x00250000); //0x03250000
    
    // write the result to BASE_2 scratchpad with a 17-cycle skew
    vme_pe3(agu_write(MODE), VME_DEF_MODE, vme_cyc(0x11));
    vme_pe3(agu_write(COUNT), VME_DEF_STEP, count);
  }
  
  vmeLibFinish();
}

void meLibOnProcess(void) {

  static u32 lastIdx = 0;

  meCoreDcacheWritebackInvalidateAll();
  meLibExceptionHandlerInit(0);
  
  vmeLibEnable();
  vmeLibWipe();
  
  vmeExtSetPadded4x4((void*)VME_TOP_BUFFER_0, (void*)sharedMat);
  vmeExtSetWord4((void*)VME_TOP_BUFFER_2, (void*)sharedVec);

  loadContext();
  vmeExtGetWord4FromPaddedMac((void*)VME_BASE_BUFFER_3, (void*)sharedRes);

  // debug
  vmeDebugFillWith((VME_BASE_BUFFERS), VME_DEBUG_BUFFER_WORD_COUNT);
  vmeLibDisable();
  
  lastIdx = sharedIdx;
  
  while (1) {
    
    meCounter += 1;
    
    if (lastIdx != sharedIdx) {
      
      vmeLibEnable();
    
      void* const vector = (void*)(&sharedVec[sharedIdx * 4]);
      vmeExtSetWord4((void*)VME_TOP_BUFFER_2, vector);

      vmeLibStart();
      vmeLibRefresh();
      vmeExtGetWord4FromPaddedMac((void*)VME_BASE_BUFFER_3, (void*)sharedRes);
      
      // debug
      vmeDebugFillWith((VME_BASE_BUFFERS), VME_DEBUG_BUFFER_WORD_COUNT);
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

#define setupData() _setupData(0)
#define cleanData() _setupData(1)

void _setupData(bool clean) {
  
  static Uncached32 _sharedMat = {&sharedMat, nullptr};
  static Uncached32 _sharedVec = {&sharedVec, nullptr};
  static Uncached32 _sharedRes = {&sharedRes, nullptr};
  
  if (clean) {
    
    meLibAllocUncached32(&_sharedMat, 0);
    meLibAllocUncached32(&_sharedVec, 0);
    meLibAllocUncached32(&_sharedRes, 0);
    return;
  }
  
  meLibAllocUncached32(&_sharedMat, 16); // 4x4 mat
  meLibAllocUncached32(&_sharedVec, 16); // array of 4 input vectors
  meLibAllocUncached32(&_sharedRes, 4);  // result vector

  vmeExtMat4x4u32* const mat = (vmeExtMat4x4u32*)sharedMat;
  mat->x = {0x00, 0x01, 0x02, 0x03};
  mat->y = {0x04, 0x05, 0x06, 0x07};
  mat->z = {0x08, 0x09, 0x0a, 0x0b};
  mat->w = {0x0c, 0x0d, 0x0e, 0x0f};
  
  vmeExtRow4u32* const vec = (vmeExtRow4u32*)sharedVec;
  vec[0].row = {0x01, 0x00, 0x00, 0x00};
  vec[1].row = {0x01, 0x01, 0x01, 0x01};
  vec[2].row = {0x01, 0x02, 0x04, 0x08};
  vec[3].row = {0x00, 0x00, 0x00, 0x01};
}

void displayData(const int x, const int y) {

  const u32* const mat = (u32*)sharedMat;
  const u32* const in = (u32*)&(sharedVec[sharedIdx * 4]);
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
    
    displayData(1, 3);
    
    sceDisplayWaitVblank();
    
  } while (!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceKernelDelayThread(100000);
  
  vmeDebugTouch();
  vmeDebugDumpBuffers();
  vmeDebugFreeBuffers();
  
  cleanData();
  sceKernelExitGame();
  return 0;
}
