#include "GRPin.hpp"
#include "GRTechnology.hpp"

GRPoint GRPin::positionDbu(const GRTechnology *tech) const {
  const auto &ap = m_access_points[m_access_idx];
  utils::BoxOnLayerT<int> box1 = tech->gcellToBox(ap);
  int64_t num_x = 0, num_y = 0, denom = 0;
  for (const auto &box2 : m_boxes_on_layer_dbu) {
    if (box2.layerIdx == box1.layerIdx) {
      auto box = box1.IntersectWith(box2);
      if (box.IsStrictValid()) {
        denom += static_cast<int64_t>(box.area());
        num_x += static_cast<int64_t>(box.area()) * box.cx();
        num_y += static_cast<int64_t>(box.area()) * box.cy();
        // printf("num_x = %zd, num_y = %zd, denom = %zd\n", num_x, num_y,
        // denom);
      }
    }
  }
  GRPoint p(ap.layerIdx, num_x / denom, num_y / denom);
  // std::printf("[%d %d] x [%d %d]\n", box1.lx(), box1.hx(), box1.ly(),
  //             box1.hy());
  // std::printf("[%d %d]\n", p.x, p.y);
  // exit(0);
  return p;
}
