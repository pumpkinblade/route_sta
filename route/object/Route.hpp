#pragma once

#include "../util/geo.hpp"
#include <functional>
#include <memory>

namespace sca {

template <class T> struct RouteSegment {
  PointOnLayerT<T> start, end;
  RouteSegment() = default;
  RouteSegment(const PointOnLayerT<T> &p1, const PointOnLayerT<T> &p2) {
    std::tie(start.x, end.x) = std::minmax(p1.x, p2.x);
    std::tie(start.y, end.y) = std::minmax(p1.y, p2.y);
    std::tie(start.layerIdx, end.layerIdx) = std::minmax(p1.layerIdx, p2.layerIdx);
  }
};

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

class Technology;

std::shared_ptr<GRTreeNode>
buildTree(const std::vector<std::pair<PointOnLayerT<int>, PointOnLayerT<int>>> &segments,
          const Technology *tech);

// This function will split the tree into segments, and reconstruct the tree

std::shared_ptr<GRTreeNode> trimTree(std::shared_ptr<GRTreeNode> tree,
                                     const Technology *tech);


} // namespace sca
