#include "GridGraph.h"
#include "../util/log.hpp"

#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace cugr2 {

using std::max;
using std::min;
using std::vector;

GridGraph::GridGraph(sca::Design *design, const Parameters &params)
    : m_design(design), parameters(params) {
  sca::Grid *grid = m_design->grid();
  sca::Technology *tech = m_design->technology();
  nLayers = tech->numLayers();
  xSize = grid->sizeX();
  ySize = grid->sizeY();

  layerDirections.resize(nLayers);
  layerMinLengths.resize(nLayers);
  DBU min_length_x = std::numeric_limits<DBU>::max();
  DBU min_length_y = std::numeric_limits<DBU>::max();
  for (int x = 0; x < xSize - 1; x++)
    min_length_x = std::min(min_length_x, grid->edgeLengthX(x));
  for (int y = 0; y < ySize - 1; y++)
    min_length_y = std::min(min_length_y, grid->edgeLengthY(y));
  for (int l = 0; l < nLayers; l++) {
    layerDirections[l] =
        tech->layer(l)->direction() == sca::LayerDirection::Horizontal
            ? MetalLayer::H
            : MetalLayer::V;
    layerMinLengths[l] =
        tech->layer(l)->direction() == sca::LayerDirection::Horizontal
            ? min_length_x
            : min_length_y;
  }

  unit_length_wire_cost = params.unit_length_wire_cost;
  unit_via_cost = params.unit_via_cost;
  // unit_length_short_costs = parser.unit_length_short_costs;
  unit_overflow_costs = params.unit_overflow_costs;

  // horizontal gridlines
  edgeLengths.resize(2);
  edgeLengths[MetalLayer::H].resize(xSize - 1);
  edgeLengths[MetalLayer::V].resize(ySize - 1);
  for (int x = 0; x < xSize - 1; x++) {
    edgeLengths[MetalLayer::H][x] = grid->edgeLengthX(x);
  }
  for (int y = 0; y < xSize - 1; y++) {
    edgeLengths[MetalLayer::H][y] = grid->edgeLengthY(y);
  }

  graphEdges.assign(nLayers,
                    vector<vector<GraphEdge>>(xSize, vector<GraphEdge>(ySize)));
  for (int layerIdx = 0; layerIdx < nLayers; layerIdx++) {
    for (int x = 0; x < xSize; x++)
      for (int y = 0; y < ySize; y++)
        graphEdges[layerIdx][x][y].capacity =
            grid->edgeCapacity(layerIdx, x, y);
  }
  flag.assign(nLayers, vector<vector<bool>>(xSize, vector<bool>(ySize)));
}

CostT GridGraph::getWireCost(const int layerIndex, const sca::PointT<int> lower,
                             const CapacityT demand) const {
  // ----- legacy cost -----
  // unsigned direction = layerDirections[layerIndex];
  // DBU edgeLength = getEdgeLength(direction, lower[direction]);
  // DBU demandLength = demand * edgeLength;
  // const auto &edge = graphEdges[layerIndex][lower.x][lower.y];
  // CostT cost = demandLength * unit_length_wire_cost;
  // cost += demandLength * unit_overflow_costs[layerIndex] * (edge.capacity
  // < 1.0 ? 1.0 : logistic(edge.capacity - edge.demand,
  // parameters.cost_logistic_slope)); return cost;

  // ----- new cost -----
  unsigned direction = layerDirections[layerIndex];
  DBU edgeLength = getEdgeLength(direction, lower[direction]);
  const auto &edge = graphEdges[layerIndex][lower.x][lower.y];
  CostT slope = edge.capacity > 0.f ? 0.5f : 1.5f;
  CostT cost = edgeLength * unit_length_wire_cost +
               unit_overflow_costs[layerIndex] *
                   exp(slope * (edge.demand - edge.capacity)) *
                   (exp(slope) - 1);
  return cost;
}

CostT GridGraph::getWireCost(const int layerIndex, const sca::PointT<int> u,
                             const sca::PointT<int> v) const {
  unsigned direction = layerDirections[layerIndex];
  assert(u[1 - direction] == v[1 - direction]);
  CostT cost = 0;
  if (direction == MetalLayer::H) {
    int l = min(u.x, v.x), h = max(u.x, v.x);
    for (int x = l; x < h; x++)
      cost += getWireCost(layerIndex, {x, u.y});
  } else {
    int l = min(u.y, v.y), h = max(u.y, v.y);
    for (int y = l; y < h; y++)
      cost += getWireCost(layerIndex, {u.x, y});
  }
  return cost;
}

