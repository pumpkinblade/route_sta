#pragma once

#include "GridGraph.h"

namespace cugr2 {

class GlobalRouter {
public:
  GlobalRouter(sca::Design *design, const Parameters &params);
  GlobalRouter(const GlobalRouter &) = delete;
  GlobalRouter &operator=(const GlobalRouter &) = delete;

  void route();

private:
  sca::Design *m_design;
  Parameters parameters;
  GridGraph gridGraph;

  int areaOfPinPatches;
  int areaOfWirePatches;

  int numofThreads;

  // for evaluation
  CostT unit_length_wire_cost;
  CostT unit_via_cost;
  std::vector<CostT> unit_overflow_costs;

  /*IrisLin*/
  std::vector<int> netOrder;
  /*IrisLin*/

  void printStatistics() const;
  void update_nonstack_via_counter(
      unsigned net_idx, const std::vector<std::vector<int>> &via_loc,
      std::vector<std::vector<std::vector<int>>> &flag,
      std::vector<std::vector<std::vector<int>>> &nonstack_via_counter) const;
};

} // namespace cugr2
