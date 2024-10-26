#pragma once
#include "GridGraph.h"

namespace cugr2 {

class GlobalRouter {
public:
  GlobalRouter(GRNetwork *network, GRTechnology *tech,
               const Parameters &params);
  GlobalRouter(const GlobalRouter &) = delete;
  GlobalRouter &operator=(const GlobalRouter &) = delete;

  void route();

  void update_nonstack_via_counter(
      int net_idx, const std::vector<std::vector<int>> &via_loc,
      std::vector<std::vector<std::vector<int>>> &flag,
      std::vector<std::vector<std::vector<int>>> &nonstack_via_counter) const;

private:
  GRNetwork *m_network;
  GRTechnology *m_tech;
  const Parameters &m_params;
  GridGraph gridGraph;

  int areaOfPinPatches;
  int areaOfWirePatches;

  // for evaluation
  CostT unit_length_wire_cost;
  CostT unit_via_cost;
  std::vector<CostT> layer_overflow_weight;

  void sortNetIndices(std::vector<int> &netIndices) const;
  void printStatistics() const;
};

} // namespace cugr2
