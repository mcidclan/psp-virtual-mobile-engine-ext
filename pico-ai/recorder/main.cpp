/*
 * Copyright mcidclan, m-c/d 2026
 */
#include "main.h"

PSP_HEAP_SIZE_KB(-1024);
PSP_MODULE_INFO("pico-ai-recorder", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

#define SAMPLE_PERIOD 16667
#define MAX_RECORDS   8
#define MIN_LOCK      90
#define CONFIG_PATH   "./sample.txt"

typedef struct {
  int label;
  signed char samples[128];
} Record;

Record recordBuffer[MAX_RECORDS];
int recordCount    = 0;
int currentLabel   = 0;
int locked         = 0;
int capturing      = 0;
int currentFileIdx = 0;
int resetRequested = 0;

int readConfig() {
  
  int n = 0;
  SceUID fd = sceIoOpen(CONFIG_PATH, PSP_O_RDONLY, 0777);
  if (fd >= 0) {
    sceIoRead(fd, &n, sizeof(int));
    sceIoClose(fd);
  }
  return n;
}

void writeConfig(int n) {
  
  SceUID fd = sceIoOpen(CONFIG_PATH, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
  if (fd >= 0) {
    sceIoWrite(fd, &n, sizeof(int));
    sceIoClose(fd);
  }
}

int saveBuffer() {
  
  int idx = readConfig();
  char path[32];
  snprintf(path, sizeof(path), "./sample.%02d.bin", idx);
  SceUID fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
  
  if (fd < 0) {
    return 0;
  }
  
  for (int i = 0; i < recordCount; i++) {
    sceIoWrite(fd, &recordBuffer[i].label, sizeof(int));
    sceIoWrite(fd, recordBuffer[i].samples, 128);
  }
  sceIoClose(fd);
  writeConfig(idx + 1);
  return 1;
}

#define JOYSTICK_ADJUSTER(v) ((8 * (int)(v / 8)) - 128)

int recorder(SceSize args, void *argp) {
  
  int* const ended = (int*)*((int*)argp);
  sceCtrlSetSamplingCycle(0);
  sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

  SceCtrlData pad;
  u32 warmupStart = sceKernelGetSystemTimeLow();
  while (sceKernelGetSystemTimeLow() - warmupStart < 1000000) {
    sceCtrlPeekBufferPositive(&pad, 1);
    sceKernelDelayThread(500);
  }

  const int PRETRIG = 8;
  const int THRESHOLD = 20;
  int preHead = 0;
  signed char preX[8] = {0};
  signed char preY[8] = {0};
  
  int capIndex = 0;
  signed char samples[128] = {0};

  u32 lastTick = sceKernelGetSystemTimeLow();
  while (!*ended) {
    
    u32 now = sceKernelGetSystemTimeLow();
    if (now - lastTick < SAMPLE_PERIOD) {
      sceKernelDelayThread(500);
      continue;
    }
    lastTick = now;

    sceCtrlPeekBufferPositive(&pad, 1);
    signed char x = (signed char)JOYSTICK_ADJUSTER(pad.Lx);
    signed char y = (signed char)JOYSTICK_ADJUSTER(pad.Ly);

    if (!capturing) {
      
      if (!locked && recordCount < MAX_RECORDS) {
        
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
      
      if (resetRequested) {
        capturing = 0;
        capIndex = 0;
        preHead = 0;
        for (int i = 0; i < PRETRIG; i++) {
          preX[i] = 0; preY[i] = 0;
        }
        resetRequested = 0;
        continue;
      }
      
      samples[capIndex] = x;
      samples[capIndex + 64] = y;
      capIndex++;
      
      if (capIndex >= 64) {
        
        int slot = recordCount;
        recordBuffer[slot].label = currentLabel;
        for (int i = 0; i < 128; i++) {
          recordBuffer[slot].samples[i] = samples[i];
        }
        recordCount++;
        capturing = 0;
        locked = 1;
        
        u32 lockTick = sceKernelGetSystemTimeLow();
        int lockFrames = 0;
        while (lockFrames < MIN_LOCK && !*ended && !resetRequested) {
          
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
            lockFrames = 0;
          } else {
            lockFrames++;
          }
        }
        
        locked = 0;
        resetRequested = 0;
        preHead = 0;
        capIndex = 0;
        for (int i = 0; i < PRETRIG; i++) {
          preX[i] = 0; preY[i] = 0;
        }
        lastTick = sceKernelGetSystemTimeLow();
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
  pspDebugScreenInit();
  
  currentFileIdx = readConfig();

  int ended;
  int thid = startThread(recorder, &ended);

  u32 prev = 0;
  int warningTimer = 0;
  
  SceCtrlData ctl;
  do {
    pspDebugScreenSetTextColor(0xffffffff);

    sceCtrlPeekBufferPositive(&ctl, 1);
    u32 pressed = ctl.Buttons & ~prev;
    
    if (warningTimer > 0) {
      warningTimer--;
    }
    if (pressed & PSP_CTRL_LEFT) {
      currentLabel = (currentLabel + 3) % 4;
    }
    if (pressed & PSP_CTRL_RIGHT) {
      currentLabel = (currentLabel + 1) % 4;
    }
    if (pressed & PSP_CTRL_SQUARE) {
      recordCount = 0;
      locked = 0;
      resetRequested = 1;
    }
    if (pressed & PSP_CTRL_CIRCLE) {
      if (recordCount >= MAX_RECORDS) {
        saveBuffer();
        recordCount = 0;
        currentFileIdx++;
      } else {
        warningTimer = 120;
      }
    }

    prev = ctl.Buttons;

    pspDebugScreenSetXY(1, 1);
    pspDebugScreenPrintf("Pico AI Recorder");
    pspDebugScreenSetXY(1, 3);
    pspDebugScreenPrintf("Recording number: %d      ", currentFileIdx);
    pspDebugScreenSetXY(1, 4);
    pspDebugScreenPrintf("Label: combo%d      ", currentLabel + 1);
    pspDebugScreenSetXY(1, 5);
    pspDebugScreenPrintf("Records: %d / %d      ", recordCount, MAX_RECORDS);
    pspDebugScreenSetXY(1, 6);
    pspDebugScreenPrintf("Remaining: %d      ", MAX_RECORDS - recordCount);
    
    pspDebugScreenSetXY(1, 8);
    if (capturing) {
      pspDebugScreenSetTextColor(0xff0ff000);
      pspDebugScreenPrintf("* Recording...               ");
      pspDebugScreenSetTextColor(0xffffffff);
    }
    else if (locked) {
      pspDebugScreenSetTextColor(0xff0000ff);
      pspDebugScreenPrintf("Recording stopped, wait...   ");
      pspDebugScreenSetTextColor(0xffffffff);
    }
    else if (warningTimer > 0) {
      pspDebugScreenPrintf("Buffer not full yet!         ");
    }
    else if (recordCount >= MAX_RECORDS) {
      pspDebugScreenPrintf("Press O to save, [] to reset ");
    }
    else {
      pspDebugScreenPrintf("Move stick to record.          ");
    }
    
    pspDebugScreenSetXY(1, 10);
    pspDebugScreenPrintf("use < > to change label, [] to reset, HOME to exit");

    sceDisplayWaitVblank();

  } while (!(ctl.Buttons & PSP_CTRL_HOME));

  endThread(&ended, thid);
  sceKernelExitGame();
  return 0;
}
