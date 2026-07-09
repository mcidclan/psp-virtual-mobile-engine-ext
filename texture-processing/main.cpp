/*
 * Copyright mcidclan, m-c/d 2026
 */
#include "main.h"

PSP_MODULE_INFO("vme-tex", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();
VME_DEBUG_SET_BUFFER_WORD_COUNT(32);

volatile u16* input __attribute__((aligned(64))) = nullptr;
volatile u32* output __attribute__((aligned(64))) = nullptr;

#define TILE_SIZE 1024
meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[1])

/*
void runContext() {
  
  vmeLibStart();
  //
  vmeLibFinish();
}
*/

void meLibOnProcess(void) {

  const u32 buffer[] = {
    /*
    0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C,
    0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C,
    0x1D, 0x1E, 0x1F, 0x20,
    */
    
    // 0x00010203, 0x00040506, 0x00070809, 0x000a0b0c, 0x00d0e0f,

    0x01020304, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
    0x01020304, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
    0x01020304, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
    0x01020304, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
    0x01020304, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
    0x01020304, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
    0x01020304, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
    0x01020304, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
  };
  
  meCoreDcacheWritebackInvalidateAll();
  meLibExceptionHandlerInit(0);
  
  vmeLibEnable();
  vmeLibWipe();
  
  //int count = sizeof(batch[0]) / sizeof(u32);
 
  // top buffers
  //vmeLibMemoryToRingBuffer((void*)&batch[0], VME_TOP_BUFF0_WOFF, count);
  //vmeLibMemoryToRingBuffer((void*)&batch[1], VME_TOP_BUFF1_WOFF, count);
  //vmeLibMemoryToRingBuffer((void*)&batch[2], VME_TOP_BUFF2_WOFF, count);
  //vmeLibMemoryToRingBuffer((void*)&batch[3], VME_TOP_BUFF3_WOFF, count);
  
  
  vmeLibMemTo16((u32)buffer, 0, 16, VME_DMAC_WAIT_FINISH);
  
  while (1) {

    vmeLibMemFrom16((u32)output, 0, 8, VME_DMAC_WAIT_FINISH);
    vmeDebugFillWith(VME_BASE_BUFFERS);
  
    //runContext();
  
    meCounter += 1;
  }
  
  vmeLibDisable();
}


#define setupSharedMemory() _setupSharedMemory(0)
#define cleanSharedMemory() _setupSharedMemory(1)

void _setupSharedMemory(bool clean) {
  
  static Uncached32 _input = {(volatile u32**)&input, nullptr};
  static Uncached32 _output = {&output, nullptr};

  if (clean) {
    
    meLibAllocUncached32(&_input, 0);
    meLibAllocUncached32(&_output, 0);
    return;
  }
  
  meLibAllocUncached32(&_input, TILE_SIZE);
  meLibAllocUncached32(&_output, TILE_SIZE);
}

int main() {
  
  setupSharedMemory();
  loadRGBA16("./tex64x64.png", (u16*)input);
  
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
      vmeDebugDisplayBuffer(VME_DEBUG_DIGIT_8, VME_DBG_IDX_BASE_0, x, y + 3);
      
        
      pspDebugScreenSetXY(1, 12);
      pspDebugScreenPrintf("0x%lx 0x%lx 0x%lx 0x%lx        ", output[0], output[1], output[2], output[3]);
    
      
      /*
      pspDebugScreenSetXY(x + 34, y + 2);
      pspDebugScreenPrintf("BASE_1:");
      vmeDebugDisplayBuffer(VME_DEBUG_DIGIT_4, VME_DBG_IDX_BASE_1, x + 34, y + 3);
      
      pspDebugScreenSetXY(x, y + 12);
      pspDebugScreenPrintf("BASE_2:");
      vmeDebugDisplayBuffer(VME_DEBUG_DIGIT_4, VME_DBG_IDX_BASE_2, x, y + 13);
      
      pspDebugScreenSetXY(x + 34, y + 12);
      pspDebugScreenPrintf("BASE_3:");
      vmeDebugDisplayBuffer(VME_DEBUG_DIGIT_4, VME_DBG_IDX_BASE_3, x + 34, y + 13);
      */
    }
  
    pspDebugScreenSetXY(1, 24);
    pspDebugScreenPrintf("meCounter: 0x%lx", meCounter);
    
    
    sceDisplayWaitVblank();
    
  } while (!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceKernelDelayThread(100000);
  
  vmeDebugTouch();
  vmeDebugDumpBuffers();
  vmeDebugFreeBuffers();
  
  cleanSharedMemory();
  
  sceKernelExitGame();
  return 0;
}
