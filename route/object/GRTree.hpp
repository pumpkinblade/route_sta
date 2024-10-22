#ifndef __GR_TREE_HPP__
#define __GR_TREE_HPP__

#include "../util/geo.hpp"
#include <functional>
#include <memory>

using GRPoint = utils::PointOnLayerT<int>;

class GRTreeNode : public GRPoint {
public:
  std::vector<std::shared_ptr<GRTreeNode>> children;

  GRTreeNode(int l, int x, int y) : GRPoint(l, x, y) {}
  GRTreeNode(const GRPoint &point) : GRPoint(point) {}
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
