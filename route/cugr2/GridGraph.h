#pragma once

#include "../object/GRNetwork.hpp"
#include "../object/GRTechnology.hpp"
#include "base.h"

namespace cugr2 {

struct GraphEdge {
  CapacityT capacity;
  CapacityT demand;
  GraphEdge() : capacity(0), demand(0) {}
  CapacityT getResource() const { return capacity - demand; }
};

template <typename Type>
class GridGraphView : public std::vector<std::vector<std::vector<Type>>> {
public:
  bool check(const utils::PointT<int> &u, const utils::PointT<int> &v) const {
    assert(u.x == v.x || u.y == v.y);
    if (u.y == v.y) {
      int l = std::min(u.x, v.x), h = std::max(u.x, v.x);
      for (int x = l; x < h; x++) {
        if ((*this)[MetalLayer::H][x][u.y])
          return true;
      }
    } else {
      int l = std::min(u.y, v.y), h = std::max(u.y, v.y);
      for (int y = l; y < h; y++) {
        if ((*this)[MetalLayer::V][u.x][y])
          return true;
      }
    }
    return false;
  }

  Type sum(const utils::PointT<int> &u, const utils::PointT<int> &v) const {
    assert(u.x == v.x || u.y == v.y);
    Type res = 0;
    if (u.y == v.y) {
      int l = std::min(u.x, v.x), h = std::max(u.x, v.x);
      for (int x = l; x < h; x++) {
        res += (*this)[MetalLayer::H][x][u.y];
      }
    } else {
      int l = std::min(u.y, v.y), h = std::max(u.y, v.y);
      for (int y = l; y < h; y++) {
        res += (*this)[MetalLayer::V][u.x][y];
      }
    }
    return res;
  }
};

class GridGraph {
public:
  GridGraph(const GRTechnology *tech, const Parameters &params);
  GridGraph(const GridGraph &) = delete;
  GridGraph &operator=(const GridGraph &) = delete;

  inline unsigned getNumLayers() const { return nLayers; }
  inline unsigned getSize(unsigned dimension) const {
    return (dimension ? ySize : xSize);
  }
  inline unsigned getLayerDirection(int layerIndex) const {
    return layerDirections[layerIndex];
  }
  inline DBU getLayerMinLength(int layerIndex) const {
    return layerMinLengths[layerIndex];
  }
  // Utility functions for cost calculation
  inline CostT getUnitLengthWireCost() const { return unit_length_wire_cost; }
  inline CostT getUnitViaCost() const { return unit_via_cost; }
  inline CostT getUnitOverflowCost(const int layerIndex) const {
    return unit_overflow_costs[layerIndex];
  }

  inline uint64_t hashCell(const GRPoint &point) const {
    return ((uint64_t)point.layerIdx * xSize + point.x) * ySize + point.y;
  };
  inline uint64_t hashCell(const int x, const int y) const {
    return (uint64_t)x * ySize + y;
  }

  inline GraphEdge getEdge(const int layerIndex, const int x,
                           const int y) const {
    return graphEdges[layerIndex][x][y];
  }

  // Costs
  DBU getEdgeLength(unsigned direction, unsigned edgeIndex) const {
    return edgeLengths[direction][edgeIndex];
  }
  CostT getWireCost(const int layerIndex, const utils::PointT<int> u,
                    const utils::PointT<int> v) const;
  CostT getViaCost(const int layerIndex, const utils::PointT<int> loc) const;
  CostT getNonStackViaCost(const int layerIndex,
                           const utils::PointT<int> loc) const;

  // Misc
  void selectAccessPoints(
      GRNet *net,
      std::unordered_map<uint64_t,
                         std::pair<utils::PointT<int>, utils::IntervalT<int>>>
          &selectedAccessPoints) const;

  // Methods for updating demands
  void commitTree(const std::shared_ptr<GRTreeNode> &tree,
                  const bool reverse = false);

  // Checks
  inline bool checkOverflow(const int layerIndex, const int x,
                            const int y) const {
    return getEdge(layerIndex, x, y).getResource() < 0.0;
  }
  int checkOverflow(const int layerIndex, const utils::PointT<int> u,
                    const utils::PointT<int> v) const; // Check wire overflow
  int checkOverflow(const std::shared_ptr<GRTreeNode> &tree)
      const; // Check routing tree overflow (Only wires are checked)

  // 2D maps
  void extractBlockageView(GridGraphView<bool> &view) const;
  void extractCongestionView(
      GridGraphView<bool> &view) const; // 2D overflow look-up table
  void extractWireCostView(GridGraphView<CostT> &view) const;
  void updateWireCostView(GridGraphView<CostT> &view,
                          std::shared_ptr<GRTreeNode> routingTree) const;

  void clearDemand() {
    totalLength = 0;
    totalNumVias = 0;
    for (int layerIdx = 0; layerIdx < nLayers; layerIdx++) {
      for (int x = 0; x < xSize; x++)
        for (int y = 0; y < ySize; y++)
          graphEdges[layerIdx][x][y].demand = 0;
    }
  };

private:
  const Parameters &parameters;

  unsigned nLayers;
  unsigned xSize;
  unsigned ySize;
  std::vector<std::vector<DBU>> edgeLengths;
  std::vector<std::string> layerNames;
  std::vector<unsigned> layerDirections;
  std::vector<DBU> layerMinLengths;

  // Unit costs
  CostT unit_length_wire_cost;
  CostT unit_via_cost;
  std::vector<CostT> unit_overflow_costs;

  DBU totalLength = 0;
  int totalNumVias = 0;
  std::vector<std::vector<std::vector<GraphEdge>>> graphEdges;
  // gridEdges[l][x][y] stores the edge {(l, x, y), (l, x+1, y)} or {(l, x, y),
  // (l, x, y+1)} depending on the routing direction of the layer

  // used in commiting routing tree
  std::vector<std::vector<std::vector<bool>>> flag;

  inline double logistic(const CapacityT &input, const double slope) const;
  CostT getWireCost(const int layerIndex, const utils::PointT<int> lower,
                    const CapacityT demand = 1.0) const;

  // Methods for updating demands
  void commitWire(const int layerIndex, const utils::PointT<int> lower,
                  const bool reverse = false);
  void commitVia(const int layerIndex, const utils::PointT<int> loc,
                 const bool reverse = false);
  void commitNonStackVia(const int layerIndex, const utils::PointT<int> loc,
                         const bool reverse);
};

} // namespace cugr2
