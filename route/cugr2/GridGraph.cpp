#include "GridGraph.h"
#include "../util/log.hpp"

namespace cugr2 {

GridGraph::GridGraph(const GRTechnology *tech, const Parameters &params)
    : params(params) {
  nLayers = tech->numLayers();
  xSize = tech->numGcellX();
  ySize = tech->numGcellY();

  layerDirections.resize(nLayers);
  layerMinLengths.resize(nLayers);
  for (int i = 0; i < nLayers; i++) {
    layerDirections[i] = tech->layerDirection(i) == LayerDirection::Horizontal
                             ? MetalLayer::H
                             : MetalLayer::V;
    layerMinLengths[i] = tech->layerMinLengthDbu(i);
  }

  // edge
  edgeLengths.resize(2);
  edgeLengths[MetalLayer::H].resize(xSize - 1);
  edgeLengths[MetalLayer::V].resize(ySize - 1);
  for (int x = 0; x < xSize - 1; x++)
    edgeLengths[MetalLayer::H][x] = tech->edgeLengthDbuX(x);
  for (int y = 0; y < ySize - 1; y++)
    edgeLengths[MetalLayer::H][y] = tech->edgeLengthDbuY(y);

  graphEdges.assign(nLayers, std::vector<std::vector<GraphEdge>>(
                                 xSize, std::vector<GraphEdge>(ySize)));
  for (int layerIdx = 0; layerIdx < nLayers; layerIdx++) {
    for (int y = 0; y < ySize; y++) {
      for (int x = 0; x < xSize; x++) {
        graphEdges[layerIdx][x][y].capacity =
            tech->edgeCapacity(layerIdx, x, y);
      }
    }
  }
  flag.assign(nLayers,
              std::vector<std::vector<bool>>(xSize, std::vector<bool>(ySize)));
}

CostT GridGraph::getWireCost(int layerIdx, utils::PointT<int> lower,
                             CapacityT) const {
  unsigned direction = layerDirections[layerIdx];
  DBU edgeLength = getEdgeLength(direction, lower[direction]);
  const auto &edge = graphEdges[layerIdx][lower.x][lower.y];
  CostT slope = edge.capacity > 0.f ? 0.5f : 1.5f;
  CostT cost = edgeLength * params.unit_length_wire_cost +
               params.layer_overflow_weight[layerIdx] *
                   exp(slope * (edge.demand - edge.capacity)) *
                   (exp(slope) - 1);
  return cost;
}

CostT GridGraph::getWireCost(int layerIdx, utils::PointT<int> u,
                             utils::PointT<int> v) const {
  CostT cost = 0;
  switch (layerDirections[layerIdx]) {
  case MetalLayer::H: {
    int l = std::min(u.x, v.x), h = std::max(u.x, v.x);
    for (int x = l; x < h; x++)
      cost += getWireCost(layerIdx, {x, u.y});
  } break;
  case MetalLayer::V: {
    int l = std::min(u.y, v.y), h = std::max(u.y, v.y);
    for (int y = l; y < h; y++)
      cost += getWireCost(layerIdx, {u.x, y});
  } break;
  }
  return cost;
}

CostT GridGraph::getViaCost(int, utils::PointT<int>) const {
  return params.unit_via_cost;
}

CostT GridGraph::getNonStackViaCost(int layerIdx,
                                    utils::PointT<int> loc) const {
  auto [x, y] = loc;
  bool isHorizontal = (layerDirections[layerIdx] == MetalLayer::H);
  if (isHorizontal ? (x == 0) : (y == 0)) {
    const auto &rightEdge = graphEdges[layerIdx][x][y];
    CostT rightSlope = rightEdge.capacity > 0.f ? 0.5f : 1.5f;
    return params.layer_overflow_weight[layerIdx] *
           std::exp(rightSlope * (rightEdge.demand - rightEdge.capacity)) *
           (std::exp(rightSlope) - 1);
  } else if (isHorizontal ? (x == xSize - 1) : (y == ySize - 1)) {
    const auto &leftEdge =
        graphEdges[layerIdx][x - isHorizontal][y - !isHorizontal];
    CostT leftSlope = leftEdge.capacity > 0.f ? 0.5f : 1.5f;
    return params.layer_overflow_weight[layerIdx] *
           std::exp(leftSlope * (leftEdge.demand - leftEdge.capacity)) *
           (std::exp(leftSlope) - 1);
  } else {
    const auto &rightEdge = graphEdges[layerIdx][x][y];
    CostT rightSlope = rightEdge.capacity > 0.f ? 0.5f : 1.5f;
    const auto &leftEdge =
        graphEdges[layerIdx][x - isHorizontal][y - !isHorizontal];
    CostT leftSlope = leftEdge.capacity > 0.f ? 0.5f : 1.5f;
    return params.layer_overflow_weight[layerIdx] *
               std::exp(rightSlope * (rightEdge.demand - rightEdge.capacity)) *
               (std::exp(0.5f * rightSlope) - 1) +
           params.layer_overflow_weight[layerIdx] *
               std::exp(leftSlope * (leftEdge.demand - leftEdge.capacity)) *
               (std::exp(0.5f * leftSlope) - 1);
  }
}

void GridGraph::selectAccessPoints(
    GRNet *net,
    std::unordered_map<uint64_t,
                       std::pair<utils::PointT<int>, utils::IntervalT<int>>>
        &selectedAccessPoints) const {
  selectedAccessPoints.clear();
  // cell hash (2d) -> access point, fixed layer interval
  selectedAccessPoints.reserve(net->pins().size());
  utils::BoxT<int> bbox;
  for (GRPin *pin : net->pins()) {
    for (const GRPoint &pt : pin->accessPoints())
      bbox.Update(pt);
  }
  utils::PointT<int> netCenter(bbox.cx(), bbox.cy());
  for (GRPin *pin : net->pins()) {
    std::pair<int, int> bestAccessDist = {0, std::numeric_limits<int>::max()};
    int bestIndex = -1;
    for (size_t index = 0; index < pin->accessPoints().size(); index++) {
      const auto &point = pin->accessPoints()[index];
      int accessibility = 0;
      if (point.layerIdx >= params.min_routing_layer) {
        unsigned direction = getLayerDirection(point.layerIdx);
        accessibility +=
            getEdge(point.layerIdx, point.x, point.y).capacity >= 1;
        if (point[direction] > 0) {
          auto lower = point;
          lower[direction] -= 1;
          accessibility +=
              getEdge(lower.layerIdx, lower.x, lower.y).capacity >= 1;
        }
      } else {
        accessibility = 1;
      }
      int distance = abs(netCenter.x - point.x) + abs(netCenter.y - point.y);
      if (accessibility > bestAccessDist.first ||
          (accessibility == bestAccessDist.first &&
           distance < bestAccessDist.second)) {
        bestIndex = index;
        bestAccessDist = {accessibility, distance};
      }
    }
    if (bestAccessDist.first == 0) {
      LOG_WARN("the pin is hard to access.");
    }
    const utils::PointT<int> selectedPoint = pin->accessPoints()[bestIndex];
    const uint64_t hash = hashCell(selectedPoint.x, selectedPoint.y);
    if (selectedAccessPoints.find(hash) == selectedAccessPoints.end()) {
      selectedAccessPoints.emplace(
          hash, std::make_pair(selectedPoint, utils::IntervalT<int>()));
    }
    utils::IntervalT<int> &fixedLayerInterval =
        selectedAccessPoints[hash].second;
    for (const auto &point : pin->accessPoints()) {
      if (point.x == selectedPoint.x && point.y == selectedPoint.y) {
        fixedLayerInterval.Update(point.layerIdx);
      }
    }
  }
  // Extend the fixed layers to 2 layers higher to facilitate track switching
  for (auto &accessPoint : selectedAccessPoints) {
    utils::IntervalT<int> &fixedLayers = accessPoint.second.second;
    fixedLayers.high = std::min(fixedLayers.high + 2, nLayers - 1);
  }
}

void GridGraph::commitWire(const int layerIdx, const utils::PointT<int> lower,
                           const bool reverse) {
  graphEdges[layerIdx][lower.x][lower.y].demand += (reverse ? -1.f : 1.f);
  unsigned direction = getLayerDirection(layerIdx);
  DBU edgeLength = getEdgeLength(direction, lower[direction]);
  totalLength += (reverse ? -edgeLength : edgeLength);
}

void GridGraph::commitVia(const int, const utils::PointT<int>,
                          const bool reverse) {
  totalNumVias += (reverse ? -1 : 1);
}

void GridGraph::commitNonStackVia(const int layerIdx,
                                  const utils::PointT<int> loc,
                                  const bool reverse) {
  auto isHorizontal = getLayerDirection(layerIdx) == MetalLayer::H;
  auto [x, y] = loc;
  if (isHorizontal ? (x == 0) : (y == 0))
    graphEdges[layerIdx][x][y].demand += (reverse ? -1.f : 1.f);
  else if (isHorizontal ? (x == xSize - 1) : (y == ySize - 1))
    graphEdges[layerIdx][x - isHorizontal][y - !isHorizontal].demand +=
        (reverse ? -1.f : 1.f);
  else {
    graphEdges[layerIdx][x][y].demand += reverse ? -.5f : .5f;
    graphEdges[layerIdx][x - isHorizontal][y - !isHorizontal].demand +=
        (reverse ? -.5f : .5f);
  }
}

void GridGraph::commitTree(const std::shared_ptr<GRTreeNode> &tree,
                           const bool reverse) {
  // reset flag
  GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
    for (const auto &child : node->children) {
      if (node->layerIdx == child->layerIdx) { // wires
        if (getLayerDirection(node->layerIdx) == MetalLayer::H) {
          for (int x = std::min(node->x, child->x),
                   xe = std::max(node->x, child->x);
               x <= xe; x++)
            flag[node->layerIdx][x][node->y] = false;
        } else {
          for (int y = std::min(node->y, child->y),
                   ye = std::max(node->y, child->y);
               y <= ye; y++)
            flag[node->layerIdx][node->x][y] = false;
        }
      } else { // vias
        for (int z = std::min(node->layerIdx, child->layerIdx),
                 ze = std::max(node->layerIdx, child->layerIdx);
             z <= ze; z++)
          flag[z][node->x][node->y] = false;
      }
    }
  });
  // commit wire
  GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
    for (const auto &child : node->children) {
      if (node->layerIdx == child->layerIdx) { // wires
        if (getLayerDirection(node->layerIdx) == MetalLayer::H) {
          for (int x = std::min(node->x, child->x),
                   xe = std::max(node->x, child->x);
               x <= xe; x++) {
            if (x < xe)
              commitWire(node->layerIdx, {x, node->y}, reverse);
            flag[node->layerIdx][x][node->y] = true;
          }
        } else {
          for (int y = std::min(node->y, child->y),
                   ye = std::max(node->y, child->y);
               y <= ye; y++) {
            if (y < ye)
              commitWire(node->layerIdx, {node->x, y}, reverse);
            flag[node->layerIdx][node->x][y] = true;
          }
        }
      }
    }
  });
  // commit (nonstack) via
  GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
    for (const auto &child : node->children) {
      if (node->layerIdx != child->layerIdx) {
        for (int z = std::min(node->layerIdx, child->layerIdx),
                 ze = std::max(node->layerIdx, child->layerIdx);
             z < ze; z++) {
          if (!flag[z][node->x][node->y]) {
            flag[z][node->x][node->y] = true;
            commitNonStackVia(z, {node->x, node->y}, reverse);
          }
          commitVia(z, {node->x, node->y}, reverse);
        }
      }
    }
  });
}

