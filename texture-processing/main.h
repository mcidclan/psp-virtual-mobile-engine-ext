#pragma once

#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
#include <me-core-mapper/me-core.h>
#include <vme-ext.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#include <stb_image.h>

#include "debug.h"


void loadRGBA16(const char* const path, u16* const out) {

  int w, h;
  u8* tex = stbi_load(path, &w, &h, nullptr, STBI_rgb_alpha);

  for (int i = 0; i < w * h; i++) {
    
    u8 r = tex[i * 4 + 0];
    u8 g = tex[i * 4 + 1];
    u8 b = tex[i * 4 + 2];
    u8 a = tex[i * 4 + 3];

    const u16 rgba16 =
        ((r >> 3) << 11) |
        ((g >> 3) <<  6) |
        ((b >> 3) <<  1) |
        (a >= 128);

    out[i] = rgba16;
  }
  
  stbi_image_free(tex);
}
