#include "viewport.h"

#include "CMU462.h"

namespace CMU462 {

    void ViewportImp::set_viewbox(float centerX, float centerY, float vspan) {

      // Task 5 (part 2):
      // Set svg coordinate to normalized device coordinate transformation. Your input
      // arguments are defined as normalized SVG canvas coordinates.
      this->centerX = centerX;
      this->centerY = centerY;
      this->vspan = vspan;
      Matrix3x3 A;
      A(0, 0) = 1.;
      A(0, 1) = 0.;
      A(0, 2) = -centerX + vspan;
      A(1, 0) = 0.;
      A(1, 1) = 1.;
      A(1, 2) = -centerY + vspan;
      A(2, 0) = 0.;
      A(2, 1) = 0.;
      A(2, 2) = 2 * vspan;
      this->set_svg_2_norm(A);
    }

    void ViewportImp::update_viewbox(float dx, float dy, float scale) {

      this->centerX -= dx;
      this->centerY -= dy;
      this->vspan *= scale;
      set_viewbox(centerX, centerY, vspan);
    }

} // namespace CMU462