int GridGraph::checkOverflow(const int layerIdx, const utils::PointT<int> u,
                             const utils::PointT<int> v) const {
  int num = 0;
  unsigned direction = layerDirections[layerIdx];
  if (direction == MetalLayer::H) {
    assert(u.y == v.y);
    int l = std::min(u.x, v.x), h = std::max(u.x, v.x);
    for (int x = l; x < h; x++) {
      if (checkOverflow(layerIdx, x, u.y))
        num++;
    }
  } else {
    assert(u.x == v.x);
    int l = std::min(u.y, v.y), h = std::max(u.y, v.y);
    for (int y = l; y < h; y++) {
      if (checkOverflow(layerIdx, u.x, y))
        num++;
    }
  }
  return num;
}

int GridGraph::checkOverflow(const std::shared_ptr<GRTreeNode> &tree) const {
  if (!tree)
    return 0;
  int num = 0;
  GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
    for (auto &child : node->children) {
      // Only check wires
      if (node->layerIdx == child->layerIdx) {
        utils::PointT<int> p(node->x, node->y);
        utils::PointT<int> q(child->x, child->y);
        num += checkOverflow(node->layerIdx, p, q);
      }
    }
  });
  return num;
}

void GridGraph::extractBlockageView(GridGraphView<bool> &view) const {
  view.assign(
      2, std::vector<std::vector<bool>>(xSize, std::vector<bool>(ySize, true)));
  for (int layerIdx = params.min_routing_layer; layerIdx < nLayers;
       layerIdx++) {
    unsigned direction = getLayerDirection(layerIdx);
    for (int x = 0; x < xSize; x++) {
      for (int y = 0; y < ySize; y++) {
        if (getEdge(layerIdx, x, y).capacity >= 1.0) {
          view[direction][x][y] = false;
        }
      }
    }
  }
}

