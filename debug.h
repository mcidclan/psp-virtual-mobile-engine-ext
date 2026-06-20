#ifndef VME_DEBUG_H
#define VME_DEBUG_H

#include <cstdio>
#include <me-core-mapper/me-lib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VME_DEBUG_DIGIT_2 '2'
#define VME_DEBUG_DIGIT_4 '4'
#define VME_DEBUG_DIGIT_6 '6'
#define VME_DEBUG_DIGIT_8 '8'

#define VME_DBG_IDX_BASE_0 0
#define VME_DBG_IDX_BASE_1 1
#define VME_DBG_IDX_BASE_2 2
#define VME_DBG_IDX_BASE_3 3

#define VME_DBG_IDX_TOP_0 4
#define VME_DBG_IDX_TOP_1 5
#define VME_DBG_IDX_TOP_2 6
#define VME_DBG_IDX_TOP_3 7

#define VME_SCRATCHPAD_BUFFER_SIZE 8192
#define VME_DEBUG_SET_BUFFER_WORD_COUNT(count)                                 \
  static volatile const int VME_DEBUG_BUFFER_WORD_COUNT __attribute__((aligned(64))) = count;

static volatile u32* vmeDebugBuffers __attribute__((aligned(64))) = nullptr;

static inline void vmeDebugTouch() {
  
  FILE *f = fopen("./log.txt", "w");
  if (f) {
    fclose(f);
  }
}

static inline void vmeDebugDump(const char* const path,
  const int size, u32* const data, const char* const name) {
  
  FILE *f = fopen(path, "a");
  
  if (!f) {
    return;
  }
  
  fprintf(f, "Buffer: %s\n", name);
  
  for (int i = 0; i < size; i += 4) {
    
    fprintf(f, "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n",
      data[i+0], data[i+1], data[i+2], data[i+3]);
  }
  
  fprintf(f, "\n");
  fclose(f);
}

#define vmeDebugSetupBuffers()                                                 \
  meLibGetUncached32(vmeDebugBuffers, 8*VME_DEBUG_BUFFER_WORD_COUNT);

#define vmeDebugFreeBuffers()                                                  \
  meLibFreeUncached32(vmeDebugBuffers);

#define vmeDebugDumpBuffers() {                                                \
                                                                               \
  const int count = VME_DEBUG_BUFFER_WORD_COUNT;                               \
  vmeDebugDump("./log.txt", count, (u32*)&(vmeDebugBuffers[count*0]), "BASE 0"); \
  vmeDebugDump("./log.txt", count, (u32*)&(vmeDebugBuffers[count*1]), "BASE 1"); \
  vmeDebugDump("./log.txt", count, (u32*)&(vmeDebugBuffers[count*2]), "BASE 2"); \
  vmeDebugDump("./log.txt", count, (u32*)&(vmeDebugBuffers[count*3]), "BASE 3"); \
}

#define vmeDebugDisplayBuffer(digit, index, x, y) {                            \
                                                                               \
  char format[36] = "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,";                     \
  format[4] = format[13] = format[22] = format[31] = digit;                    \
  const int count = VME_DEBUG_BUFFER_WORD_COUNT;                               \
  u32* const data = (u32*)&(vmeDebugBuffers[count * index]);                   \
  for (int i = 0; i < count; i += 4) {                                         \
    pspDebugScreenSetXY(x, y+i/4);                                             \
    pspDebugScreenPrintf(format, data[i+0], data[i+1], data[i+2], data[i+3]);  \
  }                                                                            \
}

#define vmeDebugFillWith(buffer) {                                             \
                                                                               \
  const int count = VME_DEBUG_BUFFER_WORD_COUNT;                               \
  const int byteCount = count*4;                                               \
  meCoreMemcpy((void*)&(vmeDebugBuffers[count*0]), (void*)(buffer + VME_SCRATCHPAD_BUFFER_SIZE*0), byteCount); \
  meCoreMemcpy((void*)&(vmeDebugBuffers[count*1]), (void*)(buffer + VME_SCRATCHPAD_BUFFER_SIZE*1), byteCount); \
  meCoreMemcpy((void*)&(vmeDebugBuffers[count*2]), (void*)(buffer + VME_SCRATCHPAD_BUFFER_SIZE*2), byteCount); \
  meCoreMemcpy((void*)&(vmeDebugBuffers[count*3]), (void*)(buffer + VME_SCRATCHPAD_BUFFER_SIZE*3), byteCount); \
}

#define vmeDebugDumpBuffer(index, name) {                                      \
                                                                               \
  const int count = VME_DEBUG_BUFFER_WORD_COUNT;                               \
  vmeDebugDump("./log.txt", count, (u32*)&(vmeDebugBuffers[count*index]), name); \
}

#define vmeDebugCopy(index) {                                                  \
                                                                               \
  const int count = VME_DEBUG_BUFFER_WORD_COUNT;                               \
  const int byteCount = count*4;                                               \
  u32 buffer = VME_BASE_BUFFER_0;                                              \
  if (index >= 4) {                                                            \
    buffer = VME_TOP_BUFFER_0;                                                 \
  }                                                                            \
  meCoreMemcpy((void*)&(vmeDebugBuffers[count*index]),                         \
    (void*)(buffer + VME_SCRATCHPAD_BUFFER_SIZE*index), byteCount);            \
}

#ifdef __cplusplus
}
#endif

#endif
