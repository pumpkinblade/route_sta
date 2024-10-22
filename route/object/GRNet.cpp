#include "GRNet.hpp"
#include <map>
#include <stack>

struct Segment {
  GRPoint p, q;
  Segment() = default;
  Segment(const GRPoint &p1, const GRPoint &p2) {
    std::tie(p.x, q.x) = std::minmax(p1.x, p2.x);
    std::tie(p.y, q.y) = std::minmax(p1.y, p2.y);
    std::tie(p.layerIdx, q.layerIdx) = std::minmax(p1.layerIdx, p2.layerIdx);
  }
};

static bool compareVia(const GRPoint &p, const GRPoint &q) {
  return p.x < q.x || (p.x == q.x && p.y < q.y) ||
         (p.x == q.x && p.y == q.y && p.layerIdx < q.layerIdx);
}

static bool compareHorizontal(const GRPoint &p, const GRPoint &q) {
  return p.layerIdx < q.layerIdx || (p.layerIdx == q.layerIdx && p.y < q.y) ||
         (p.layerIdx == q.layerIdx && p.y == q.y && p.x < q.x);
}

static bool compareVertical(const GRPoint &p, const GRPoint &q) {
  return p.layerIdx < q.layerIdx || (p.layerIdx == q.layerIdx && p.x < q.x) ||
         (p.layerIdx == q.layerIdx && p.x == q.x && p.y < q.y);
}

static bool compareViaS(const Segment &s1, const Segment &s2) {
  return compareVia(s1.p, s2.p);
}

static bool compareHorizontalS(const Segment &s1, const Segment &s2) {
  return compareHorizontal(s1.p, s2.p);
}

static bool compareVerticalS(const Segment &s1, const Segment &s2) {
  return compareVertical(s1.p, s2.p);
}