void GridGraph::extractCongestionView(GridGraphView<bool> &view) const {
  view.assign(2, std::vector<std::vector<bool>>(
                     xSize, std::vector<bool>(ySize, false)));
  for (int layerIdx = params.min_routing_layer; layerIdx < nLayers;
       layerIdx++) {
    unsigned direction = getLayerDirection(layerIdx);
    for (int x = 0; x < xSize; x++) {
      for (int y = 0; y < ySize; y++) {
        if (checkOverflow(layerIdx, x, y)) {
          view[direction][x][y] = true;
        }
      }
    }
  }
}

void GridGraph::extractWireCostView(GridGraphView<CostT> &view) const {
  view.assign(2, std::vector<std::vector<CostT>>(
                     xSize, std::vector<CostT>(
                                ySize, std::numeric_limits<CostT>::max())));
  for (unsigned direction = 0; direction < 2; direction++) {
    std::vector<int> layerIndices;
    CostT overflowWeight = std::numeric_limits<CostT>::max();
    for (int layerIdx = params.min_routing_layer; layerIdx < getNumLayers();
         layerIdx++) {
      if (getLayerDirection(layerIdx) == direction) {
        layerIndices.emplace_back(layerIdx);
        overflowWeight =
            std::min(overflowWeight, getLayerOverflowWeight(layerIdx));
      }
    }
    for (int x = 0; x < xSize; x++) {
      for (int y = 0; y < ySize; y++) {
        int edgeIndex = direction == MetalLayer::H ? x : y;
        if (edgeIndex >= getSize(direction) - 1)
          continue;
        CapacityT capacity = 0;
        CapacityT demand = 0;
        for (int layerIdx : layerIndices) {
          const auto &edge = getEdge(layerIdx, x, y);
          capacity += edge.capacity;
          demand += edge.demand;
        }
        DBU length = getEdgeLength(direction, edgeIndex);
        // ------ legacy cost ------
        // view[direction][x][y] =
        //     length *
        //     (params.unit_length_wire_cost +
        //      overflowWeight * (capacity < 1.0
        //                            ? 1.0
        //                            : logistic(capacity - demand,
        //                                       params.maze_logistic_slope)));

        // ------ new cost ------
        CostT slope = capacity > 0.f ? 0.5f : 1.5f;
        view[direction][x][y] = length * params.unit_length_wire_cost +
                                50 * overflowWeight *
                                    exp(slope * (demand - capacity)) *
                                    (exp(slope) - 1);
      }
    }
  }
}

