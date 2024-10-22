#ifndef __GR_LAYER_HPP__
#define __GR_LAYER_HPP__

#include "../util/common.hpp"
#include <string>

class GRLayer {
public:
  static constexpr int H = 0;
  static constexpr int V = 1;

  GRLayer(const std::string &name) : m_name(name) {}

  void setDirection(LayerDirection dir) { m_direction = dir; }
  void setRes(float r) { m_res = r; }
  void setCap(float c) { m_cap = c; }

  LayerDirection direction() const { return m_direction; }
  float res() const { return m_res; }
  float cap() const { return m_cap; }

private:
  std::string m_name;
  LayerDirection m_direction;
  float m_res; // Ohm / um
  float m_cap; // pF / um
};

class GRCutLayer {
public:
  GRCutLayer(const std::string &name) : m_name(name) {}

  void setRes(float r) { m_res = r; }

  float res() const { return m_res; }

private:
  std::string m_name;
  float m_res; // Ohm
};

#endif
