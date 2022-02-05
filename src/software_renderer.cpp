#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CMU462 {


// Implements SoftwareRenderer //

void SoftwareRendererImp::draw_svg( SVG& svg ) {

  // set top level transformation
  transformation = svg_2_screen;

  // draw all elements
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y--;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y--;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y++;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y++;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  resolve();

}

void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {

  // Task 4: 
  // You may want to modify this for supersampling support
  /**
   * this function is called whenever the user changes the screen sampling rate
   */
  this->sample_rate = sample_rate;
  w = sample_rate * target_w;
  h = sample_rate * target_h;
  sample_buffer.resize(w * h * 4);  // width * height * num_channels
  std::fill(sample_buffer.begin(), sample_buffer.end(), 255);
}

void SoftwareRendererImp::set_render_target( unsigned char* render_target,
                                             size_t width, size_t height ) {

  // Task 4: 
  // You may want to modify this for supersampling support
  /** this function is called whenever the user resizes the application window,
   * we need to adjust target_w, target_h, w, h, sample_buffer accordingly
   */
  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;

  w = sample_rate * target_w;
  h = sample_rate * target_h;
  sample_buffer.resize(w * h * 4);  // width * height * num_channels
  std::fill(sample_buffer.begin(), sample_buffer.end(), 255);
}

void SoftwareRendererImp::draw_element( SVGElement* element ) {

  // Task 5 (part 1):
  // Modify this to implement the transformation stack

  switch(element->type) {
    case POINT:
      draw_point(static_cast<Point&>(*element));
      break;
    case LINE:
      draw_line(static_cast<Line&>(*element));
      break;
    case POLYLINE:
      draw_polyline(static_cast<Polyline&>(*element));
      break;
    case RECT:
      draw_rect(static_cast<Rect&>(*element));
      break;
    case POLYGON:
      draw_polygon(static_cast<Polygon&>(*element));
      break;
    case ELLIPSE:
      draw_ellipse(static_cast<Ellipse&>(*element));
      break;
    case IMAGE:
      draw_image(static_cast<Image&>(*element));
      break;
    case GROUP:
      draw_group(static_cast<Group&>(*element));
      break;
    default:
      break;
  }

}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Extra credit 

}

