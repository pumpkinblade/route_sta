#include "Technology.hpp"
#include "Route.hpp"
#include <map>
#include <stack>

namespace sca {

static bool compareHorizontal(const PointOnLayerT<int> &p, const PointOnLayerT<int> &q) {
  bool b1 = p.layerIdx < q.layerIdx;
  bool b2 = p.layerIdx == q.layerIdx && p.y < q.y;
  bool b3 = p.layerIdx == q.layerIdx && p.y == q.y && p.x < q.x;
  return b1 || b2 || b3;
}

static bool compareVertical(const PointOnLayerT<int> &p, const PointOnLayerT<int> &q) {
  bool b1 = p.layerIdx < q.layerIdx;
  bool b2 = p.layerIdx == q.layerIdx && p.x < q.x;
  bool b3 = p.layerIdx == q.layerIdx && p.x == q.x && p.y < q.y;
  return b1 || b2 || b3;
}

static bool compareVia(const PointOnLayerT<int> &p, const PointOnLayerT<int> &q) {
  bool b1 = p.x < q.x;
  bool b2 = p.x == q.x && p.y < q.y;
  bool b3 = p.x == q.x && p.y == q.y && p.layerIdx < q.layerIdx;
  return b1 || b2 || b3;
}
    
static bool compareHorizontalS(const RouteSegment<int> &s1, const RouteSegment<int> &s2) {
  return compareHorizontal(s1.start, s2.start);
}

static bool compareVerticalS(const RouteSegment<int> &s1, const RouteSegment<int> &s2) {
  return compareVertical(s1.start, s2.end);
}

static bool compareViaS(const RouteSegment<int> &s1, const RouteSegment<int> &s2) {
  return compareVia(s1.start, s2.start);
}

    
static void removeOverlap(std::vector<RouteSegment<int>> &segs,
                          const Technology *tech) {
  // partition wire and via
  auto via_begin_it =
      std::partition(segs.begin(), segs.end(), [](const RouteSegment<int> &s) {
        return s.start.layerIdx == s.end.layerIdx;
      });
  // partition horizontal wire and vertical wire
  auto vert_begin_it =
      std::partition(segs.begin(), via_begin_it, [&](const RouteSegment<int> &s) {
        return tech->layer(s.start.layerIdx)->direction() != LayerDirection::Vertical;
      });
  auto hori_begin_it = segs.begin();

  // sort horizontal wire
  auto hori_end_it = vert_begin_it;
  if (hori_begin_it != hori_end_it) {
    std::sort(hori_begin_it, hori_end_it, compareHorizontalS);
    auto curr = hori_begin_it;
    auto next = hori_begin_it;
    while (++next != hori_end_it) {
      if (curr->start.layerIdx == next->start.layerIdx && curr->start.y == next->start.y &&
          curr->end.x >= next->start.x) {
        curr->end.x = std::max(curr->end.x, next->end.x);
      } else {
        curr++;
        *curr = *next;
      }
    }
    hori_end_it = ++curr;
  }

  // sort vertical wire
  auto vert_end_it = via_begin_it;
  if (vert_begin_it != vert_end_it) {
    std::sort(vert_begin_it, vert_end_it, compareVerticalS);
    auto curr = vert_begin_it;
    auto next = vert_begin_it;
    while (++next != vert_end_it) {
      if (curr->start.layerIdx == next->start.layerIdx && curr->start.x == next->start.x &&
          curr->end.y >= next->start.y) {
        curr->end.y = std::max(curr->end.y, next->end.y);
      } else {
        curr++;
        *curr = *next;
      }
    }
    vert_end_it = ++curr;
  }

  // sort via
  auto via_end_it = segs.end();
  if (via_begin_it != via_end_it) {
    std::sort(via_begin_it, via_end_it, compareViaS);
    auto curr = via_begin_it;
    auto next = via_begin_it;
    while (++next != via_end_it) {
      if (curr->start.layerIdx == next->start.layerIdx && curr->start.x == next->start.x &&
          curr->end.y >= next->start.y) {
        curr->end.y = std::max(curr->end.y, next->end.y);
      } else {
        curr++;
        *curr = *next;
      }
    }
    via_end_it = ++curr;
  }

  auto seg_end = hori_end_it;
  for (auto it = vert_begin_it; it != vert_end_it; it++) {
    *seg_end = *it;
    seg_end++;
  }
  for (auto it = via_begin_it; it != via_end_it; it++) {
    *seg_end = *it;
    seg_end++;
  }
  segs.erase(seg_end, segs.end());
}

static void getCrossPoints(std::vector<PointOnLayerT<int>> &points,
                           std::vector<RouteSegment<int>> &segs, const Technology *) {
  points.clear();
  // endpoints
  for (const auto &[p, q] : segs) {
    points.push_back(p);
    points.push_back(q);
  }
  // cross points
  auto via_begin_it =
      std::partition(segs.begin(), segs.end(), [](const RouteSegment<int> &s) {
        return s.start.layerIdx == s.end.layerIdx;
      });
  for (auto wit = segs.begin(); wit != via_begin_it; wit++) {
    for (auto vit = via_begin_it; vit != segs.end(); vit++) {
      if (wit->start.x <= vit->start.x && vit->start.x <= wit->end.x &&
          wit->start.y <= vit->start.y && vit->start.y <= wit->end.y &&
          vit->start.layerIdx <= wit->start.layerIdx &&
          wit->start.layerIdx <= vit->start.layerIdx) {
        points.emplace_back(wit->start.layerIdx, vit->start.x, vit->start.y);
      }
    }
  }
  std::sort(points.begin(), points.end());
  points.erase(std::unique(points.begin(), points.end()), points.end());
}

static void splitSegment(std::vector<PointOnLayerT<int>> &points,
                         std::vector<RouteSegment<int>> &segs, const Technology *tech) {
  std::vector<RouteSegment<int>> new_segs;

  auto via_begin_it =
      std::partition(segs.begin(), segs.end(), [](const RouteSegment<int> &s) {
        return s.start.layerIdx == s.end.layerIdx;
      });
  auto vert_begin_it =
      std::partition(segs.begin(), via_begin_it, [&](const RouteSegment<int> &s) {
        return tech->layer(s.start.layerIdx)->direction() != LayerDirection::Vertical;
      });

  // split horizontal wire
  std::sort(points.begin(), points.end(), compareHorizontal);
  for (auto it = segs.begin(); it != vert_begin_it; it++) {
    auto [p, q] = *it;
    std::tie(p, q) = std::minmax(p, q, compareHorizontal);
    auto lower_it =
        std::lower_bound(points.begin(), points.end(), p, compareHorizontal);
    auto upper_it =
        std::upper_bound(points.begin(), points.end(), q, compareHorizontal);
    for (auto it = lower_it, next_it = it + 1; next_it != upper_it;
         it++, next_it++) {
      new_segs.emplace_back(*it, *next_it);
    }
  }

  // split vertical wire
  std::sort(points.begin(), points.end(), compareVertical);
  for (auto it = vert_begin_it; it != via_begin_it; it++) {
    auto [p, q] = *it;
    std::tie(p, q) = std::minmax(p, q, compareVertical);
    auto lower_it =
        std::lower_bound(points.begin(), points.end(), p, compareVertical);
    auto upper_it =
        std::upper_bound(points.begin(), points.end(), q, compareVertical);
    for (auto it = lower_it, next_it = it + 1; next_it != upper_it;
         it++, next_it++) {
      new_segs.emplace_back(*it, *next_it);
    }
  }

  // split via
  std::sort(points.begin(), points.end(), compareVia);
  for (auto it = via_begin_it; it != segs.end(); it++) {
    auto [p, q] = *it;
    std::tie(p, q) = std::minmax(p, q, compareVia);
    auto lower_it =
        std::lower_bound(points.begin(), points.end(), p, compareVia);
    auto upper_it =
        std::upper_bound(points.begin(), points.end(), q, compareVia);
    for (auto it = lower_it, next_it = it + 1; next_it != upper_it;
         it++, next_it++) {
      new_segs.emplace_back(*it, *next_it);
    }
  }

  segs.swap(new_segs);
}


static std::shared_ptr<GRTreeNode> constructTree(std::vector<RouteSegment<int>> &segs) {
  // build graph
  std::map<PointOnLayerT<int>, std::vector<PointOnLayerT<int>>> links;
  for (const auto &[p, q] : segs) {
    links[p].push_back(q);
    links[q].push_back(p);
  }

  // do dfs and get the routing tree
  std::map<PointOnLayerT<int>, bool> mark;
  std::stack<std::shared_ptr<GRTreeNode>> stk;
  PointOnLayerT<int> root_pt = segs.front().start;
  std::shared_ptr<GRTreeNode> root =
      std::make_shared<GRTreeNode>(root_pt.layerIdx, root_pt.x, root_pt.y);
  stk.push(root);
  mark[root_pt] = true;
  while (!stk.empty()) {
    auto node = stk.top();
    PointOnLayerT<int> node_pt(node->layerIdx, node->x, node->y);
    stk.pop();
    for (const PointOnLayerT<int> &child_pt : links.at(node_pt)) {
      if (mark[child_pt])
        continue;
      auto child = std::make_shared<GRTreeNode>(child_pt.layerIdx, child_pt.x,
                                                child_pt.y);
      node->children.push_back(child);
      stk.push(child);
      mark[child_pt] = true;
    }
  }

  // remove intermediate node
  GRTreeNode::preorder(root, [](std::shared_ptr<GRTreeNode> node) {
    for (auto &child : node->children) {
      auto tmp = child;
      while (tmp->children.size() == 1) {
        bool both_via = (tmp->children[0]->layerIdx != tmp->layerIdx &&
                         node->layerIdx != tmp->layerIdx);
        bool both_hori = (tmp->children[0]->x != tmp->x && node->x != tmp->x);
        bool both_vert = (tmp->children[0]->y != tmp->y && node->y != tmp->y);
        if (both_via || both_hori || both_vert)
          tmp = tmp->children[0];
        else
          break;
      }
      child = tmp;
    }
  });

  return root;
}


std::shared_ptr<GRTreeNode>
buildTree(const std::vector<RouteSegment<int>> &segments,
          const Technology *tech) {
  if (segments.size() == 0)
    return nullptr;

  std::vector<RouteSegment<int>> segs(segments);

  // printf("segments: \n");
  // for (const auto &[p, q] : segs)
  //   std::printf("(%d, %d, %d) -> (%d, %d, %d)\n", p.x, p.y, p.layerIdx, q.x,
  //               q.y, q.layerIdx);

  removeOverlap(segs, tech);
  // printf("no overlap segments: \n");
  // for (const auto &[p, q] : segs)
  //   std::printf("(%d, %d, %d) -> (%d, %d, %d)\n", p.x, p.y, p.layerIdx, q.x,
  //               q.y, q.layerIdx);

  std::vector<PointOnLayerT<int>> points;
  getCrossPoints(points, segs, tech);
  // printf("cross points: \n");
  // for (const auto &p : points)
  //   std::printf("(%d, %d, %d)\n", p.x, p.y, p.layerIdx);

  splitSegment(points, segs, tech);
  // printf("split segments: \n");
  // for (const auto &[p, q] : segs)
  //   std::printf("(%d, %d, %d) -> (%d, %d, %d)\n", p.x, p.y, p.layerIdx, q.x,
  //               q.y, q.layerIdx);

  auto tree = constructTree(segs);
  // printf("tree: \n");
  // GRTreeNode::preorder(tree, [](std::shared_ptr<GRTreeNode> node) {
  //   for (const auto &child : node->children) {
  //     std::printf("(%d, %d, %d) -> (%d, %d, %d)\n", node->x, node->y,
  //                 node->layerIdx, child->x, child->y, child->layerIdx);
  //   }
  // });

  return tree;
}

std::shared_ptr<GRTreeNode> trimTree(std::shared_ptr<GRTreeNode> tree,
                                     const Technology *tech) {
  std::vector<RouteSegment<int>> segments;
  GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
    for (auto child : node->children) {
      segments.emplace_back(PointOnLayerT<int>(node->layerIdx, node->x, node->y),
                            PointOnLayerT<int>(child->layerIdx, child->x, child->y));
    }
  });
  return buildTree(segments, tech);
}


}
