#ifndef __GR_NET_HPP__
#define __GR_NET_HPP__

#include "GRPin.hpp"
#include "GRTree.hpp"
#include <vector>

class GRNet {
public:
  GRNet(const std::string &name) : m_name(name) {}

  void addPin(GRPin *pin) { m_pins.push_back(pin); }
  void setRoutingTree(std::shared_ptr<GRTreeNode> tree) { m_tree = tree; }
  void setSlack(float s) { m_slack = s; }

  const std::string &name() const { return m_name; }
  const std::vector<GRPin *> &pins() const { return m_pins; }
  const std::shared_ptr<GRTreeNode> &routingTree() const { return m_tree; }
  float slack() const { return m_slack; }

private:
  std::string m_name;
  std::vector<GRPin *> m_pins;
  std::shared_ptr<GRTreeNode> m_tree;
  float m_slack;
};

class GRTechnology;

std::shared_ptr<GRTreeNode>
buildTree(const std::vector<std::pair<GRPoint, GRPoint>> &segs,
          const GRTechnology *tech);

#endif
