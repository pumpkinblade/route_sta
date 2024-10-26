#include "GlobalRouter.h"
#include "../util/log.hpp"
#include "GridGraph.h"
#include "MazeRoute.h"
#include "PatternRoute.h"
#include <cmath>

using std::vector;

namespace cugr2 {

GlobalRouter::GlobalRouter(GRNetwork *network, GRTechnology *tech,
                           const Parameters &params)
    : m_network(network), m_tech(tech), m_params(params),
      gridGraph(tech, params) {
  unit_length_wire_cost = params.unit_length_wire_cost;
  unit_via_cost = params.unit_via_cost;
  layer_overflow_weight = params.layer_overflow_weight;
}

void GlobalRouter::route() {
  int n1 = 0, n2 = 0, n3 = 0;
  double t1 = 0, t2 = 0, t3 = 0;
  auto t = eplaseTime();

  vector<int> netIndices;
  vector<int> netOverflows(m_network->nets().size(), 0);
  netIndices.reserve(m_network->nets().size());
  for (size_t i = 0; i < m_network->nets().size(); i++)
    netIndices.push_back(i);

  // Stage 1: Pattern routing
  n1 = netIndices.size();
  LOG_TRACE("stage 1: pattern routing");
  PatternRoute::readFluteLUT();
  sortNetIndices(netIndices);
  for (int id : netIndices) {
    GRNet *net = m_network->nets()[id];
    PatternRoute patternRoute(net, gridGraph, m_params);
    patternRoute.constructSteinerTree();
    patternRoute.constructRoutingDAG();
    patternRoute.run();
    gridGraph.commitTree(net->routingTree());
  }
  netIndices.clear();
  for (size_t i = 0; i < m_network->nets().size(); i++) {
    GRNet *net = m_network->nets()[i];
    int netOverflow = gridGraph.checkOverflow(net->routingTree());
    if (netOverflow > 0) {
      netIndices.push_back(i);
      netOverflows[i] = netOverflow;
    }
  }
  LOG_INFO("stage 1: %zu/%zu nets have overflows", netIndices.size(),
           m_network->nets().size());
  t1 = eplaseTime() - t;
  t = eplaseTime();

  // Stage 2: Pattern routing with possible detours
  n2 = netIndices.size();
  LOG_TRACE("stage 2: pattern routing with possible detours");
  if (netIndices.size() > 0) {
    // (2d) direction -> x -> y -> has overflow?
    GridGraphView<bool> congestionView;
    gridGraph.extractCongestionView(congestionView);
    sortNetIndices(netIndices);

    // reverse
    for (int id : netIndices) {
      GRNet *net = m_network->nets()[id];
      gridGraph.commitTree(net->routingTree(), true);
    }

    // route
    for (int id : netIndices) {
      GRNet *net = m_network->nets()[id];
      PatternRoute patternRoute(net, gridGraph, m_params);
      patternRoute.constructSteinerTree();
      patternRoute.constructRoutingDAG();
      patternRoute.constructDetours(congestionView);
      patternRoute.run();
      gridGraph.commitTree(net->routingTree());
    }
    netIndices.clear();
    for (size_t i = 0; i < m_network->nets().size(); i++) {
      GRNet *net = m_network->nets()[i];
      int netOverflow = gridGraph.checkOverflow(net->routingTree());
      if (netOverflow > 0) {
        netIndices.push_back(i);
        netOverflows[i] = netOverflow;
      }
    }
  }
  LOG_INFO("stage 2: %zu/%zu nets have overflows", netIndices.size(),
           m_network->nets().size());
  t2 = eplaseTime() - t;
  t = eplaseTime();

  // Stage 3: maze routing
  n3 = netIndices.size();
  LOG_TRACE("stage 3: maze routing on sparsified routing graph");
  if (netIndices.size() > 0) {
    for (int id : netIndices) {
      GRNet *net = m_network->nets()[id];
      gridGraph.commitTree(net->routingTree(), true);
    }
    GridGraphView<CostT> wireCostView;
    gridGraph.extractWireCostView(wireCostView);

    SparseGrid grid(10, 10, 0, 0);
    for (int id : netIndices) {
      GRNet *net = m_network->nets()[id];

      MazeRoute mazeRoute(net, gridGraph, m_params);
      mazeRoute.constructSparsifiedGraph(wireCostView, grid);
      mazeRoute.run();
      std::shared_ptr<SteinerTreeNode> tree = mazeRoute.getSteinerTree();
      assert(tree != nullptr);

      PatternRoute patternRoute(net, gridGraph, m_params);
      patternRoute.setSteinerTree(tree);
      patternRoute.constructRoutingDAG();
      patternRoute.run();

      gridGraph.commitTree(net->routingTree());
      gridGraph.updateWireCostView(wireCostView, net->routingTree());
      grid.step();
    }
  }
  t3 = eplaseTime() - t;
  t = eplaseTime();

  LOG_INFO("step routed #nets: %d, %d, %d", n1, n2, n3);
  LOG_INFO("step time consumption: %.3f, %.3f, %.3f", t1, t2, t3);
  printStatistics();
}

void GlobalRouter::sortNetIndices(vector<int> &netIndices) const {
  vector<int> halfm_params(m_network->nets().size());
  for (int id : netIndices) {
    GRNet *net = m_network->nets()[id];
    utils::BoxT<int> bbox;
    for (GRPin *pin : net->pins()) {
      for (const GRPoint &pt : pin->accessPoints()) {
        bbox.Update(pt);
      }
    }
    halfm_params[id] = bbox.hp();
  }
  sort(netIndices.begin(), netIndices.end(),
       [&](int lhs, int rhs) { return halfm_params[lhs] < halfm_params[rhs]; });
}

void GlobalRouter::printStatistics() const {
  LOG_INFO("routing statistics: ");
  // wire length and via count
  uint64_t wireLength = 0;
  int viaCount = 0;
  vector<vector<vector<int>>> wireUsage;
  vector<vector<vector<int>>> nonstack_via_counter;
  vector<vector<vector<int>>> flag;
  wireUsage.assign(gridGraph.getNumLayers(),
                   vector<vector<int>>(gridGraph.getSize(0),
                                       vector<int>(gridGraph.getSize(1), 0)));
  nonstack_via_counter.assign(
      gridGraph.getNumLayers(),
      vector<vector<int>>(gridGraph.getSize(0),
                          vector<int>(gridGraph.getSize(1), 0)));
  flag.assign(gridGraph.getNumLayers(),
              vector<vector<int>>(gridGraph.getSize(0),
                                  vector<int>(gridGraph.getSize(1), -1)));
  for (int id = 0; id < m_network->nets().size(); id++) {
    GRNet *net = m_network->nets()[id];
    vector<vector<int>> via_loc;
    if (net->routingTree() == nullptr) {
      LOG_ERROR("null GRTree net (name=%s)", net->name().c_str());
      exit(-1);
    }
    if (net->routingTree()->children.size() == 0) {
      viaCount++;
    }
    GRTreeNode::preorder(
        net->routingTree(), [&](std::shared_ptr<GRTreeNode> node) {
          for (const auto &child : node->children) {
            if (node->layerIdx == child->layerIdx) {
              unsigned direction = gridGraph.getLayerDirection(node->layerIdx);
              int l = std::min((*node)[direction], (*child)[direction]);
              int h = std::max((*node)[direction], (*child)[direction]);
              int r = (*node)[1 - direction];
              for (int c = l; c < h; c++) {
                wireLength += gridGraph.getEdgeLength(direction, c);
                int x = direction == MetalLayer::H ? c : r;
                int y = direction == MetalLayer::H ? r : c;
                wireUsage[node->layerIdx][x][y] += 1;
                flag[node->layerIdx][x][y] = id;
              }
              int x = direction == MetalLayer::H ? h : r;
              int y = direction == MetalLayer::H ? r : h;
              flag[node->layerIdx][x][y] = id;
            } else {
              int minLayerIndex = std::min(node->layerIdx, child->layerIdx);
              int maxLayerIndex = std::max(node->layerIdx, child->layerIdx);
              for (int layerIdx = minLayerIndex; layerIdx < maxLayerIndex;
                   layerIdx++) {
                via_loc.push_back({node->x, node->y, layerIdx});
              }
              viaCount += abs(node->layerIdx - child->layerIdx);
            }
          }
        });
    update_nonstack_via_counter(id, via_loc, flag, nonstack_via_counter);
  }

  // resource
  CapacityT overflow_cost = 0;
  double overflow_slope = 0.5;

  CapacityT minResource = std::numeric_limits<CapacityT>::max();
  GRPoint bottleneck(-1, -1, -1);

  for (int z = m_params.min_routing_layer; z < gridGraph.getNumLayers(); z++) {
    unsigned long long num_overflows = 0;
    unsigned long long total_wl = 0;
    double layer_overflows = 0;
    double overflow = 0;
    unsigned layer_nonstack_via_counter = 0;
    for (int x = 0; x < gridGraph.getSize(0); x++) {
      for (int y = 0; y < gridGraph.getSize(1); y++) {
        layer_nonstack_via_counter += nonstack_via_counter[z][x][y];
        int usage = 2 * wireUsage[z][x][y] + nonstack_via_counter[z][x][y];
        double capacity = std::max(gridGraph.getEdge(z, x, y).capacity, 0.0);
        if (usage > 2 * capacity) {
          num_overflows += usage - 2 * capacity;
        }

        if (capacity > 0) {
          overflow = double(usage) - 2 * double(capacity);
          layer_overflows += exp((overflow / 2) * overflow_slope);
        } else if (capacity == 0 && usage > 0) {
          layer_overflows += exp(1.5 * double(usage) * overflow_slope);
        } else if (capacity < 0) {
          printf("Capacity error (%d, %d, %d)\n", x, y, z);
        }
      }
    }
    overflow_cost += layer_overflows * layer_overflow_weight[z];
    LOG_INFO(
        "layer %d, num_overflows: %llu, layer_overflows: %f, overflow cost: %f",
        z, num_overflows, layer_overflows, overflow_cost);
  }

  double via_cost_scale = 1.0;
  double overflow_cost_scale = 1.0;
  double wireCost = wireLength * unit_length_wire_cost;
  double viaCost = viaCount * unit_via_cost * via_cost_scale;
  double overflowCost = overflow_cost * overflow_cost_scale;
  double totalCost = wireCost + viaCost + overflowCost;

  LOG_INFO("routing result: \nwire cost: %f\nvia cost: %f\noverflow cost: %f\n "
           "total cost: %f",
           wireCost, viaCost, overflowCost, totalCost);
}

void GlobalRouter::update_nonstack_via_counter(
    int net_idx, const std::vector<vector<int>> &via_loc,
    std::vector<std::vector<std::vector<int>>> &flag,
    std::vector<std::vector<std::vector<int>>> &nonstack_via_counter) const {
  for (const auto &pp : via_loc) {
    if (flag[pp[2]][pp[0]][pp[1]] != net_idx) {
      flag[pp[2]][pp[0]][pp[1]] = net_idx;

      int direction = gridGraph.getLayerDirection(pp[2]);
      int size = gridGraph.getSize(direction);
      if (direction == 0) {
        if ((pp[0] > 0) && (pp[0] < size - 1)) {
          nonstack_via_counter[pp[2]][pp[0] - 1][pp[1]]++;
          nonstack_via_counter[pp[2]][pp[0]][pp[1]]++;
        } else if (pp[0] > 0) {
          nonstack_via_counter[pp[2]][pp[0] - 1][pp[1]] += 2;
        } else if (pp[0] < size - 1) {
          nonstack_via_counter[pp[2]][pp[0]][pp[1]] += 2;
        }
      } else if (direction == 1) {
        if ((pp[1] > 0) && (pp[1] < size - 1)) {
          nonstack_via_counter[pp[2]][pp[0]][pp[1] - 1]++;
          nonstack_via_counter[pp[2]][pp[0]][pp[1]]++;
        } else if (pp[1] > 0) {
          nonstack_via_counter[pp[2]][pp[0]][pp[1] - 1] += 2;
        } else if (pp[1] < size - 1) {
          nonstack_via_counter[pp[2]][pp[0]][pp[1]] += 2;
        }
      }
    }
  }
}

} // namespace cugr2
