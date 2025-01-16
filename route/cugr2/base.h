#pragma once

#include "../object/Helper.hpp"
#include <cmath>
#include <vector>

namespace cugr2 {

using CapacityT = double;
using CostT = double;
using DBU = sca::DBU;

struct MetalLayer {
  static constexpr unsigned H = 0;
  static constexpr unsigned V = 1;
};

struct Parameters {
  int threads;

  double unit_length_wire_cost;
  double unit_via_cost;
  std::vector<double> unit_overflow_costs;
  
  /*IrisLin*/
  std::vector<int> netOrder;
  /*IrisLin*/

  int min_routing_layer;

  double cost_logistic_slope;
  double maze_logistic_slope;
  double via_multiplier;
  int target_detour_count;
  double max_detour_ratio;
};

inline double logistic(double input, double slope) {
  return 1.0 / (1.0 + std::exp(input * slope));
}

} // namespace cugr2
