#ifndef __GR_ROUTE_HPP__
#define __GR_ROUTE_HPP__

#include <cmath>
#include <vector>

struct GRSegment {
  int init_x;
  int init_y;
  int init_layer;
  int final_x;
  int final_y;
  int final_layer;
  GRSegment() = default;
  GRSegment(int x0, int y0, int l0, int x1, int y1, int l1) {
    init_x = std::min(x0, x1);
    init_y = std::min(y0, y1);
    init_layer = l0;
    final_x = std::max(x0, x1);
    final_y = std::max(y0, y1);
    final_layer = l1;
  }
  bool isVia() const { return (init_x == final_x && init_y == final_y); }
  int length() {
    return std::abs(init_x - final_x) + std::abs(init_y - final_y);
  }
  bool operator==(const GRSegment &segment) const {
    return init_layer == segment.init_layer &&
           final_layer == segment.final_layer && init_x == segment.init_x &&
           init_y == segment.init_y && final_x == segment.final_x &&
           final_y == segment.final_y;
  }
};

using GRRoute = std::vector<GRSegment>;

#endif
