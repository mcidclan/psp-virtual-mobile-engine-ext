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

#define TILE_COUNT (32*32)
#define TILE_SIZE (TILE_COUNT * 2)
#define TEXTURE_COUNT (64*64)
#define TEXTURE_SIZE (TEXTURE_COUNT * 2)
meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[1])

/*
void runContext() {
  
  vmeLibStart();
  //
  vmeLibFinish();
}
*/

  const u32 buffer[] = {

//    0x01, 0x02, 0x03, 0x04,
//    0x05, 0x06, 0x07, 0x08,
//    0x09, 0x0A, 0x0B, 0x0C,
//    0x0D, 0x0E, 0x0F, 0x10,
//    0x11, 0x12, 0x13, 0x14,
//    0x15, 0x16, 0x17, 0x18,
//    0x19, 0x1A, 0x1B, 0x1C,
//    0x1D, 0x1E, 0x1F, 0x20,
    
    // 0x00010203, 0x00040506, 0x00070809, 0x000a0b0c, 0x00d0e0f,

    0x01020301, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
    0x01020302, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
    0x01020303, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
    0x01020304, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
    0x01020305, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
    0x01020306, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
    0x01020307, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
    0x01020308, 0x05060708, 0x090a0b0c, 0x0c0d0e0f,
  };
  
void meLibOnProcess(void) {
  
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
  
  hw(TILE_SIZE*0 + (u32)input) = 1;
  hw(TILE_SIZE*1 + (u32)input) = 2;
  hw(TILE_SIZE*2 + (u32)input) = 3;
  hw(TILE_SIZE*3 + (u32)input) = 4;
  

  // tile 0
  vmeLibMemTo16(TILE_SIZE*0 + (u32)input,
    VME_TOP_BUFF0_WOFF, TILE_COUNT, VME_DMAC_TRANSFERT_WAIT_FINISH); // no wait
  // tile 1
  vmeLibMemTo16(TILE_SIZE*1 + (u32)input,
    VME_TOP_BUFF1_WOFF, TILE_COUNT, VME_DMAC_TRANSFERT_WAIT_FINISH); // no wait
  // tile 2
  vmeLibMemTo16(TILE_SIZE*2 + (u32)input,
    VME_TOP_BUFF2_WOFF, TILE_COUNT, VME_DMAC_TRANSFERT_WAIT_FINISH); // no wait
  // tile 3
  vmeLibMemTo16(TILE_SIZE*3 + (u32)input,
    VME_TOP_BUFF3_WOFF, TILE_COUNT, VME_DMAC_TRANSFERT_WAIT_FINISH); // wait

  vmeLibStart();
    
  // configure pe0 READ TOP and WRITE AGUs
  vme_pe0(agu_top(MODE), VME_DEF_MODE);
  vme_pe0(agu_top(COUNT), VME_DEF_STEP, TILE_COUNT);
  vme_pe0(agu_write(MODE), VME_DEF_MODE, VME_CYCLE_6);
  vme_pe0(agu_write(COUNT), VME_DEF_STEP, TILE_COUNT);

  // replicate pe0 AGUs configuration over other PEs
  vme_icn(AGU_TOP, 0);
  vme_icn(AGU_BASE, 0);
  vme_icn(AGU_WRITE, 0);

  {
    const u32 mux = 0x10000000; //vme_mux(TOP_0, TOP_0);
    vme_pe0(vme_fu(PRIMARY), mux, 0x00000000);
  }
  
//  {
//    const u32 mux = vme_mux(TOP_1, BASE_0);
//    vme_pe1(vme_fu(PRIMARY), mux, 0x00004000);
//  }
  
//  {
//    const u32 mux = vme_mux(TOP_2, BASE_0);
//    vme_pe2(vme_fu(PRIMARY), mux, 0x00004000);
//  }
  
//  {
//    const u32 mux = vme_mux(TOP_3, BASE_0);
//    vme_pe3(vme_fu(PRIMARY), mux, 0x00004000);
//  }
 
  vmeLibFinish();
  
  //vmeDebugFillWith(VME_BASE_BUFFERS);
  vmeDebugFillWith(VME_BASE_BUFFERS);

  while (1) {
    
    //vmeLibMemFrom16((u32)output, 0, TILE_COUNT, VME_DMAC_TRANSFERT_WAIT_FINISH);
  
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
