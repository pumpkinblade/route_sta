#pragma once

#include "../util/geo.hpp"
#include <functional>
#include <memory>

namespace sca {

struct RouteSegment {};

class GRTreeNode : public PointOnLayerT<int> {
public:
  std::vector<std::shared_ptr<GRTreeNode>> children;

  GRTreeNode(int l, int x, int y) : PointOnLayerT<int>(l, x, y) {}
  GRTreeNode(const PointOnLayerT<int> &point) : PointOnLayerT<int>(point) {}
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

} // namespace sca
