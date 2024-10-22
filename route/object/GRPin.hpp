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
  void setPosition(const GRPoint &pos) { m_pos = pos; }
  void setAccessPoints(const std::vector<GRPoint> &pts) {
    m_access_points = pts;
  }

  // get info get
  GRPoint position() const { return m_pos; }
  const std::vector<GRPoint> &accessPoints() const { return m_access_points; }

private:
  // network info
  std::string m_name;
  GRInstance *m_instance;
  GRNet *m_net;

  // geo info
  GRPoint m_pos;
  std::vector<GRPoint> m_access_points;
};

#endif
