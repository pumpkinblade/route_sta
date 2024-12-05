#ifndef __GR_PIN_HPP__
#define __GR_PIN_HPP__

#include "GRTree.hpp"
#include <string>

class GRInstance;
class GRNet;

class GRPin {
public:
  GRPin(const std::string &name) : m_name(name) {}

  // network info set
  void setInstance(GRInstance *inst) { m_instance = inst; }
  void setNet(GRNet *n) { m_net = n; }
  // network info get
  std::string name() const { return m_name; }
  GRInstance *instance() const { return m_instance; }
  GRNet *net() const { return m_net; }

  // geo info set
  void setAccessIndex(size_t idx) { m_access_idx = idx; }
  void setAccessPoints(const std::vector<GRPoint> &pts) {
    m_access_points = pts;
  }
  void setBoxexOnLayerDbu(const std::vector<utils::BoxOnLayerT<int>> &boxes) {
    m_boxes_on_layer_dbu = boxes;
  }

  // get info get
  size_t accessIndex() const { return m_access_idx; }
  const GRPoint &accessPoint() const { return m_access_points[m_access_idx]; }
  const std::vector<GRPoint> &accessPoints() const { return m_access_points; }
  const auto &boxesOnLayerDbu() const { return m_boxes_on_layer_dbu; }
  GRPoint positionDbu(const GRTechnology *tech) const;

private:
  // network info
  std::string m_name;
  GRInstance *m_instance;
  GRNet *m_net;

  // geo info
  size_t m_access_idx;
  std::vector<GRPoint> m_access_points;
  std::vector<utils::BoxOnLayerT<int>> m_boxes_on_layer_dbu;
};

#endif