void SoftwareRendererImp::draw_image( Image& image ) {

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color ) {

  // fill in the nearest pixel
  int sx = (int) floor(x);
  int sy = (int) floor(y);

  // check bounds
  if ( sx < 0 || sx >= target_w ) return;
  if ( sy < 0 || sy >= target_h ) return;

  fill_pixel(sx, sy, color);
  // fill sample - NOT doing alpha blending!
//  render_target[4 * (sx + sy * target_w)    ] = (uint8_t) (color.r * 255);
//  render_target[4 * (sx + sy * target_w) + 1] = (uint8_t) (color.g * 255);
//  render_target[4 * (sx + sy * target_w) + 2] = (uint8_t) (color.b * 255);
//  render_target[4 * (sx + sy * target_w) + 3] = (uint8_t) (color.a * 255);

}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color) {
    // Task 2:
  // Implement line rasterization
  int sx0 = (int) floor(x0);
  int sy0 = (int) floor(y0);
  int sx1 = (int) floor(x1);
  int sy1 = (int) floor(y1);
  int dx = sx1 - sx0;
  int dy = sy1 - sy0;
  int x, y;

  if (dy == 0) {  // horizontal
      if (sx0 > sx1) {
          swap(sx0, sx1);
      }
      for (x = sx0; x <= sx1; x++) {
          rasterize_point(x, sy0, color);
      }
  } else if (dx == 0) {  // vertical
      if (sy0 > sy1) {
          swap(sy0, sy1);
      }
      for (y = sy0; y <= sy1; y++) {
          rasterize_point(sx0, y, color);
      }
  }
  else {
      int error = 0;
      if (abs(dy) <= abs(dx)) {  // acute angle
          if (dx < 0) {  // point1 always to the right of point0
              swap(sx0, sx1);
              swap(sy0, sy1);
              dx = -dx;
              dy = -dy;
          }
          y = sy0;
          bool pos_tan = dy > 0;
          dy = abs(dy);
          for (x = sx0; x <= sx1; x++) {
              rasterize_point(x, y, color);
              error += dy;
              if ((error << 1) >= dx) {
                  if (pos_tan) {
                      y++;
                  } else {
                      y--;
                  }
                  error -= dx;
              }
          }
      }
      else {
          if (dy < 0) {
              swap(sx0, sx1);
              swap(sy0, sy1);
              dx = -dx;
              dy = -dy;
          }
          x = sx0;
          bool pos_tan = dx > 0;
          dx = abs(dx);
          for (y = sy0; y <= sy1; y++) {
              rasterize_point(x, y, color);
              error += dx;
              if ((error << 1) >= dy) {
                  if (pos_tan) {
                      x++;
                  } else {
                      x--;
                  }
                  error -= dy;
              }
          }
      }
  }


}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 3: 
  // Implement triangle rasterization
  float xmin = floor(min({x0, x1, x2}));
  float xmax = ceil(max({x0, x1, x2}));
  float ymin = floor(min({y0, y1, y2}));
  float ymax = ceil(max({y0, y1, y2}));
  Vector2D ab(x1 - x0, y1 - y0);
  Vector2D bc(x2 - x1, y2 - y1);
  Vector2D ca(x0 - x2, y0 - y2);

  double step_size = 1.0 / sample_rate;

  for (float x = xmin; x <= xmax; x += step_size) {
      bool flag = false;
      for (float y = ymin; y <= ymax; y += step_size) {
        float center_x = x + 1 / (2 * sample_rate);
        float center_y = y + 1 / (2 * sample_rate);
        Vector2D ap(center_x - x0, center_y - y0);
        Vector2D bp(center_x - x1, center_y - y1);
        Vector2D cp(center_x - x2, center_y - y2);
        double cross_a = cross(ab, ap);
        double cross_b = cross(bc, bp);
        double cross_c = cross(ca, cp);
        bool lit = (cross_a * cross_b) >= 0 && (cross_c * cross_b) >= 0;
        if (lit) {
          fill_sample(x * sample_rate, y * sample_rate, color);
          if (!flag) {
            flag = true;
          }
        } else {
          if (flag) break;  // early out
        }
      }
  }

}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task 6: 
  // Implement image rasterization

}

// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {

  // Task 4: 
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 4".
  memset(render_target, 0, 4 * target_w * target_h);
  for (int i = 0; i < target_w; i++) {
    for (int j = 0; j < target_h; j++) {
      int target_start_idx = 4 * (target_w * j + i);
      for (int dx = 0; dx < sample_rate; dx++ ) {
        for (int dy = 0; dy < sample_rate; dy++) {
          int x = sample_rate * i + dx;
          int y = sample_rate * j + dy;
          render_target[target_start_idx] += sample_buffer[4 * (w * y + x)];
          render_target[target_start_idx + 1] += sample_buffer[4 * (w * y + x) + 1];
          render_target[target_start_idx + 2] += sample_buffer[4 * (w * y + x) + 2];
          render_target[target_start_idx + 3] += sample_buffer[4 * (w * y + x) + 3];
        }
      }
      render_target[target_start_idx] /= (sample_rate * sample_rate);
      render_target[target_start_idx + 1] /= (sample_rate * sample_rate);
      render_target[target_start_idx + 2] /= (sample_rate * sample_rate);
      render_target[target_start_idx + 3] /= (sample_rate * sample_rate);
    }
  }
}

void SoftwareRendererImp::fill_sample( int sx, int sy, const Color& c ) {

  sample_buffer[4 * (sx + sy * w)    ] = (uint8_t) (c.r * 255);
  sample_buffer[4 * (sx + sy * w) + 1] = (uint8_t) (c.g * 255);
  sample_buffer[4 * (sx + sy * w) + 2] = (uint8_t) (c.b * 255);
  sample_buffer[4 * (sx + sy * w) + 3] = (uint8_t) (c.a * 255);
}

void SoftwareRendererImp::fill_pixel( int x, int y, const Color& c ) {
  int i = x * sample_rate;
  int j = y * sample_rate;
  int x_end = i + sample_rate;
  int y_end = j + sample_rate;
  for (; i < x_end; i++) {
    for (; j < y_end; j++) {
      fill_sample(i, j, c);
    }
  }
}

} // namespace CMU462
