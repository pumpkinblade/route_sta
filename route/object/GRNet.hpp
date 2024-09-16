#ifndef __GR_NET_HPP__
#define __GR_NET_HPP__

#include "GRPin.hpp"
#include <vector>

class GRNet {
public:
  GRNet(const std::string &name) : m_name(name) {}

  void addPin(GRPin *pin) { m_pins.push_back(pin); }

  std::string getName() const { return m_name; }
  int getNumPins() const { return m_pins.size(); }
  const std::vector<GRPin *> &getPins() const { return m_pins; }
  // const std::shared_ptr<GRTreeNode> &getRoutingTree() const { return m_tree;
  // }

  // void setRoutingTree(std::shared_ptr<GRTreeNode> tree) { m_tree = tree; }
  // void clearRoutingTree() { m_tree = nullptr; }

  float getSlack() const { return m_slack; }
  void setSlack(float slack) { m_slack = slack; }

private:
  std::string m_name;
  std::vector<GRPin *> m_pins;

  // std::shared_ptr<GRTreeNode> m_tree;
  float m_slack;
};

#endif
