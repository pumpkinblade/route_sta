#pragma once


#include "GRPin.hpp"
#include <vector>

class GRInstance {
public:
  GRInstance(const std::string &name) : m_name(name) {}

  // network info set
  void setLibcellName(const std::string &lib) { m_libcell_name = lib; }
  void addPin(GRPin *pin) { m_pins.push_back(pin); }
  // network info get
  const std::string &name() const { return m_name; }
  const std::string &libcellName() const { return m_libcell_name; }
  const std::vector<GRPin *> &pins() const { return m_pins; }

private:
  // network info
  std::string m_name;
  std::string m_libcell_name;
  std::vector<GRPin *> m_pins;

  // instance's geo info are useless
};


