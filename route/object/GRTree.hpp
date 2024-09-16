#ifndef __GR_TREE_HPP__
#define __GR_TREE_HPP__

#include "geo.hpp"
#include <functional>
#include <memory>

class GRPoint : public utils::PointT<int> {
public:
  // int x
  // int y
  int layerIdx;
  GRPoint(int l, int _x, int _y) : layerIdx(l), PointT<int>(_x, _y) {}
};

class GRTreeNode : public GRPoint {
public:
  std::vector<std::shared_ptr<GRTreeNode>> children;

  GRTreeNode(int l, int _x, int _y) : GRPoint(l, _x, _y) {}
  GRTreeNode(const GRPoint &point) : GRPoint(point) {}
  // ~GRTreeNode() { for (auto& child : children) child->~GRTreeNode(); }
  static void preorder(std::shared_ptr<GRTreeNode> node,
                       std::function<void(std::shared_ptr<GRTreeNode>)> visit);
};

inline void
GRTreeNode::preorder(std::shared_ptr<GRTreeNode> node,
                     std::function<void(std::shared_ptr<GRTreeNode>)> visit) {
  visit(node);
  for (auto &child : node->children)
    preorder(child, visit);
}

#endif
