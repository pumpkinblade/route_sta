#ifndef __GR_PIN_HPP__
#define __GR_PIN_HPP__

#include <string>

class GRInstance;
class GRNet;

class GRPin {
public:
  GRPin(const std::string &name) : m_name(name) {}

  void setInstance(GRInstance *instance) { m_instance = instance; }
  void setNet(GRNet *net) { m_net = net; }

  std::string getName() const { return m_name; }
  // GRPoint getPosition() const { return GRPoint(m_layer, m_x_dbu, m_y_dbu); }
  // GRPoint getOnGridPosition() const {
  //   return GRPoint(m_layer, m_x_grid, m_y_grid);
  // }
  // int getConnectionLayer() const { return m_layer; }

  GRInstance *getInstance() const { return m_instance; }
  GRNet *getNet() const { return m_net; }
  unsigned int getDirection() const { return m_direction; }

private:
  std::string m_name;
  // int m_x_dbu, m_y_dbu;
  // int m_x_grid, m_y_grid;
  // int m_layer;

  GRInstance *m_instance;
  GRNet *m_net;
  unsigned int m_direction;
};

#endif
