#include "GlobalRouter.h"
#include "../util/log.hpp"
#include "MazeRoute.h"
#include "PatternRoute.h"

#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace cugr2 {

using std::max;
using std::min;
using std::vector;

GlobalRouter::GlobalRouter(sca::Design *design, const Parameters &params)
    : m_design(design), parameters(params), gridGraph(design, params) {
  numofThreads = params.threads;
  unit_length_wire_cost = params.unit_length_wire_cost;
  unit_via_cost = params.unit_via_cost;
  unit_overflow_costs = params.unit_overflow_costs;
}

void GlobalRouter::route() {
  int n1 = 0, n2 = 0, n3 = 0;
  double t1 = 0, t2 = 0, t3 = 0;

  auto t = eplaseTime();
  // std::ofstream  afile;
  // afile.open("time", std::ios::app);

  vector<int> netIndices(m_design->numNets());
  vector<int> netOverflows(m_design->numNets(), 0);
  for (int i = 0; i < m_design->numNets(); i++) {
    netIndices[i] = i;
  }

  // Stage 1: Pattern routing
  LOG_TRACE("stage 1: pattern routing");
  n1 = netIndices.size();
  gridGraph.clearDemand();
  for (const int netIndex : netIndices) {
    PatternRoute patternRoute(m_design->net(netIndex), gridGraph, parameters);
    patternRoute.constructSteinerTree();
    patternRoute.constructRoutingDAG();
    patternRoute.run();
    gridGraph.commitTree(m_design->net(netIndex)->routingTree());
  }
  netIndices.clear();
  for (int i = 0; i < m_design->numNets(); i++) {
    sca::Net *net = m_design->net(i);
    int netOverflow = gridGraph.checkOverflow(net->routingTree());
    if (netOverflow > 0) {
      netIndices.push_back(i);
      netOverflows[i] = netOverflow;
    }
  }
  LOG_TRACE("stage 1: %zu/%i nets have overflows", netIndices.size(),
            m_design->numNets());
  t1 = eplaseTime() - t;
  t = eplaseTime();

  // Stage 2: Pattern routing with possible detours
  LOG_TRACE("stage 2: pattern routing with detour");
  n2 = netIndices.size();
  // if (netIndices.size() > 0) {
  //   GridGraphView<bool>
  //       congestionView; // (2d) direction -> x -> y -> has overflow?
  //   gridGraph.extractCongestionView(congestionView);
  //   for (const int netIndex : netIndices) {
  //     gridGraph.commitTree(m_design->net(netIndex)->routingTree(), true);
  //   }
  //   for (const int netIndex : netIndices) {
  //     PatternRoute patternRoute(m_design->net(netIndex), gridGraph,
  //     parameters); patternRoute.constructSteinerTree();
  //     patternRoute.constructRoutingDAG();
  //     patternRoute.constructDetours(congestionView);
  //     patternRoute.run();
  //     gridGraph.commitTree(m_design->net(netIndex)->routingTree());
  //   }
  //   netIndices.clear();
  //   for (int i = 0; i < m_design->numNets(); i++) {
  //     sca::Net *net = m_design->net(i);
  //     int netOverflow = gridGraph.checkOverflow(net->routingTree());
  //     if (netOverflow > 0) {
  //       netIndices.push_back(i);
  //       netOverflows[i] = netOverflow;
  //     }
  //   }
  // }
  LOG_TRACE("stage 2: %zu/%i nets have overflows", netIndices.size(),
            m_design->numNets());
  t2 = eplaseTime() - t;
  t = eplaseTime();

  // Stage 3: maze routing
  LOG_TRACE("stage 3: maze routing");
  n3 = netIndices.size();
  // if (netIndices.size() > 0) {
  //   for (const int netIndex : netIndices) {
  //     sca::Net *net = m_design->net(netIndex);
  //     gridGraph.commitTree(net->routingTree(), true);
  //   }
  //   GridGraphView<CostT> wireCostView;
  //   gridGraph.extractWireCostView(wireCostView);
  //   SparseGrid grid(10, 10, 0, 0);
  //   for (const int netIndex : netIndices) {
  //     sca::Net *net = m_design->net(netIndex);
  //     // gridGraph.commitTree(net.getRoutingTree(), true);
  //     // gridGraph.updateWireCostView(wireCostView, net.getRoutingTree());
  //     MazeRoute mazeRoute(net, gridGraph, parameters);
  //     mazeRoute.constructSparsifiedGraph(wireCostView, grid);
  //     mazeRoute.run();
  //     std::shared_ptr<SteinerTreeNode> tree = mazeRoute.getSteinerTree();
  //     assert(tree != nullptr);

  //     PatternRoute patternRoute(net, gridGraph, parameters);
  //     patternRoute.setSteinerTree(tree);
  //     patternRoute.constructRoutingDAG();
  //     patternRoute.run();

  //     gridGraph.commitTree(net->routingTree());
  //     gridGraph.updateWireCostView(wireCostView, net->routingTree());
  //     grid.step();
  //   }
  //   netIndices.clear();
  //   for (int i = 0; i < m_design->numNets(); i++) {
  //     sca::Net *net = m_design->net(i);
  //     int netOverflow = gridGraph.checkOverflow(net->routingTree());
  //     if (netOverflow > 0) {
  //       netIndices.push_back(i);
  //       netOverflows[i] = netOverflow;
  //     }
  //   }
  // }
  LOG_TRACE("stage 3: %zu/%i nets have overflows", netIndices.size(),
            m_design->numNets());
  t3 = eplaseTime() - t;
  t = eplaseTime();

  LOG_TRACE("step routed #nets: %d, %d, %d", n1, n2, n3);
  LOG_TRACE("step time: %.3f %.3f %.3f", t1, t2, t3);
  printStatistics();
}

