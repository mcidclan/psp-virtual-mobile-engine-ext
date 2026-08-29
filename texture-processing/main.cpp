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

PSP_MODULE_INFO("vme-tex", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();

volatile u16* input __attribute__((aligned(64))) = nullptr;
volatile u32* output __attribute__((aligned(64))) = nullptr;

#define TILE_COUNT (32*32)
#define TILE_SIZE (TILE_COUNT * 2)
#define TEXTURE_COUNT (64*64)
#define TEXTURE_SIZE (TEXTURE_COUNT * 2)

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[0])
#define meValue      (meLibSharedMemory[1])
#define meCanUpdate  (meLibSharedMemory[2])
  
void meLibOnProcess(void) {
  
  meCoreDcacheWritebackInvalidateAll();
  meLibExceptionHandlerInit(0);
  
  vmeLibEnable();
  vmeLibWipe();
  
  // tile 0
  vmeLibMemTo16(TILE_SIZE*0 + (u32)input,
    VME_TOP_BUFF0_WOFF, TILE_COUNT, VME_DMAC_TRANSFERT_NO_WAIT);
  // tile 1
  vmeLibMemTo16(TILE_SIZE*1 + (u32)input,
    VME_TOP_BUFF1_WOFF, TILE_COUNT, VME_DMAC_TRANSFERT_WAIT_FINISH);
  // tile 2
  vmeLibMemTo16(TILE_SIZE*2 + (u32)input,
    VME_TOP_BUFF2_WOFF, TILE_COUNT, VME_DMAC_TRANSFERT_NO_WAIT);
  // tile 3
  vmeLibMemTo16(TILE_SIZE*3 + (u32)input,
    VME_TOP_BUFF3_WOFF, TILE_COUNT, VME_DMAC_TRANSFERT_WAIT_FINISH);

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

  const u32 opcode = fu_op(OR);

  {
    const u32 mux = vme_mux(NONE, TOP_0);
    vme_pe0(vme_fu(PRIMARY), mux, opcode);
  }
  
  {
    const u32 mux = vme_mux(NONE, TOP_1);
    vme_pe1(vme_fu(PRIMARY), mux, opcode);
  }
  
  {
    const u32 mux = vme_mux(NONE, TOP_2);
    vme_pe2(vme_fu(PRIMARY), mux, opcode);
  }
  
  {
    const u32 mux = vme_mux(NONE, TOP_3);
    vme_pe3(vme_fu(PRIMARY), mux, opcode);
  }
 
  vmeLibFinish();

  while (1) {
    
    if (meCanUpdate) {
      
      // tile 0
      vmeLibMemFrom16(TILE_SIZE*0 + (u32)output,
        VME_BASE_BUFF0_WOFF, TILE_COUNT, VME_DMAC_TRANSFERT_NO_WAIT);
      // tile 1
      vmeLibMemFrom16(TILE_SIZE*1 + (u32)output,
        VME_BASE_BUFF1_WOFF, TILE_COUNT, VME_DMAC_TRANSFERT_WAIT_FINISH);
      // tile 2
      vmeLibMemFrom16(TILE_SIZE*2 + (u32)output,
        VME_BASE_BUFF2_WOFF, TILE_COUNT, VME_DMAC_TRANSFERT_NO_WAIT);
      // tile 3
      vmeLibMemFrom16(TILE_SIZE*3 + (u32)output,
        VME_BASE_BUFF3_WOFF, TILE_COUNT, VME_DMAC_TRANSFERT_WAIT_FINISH);
      
      vmeLibStart();
      vme_pe0(fu_reg(PRIMARY, B), meValue);
      vme_pe1(fu_reg(PRIMARY, B), meValue);
      vme_pe2(fu_reg(PRIMARY, B), meValue);
      vme_pe3(fu_reg(PRIMARY, B), meValue);
      vmeLibFinish();
      
      meCanUpdate = 0;
    }
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

#define IMG_POS_X 128
#define IMG_POS_Y 128

int main() {
  
  scePowerSetClockFrequency(333, 333, 166);
  setupSharedMemory();
  
  const int error = loadRGBA16("./mcid64x64.png", (u16*)input);
  if (error >= 0) {
  
    meLibDefaultInit();
    guInit();
    
    pspDebugScreenInitEx(0x0, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
    pspDebugScreenEnableBackColor(0);
    int buffer = DRAW_BUF_0;
    pspDebugScreenSetOffset(buffer);
    
    int dir = 1;
    int move = 0;
    
    float elapsed = 0.0f;
    u64 lastTime = sceKernelGetSystemTimeWide();

    SceCtrlData ctl;
    do {
      
      u64 now = sceKernelGetSystemTimeWide();
      float deltaTime = (now - lastTime) / 1000000.0f;
      elapsed += deltaTime;
      lastTime = now;
      
      sceCtrlPeekBufferPositive(&ctl, 1);

      sceGuStart(GU_DIRECT, list);
      sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
      sceGuTexImage(0, 64, 64, 64, (void*)output);
      
      {
        Vertex* const sprite = (Vertex*)sceGuGetMemory(sizeof(Vertex) * 2);
        move += dir;
        if(move > 128) {
          dir = -1;
        } else if(move < -128) {
          dir = 1;
        }
        
        sprite[0].u =
        sprite[0].v = 0;
        sprite[0].color = 0;
        sprite[0].x = 208 + move;
        sprite[0].y = 104;
        sprite[0].z = 0;
        
        
        sprite[1].u =
        sprite[1].v = 64;
        sprite[1].color = 0;
        sprite[1].x = 64 + 208 + move;
        sprite[1].y = 64 + 104;
        sprite[1].z = 0;
      
        sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
          GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, sprite);
      }
      
      const u32 offset = (buffer == DRAW_BUF_0) ? DRAW_BUF_1 : DRAW_BUF_0;
      pspDebugScreenSetOffset(offset);
      
      {
        pspDebugScreenSetXY(1, 1);
        pspDebugScreenPrintf("VME Texture Processing POC");

        pspDebugScreenSetXY(1, 2);
        pspDebugScreenPrintf("meCounter: 0x%lx", meCounter);
      }
      
      
      sceGuFinish();
      sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
      
      sceDisplayWaitVblankStart();
      buffer = (int)sceGuSwapBuffers();
      
      if (!meCanUpdate) {
        
        meValue = meValueAdditive(elapsed);
        meCanUpdate = 1;
      }

    } while (!(ctl.Buttons & PSP_CTRL_HOME));
    
    sceGuTerm();
  }
  else {
    pspDebugScreenInit();
    pspDebugScreenPrintf("Error while loading image, exit...");
  }
  
  sceKernelDelayThread(500000);
  cleanSharedMemory();
  sceKernelExitGame();
  return 0;
}
