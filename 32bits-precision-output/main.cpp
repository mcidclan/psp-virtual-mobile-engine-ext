/*
 * Copyright mcidclan, m-c/d 2026
 */
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
#include <me-core-mapper/me-core.h>
#include <me-core-mapper/vme-fu-opcodes.h>

PSP_MODULE_INFO("vme-acc-32", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

ME_LIB_SETUP_SIMPLE_SUSPEND_HANDLER();

meLibSetSharedUncached32(10);
#define meCounter    (meLibSharedMemory[1])

volatile u32 __attribute__((aligned(64))) back[16] = {
  
  0x012345, 0xFDCBAA, 0xFCBA99, 0x045678,
  0x056789, 0x06789A, 0xF87655, 0xF76544,
  0x09ABCD, 0xF54322, 0x0BCDEF, 0xF32110,
  0xF210FF, 0x0EF012, 0x0F0123, 0xEFFDCC,
};

volatile u32 __attribute__((aligned(64))) front[16] = {
  
  0x000007, 0xFFFFF4, 0x000005, 0xFFFFEC,
  0xFFFFF1, 0x000003, 0x000012, 0xFFFFF7,
  0xFFFFFC, 0x000011, 0x00000B, 0xFFFFFA,
  0x00000E, 0x000008, 0xFFFFFE, 0xFFFFED,
};

/*
// Expected Output
0x0007F6E3, 0x00226AEB, 0x00120FE8, 0xFFBB4E88,
0xFF6A3D81, 0xFF7DA74F, 0xFEF5F949, 0xFF4369E5,
0xFF1CBAB1, 0xFE662FF3, 0xFEE80938, 0xFF3542D8,
0xFE7230CA, 0xFEE9B15A, 0xFECBAF14, 0xFFFBD8F0,
*/

volatile u32 __attribute__((aligned(64))) sharedRes[16] = {0};

/*
 * 32-bit MAC output split into two 16-bit lanes with saturation
 */
VME_LIB_CONTEXT_BUILDER(setupMAC32, param, {
  
  vme_icn(AGU_TOP, 0);
  vme_icn(AGU_BASE, 0);
  vme_icn(AGU_WRITE, 0x210);

  vme_set(ENABLE, FU_1, 8 << 28);

  const int step = 2;
  const int OP_MAC = 0x240;
  const int count = 16 * step;

  vme_pe0(vme_fu(PRIMARY), vme_mux(TOP_0, TOP_1), fu_code(OP_MAC));
  vme_pe0(vme_fu(SECONDARY), vme_mux(NONE, STAGING_0), fu_code(OP_AND_CST));
  vme_pe0(fu_reg(SECONDARY, B), 0xffff);

  const int lostCycles = 12;
  vme_pe0(agu_top(MODE), agu_mode(4));
  vme_pe0(agu_top(COUNT), agu_step(1), count - 1);
  vme_pe0(agu_write(MODE), agu_mode(4), vme_cyc(3));
  vme_pe0(agu_write(COUNT), agu_step(step), (count - step) + lostCycles);

  vme_pe1(vme_fu(PRIMARY), vme_mux(TOP_0, TOP_1), fu_code(OP_MAC), 16, fu_sat(16));
  vme_pe1(agu_write(MODE), agu_mode(4), vme_cyc(0), 1);
  vme_pe1(agu_write(COUNT), agu_step(step), (count - step) + lostCycles);

  vme_pe0(agu_base(MODE), agu_mode(4));
  vme_pe0(agu_base(COUNT), agu_step(1), (count - 1) + lostCycles);

  vme_pe2(vme_fu(PRIMARY), vme_mux(BASE_0, BASE_1), fu_code(OP_OR));
  vme_pe2(agu_write(MODE), agu_mode(4), vme_cyc(18));
  vme_pe2(agu_write(COUNT), agu_step(1), count - 1);

});

void meLibOnProcess(void) {

  meLibExceptionHandlerInit(0);
  
  void* const MAC32Ctx = setupMAC32(nullptr);
  
  vmeLibEnable();
  vmeLibWipe();

  vmeLibSendCustomContext(MAC32Ctx);
  vmeLibMemoryToRingBuffer((void*)front, VME_TOP_BUFF0_WOFF, sizeof(front) / 4);
  vmeLibMemoryToRingBuffer((void*)back, VME_TOP_BUFF1_WOFF, sizeof(back) / 4);
  
  vmeLibDisable();

  while (1) {

    vmeLibEnable();
    vmeLibTrigger();
    meCoreDMACPrimWaitVMEFinish();
    vmeLibDisable();

    if (meCoreHwMutexTryLock() >= 0) {

      meCoreBusClockEnableDMACPrimMux();
      vmeLibMemFrom16((u32)sharedRes, VME_BASE_BUFF2_WOFF,
        sizeof(sharedRes), VME_DMAC_TRANSFERT_WAIT_FINISH);
      meCoreBusClockDisableDMACPrimMux();
      meCoreHwMutexUnlock();
    }
    
    meLibDelayPipeline();
    meCounter += 1;
  }
}
  
void displayData() {
  
  while (meLibCallHwMutexTryLock() < 0) {
    sceKernelDelayThread(1);
  }
  
  const u32* const out = (u32*)(0x40000000 | (u32)sharedRes);

  pspDebugScreenSetXY(1, 2);
  pspDebugScreenPrintf("VME Output Vectors:");
  pspDebugScreenSetXY(1, 3);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx", out[0], out[1], out[2], out[3]);
  pspDebugScreenSetXY(1, 4);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx", out[4], out[5], out[6], out[7]);
  pspDebugScreenSetXY(1, 5);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx", out[8], out[9], out[10], out[11]);
  pspDebugScreenSetXY(1, 6);
  pspDebugScreenPrintf("%08lx, %08lx, %08lx, %08lx", out[12], out[13], out[14], out[15]);
  
  meLibCallHwMutexUnlock();
}

int main() {
  
  meLibDefaultInit();
  pspDebugScreenInit();
  
  SceCtrlData ctl;
  do {
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    
    pspDebugScreenSetXY(1, 0);
    pspDebugScreenPrintf("ME Counter: 0x%08lx", meCounter);
    displayData();
    
    sceDisplayWaitVblank();
    
  } while (!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceKernelDelayThread(100000);
  
  sceKernelExitGame();
  return 0;
}