static void removeOverlap(std::vector<Segment> &segs) {
  // partition wire and via
  auto via_begin_it =
      std::partition(segs.begin(), segs.end(), [](const Segment &s) {
        return s.p.layerIdx != s.q.layerIdx;
      });
  // partition horizontal wire and vertical wire
  auto vert_begin_it =
      std::partition(segs.begin(), via_begin_it,
                     [](const Segment &s) { return s.p.x != s.q.x; });
  auto hori_begin_it = segs.begin();

  // sort horizontal wire
  std::sort(segs.begin(), vert_begin_it, compareHorizontalS);
  auto hori_end_it = vert_begin_it;
  if (hori_begin_it != hori_end_it) {
    auto curr = hori_begin_it;
    auto next = hori_begin_it;
    while (++next != hori_end_it) {
      if (curr->p.layerIdx == next->p.layerIdx && curr->p.y == next->p.y &&
          curr->q.x >= next->p.x) {
        curr->q.x = std::max(curr->q.x, next->q.x);
      } else {
        curr++;
        *curr = *next;
      }
    }
    hori_end_it = curr++;
  }

  // sort vertical wire
  std::sort(vert_begin_it, via_begin_it, compareVerticalS);
  auto vert_end_it = via_begin_it;
  if (vert_begin_it != vert_end_it) {
    auto curr = vert_begin_it;
    auto next = vert_begin_it;
    while (++next != vert_end_it) {
      if (curr->p.layerIdx == next->p.layerIdx && curr->p.x == next->p.x &&
          curr->q.y >= next->p.y) {
        curr->q.y = std::max(curr->q.y, next->q.y);
      } else {
        curr++;
        *curr = *next;
      }
    }
    vert_end_it = curr++;
  }

  // sort via
  std::sort(via_begin_it, segs.end(), compareViaS);
  auto via_end_it = segs.end();
  if (via_begin_it != via_end_it) {
    auto curr = via_begin_it;
    auto next = via_begin_it;
    while (++next != via_end_it) {
      if (curr->p.layerIdx == next->p.layerIdx && curr->p.x == next->p.x &&
          curr->q.y >= next->p.y) {
        curr->q.y = std::max(curr->q.y, next->q.y);
      } else {
        curr++;
        *curr = *next;
      }
    }
    via_end_it = curr++;
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

static void getCrossPoints(std::vector<GRPoint> &points,
                           std::vector<Segment> &segs) {
  points.clear();
  // endpoints
  for (const auto &[p, q] : segs) {
    points.push_back(p);
    points.push_back(q);
  }
  // cross points
  auto via_begin_it =
      std::partition(segs.begin(), segs.end(), [](const Segment &s) {
        return s.p.layerIdx != s.q.layerIdx;
      });
  for (auto wit = segs.begin(); wit != via_begin_it; wit++) {
    for (auto vit = via_begin_it; vit != segs.end(); vit++) {
      if (wit->p.x <= vit->p.x && vit->p.x <= wit->q.x &&
          wit->p.y <= vit->p.y && vit->p.y <= wit->q.y &&
          vit->p.layerIdx <= wit->p.layerIdx &&
          wit->p.layerIdx <= vit->q.layerIdx) {
        points.emplace_back(wit->p.layerIdx, vit->p.layerIdx, vit->p.layerIdx);
      }
    }
  }
  std::sort(points.begin(), points.end());
  points.erase(std::unique(points.begin(), points.end()), points.end());
}

static void splitSegment(std::vector<GRPoint> &points,
                         std::vector<Segment> &segs) {
  std::vector<Segment> new_segs;

  auto via_begin_it =
      std::partition(segs.begin(), segs.end(), [](const Segment &s) {
        return s.p.layerIdx != s.q.layerIdx;
      });
  auto vert_begin_it =
      std::partition(segs.begin(), via_begin_it,
                     [](const Segment &s) { return s.p.x != s.q.x; });

  // split horizontal wire
  std::sort(points.begin(), points.end(), compareHorizontal);
  for (auto it = segs.begin(); it != vert_begin_it; it++) {
    auto [p, q] = *it;
    std::tie(p, q) = std::minmax(p, q, compareHorizontal);
    auto lower_it =
        std::upper_bound(points.begin(), points.end(), p, compareHorizontal);
    auto upper_it =
        std::lower_bound(points.begin(), points.end(), q, compareHorizontal);
    for (auto it2 = lower_it; it2 != upper_it; it2++) {
      new_segs.emplace_back(p, *it2);
      p = *it2;
    }
    new_segs.emplace_back(p, q);
  }

  // split vertical wire
  std::sort(points.begin(), points.end(), compareVertical);
  for (auto it = vert_begin_it; it != via_begin_it; it++) {
    auto [p, q] = *it;
    std::tie(p, q) = std::minmax(p, q, compareVertical);
    auto lower_it =
        std::upper_bound(points.begin(), points.end(), p, compareVertical);
    auto upper_it =
        std::lower_bound(points.begin(), points.end(), q, compareVertical);
    for (auto it2 = lower_it; it2 != upper_it; it2++) {
      new_segs.emplace_back(p, *it2);
      p = *it2;
    }
    new_segs.emplace_back(p, q);
  }

  // split via
  std::sort(points.begin(), points.end(), compareVia);
  for (auto it = via_begin_it; it != segs.end(); it++) {
    auto [p, q] = *it;
    std::tie(p, q) = std::minmax(p, q, compareVia);
    auto lower_it =
        std::upper_bound(points.begin(), points.end(), p, compareVia);
    auto upper_it =
        std::lower_bound(points.begin(), points.end(), q, compareVia);
    for (auto it2 = lower_it; it2 != upper_it; it2++) {
      new_segs.emplace_back(p, *it2);
      p = *it2;
    }
    new_segs.emplace_back(p, q);
  }

  segs.swap(new_segs);
}

static std::shared_ptr<GRTreeNode> constructTree(std::vector<Segment> &segs) {
  // build graph
  std::map<GRPoint, std::vector<GRPoint>> links;
  for (const auto &[p, q] : segs) {
    links[p].push_back(q);
    links[q].push_back(p);
  }

  // do dfs and get the routing tree
  std::map<GRPoint, bool> mark;
  std::stack<std::shared_ptr<GRTreeNode>> stk;
  GRPoint root_pt = segs.front().p;
  std::shared_ptr<GRTreeNode> root =
      std::make_shared<GRTreeNode>(root_pt.layerIdx, root_pt.x, root_pt.y);
  mark[root_pt] = true;
  while (!stk.empty()) {
    auto node = stk.top();
    GRPoint node_pt(node->layerIdx, node->x, node->y);
    stk.pop();
    for (const GRPoint &next_pt : links.at(node_pt)) {
      if (mark[next_pt])
        continue;
      auto child =
          std::make_shared<GRTreeNode>(next_pt.layerIdx, next_pt.x, next_pt.y);
      node->children.push_back(child);
      stk.push(child);
      mark[next_pt] = true;
    }
  }
  return root;
}

std::shared_ptr<GRTreeNode>
buildTree(const std::vector<std::pair<GRPoint, GRPoint>> &segments) {
  std::vector<Segment> segs;
  segs.reserve(segments.size());
  for (const auto &[p, q] : segments) {
    segs.emplace_back(p, q);
  }
  removeOverlap(segs);

  std::vector<GRPoint> points;
  getCrossPoints(points, segs);
  splitSegment(points, segs);
  return constructTree(segs);
}
