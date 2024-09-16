#ifndef __GR_INSTANCE_HPP__
#define __GR_INSTANCE_HPP__

#include "GRPin.hpp"
#include <vector>

class GRInstance {
public:
  GRInstance(const std::string &name) : m_name(name) {}

  void addPin(GRPin *pin) { m_pins.push_back(pin); }

  void setLibcellName(const std::string &libcell_name) {
    m_libcell_name = libcell_name;
  }

  const std::string &getName() const { return m_name; }
  const std::string &getLibcellName() const { return m_libcell_name; }
  const std::vector<GRPin *> &getPins() const { return m_pins; }

private:
  std::string m_name;
  std::string m_libcell_name;
  std::vector<GRPin *> m_pins;
  // int m_x_dbu, m_y_dbu;
  // int m_w_dbu, m_h_dbu;
};

#endif