CostT GridGraph::getViaCost(const int layerIndex,
                            const sca::PointT<int> loc) const {
  assert(layerIndex + 1 < nLayers);

  // ----- legacy cost -----
  // CostT cost = unit_via_cost;
  // // Estimated wire cost to satisfy min-area
  // for (int l = layerIndex; l <= layerIndex + 1; l++)
  // {
  //     unsigned direction = layerDirections[l];
  //     sca::PointT<int> lowerLoc = loc;
  //     lowerLoc[direction] -= 1;
  //     DBU lowerEdgeLength = loc[direction] > 0 ? getEdgeLength(direction,
  //     lowerLoc[direction]) : 0; DBU higherEdgeLength = loc[direction] <
  //     getSize(direction) - 1 ? getEdgeLength(direction, loc[direction]) : 0;
  //     CapacityT demand = (CapacityT)layerMinLengths[l] / (lowerEdgeLength +
  //     higherEdgeLength) * parameters.via_multiplier; if (lowerEdgeLength > 0)
  //         cost += getWireCost(l, lowerLoc, demand);
  //     if (higherEdgeLength > 0)
  //         cost += getWireCost(l, loc, demand);
  // }
  // return cost;

  // ----- new cost -----
  return unit_via_cost;
}

CostT GridGraph::getNonStackViaCost(const int layerIndex,
                                    const sca::PointT<int> loc) const {
  // ----- legacy cost -----
  // return 0;

  // ----- new cost -----
  auto [x, y] = loc;
  bool isHorizontal = (layerDirections[layerIndex] == MetalLayer::H);
  if (isHorizontal ? (x == 0) : (y == 0)) {
    const auto &rightEdge = graphEdges[layerIndex][x][y];
    CostT rightSlope = rightEdge.capacity > 0.f ? 0.5f : 1.5f;
    return unit_overflow_costs[layerIndex] *
           std::exp(rightSlope * (rightEdge.demand - rightEdge.capacity)) *
           (std::exp(rightSlope) - 1);
  } else if (isHorizontal ? (x == xSize - 1) : (y == ySize - 1)) {
    const auto &leftEdge =
        graphEdges[layerIndex][x - isHorizontal][y - !isHorizontal];
    CostT leftSlope = leftEdge.capacity > 0.f ? 0.5f : 1.5f;
    return unit_overflow_costs[layerIndex] *
           std::exp(leftSlope * (leftEdge.demand - leftEdge.capacity)) *
           (std::exp(leftSlope) - 1);
  } else {
    const auto &rightEdge = graphEdges[layerIndex][x][y];
    CostT rightSlope = rightEdge.capacity > 0.f ? 0.5f : 1.5f;
    const auto &leftEdge =
        graphEdges[layerIndex][x - isHorizontal][y - !isHorizontal];
    CostT leftSlope = leftEdge.capacity > 0.f ? 0.5f : 1.5f;
    return unit_overflow_costs[layerIndex] *
               std::exp(rightSlope * (rightEdge.demand - rightEdge.capacity)) *
               (std::exp(0.5f * rightSlope) - 1) +
           unit_overflow_costs[layerIndex] *
               std::exp(leftSlope * (leftEdge.demand - leftEdge.capacity)) *
               (std::exp(0.5f * leftSlope) - 1);
  }
}

