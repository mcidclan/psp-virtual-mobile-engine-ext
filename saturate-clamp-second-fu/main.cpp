/*
 * Copyright mcidclan, m-c/d 2026
 */
#include "main.h"

PSP_MODULE_INFO("vme-sat", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();
VME_DEBUG_SET_BUFFER_WORD_COUNT(32);

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[1])

#define SHARED_BASE_0_INPUT_OFFSET 32

volatile u32 __attribute__((aligned(64))) sharedTop0[16] = {
  
  0x10, 0x11, 0x12, 0x13,
  0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b,
  0x1c, 0x1d, 0x1e, 0x1f,
};

volatile u32 __attribute__((aligned(64))) sharedTop1[16] = {
  
  0x20, 0x21, 0x22, 0x23,
  0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2a, 0x2b,
  0x2c, 0x2d, 0x2e, 0x2f,
};

volatile u32 __attribute__((aligned(64))) sharedTop2[16] = {
  
  0x30, 0x31, 0x32, 0x33,
  0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3a, 0x3b,
  0x3c, 0x3d, 0x3e, 0x3f,
};

volatile u32 __attribute__((aligned(64))) sharedTop3[16] = {
  
  0x40, 0x41, 0x42, 0x43,
  0x44, 0x45, 0x46, 0x47,
  0x48, 0x49, 0x4a, 0x4b,
  0x4c, 0x4d, 0x4e, 0x4f,
};

volatile u32 __attribute__((aligned(64))) sharedBase0[16] = {
  
  0x01, 0x10, 0x01, (0x7fffff + 0x02),
  0x01, 0x20, 0x01, (0x7fffff + 0x02),
  0x01, 0x30, 0x01, (0x7fffff + 0x02),
  0x01, 0x40, 0x01, (0x7fffff + 0x02),
};

void runContext() {
  
  vmeLibStart();
  // enable all secondary FUs
  vme_set(ENABLE, FU_1, 0x0f << 28);
  // configure interconnect with same params for each top READ and WRITE AGUs (0)
  // configure interconnect for base READ AGUs, 1, 2 and 3 not configured.
  vme_icn(AGU_TOP, 0);
  vme_icn(AGU_BASE, 0x4440);
  vme_icn(AGU_WRITE, 0);
  
  const int count = 16 - 1;
  const int sat = 7 << 7;
  const u32 add = fu_op(ADD, IBUF, RSHIFT);
  const u32 clamp = fu_op(CLAMP, BACK);
  const u32 maxClamp = 0x7fffff;
  const u32 minClamp = 0;
  {
    // apply and ADD computation between TOP_0 and BASE_0
    const u32 fu0Mux = vme_mux(TOP_0, BASE_0); 
    vme_pe0(vme_fu(PRIMARY), fu0Mux, add, sat);
    // clamp last computation between 0 and 128
    const u32 fu1Mux = vme_mux(NONE, STAGING_0);
    vme_pe0(vme_fu(SECONDARY), fu1Mux, clamp);
    vme_pe0(fu_reg(SECONDARY, A), maxClamp);
    vme_pe0(fu_reg(SECONDARY, B), minClamp);
    // configure AGUs
    vme_pe0(agu_top(MODE), VME_DEF_MODE);
    vme_pe0(agu_top(COUNT), VME_DEF_STEP, count);
    vme_pe0(agu_base(MODE), VME_DEF_MODE, SHARED_BASE_0_INPUT_OFFSET);
    vme_pe0(agu_base(COUNT), VME_DEF_STEP, count);
    vme_pe0(agu_write(MODE), VME_DEF_MODE, vme_cyc(0x09));
    vme_pe0(agu_write(COUNT), VME_DEF_STEP, count);
  }

  {
    const u32 fu0Mux = vme_mux(TOP_1, BASE_0);
    vme_pe1(vme_fu(PRIMARY), fu0Mux, add, sat);
    
    const u32 fu1Mux = vme_mux(NONE, STAGING_1);
    vme_pe1(vme_fu(SECONDARY), fu1Mux, clamp);
    vme_pe1(fu_reg(SECONDARY, A), maxClamp);
    vme_pe1(fu_reg(SECONDARY, B), minClamp);
  }

  {
    const u32 fu0Mux = vme_mux(TOP_2, BASE_0);
    const u32 fu1Mux = vme_mux(NONE, STAGING_2);
    vme_pe2(vme_fu(PRIMARY), fu0Mux, add, sat);
    vme_pe2(vme_fu(SECONDARY), fu1Mux, clamp);
    vme_pe2(fu_reg(SECONDARY, A), maxClamp);
    vme_pe2(fu_reg(SECONDARY, B), minClamp);
  }
  
  {
    const u32 fu0Mux = vme_mux(TOP_3, BASE_0);
    const u32 fu1Mux = vme_mux(NONE, STAGING_3);
    vme_pe3(vme_fu(PRIMARY), fu0Mux, add, sat);
    vme_pe3(vme_fu(SECONDARY), fu1Mux, clamp);
    vme_pe3(fu_reg(SECONDARY, A), maxClamp);
    vme_pe3(fu_reg(SECONDARY, B), minClamp);
  }
  
  vmeLibFinish();
}

void meLibOnProcess(void) {

  meCoreDcacheWritebackInvalidateAll();
  meLibExceptionHandlerInit(0);
  
  vmeLibEnable();
  vmeLibWipe();
  
  int count = sizeof(sharedTop0) / sizeof(u32);
 
  // top buffers
  vmeLibMemoryToRingBuffer((void*)sharedTop0, VME_TOP_BUFF0_WOFF, count);
  vmeLibMemoryToRingBuffer((void*)sharedTop1, VME_TOP_BUFF1_WOFF, count);
  vmeLibMemoryToRingBuffer((void*)sharedTop2, VME_TOP_BUFF2_WOFF, count);
  vmeLibMemoryToRingBuffer((void*)sharedTop3, VME_TOP_BUFF3_WOFF, count);
  // base buffers
  vmeLibMemoryToRingBuffer((void*)sharedBase0,
    VME_BASE_BUFF0_WOFF + SHARED_BASE_0_INPUT_OFFSET, count);
  
  runContext();
  
  // debug
  vmeDebugFillWith(VME_BASE_BUFFERS);
  vmeLibDisable();
  
  while (1) {
    
    meCounter += 1;
  }
}

int main() {
  
  scePowerSetClockFrequency(333, 333, 166);
  vmeDebugSetupBuffers();

  meLibDefaultInit();
  
  pspDebugScreenInit();
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
  
  sceKernelExitGame();
  return 0;
}
