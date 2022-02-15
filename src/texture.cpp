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
      assert (idx < mip.width * mip.height);
      uint8_to_float(&c.r, &mip.texels[4 * idx]);
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
      for (size_t i = 1; i < tex.mipmap.size(); ++i) {
        MipLevel &curr_mip = tex.mipmap[i];
        MipLevel &prev_mip = tex.mipmap[i - 1];
        for (int j = 0; j < curr_mip.height; j++) {
          for (int k = 0; k < curr_mip.width; k++) {
            Color c;
            c += color_at_tex(prev_mip, 2 * k, 2 * j);
            c += color_at_tex(prev_mip, 2 * k + 1, 2 * j);
            c += color_at_tex(prev_mip, 2 * k, 2 * j + 1);
            c += color_at_tex(prev_mip, 2 * k + 1, 2 * j + 1);
            c *= 0.25;
            int idx = j * curr_mip.width + k;
            float_to_uint8(&curr_mip.texels[4 * idx], &c.r);
          }
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
      float level = log2f(max(tex.width / u_scale, tex.height / v_scale));
      if (level <= 0.) return sample_bilinear(tex, u, v, 0);
      if (level >= tex.mipmap.size() - 1) return sample_bilinear(tex, u, v,
                                                                 tex.mipmap.size() -
                                                                 1);
      int level_low = int(level);
      int level_high = level_low + 1;
      return (level - level_low) * sample_bilinear(tex, u, v, level_high) +
             (level_high - level) * sample_bilinear(tex, u, v, level_low);
    }

} // namespace CMU462
