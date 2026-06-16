#include <cstdio>
#include <me-core-mapper/me-lib.h>

static volatile u32* vmeDebugBuffers __attribute__((aligned(64))) = nullptr;
static u32 vmeDebugBufferSize = 0;

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


#define vmeDebugSetupBuffers(size)    \
  vmeDebugBufferSize = size;          \
  meLibGetUncached32(vmeDebugBuffers, 4*vmeDebugBufferSize);

#define vmeDebugFreeBuffers()         \
  meLibFreeUncached32(vmeDebugBuffers);

static inline void vmeDebugDumpBuffers() {
  
  const int size = vmeDebugBufferSize;
  vmeDebugDump("./log.txt", size, (u32*)&(vmeDebugBuffers[size*0]), "BASE 0");
  vmeDebugDump("./log.txt", size, (u32*)&(vmeDebugBuffers[size*1]), "BASE 1");
  vmeDebugDump("./log.txt", size, (u32*)&(vmeDebugBuffers[size*2]), "BASE 2");
  vmeDebugDump("./log.txt", size, (u32*)&(vmeDebugBuffers[size*3]), "BASE 3");
}

static inline void vmeDebugFillWith(const u32 buffer, const int count) {
  
  const int byteCount = count*4;
  meCoreMemcpy((void*)&(vmeDebugBuffers[count*0]), (void*)(buffer + 8192*0), byteCount);
  meCoreMemcpy((void*)&(vmeDebugBuffers[count*1]), (void*)(buffer + 8192*1), byteCount);
  meCoreMemcpy((void*)&(vmeDebugBuffers[count*2]), (void*)(buffer + 8192*2), byteCount);
  meCoreMemcpy((void*)&(vmeDebugBuffers[count*3]), (void*)(buffer + 8192*3), byteCount);
}