void GridGraph::selectAccessPoints(
    sca::Net *net,
    std::unordered_map<uint64_t,
                       std::pair<sca::PointT<int>, sca::IntervalT<int>>>
        &selectedAccessPoints) const {
  selectedAccessPoints.clear();
  // cell hash (2d) -> access point, fixed layer interval
  selectedAccessPoints.reserve(net->numPins());
  sca::BoxT<int> bbox;

  for (int i = 0; i < net->numPins(); i++) {
    sca::Pin *pin = net->pin(i);
    std::vector<sca::PointOnLayerT<int>> pts;
    m_design->grid()->computeAccessPoints(pin, pts);
    for (const sca::PointOnLayerT<int> &pt : pts) {
      bbox.Update(pt);
    }
  }
  sca::PointT<int> netCenter(bbox.cx(), bbox.cy());
  for (int i = 0; i < net->numPins(); i++) {
    sca::Pin *pin = net->pin(i);
    std::vector<sca::PointOnLayerT<int>> accessPoints;
    m_design->grid()->computeAccessPoints(pin, accessPoints);
    std::pair<int, int> bestAccessDist = {0, std::numeric_limits<int>::max()};
    int bestIndex = -1;
    for (int index = 0; index < accessPoints.size(); index++) {
      const auto &point = accessPoints[index];
      int accessibility = 0;
      if (point.layerIdx >= parameters.min_routing_layer) {
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
    pin->setPosition(accessPoints[bestIndex]);
    const sca::PointT<int> selectedPoint = accessPoints[bestIndex];
    const uint64_t hash = hashCell(selectedPoint.x, selectedPoint.y);
    if (selectedAccessPoints.find(hash) == selectedAccessPoints.end()) {
      selectedAccessPoints.emplace(
          hash, std::make_pair(selectedPoint, sca::IntervalT<int>()));
    }
    sca::IntervalT<int> &fixedLayerInterval = selectedAccessPoints[hash].second;
    for (const auto &point : accessPoints) {
      if (point.x == selectedPoint.x && point.y == selectedPoint.y) {
        fixedLayerInterval.Update(point.layerIdx);
      }
    }
  }
  // // Extend the fixed layers to 2 layers higher to facilitate track switching
  // for (auto &accessPoint : selectedAccessPoints)
  // {
  //     sca::IntervalT<int> &fixedLayers = accessPoint.second.second;
  //     fixedLayers.high = min(fixedLayers.high + 2, (int)getNumLayers() - 1);
  // }
}

void GridGraph::commitWire(const int layerIndex, const sca::PointT<int> lower,
                           const bool reverse) {
  graphEdges[layerIndex][lower.x][lower.y].demand += (reverse ? -1.f : 1.f);
  unsigned direction = getLayerDirection(layerIndex);
#ifdef CONGESTION_UPDATE
  if (checkOverflow(layerIndex, lower.x, lower.y)) {
    congestionView[direction][lower.x][lower.y] = true;
  }
  // else{
  //     congestionView[direction][lower.x][lower.y] = false;
  // }
#endif
  DBU edgeLength = getEdgeLength(direction, lower[direction]);
  totalLength += (reverse ? -edgeLength : edgeLength);
}

void GridGraph::commitVia(const int layerIndex, const sca::PointT<int> loc,
                          const bool reverse) {
  assert(layerIndex + 1 < nLayers);
  totalNumVias += (reverse ? -1 : 1);
}

void GridGraph::commitNonStackVia(const int layerIndex,
                                  const sca::PointT<int> loc,
                                  const bool reverse) {
  auto isHorizontal = getLayerDirection(layerIndex) == MetalLayer::H;
  auto [x, y] = loc;
  if (isHorizontal ? (x == 0) : (y == 0))
    graphEdges[layerIndex][x][y].demand += (reverse ? -1.f : 1.f);
  else if (isHorizontal ? (x == xSize - 1) : (y == ySize - 1))
    graphEdges[layerIndex][x - isHorizontal][y - !isHorizontal].demand +=
        (reverse ? -1.f : 1.f);
  else {
    graphEdges[layerIndex][x][y].demand += reverse ? -.5f : .5f;
    graphEdges[layerIndex][x - isHorizontal][y - !isHorizontal].demand +=
        (reverse ? -.5f : .5f);
  }
}

// void GridGraph::commitTree(const std::shared_ptr<GRTreeNode> &tree, const
// bool reverse)
// {
//     GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node)
//                          {
//         for (const auto& child : node->children) {
//             if (node->layerIdx == child->layerIdx) {
//                 unsigned direction = layerDirections[node->layerIdx];
//                 if (direction == MetalLayer::H) {
//                     assert(node->y == child->y);
//                     int l = min(node->x, child->x), h = max(node->x,
//                     child->x); for (int x = l; x < h; x++) {
//                         commitWire(node->layerIdx, {x, node->y}, reverse);
//                     }
//                 } else {
//                     assert(node->x == child->x);
//                     int l = min(node->y, child->y), h = max(node->y,
//                     child->y); for (int y = l; y < h; y++) {
//                         commitWire(node->layerIdx, {node->x, y}, reverse);
//                     }
//                 }
//             } else {
//                 int maxLayerIndex = max(node->layerIdx, child->layerIdx);
//                 for (int layerIdx = min(node->layerIdx, child->layerIdx);
//                 layerIdx < maxLayerIndex; layerIdx++) {
//                     if(layerIdx > min(node->layerIdx, child->layerIdx) &&
//                     layerIdx < maxLayerIndex-1){
//                         // non_stack_via
//                         commitNonStackVia(layerIdx,{node->x, node->y},
//                         reverse);
//                     }
//                     commitVia(layerIdx, {node->x, node->y}, reverse);
//                 }
//             }
//         } });
// }

void GridGraph::commitTree(const std::shared_ptr<sca::GRTreeNode> &tree,
                           const bool reverse) {
  // LOG_TRACE("reset flag");
  sca::GRTreeNode::preorder(tree, [&](std::shared_ptr<sca::GRTreeNode> node) {
    for (const auto &child : node->children) {
      if (node->layerIdx == child->layerIdx) { // wires
        if (getLayerDirection(node->layerIdx) == MetalLayer::H) {
          for (int x = min(node->x, child->x), xe = max(node->x, child->x);
               x <= xe; x++)
            flag[node->layerIdx][x][node->y] = false;
        } else {
          for (int y = min(node->y, child->y), ye = max(node->y, child->y);
               y <= ye; y++)
            flag[node->layerIdx][node->x][y] = false;
        }
      } else { // vias
        for (int z = min(node->layerIdx, child->layerIdx),
                 ze = max(node->layerIdx, child->layerIdx);
             z <= ze; z++)
          flag[z][node->x][node->y] = false;
      }
    }
  });
  // LOG_TRACE("commit wire");
  sca::GRTreeNode::preorder(tree, [&](std::shared_ptr<sca::GRTreeNode> node) {
    for (const auto &child : node->children) {
      if (node->layerIdx == child->layerIdx) { // wires
        if (getLayerDirection(node->layerIdx) == MetalLayer::H) {
          for (int x = min(node->x, child->x), xe = max(node->x, child->x);
               x <= xe; x++) {
            if (x < xe)
              commitWire(node->layerIdx, {x, node->y}, reverse);
            flag[node->layerIdx][x][node->y] = true;
          }
        } else {
          for (int y = min(node->y, child->y), ye = max(node->y, child->y);
               y <= ye; y++) {
            if (y < ye)
              commitWire(node->layerIdx, {node->x, y}, reverse);
            flag[node->layerIdx][node->x][y] = true;
          }
        }
      }
    }
  });
  // LOG_TRACE("commit non-stacked via");
  sca::GRTreeNode::preorder(tree, [&](std::shared_ptr<sca::GRTreeNode> node) {
    for (const auto &child : node->children) {
      if (node->layerIdx != child->layerIdx) {
        for (int z = min(node->layerIdx, child->layerIdx),
                 ze = max(node->layerIdx, child->layerIdx);
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

int GridGraph::checkOverflow(const int layerIndex, const sca::PointT<int> u,
                             const sca::PointT<int> v) const {
  int num = 0;
  unsigned direction = layerDirections[layerIndex];
  if (direction == MetalLayer::H) {
    assert(u.y == v.y);
    int l = min(u.x, v.x), h = max(u.x, v.x);
    for (int x = l; x < h; x++) {
      if (checkOverflow(layerIndex, x, u.y))
        num++;
    }
  } else {
    assert(u.x == v.x);
    int l = min(u.y, v.y), h = max(u.y, v.y);
    for (int y = l; y < h; y++) {
      if (checkOverflow(layerIndex, u.x, y))
        num++;
    }
  }
  return num;
}

int GridGraph::checkOverflow(
    const std::shared_ptr<sca::GRTreeNode> &tree) const {
  if (!tree)
    return 0;
  int num = 0;
  sca::GRTreeNode::preorder(tree, [&](std::shared_ptr<sca::GRTreeNode> node) {
    for (auto &child : node->children) {
      // Only check wires
      if (node->layerIdx == child->layerIdx) {
        num += checkOverflow(node->layerIdx, (sca::PointT<int>)*node,
                             (sca::PointT<int>)*child);
      }
    }
  });
  return num;
}

void GridGraph::extractBlockageView(GridGraphView<bool> &view) const {
  view.assign(2, vector<vector<bool>>(xSize, vector<bool>(ySize, true)));
  for (int layerIndex = parameters.min_routing_layer; layerIndex < nLayers;
       layerIndex++) {
    unsigned direction = getLayerDirection(layerIndex);
    for (int x = 0; x < xSize; x++) {
      for (int y = 0; y < ySize; y++) {
        if (getEdge(layerIndex, x, y).capacity >= 1.0) {
          view[direction][x][y] = false;
        }
      }
    }
  }
}

void GridGraph::extractCongestionView(GridGraphView<bool> &view) const {
  view.assign(2, vector<vector<bool>>(xSize, vector<bool>(ySize, false)));
  for (int layerIndex = parameters.min_routing_layer; layerIndex < nLayers;
       layerIndex++) {
    unsigned direction = getLayerDirection(layerIndex);
    for (int x = 0; x < xSize; x++) {
      for (int y = 0; y < ySize; y++) {
        if (checkOverflow(layerIndex, x, y)) {
          view[direction][x][y] = true;
        }
      }
    }
  }
}

// void GridGraph::extractCongestionView() const
// {
//     // view.assign(2, vector<vector<bool>>(xSize, vector<bool>(ySize,
//     false))); for (int layerIndex = parameters.min_routing_layer; layerIndex
//     < nLayers; layerIndex++)
//     {
//         unsigned direction = getLayerDirection(layerIndex);
//         for (int x = 0; x < xSize; x++)
//         {
//             for (int y = 0; y < ySize; y++)
//             {
//                 if (checkOverflow(layerIndex, x, y))
//                 {
//                     congestionView[direction][x][y] = true;
//                 }
//             }
//         }
//     }
// }

void GridGraph::extractWireCostView(GridGraphView<CostT> &view) const {
  view.assign(
      2, vector<vector<CostT>>(
             xSize, vector<CostT>(ySize, std::numeric_limits<CostT>::max())));
  for (unsigned direction = 0; direction < 2; direction++) {
    vector<int> layerIndices;
    CostT unitOverflowCost = std::numeric_limits<CostT>::max();
    for (int layerIndex = parameters.min_routing_layer;
         layerIndex < getNumLayers(); layerIndex++) {
      if (getLayerDirection(layerIndex) == direction) {
        layerIndices.emplace_back(layerIndex);
        unitOverflowCost =
            min(unitOverflowCost, getUnitOverflowCost(layerIndex));
      }
    }
    for (int x = 0; x < xSize; x++) {
      for (int y = 0; y < ySize; y++) {
        int edgeIndex = direction == MetalLayer::H ? x : y;
        if (edgeIndex >= getSize(direction) - 1)
          continue;
        CapacityT capacity = 0;
        CapacityT demand = 0;
        for (int layerIndex : layerIndices) {
          const auto &edge = getEdge(layerIndex, x, y);
          capacity += edge.capacity;
          demand += edge.demand;
        }
        DBU length = getEdgeLength(direction, edgeIndex);
        // ------ legacy cost ------
        // view[direction][x][y] = length * (unit_length_wire_cost +
        // unitOverflowCost * (capacity < 1.0 ? 1.0 : logistic(capacity -
        // demand, parameters.maze_logistic_slope)));

        // ------ new cost ------
        CostT slope = capacity > 0.f ? 0.5f : 1.5f;
        view[direction][x][y] = length * unit_length_wire_cost +
                                50 * unitOverflowCost *
                                    exp(slope * (demand - capacity)) *
                                    (exp(slope) - 1);
      }
    }
  }
}

void GridGraph::updateWireCostView(
    GridGraphView<CostT> &view,
    std::shared_ptr<sca::GRTreeNode> routingTree) const {
  vector<vector<int>> sameDirectionLayers(2);
  vector<CostT> unitOverflowCost(2, std::numeric_limits<CostT>::max());
  for (int layerIndex = parameters.min_routing_layer;
       layerIndex < getNumLayers(); layerIndex++) {
    unsigned direction = getLayerDirection(layerIndex);
    sameDirectionLayers[direction].emplace_back(layerIndex);
    unitOverflowCost[direction] =
        min(unitOverflowCost[direction], getUnitOverflowCost(layerIndex));
  }
  auto update = [&](unsigned direction, int x, int y) {
    int edgeIndex = direction == MetalLayer::H ? x : y;
    if (edgeIndex >= getSize(direction) - 1)
      return;
    CapacityT capacity = 0;
    CapacityT demand = 0;
    for (int layerIndex : sameDirectionLayers[direction]) {
      if (getLayerDirection(layerIndex) != direction)
        continue;
      const auto &edge = getEdge(layerIndex, x, y);
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
    view[direction][x][y] =
        length * unit_length_wire_cost + 50 * unitOverflowCost[direction] *
                                             exp(slope * (demand - capacity)) *
                                             (exp(slope) - 1);
  };
  sca::GRTreeNode::preorder(
      routingTree, [&](std::shared_ptr<sca::GRTreeNode> node) {
        for (const auto &child : node->children) {
          if (node->layerIdx == child->layerIdx) {
            unsigned direction = getLayerDirection(node->layerIdx);
            if (direction == MetalLayer::H) {
              assert(node->y == child->y);
              int l = min(node->x, child->x), h = max(node->x, child->x);
              for (int x = l; x < h; x++) {
                update(direction, x, node->y);
              }
            } else {
              assert(node->x == child->x);
              int l = min(node->y, child->y), h = max(node->y, child->y);
              for (int y = l; y < h; y++) {
                update(direction, node->x, y);
              }
            }
          } else {
            int maxLayerIndex = max(node->layerIdx, child->layerIdx);
            for (int layerIdx = min(node->layerIdx, child->layerIdx);
                 layerIdx < maxLayerIndex; layerIdx++) {
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
