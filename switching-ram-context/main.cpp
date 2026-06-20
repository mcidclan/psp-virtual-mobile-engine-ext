/*
 * Copyright mcidclan, m-c/d 2026
 */
#include "main.h"

PSP_MODULE_INFO("vme-ctx", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();
VME_DEBUG_SET_BUFFER_WORD_COUNT(32);

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[0])

// (back[n] + front[n]) >> k
VME_LIB_CONTEXT_BUILDER(setupAddContext, param, {
  
  vme_icn(INPUT, 0);
  vme_icn(FLOW, VME_DEF_MAPPER);
  vme_icn(ARCH, VME_DEF_MAPPER);
    
  const u32 op = 0x00010000; // add
  const u32 mux = vme_mux(BASE_0, TOP_0);
  vme_pe0(vme_fu(PRIMARY), mux, op);
  
  const int count = 32 - 1;
  vme_pe0(agu_base(MODE), VME_DEF_MODE);
  vme_pe0(agu_base(COUNT), VME_DEF_STEP, count);
  vme_pe0(agu_top(MODE), VME_DEF_MODE);
  vme_pe0(agu_top(COUNT), VME_DEF_STEP, count);
  
  vme_pe0(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_6);
  vme_pe0(agu_write(COUNT), VME_DEF_STEP, count);
});

// (front[n] - back[n]) >> k
VME_LIB_CONTEXT_BUILDER(setupSubContext, param, {
  
  vme_icn(INPUT, 1);
  vme_icn(FLOW, VME_DEF_MAPPER);
  vme_icn(ARCH, VME_DEF_MAPPER);
    
  const u32 op = 0x00050000; // sub
  const u32 mux = vme_mux(BASE_1, TOP_0);
  vme_pe1(vme_fu(PRIMARY), mux, op);
  
  const int count = 32 - 1;
  vme_pe1(agu_base(MODE), VME_DEF_MODE);
  vme_pe1(agu_base(COUNT), VME_DEF_STEP, count);
  vme_pe1(agu_top(MODE), VME_DEF_MODE);
  vme_pe1(agu_top(COUNT), VME_DEF_STEP, count);
  
  vme_pe1(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_6);
  vme_pe1(agu_write(COUNT), VME_DEF_STEP, count);
});

// (back[n] * front[n]) >> k
VME_LIB_CONTEXT_BUILDER(setupMulContext, param, {
  
  vme_icn(INPUT, 2);
  vme_icn(FLOW, VME_DEF_MAPPER);
  vme_icn(ARCH, VME_DEF_MAPPER);
    
  const u32 op = 0x00200000; // mul
  const u32 mux = vme_mux(BASE_2, TOP_0);
  vme_pe2(vme_fu(PRIMARY), mux, op);
  
  const int count = 32 - 1;
  vme_pe2(agu_base(MODE), VME_DEF_MODE);
  vme_pe2(agu_base(COUNT), VME_DEF_STEP, count);
  vme_pe2(agu_top(MODE), VME_DEF_MODE);
  vme_pe2(agu_top(COUNT), VME_DEF_STEP, count);
  
  vme_pe2(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_6);
  vme_pe2(agu_write(COUNT), VME_DEF_STEP, count);
});

void setupScratchpadData() {

  {
    vmeExtRow4u32* const data = (vmeExtRow4u32*)VME_TOP_BUFFER_0;
    data[0].row = {0x01, 0x02, 0x03, 0x04};
    data[1].row = {0x05, 0x06, 0x07, 0x08};
    data[2].row = {0x09, 0x0a, 0x0b, 0x0c};
    data[3].row = {0x0d, 0x0e, 0x0f, 0x10};
  }

  {
    vmeExtRow4u32* const add = (vmeExtRow4u32*)VME_BASE_BUFFER_0;
    add[0].row = {0x10, 0x0f, 0x0e, 0x0d};
    add[1].row = {0x0c, 0x0b, 0x0a, 0x09};
    add[2].row = {0x08, 0x07, 0x06, 0x05};
    add[3].row = {0x04, 0x03, 0x02, 0x01};
  }

  {
    vmeExtRow4u32* const sub = (vmeExtRow4u32*)VME_BASE_BUFFER_1;
    sub[0].row = {0x01, 0x01, 0x01, 0x01};
    sub[1].row = {0x05, 0x05, 0x05, 0x05};
    sub[2].row = {0x05, 0x05, 0x05, 0x05};
    sub[3].row = {0x09, 0x09, 0x09, 0x09};
  }

  {
    vmeExtRow4u32* const mul = (vmeExtRow4u32*)VME_BASE_BUFFER_2;
    mul[0].row = {0x01, 0x02, 0x01, 0x02};
    mul[1].row = {0x01, 0x02, 0x01, 0x02};
    mul[2].row = {0x01, 0x02, 0x01, 0x02};
    mul[3].row = {0x01, 0x02, 0x01, 0x02};
  }
}

void meLibOnProcess(void) {

  meCoreDcacheWritebackInvalidateAll();
  meLibExceptionHandlerInit(0);
  
  void* const addContext = setupAddContext(nullptr);
  void* const subContext = setupSubContext(nullptr);
  void* const mulContext = setupMulContext(nullptr);
  
  vmeLibEnable();
  
  {
    vmeLibWipe();
    setupScratchpadData();
    
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
  vmeDebugCopy(VME_DBG_IDX_TOP_0);
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
      pspDebugScreenPrintf("Result of 3 different VME contexts executed one after the other:");
      
      pspDebugScreenSetXY(x, y + 2);
      pspDebugScreenPrintf("BASE_0:");
      vmeDebugDisplayBuffer(VME_DEBUG_DIGIT_4, VME_DBG_IDX_BASE_0, x, y + 3);

      pspDebugScreenSetXY(x + 34, y + 2);
      pspDebugScreenPrintf("BASE_1:");
      vmeDebugDisplayBuffer(VME_DEBUG_DIGIT_4, VME_DBG_IDX_BASE_1, x + 34, y + 3);
      
      pspDebugScreenSetXY(x, y + 12);
      pspDebugScreenPrintf("BASE_2:");
      vmeDebugDisplayBuffer(VME_DEBUG_DIGIT_4, VME_DBG_IDX_BASE_2, x, y + 13);
    }
  
    pspDebugScreenSetXY(1, 24);
    pspDebugScreenPrintf("meCounter: 0x%lx", meCounter);
    
    sceDisplayWaitVblank();
    
  } while (!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceKernelDelayThread(100000);
  
  vmeDebugTouch();
  vmeDebugDumpBuffers();
  vmeDebugDumpBuffer(VME_DBG_IDX_TOP_0, "TOP_0");
  vmeDebugFreeBuffers();
  
  sceKernelExitGame();
  return 0;
}
