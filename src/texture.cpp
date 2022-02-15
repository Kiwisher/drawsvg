#include "texture.h"
#include "color.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CMU462 {

    inline void uint8_to_float(float dst[4], unsigned char *src) {
      uint8_t *src_uint8 = (uint8_t *) src;
      dst[0] = src_uint8[0] / 255.f;
      dst[1] = src_uint8[1] / 255.f;
      dst[2] = src_uint8[2] / 255.f;
      dst[3] = src_uint8[3] / 255.f;
    }

    inline void float_to_uint8(unsigned char *dst, float src[4]) {
      uint8_t *dst_uint8 = (uint8_t *) dst;
      dst_uint8[0] = (uint8_t) (255.f * max(0.0f, min(1.0f, src[0])));
      dst_uint8[1] = (uint8_t) (255.f * max(0.0f, min(1.0f, src[1])));
      dst_uint8[2] = (uint8_t) (255.f * max(0.0f, min(1.0f, src[2])));
      dst_uint8[3] = (uint8_t) (255.f * max(0.0f, min(1.0f, src[3])));
    }

    inline Color color_at_tex(MipLevel &mip, int x, int y) {
      Color c;
      int width = mip.width;
      int idx = x + y * width;
      uint8_to_float(&c.r, &mip.texels[4 * idx]);
      uint8_to_float(&c.g, &mip.texels[4 * idx + 1]);
      uint8_to_float(&c.b, &mip.texels[4 * idx + 2]);
      uint8_to_float(&c.a, &mip.texels[4 * idx + 3]);
      return c;
    }

    void Sampler2DImp::generate_mips(Texture &tex, int startLevel) {

      // NOTE:
      // This starter code allocates the mip levels and generates a level
      // map by filling each level with placeholder data in the form of a
      // color that differs from its neighbours'. You should instead fill
      // with the correct data!

      // Task 7: Implement this

      // check start level
      if (startLevel >= tex.mipmap.size()) {
        std::cerr << "Invalid start level";
      }

      // allocate sublevels
      int baseWidth = tex.mipmap[startLevel].width;
      int baseHeight = tex.mipmap[startLevel].height;
      int numSubLevels = (int) (log2f((float) max(baseWidth, baseHeight)));

      numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
      tex.mipmap.resize(startLevel + numSubLevels + 1);

      int width = baseWidth;
      int height = baseHeight;
      for (int i = 1; i <= numSubLevels; i++) {

        MipLevel &level = tex.mipmap[startLevel + i];

        // handle odd size texture by rounding down
        width = max(1, width / 2);
        assert(width > 0);
        height = max(1, height / 2);
        assert(height > 0);

        level.width = width;
        level.height = height;
        level.texels = vector<unsigned char>(4 * width * height);

      }

      // fill all 0 sub levels with interchanging colors (JUST AS A PLACEHOLDER)
      Color colors[3] = {Color(1, 0, 0, 1), Color(0, 1, 0, 1),
                         Color(0, 0, 1, 1)};
      for (size_t i = 1; i < tex.mipmap.size(); ++i) {

        Color c = colors[i % 3];
        MipLevel &mip = tex.mipmap[i];

        for (size_t i = 0; i < 4 * mip.width * mip.height; i += 4) {
          float_to_uint8(&mip.texels[i], &c.r);
        }
      }

    }

    Color Sampler2DImp::sample_nearest(Texture &tex,
                                       float u, float v,
                                       int level) {

      // Task 6: Implement nearest neighbour interpolation

      // return magenta for invalid level
      if (u < 0.f || u > 1.f || v < 0.f || v > 1.f) {
        return Color(1, 0, 1, 1);
      }
      Color c;
      MipLevel &mip = tex.mipmap[level];
      int width = mip.width;
      int height = mip.height;
      return color_at_tex(mip, floor(u * width), floor(v * height));
    }

    Color Sampler2DImp::sample_bilinear(Texture &tex,
                                        float u, float v,
                                        int level) {

      // Task 6: Implement bilinear filtering

      // return magenta for invalid level
      if (u < 0.f || u > 1.f || v < 0.f || v > 1.f) {
        return Color(1, 0, 1, 1);
      }
      MipLevel &mip = tex.mipmap[level];
      int width = mip.width;
      int height = mip.height;
      float x = width * u;
      float y = height * v;
      int x0, x1, y0, y1;
      x0 = max(0, int(floor(x - 0.5)));
      x1 = min(width - 1, int(floor(x + 0.5)));
      y0 = max(0, int(floor(y - 0.5)));
      y1 = min(height - 1, int(floor(y + 0.5)));
      Color c00 = color_at_tex(mip, x0, y0);
      Color c01 = color_at_tex(mip, x0, y1);
      Color c10 = color_at_tex(mip, x1, y0);
      Color c11 = color_at_tex(mip, x1, y1);
      float s = max(x - x0 - 0.5, 0.);
      float t = max(y - y0 - 0.5, 0.);
      return (1 - t) * ((1 - s) * c00 + s * c10) +
             t * ((1 - s) * c01 + s * c11);
    }

    Color Sampler2DImp::sample_trilinear(Texture &tex,
                                         float u, float v,
                                         float u_scale, float v_scale) {

      // Task 7: Implement trilinear filtering

      // return magenta for invalid level
      return Color(1, 0, 1, 1);

    }

} // namespace CMU462