void GlobalRouter::printStatistics() const {
  LOG_INFO("routing statistics");

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
  for (int id = 0; id < m_design->numNets(); id++) {
    sca::Net *net = m_design->net(id);
    vector<vector<int>> via_loc;
    if (net->routingTree() == nullptr) {
      LOG_ERROR("null GRTree net(id = %d)", id);
      exit(-1);
    }
    if (net->routingTree()->children.size() == 0) {
      viaCount++;
    }
    sca::GRTreeNode::preorder(
        net->routingTree(), [&](std::shared_ptr<sca::GRTreeNode> node) {
          for (const auto &child : node->children) {
            if (node->layerIdx == child->layerIdx) {
              unsigned direction = gridGraph.getLayerDirection(node->layerIdx);
              int l = min((*node)[direction], (*child)[direction]);
              int h = max((*node)[direction], (*child)[direction]);
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
              int minLayerIndex = min(node->layerIdx, child->layerIdx);
              int maxLayerIndex = max(node->layerIdx, child->layerIdx);
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
  sca::PointOnLayerT<int> bottleneck(-1, -1, -1);

  for (unsigned z = parameters.min_routing_layer; z < gridGraph.getNumLayers();
       z++) {
    unsigned long long num_overflows = 0;
    unsigned long long total_wl = 0;
    double layer_overflows = 0;
    double overflow = 0;
    unsigned layer_nonstack_via_counter = 0;
    for (unsigned x = 0; x < gridGraph.getSize(0); x++) {
      for (unsigned y = 0; y < gridGraph.getSize(1); y++) {
        layer_nonstack_via_counter += nonstack_via_counter[z][x][y];
        int usage = 2 * wireUsage[z][x][y] + nonstack_via_counter[z][x][y];
        double capacity = max(gridGraph.getEdge(z, x, y).capacity, 0.0);
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
    overflow_cost += layer_overflows * unit_overflow_costs[z];
    std::printf("Layer = %d, num_overflows = %llu, layer_overflows = %f, "
                "overflow_cost = %f\n",
                z, num_overflows, layer_overflows, overflow_cost);
  }

  double via_cost_scale = 1.0;
  double overflow_cost_scale = 1.0;
  double wireCost = wireLength * unit_length_wire_cost;
  double viaCost = viaCount * unit_via_cost * via_cost_scale;
  double overflowCost = overflow_cost * overflow_cost_scale;
  double totalCost = wireCost + viaCost + overflowCost;

  std::printf(
      "wire cost: %f\nvia cost: %f\noverflow cost: %f\ntotal cost: %f\n",
      wireCost, viaCost, overflowCost, totalCost);
  LOG_INFO("===============");
}

void GlobalRouter::update_nonstack_via_counter(
    unsigned net_idx, const std::vector<vector<int>> &via_loc,
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
