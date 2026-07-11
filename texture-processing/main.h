#pragma once

#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
#include <pspgu.h>
#include <me-core-mapper/me-core.h>
#include <vme-ext.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#include <stb_image.h>

#define BUF_WIDTH     512
#define SCR_WIDTH     480
#define SCR_HEIGHT    272

#define DRAW_BUF_0 0
#define DRAW_BUF_1 0x88000
#define DEPTH_BUF  0x110000

struct Vertex {
  u16 u, v;
  u32 color;
  u16 x, y, z;
} __attribute__((aligned(4), packed));

static unsigned int __attribute__((aligned(4))) list[1024] = {0};

inline void guInit() {
  sceGuInit();
  sceGuStart(GU_DIRECT, list);
  sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)0x88000, BUF_WIDTH);
  sceGuDepthBuffer((void*)0x110000, BUF_WIDTH);
  sceGuClearColor(0xff100808);
  sceGuDisable(GU_SCISSOR_TEST);
  sceGuDisable(GU_DEPTH_TEST);
  sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
  sceGuTexMode(GU_PSM_5551, 0, 1, 0);
  sceGuTexWrap(GU_CLAMP, GU_CLAMP);
  sceGuEnable(GU_TEXTURE_2D);
  sceGuDisplay(GU_TRUE);
  sceGuFinish();
  sceGuSync(0,0);
}

inline int loadRGBA16(const char* const path, u16* const out) {
  
  int w, h;
  u8* tex = stbi_load(path, &w, &h, nullptr, STBI_rgb_alpha);
  if (!tex) {
    return -1;
  }

  for (int i = 0; i < w * h; i++) {

    u8 r = tex[i * 4 + 0];
    u8 g = tex[i * 4 + 1];
    u8 b = tex[i * 4 + 2];
    u8 a = tex[i * 4 + 3];
    const u16 rgba16 =
      (a >= 128 ? 0x8000 : 0) |
      ((b >> 3) << 10) |
      ((g >> 3) <<  5) |
      ( r >> 3);

    out[i] = rgba16;
  }

  stbi_image_free(tex);

  return 0;
}

inline u16 meValueAdditive(float t, float speed = 4.0f,
  int minVal = 0, int maxVal = 31) {
    
    float s = 0.5f * (1.0f + sinf(t * speed));
    u16 v = static_cast<u16>(minVal + s * (maxVal - minVal) + 0.5f);
    return v;
}
