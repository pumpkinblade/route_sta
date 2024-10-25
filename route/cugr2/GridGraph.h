#pragma once
#include "../object/GRNetwork.hpp"
#include "../object/GRTechnology.hpp"
#include "../object/GRTree.hpp"
#include "cugr2.h"

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
  int getNumLayers() const { return nLayers; }
  int getSize(unsigned dimension) const { return (dimension ? ySize : xSize); }
  std::string getLayerName(int layerIndex) const {
    return layerNames[layerIndex];
  }
  unsigned getLayerDirection(int layerIndex) const {
    return layerDirections[layerIndex];
  }
  DBU getLayerMinLength(int layerIndex) const {
    return layerMinLengths[layerIndex];
  }
  // Utility functions for cost calculation
  CostT getUnitLengthWireCost() const { return unit_length_wire_cost; }
  CostT getUnitViaCost() const { return unit_via_cost; }
  CostT getLayerOverflowWeight(int layerIndex) const {
    return layer_overflow_weight[layerIndex];
  }

  uint64_t hashCell(const GRPoint &pt) const {
    return (static_cast<uint64_t>(pt.layerIdx) * xSize + pt.x) * ySize + pt.y;
  };
  uint64_t hashCell(int x, int y) const {
    return static_cast<uint64_t>(x) * ySize + y;
  }

  // Costs
  GraphEdge getEdge(int layerIndex, int x, int y) const {
    return graphEdges[layerIndex][x][y];
  }
  DBU getEdgeLength(unsigned direction, int edgeIndex) const {
    return edgeLengths[direction][edgeIndex];
  }
  CostT getWireCost(int layerIndex, utils::PointT<int> u,
                    utils::PointT<int> v) const;
  CostT getViaCost(int layerIndex, utils::PointT<int> loc) const;
  CostT getNonStackViaCost(int layerIndex, utils::PointT<int> loc) const;

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
  bool checkOverflow(int layerIndex, int x, int y) const {
    return getEdge(layerIndex, x, y).getResource() < 0.0;
  }
  // check wire overflow
  int checkOverflow(int layerIndex, utils::PointT<int> u,
                    utils::PointT<int> v) const;
  // check routing tree overflow
  int checkOverflow(const std::shared_ptr<GRTreeNode> &tree) const;

  // 2D maps
  void extractBlockageView(GridGraphView<bool> &view) const;
  // 2D overflow look-up table
  void extractCongestionView(GridGraphView<bool> &view) const;
  void extractWireCostView(GridGraphView<CostT> &view) const;
  void updateWireCostView(GridGraphView<CostT> &view,
                          std::shared_ptr<GRTreeNode> routingTree) const;

private:
  // const Parameters &parameters;

  int nLayers;
  int xSize;
  int ySize;
  std::vector<std::string> layerNames;
  std::vector<unsigned> layerDirections;
  std::vector<DBU> layerMinLengths;

  double cost_logistic_slope;
  double maze_logistic_slope;
  double via_multiplier;
  int min_routing_layer;

  // Unit costs
  CostT unit_length_wire_cost;
  CostT unit_via_cost;
  std::vector<CostT> layer_overflow_weight;

  std::vector<std::vector<DBU>> edgeLengths;
  std::vector<std::vector<std::vector<GraphEdge>>> graphEdges;
  // used in commiting routing tree
  std::vector<std::vector<std::vector<bool>>> flag;

  DBU totalLength = 0;
  int totalNumVias = 0;

  inline double logistic(const CapacityT &input, const double slope) const;
  CostT getWireCost(const int layerIndex, const utils::PointT<int> lower,
                    const CapacityT demand = 1.0) const;

  // Methods for updating demands
  void commit(const int layerIndex, const utils::PointT<int> lower,
              const CapacityT demand);
  void commitWire(const int layerIndex, const utils::PointT<int> lower,
                  const bool reverse = false);
  void commitVia(const int layerIndex, const utils::PointT<int> loc,
                 const bool reverse = false);
  void commitNonStackVia(const int layerIndex, const utils::PointT<int> loc,
                         const bool reverse = false);
};

} // namespace cugr2