void GridGraph::updateWireCostView(
    GridGraphView<CostT> &view, std::shared_ptr<GRTreeNode> routingTree) const {
  std::vector<std::vector<int>> sameDirectionLayers(2);
  std::vector<CostT> overflowWeight(2, std::numeric_limits<CostT>::max());
  for (int layerIdx = params.min_routing_layer; layerIdx < getNumLayers();
       layerIdx++) {
    unsigned direction = getLayerDirection(layerIdx);
    sameDirectionLayers[direction].emplace_back(layerIdx);
    overflowWeight[direction] =
        std::min(overflowWeight[direction], getLayerOverflowWeight(layerIdx));
  }
  auto update = [&](unsigned direction, int x, int y) {
    int edgeIndex = direction == MetalLayer::H ? x : y;
    if (edgeIndex >= getSize(direction) - 1)
      return;
    CapacityT capacity = 0;
    CapacityT demand = 0;
    for (int layerIdx : sameDirectionLayers[direction]) {
      if (getLayerDirection(layerIdx) != direction)
        continue;
      const auto &edge = getEdge(layerIdx, x, y);
      capacity += edge.capacity;
      demand += edge.demand;
    }
    DBU length = getEdgeLength(direction, edgeIndex);
    // ------ legacy cost ------
    // view[direction][x][y] = length * (unit_length_wire_cost +
    // unitOverflowCost[direction] * (capacity < 1.0 ? 1.0 : logistic(capacity -
    // demand, parameters.maze_logistic_slope)));

    // ------ new cost ------
    CostT slope = capacity > 0.f ? 0.5f : 1.5f;
    view[direction][x][y] = length * params.unit_length_wire_cost +
                            50 * overflowWeight[direction] *
                                exp(slope * (demand - capacity)) *
                                (exp(slope) - 1);
  };
  GRTreeNode::preorder(routingTree, [&](std::shared_ptr<GRTreeNode> node) {
    for (const auto &child : node->children) {
      if (node->layerIdx == child->layerIdx) {
        unsigned direction = getLayerDirection(node->layerIdx);
        if (direction == MetalLayer::H) {
          assert(node->y == child->y);
          auto [l, h] = std::minmax(node->x, child->x);
          for (int x = l; x < h; x++) {
            update(direction, x, node->y);
          }
        } else {
          assert(node->x == child->x);
          auto [l, h] = std::minmax(node->y, child->y);
          for (int y = l; y < h; y++) {
            update(direction, node->x, y);
          }
        }
      } else {
        auto [l, le] = std::minmax(node->layerIdx, child->layerIdx);
        for (int layerIdx = l; layerIdx < le; layerIdx++) {
          unsigned direction = getLayerDirection(layerIdx);
          update(direction, node->x, node->y);
          if ((*node)[direction] > 0)
            update(direction, node->x - 1 + direction, node->y - direction);
        }
      }
    }
  });
}

} // namespace cugr2
