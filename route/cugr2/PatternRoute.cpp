#include "PatternRoute.h"
#include "../stt/flute.h"
#include "../stt/steiner.h"
#include "../util/log.hpp"

#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace cugr2 {

using std::vector;

void SteinerTreeNode::preorder(
    std::shared_ptr<SteinerTreeNode> node,
    std::function<void(std::shared_ptr<SteinerTreeNode>)> visit) {
  visit(node);
  for (auto &child : node->children)
    preorder(child, visit);
}

void PatternRoute::constructSteinerTree() {
  // 1. Select access points
  std::unordered_map<uint64_t, std::pair<sca::PointT<int>, sca::IntervalT<int>>>
      selectedAccessPoints;
  gridGraph.selectAccessPoints(net, selectedAccessPoints);

  // 2. Construct Steiner tree
  const int degree = selectedAccessPoints.size();
  if (degree == 1) {
    for (auto &accessPoint : selectedAccessPoints) {
      steinerTree = std::make_shared<SteinerTreeNode>(
          accessPoint.second.first, accessPoint.second.second);
    }
  } else {
    std::vector<int> xs, ys;
    for (auto &accessPoint : selectedAccessPoints) {
      xs.push_back(accessPoint.second.first.x);
      ys.push_back(accessPoint.second.first.y);
    }
    stt::Tree flutetree = stt::flute(xs, ys, FLUTE_ACCURACY);

    const int numBranches = degree + degree - 2;
    vector<sca::PointT<int>> steinerPoints;
    if (numBranches <= 0) {
        LOG_WARN("numBranches is invalid! numBranches is %d",numBranches);
        if(steinerTree==nullptr)
          LOG_WARN("steinerTree is nullptr!");
          //Case 3 :Preventing segment fault errors
          steinerTree = std::make_shared<SteinerTreeNode>(sca::PointT<int>(0,0)); 
        return;
    }
    steinerPoints.reserve(numBranches);
    vector<vector<int>> adjacentList(numBranches);
    for (int branchIndex = 0; branchIndex < numBranches; branchIndex++) {
      const stt::Branch &branch = flutetree.branch[branchIndex];
      steinerPoints.emplace_back(branch.x, branch.y);
      if (branchIndex == branch.n)
        continue;
      adjacentList[branchIndex].push_back(branch.n);
      adjacentList[branch.n].push_back(branchIndex);
    }
    std::function<void(std::shared_ptr<SteinerTreeNode> &, int, int)>
        constructTree = [&](std::shared_ptr<SteinerTreeNode> &parent,
                            int prevIndex, int curIndex) {
          std::shared_ptr<SteinerTreeNode> current =
              std::make_shared<SteinerTreeNode>(steinerPoints[curIndex]);
          if (parent != nullptr && parent->x == current->x &&
              parent->y == current->y) {
            for (int nextIndex : adjacentList[curIndex]) {
              if (nextIndex == prevIndex)
                continue;
              constructTree(parent, curIndex, nextIndex);
            }
            return;
          }
          // Build subtree
          for (int nextIndex : adjacentList[curIndex]) {
            if (nextIndex == prevIndex)
              continue;
            constructTree(current, curIndex, nextIndex);
          }
          // Set fixed layer interval
          uint64_t hash = gridGraph.hashCell(current->x, current->y);
          if (selectedAccessPoints.find(hash) != selectedAccessPoints.end()) {
            current->fixedLayers = selectedAccessPoints[hash].second;
          }
          // Connect current to parent
          if (parent == nullptr) {
            parent = current;
          } else {
            parent->children.emplace_back(current);
          }
        };
    // Pick a root having degree 1
    int root = 0;
    std::function<bool(int)> hasDegree1 = [&](int index) {
      if (adjacentList[index].size() == 1) {
        int nextIndex = adjacentList[index][0];
        if (steinerPoints[index] == steinerPoints[nextIndex]) {
          return hasDegree1(nextIndex);
        } else {
          return true;
        }
      } else {
        return false;
      }
    };
    for (int i = 0; i < steinerPoints.size(); i++) {
      if (hasDegree1(i)) {
        root = i;
        break;
      }
    }
    constructTree(steinerTree, -1, root);
  }
}

void PatternRoute::constructRoutingDAG() {
  std::function<void(std::shared_ptr<PatternRoutingNode> &,
                     std::shared_ptr<SteinerTreeNode> &)>
      constructDag = [&](std::shared_ptr<PatternRoutingNode> &dstNode,
                         std::shared_ptr<SteinerTreeNode> &steiner) {
        std::shared_ptr<PatternRoutingNode> current =
            std::make_shared<PatternRoutingNode>(*steiner, steiner->fixedLayers,
                                                 numDagNodes++);
        for (auto steinerChild : steiner->children) {
          constructDag(current, steinerChild);
        }
        if (dstNode == nullptr) {
          dstNode = current;
        } else {
          dstNode->children.emplace_back(current);
          constructPaths(dstNode, current);
        }
      };
  constructDag(routingDag, steinerTree);
}

void PatternRoute::constructPaths(std::shared_ptr<PatternRoutingNode> &start,
                                  std::shared_ptr<PatternRoutingNode> &end,
                                  int childIndex) {
  if (childIndex == -1) {
    childIndex = start->paths.size();
    start->paths.emplace_back();
  }
  vector<std::shared_ptr<PatternRoutingNode>> &childPaths =
      start->paths[childIndex];
  if (start->x == end->x || start->y == end->y) {
    childPaths.push_back(end);
  } else {
    for (int pathIndex = 0; pathIndex <= 1;
         pathIndex++) { // two paths of different L-shape
      sca::PointT<int> midPoint = pathIndex
                                      ? sca::PointT<int>(start->x, end->y)
                                      : sca::PointT<int>(end->x, start->y);
      std::shared_ptr<PatternRoutingNode> mid =
          std::make_shared<PatternRoutingNode>(midPoint, numDagNodes++, true);
      mid->paths = {{end}};
      childPaths.push_back(mid);
    }
  }
}

void PatternRoute::constructDetours(GridGraphView<bool> &congestionView) {
  struct ScaffoldNode {
    std::shared_ptr<PatternRoutingNode> node;
    vector<std::shared_ptr<ScaffoldNode>> children;
    ScaffoldNode(std::shared_ptr<PatternRoutingNode> n) : node(n) {}
  };

  vector<vector<std::shared_ptr<ScaffoldNode>>> scaffolds(2);
  vector<vector<std::shared_ptr<ScaffoldNode>>> scaffoldNodes(
      2,
      vector<std::shared_ptr<ScaffoldNode>>(
          numDagNodes, nullptr)); // direction -> numDagNodes -> scaffold node
  vector<bool> visited(numDagNodes, false);

  std::function<void(std::shared_ptr<PatternRoutingNode>)> buildScaffolds =
      [&](std::shared_ptr<PatternRoutingNode> node) {
        if (visited[node->index])
          return;
        visited[node->index] = true;

        if (node->optional) {
          assert(node->paths.size() == 1 && node->paths[0].size() == 1 &&
                 !node->paths[0][0]->optional);
          auto &path = node->paths[0][0];
          buildScaffolds(path);
          unsigned direction =
              (node->y == path->y ? MetalLayer::H : MetalLayer::V);
          if (!scaffoldNodes[direction][path->index] &&
              congestionView.check(*node, *path)) {
            scaffoldNodes[direction][path->index] =
                std::make_shared<ScaffoldNode>(path);
          }
        } else {
          for (auto &childPaths : node->paths) {
            for (auto &path : childPaths) {
              buildScaffolds(path);
              unsigned direction =
                  (node->y == path->y ? MetalLayer::H : MetalLayer::V);
              if (path->optional) {
                if (!scaffoldNodes[direction][node->index] &&
                    congestionView.check(*node, *path)) {
                  scaffoldNodes[direction][node->index] =
                      std::make_shared<ScaffoldNode>(node);
                }
              } else {
                if (congestionView.check(*node, *path)) {
                  if (!scaffoldNodes[direction][node->index]) {
                    scaffoldNodes[direction][node->index] =
                        std::make_shared<ScaffoldNode>(node);
                  }
                  if (!scaffoldNodes[direction][path->index]) {
                    scaffoldNodes[direction][node->index]
                        ->children.emplace_back(
                            std::make_shared<ScaffoldNode>(path));
                  } else {
                    scaffoldNodes[direction][node->index]
                        ->children.emplace_back(
                            scaffoldNodes[direction][path->index]);
                    scaffoldNodes[direction][path->index] = nullptr;
                  }
                }
              }
            }
            for (auto &child : node->children) {
              for (unsigned direction = 0; direction < 2; direction++) {
                if (scaffoldNodes[direction][child->index]) {
                  scaffolds[direction].emplace_back(
                      std::make_shared<ScaffoldNode>(node));
                  scaffolds[direction].back()->children.emplace_back(
                      scaffoldNodes[direction][child->index]);
                  scaffoldNodes[direction][child->index] = nullptr;
                }
              }
            }
          }
        }
      };

  buildScaffolds(routingDag);
  for (unsigned direction = 0; direction < 2; direction++) {
    if (scaffoldNodes[direction][routingDag->index]) {
      scaffolds[direction].emplace_back(
          std::make_shared<ScaffoldNode>(nullptr));
      scaffolds[direction].back()->children.emplace_back(
          scaffoldNodes[direction][routingDag->index]);
    }
  }

  // std::function<void(std::shared_ptr<ScaffoldNode>)> printScaffold =
  //     [&] (std::shared_ptr<ScaffoldNode> scaffoldNode) {
  //
  //         for (auto& scaffoldChild : scaffoldNode->children) {
  //             if (scaffoldNode->node) std::cout <<
  //             (sca::PointT<int>)*scaffoldNode->node << " " <<
  //             scaffoldNode->children.size(); else std::cout << "null";
  //             std::cout << " -> " <<
  //             (sca::PointT<int>)*scaffoldChild->node; if
  //             (scaffoldNode->node) std::cout << " " <<
  //             congested(scaffoldNode->node, scaffoldChild->node); std::cout
  //             << std::endl; printScaffold(scaffoldChild);
  //         }
  //     };
  //
  // for (unsigned direction = 0; direction < 1; direction++) {
  //     for (auto scaffold : scaffolds[direction]) {
  //         printScaffold(scaffold);
  //         std::cout << std::endl;
  //     }
  // }

  std::function<void(std::shared_ptr<ScaffoldNode>, sca::IntervalT<int> &,
                     vector<int> &, unsigned, bool)>
      getTrunkAndStems = [&](std::shared_ptr<ScaffoldNode> scaffoldNode,
                             sca::IntervalT<int> &trunk, vector<int> &stems,
                             unsigned direction, bool starting) {
        if (starting) {
          if (scaffoldNode->node) {
            stems.emplace_back((*scaffoldNode->node)[1 - direction]);
            trunk.Update((*scaffoldNode->node)[direction]);
          }
          for (auto &scaffoldChild : scaffoldNode->children)
            getTrunkAndStems(scaffoldChild, trunk, stems, direction, false);
        } else {
          trunk.Update((*scaffoldNode->node)[direction]);
          if (scaffoldNode->node->fixedLayers.IsValid()) {
            stems.emplace_back((*scaffoldNode->node)[1 - direction]);
          }
          for (auto &treeChild : scaffoldNode->node->children) {
            bool scaffolded = false;
            for (auto &scaffoldChild : scaffoldNode->children) {
              if (treeChild == scaffoldChild->node) {
                getTrunkAndStems(scaffoldChild, trunk, stems, direction, false);
                scaffolded = true;
                break;
              }
            }
            if (!scaffolded) {
              stems.emplace_back((*treeChild)[1 - direction]);
              trunk.Update((*treeChild)[direction]);
            }
          }
        }
      };

  auto getTotalStemLength = [&](const vector<int> &stems, const int pos) {
    int length = 0;
    for (int stem : stems)
      length += abs(stem - pos);
    return length;
  };

  std::function<std::shared_ptr<PatternRoutingNode>(
      std::shared_ptr<ScaffoldNode>, unsigned, int)>
      buildDetour = [&](std::shared_ptr<ScaffoldNode> scaffoldNode,
                        unsigned direction, int shiftAmount) {
        std::shared_ptr<PatternRoutingNode> treeNode = scaffoldNode->node;
        if (treeNode->fixedLayers.IsValid()) {
          std::shared_ptr<PatternRoutingNode> dupTreeNode =
              std::make_shared<PatternRoutingNode>((sca::PointT<int>)*treeNode,
                                                   treeNode->fixedLayers,
                                                   numDagNodes++);
          std::shared_ptr<PatternRoutingNode> shiftedTreeNode =
              std::make_shared<PatternRoutingNode>((sca::PointT<int>)*treeNode,
                                                   numDagNodes++);
          (*shiftedTreeNode)[1 - direction] += shiftAmount;
          constructPaths(shiftedTreeNode, dupTreeNode);
          for (auto &treeChild : treeNode->children) {
            bool built = false;
            for (auto &scaffoldChild : scaffoldNode->children) {
              if (treeChild == scaffoldChild->node) {
                auto shiftedChildTreeNode =
                    buildDetour(scaffoldChild, direction, shiftAmount);
                constructPaths(shiftedTreeNode, shiftedChildTreeNode);
                built = true;
                break;
              }
            }
            if (!built) {
              constructPaths(shiftedTreeNode, treeChild);
            }
          }
          return shiftedTreeNode;
        } else {
          std::shared_ptr<PatternRoutingNode> shiftedTreeNode =
              std::make_shared<PatternRoutingNode>((sca::PointT<int>)*treeNode,
                                                   numDagNodes++);
          (*shiftedTreeNode)[1 - direction] += shiftAmount;
          for (auto &treeChild : treeNode->children) {
            bool built = false;
            for (auto &scaffoldChild : scaffoldNode->children) {
              if (treeChild == scaffoldChild->node) {
                auto shiftedChildTreeNode =
                    buildDetour(scaffoldChild, direction, shiftAmount);
                constructPaths(shiftedTreeNode, shiftedChildTreeNode);
                built = true;
                break;
              }
            }
            if (!built) {
              constructPaths(shiftedTreeNode, treeChild);
            }
          }
          return shiftedTreeNode;
        }
      };

  for (unsigned direction = 0; direction < 2; direction++) {
    for (std::shared_ptr<ScaffoldNode> scaffold : scaffolds[direction]) {
      assert(scaffold->children.size() == 1);

      sca::IntervalT<int> trunk;
      vector<int> stems;
      getTrunkAndStems(scaffold, trunk, stems, direction, true);
      std::sort(stems.begin(), stems.end());
      int trunkPos = (*scaffold->children[0]->node)[1 - direction];
      int originalLength = getTotalStemLength(stems, trunkPos);
      sca::IntervalT<int> shiftInterval(trunkPos);
      int maxLengthIncrease = trunk.range() * parameters.max_detour_ratio;
      while (shiftInterval.low - 1 >= 0 &&
             getTotalStemLength(stems, shiftInterval.low - 1) -
                     originalLength <=
                 maxLengthIncrease)
        shiftInterval.low--;
      while (shiftInterval.high + 1 < gridGraph.getSize(1 - direction) &&
             getTotalStemLength(stems, shiftInterval.high - 1) -
                     originalLength <=
                 maxLengthIncrease)
        shiftInterval.high++;
      int step = 1;
      while ((trunkPos - shiftInterval.low) / (step + 1) +
                 (shiftInterval.high - trunkPos) / (step + 1) >=
             parameters.target_detour_count)
        step++;
      sca::IntervalT<int> dupShiftInterval = shiftInterval;
      shiftInterval.low =
          trunkPos - (trunkPos - shiftInterval.low) / step * step;
      shiftInterval.high =
          trunkPos + (shiftInterval.high - trunkPos) / step * step;
      for (double pos = shiftInterval.low; pos <= shiftInterval.high;
           pos += step) {
        int shiftAmount = (pos - trunkPos);
        if (shiftAmount == 0)
          continue;
        if (scaffold->node) {
          auto &scaffoldChild = scaffold->children[0];
          if ((*scaffoldChild->node)[1 - direction] + shiftAmount < 0 ||
              (*scaffoldChild->node)[1 - direction] + shiftAmount >=
                  gridGraph.getSize(1 - direction)) {
            continue;
          }
          for (int childIndex = 0; childIndex < scaffold->node->children.size();
               childIndex++) {
            auto &treeChild = scaffold->node->children[childIndex];
            if (treeChild == scaffoldChild->node) {
              std::shared_ptr<PatternRoutingNode> shiftedChild =
                  buildDetour(scaffoldChild, direction, shiftAmount);
              constructPaths(scaffold->node, shiftedChild, childIndex);
            }
          }
        } else {
          std::shared_ptr<ScaffoldNode> scaffoldNode = scaffold->children[0];
          auto treeNode = scaffoldNode->node;
          if (treeNode->children.size() == 1) {
            if ((*treeNode)[1 - direction] + shiftAmount < 0 ||
                (*treeNode)[1 - direction] + shiftAmount >=
                    gridGraph.getSize(1 - direction)) {
              continue;
            }
            std::shared_ptr<PatternRoutingNode> shiftedTreeNode =
                std::make_shared<PatternRoutingNode>(
                    (sca::PointT<int>)*treeNode, numDagNodes++);
            (*shiftedTreeNode)[1 - direction] += shiftAmount;
            constructPaths(treeNode, shiftedTreeNode, 0);
            for (auto &treeChild : treeNode->children) {
              bool built = false;
              for (auto &scaffoldChild : scaffoldNode->children) {
                if (treeChild == scaffoldChild->node) {
                  auto shiftedChildTreeNode =
                      buildDetour(scaffoldChild, direction, shiftAmount);
                  constructPaths(shiftedTreeNode, shiftedChildTreeNode);
                  built = true;
                  break;
                }
              }
              if (!built) {
                constructPaths(shiftedTreeNode, treeChild);
              }
            }
          } else {
            LOG_WARN("the root has not exactly one child");
          }
        }
      }
    }
  }
}

void PatternRoute::run() {
  calculateRoutingCosts(routingDag);
  net->setRoutingTree(getRoutingTree(routingDag));
}

void PatternRoute::calculateRoutingCosts(
    std::shared_ptr<PatternRoutingNode> &node) {
  if (node->costs.size() != 0)
    return;
  vector<vector<std::pair<CostT, int>>>
      childCosts; // childIndex -> layerIndex -> (cost, pathIndex)
  // Calculate child costs
  if (node->paths.size() > 0)
    childCosts.resize(node->paths.size());
  for (int childIndex = 0; childIndex < node->paths.size(); childIndex++) {
    auto &childPaths = node->paths[childIndex];
    auto &costs = childCosts[childIndex];
    costs.assign(gridGraph.getNumLayers(),
                 {std::numeric_limits<CostT>::max(), -1});
    for (int pathIndex = 0; pathIndex < childPaths.size(); pathIndex++) {
      std::shared_ptr<PatternRoutingNode> &path = childPaths[pathIndex];
      calculateRoutingCosts(path);
      unsigned direction = node->x == path->x ? MetalLayer::V : MetalLayer::H;
      assert((*node)[1 - direction] == (*path)[1 - direction]);
      for (int layerIndex = parameters.min_routing_layer;
           layerIndex < gridGraph.getNumLayers(); layerIndex++) {
        if (gridGraph.getLayerDirection(layerIndex) != direction)
          continue;
        CostT cost = path->costs[layerIndex] +
                     gridGraph.getWireCost(layerIndex, *node, *path);
        if (cost < costs[layerIndex].first)
          costs[layerIndex] = std::make_pair(cost, pathIndex);
      }
    }
  }

  node->costs.assign(gridGraph.getNumLayers(),
                     std::numeric_limits<CostT>::max());
  node->bestPaths.resize(gridGraph.getNumLayers());
  if (node->paths.size() > 0) {
    for (int layerIndex = 1; layerIndex < gridGraph.getNumLayers();
         layerIndex++) {
      node->bestPaths[layerIndex].assign(node->paths.size(), {-1, -1});
    }
  }
  // Calculate the partial sum of the via costs
  vector<CostT> viaCosts(gridGraph.getNumLayers());
  viaCosts[0] = 0;
  for (int layerIndex = 1; layerIndex < gridGraph.getNumLayers();
       layerIndex++) {
    viaCosts[layerIndex] =
        viaCosts[layerIndex - 1] + gridGraph.getViaCost(layerIndex - 1, *node);
  }
  vector<CostT> nonStackViaCosts(gridGraph.getNumLayers());
  nonStackViaCosts[0] = 0.f;
  for (int layerIndex = 1; layerIndex < gridGraph.getNumLayers(); layerIndex++)
    nonStackViaCosts[layerIndex] =
        nonStackViaCosts[layerIndex - 1] +
        gridGraph.getNonStackViaCost(layerIndex, *node);
  sca::IntervalT<int> fixedLayers = node->fixedLayers;
  fixedLayers.low =
      std::min(fixedLayers.low, static_cast<int>(gridGraph.getNumLayers()) - 1);
  fixedLayers.high = std::max(fixedLayers.high, parameters.min_routing_layer);

  for (int lowLayerIndex = 0; lowLayerIndex <= fixedLayers.low;
       lowLayerIndex++) {
    vector<CostT> minChildCosts;
    vector<std::pair<int, int>> bestPaths;
    if (node->paths.size() > 0) {
      minChildCosts.assign(node->paths.size(),
                           std::numeric_limits<CostT>::max());
      bestPaths.assign(node->paths.size(), {-1, -1});
    }
    for (int layerIndex = lowLayerIndex; layerIndex < gridGraph.getNumLayers();
         layerIndex++) {
      for (int childIndex = 0; childIndex < node->paths.size(); childIndex++) {
        if (childCosts[childIndex][layerIndex].first <
            minChildCosts[childIndex]) {
          minChildCosts[childIndex] = childCosts[childIndex][layerIndex].first;
          bestPaths[childIndex] = std::make_pair(
              childCosts[childIndex][layerIndex].second, layerIndex);
        }
      }
      if (layerIndex >= fixedLayers.high) {
        CostT cost = viaCosts[layerIndex] - viaCosts[lowLayerIndex];
        cost += (layerIndex - lowLayerIndex >= 2)
                    ? (nonStackViaCosts[layerIndex - 1] -
                       nonStackViaCosts[lowLayerIndex])
                    : 0.f;
        for (CostT childCost : minChildCosts)
          cost += childCost;
        if (cost < node->costs[layerIndex]) {
          node->costs[layerIndex] = cost;
          node->bestPaths[layerIndex] = bestPaths;
        }
      }
    }
    for (int layerIndex = gridGraph.getNumLayers() - 2;
         layerIndex >= lowLayerIndex; layerIndex--) {
      if (node->costs[layerIndex + 1] < node->costs[layerIndex]) {
        node->costs[layerIndex] = node->costs[layerIndex + 1];
        node->bestPaths[layerIndex] = node->bestPaths[layerIndex + 1];
      }
    }
  }
}

std::shared_ptr<sca::GRTreeNode>
PatternRoute::getRoutingTree(std::shared_ptr<PatternRoutingNode> &node,
                             int parentLayerIndex) {
  if (parentLayerIndex == -1) {
    CostT minCost = std::numeric_limits<CostT>::max();
    for (int layerIndex = 0; layerIndex < gridGraph.getNumLayers();
         layerIndex++) {
      if (routingDag->costs[layerIndex] < minCost) {
        minCost = routingDag->costs[layerIndex];
        parentLayerIndex = layerIndex;
      }
    }
  }
  std::shared_ptr<sca::GRTreeNode> routingNode =
      std::make_shared<sca::GRTreeNode>(parentLayerIndex, node->x, node->y);
  std::shared_ptr<sca::GRTreeNode> lowestRoutingNode = routingNode;
  std::shared_ptr<sca::GRTreeNode> highestRoutingNode = routingNode;
  if (node->paths.size() > 0) {
    int pathIndex, layerIndex;
    vector<vector<std::shared_ptr<PatternRoutingNode>>> pathsOnLayer(
        gridGraph.getNumLayers());
    for (int childIndex = 0; childIndex < node->paths.size(); childIndex++) {
      std::tie(pathIndex, layerIndex) =
          node->bestPaths[parentLayerIndex][childIndex];
      pathsOnLayer[layerIndex].push_back(node->paths[childIndex][pathIndex]);
    }
    if (pathsOnLayer[parentLayerIndex].size() > 0) {
      for (auto &path : pathsOnLayer[parentLayerIndex]) {
        routingNode->children.push_back(getRoutingTree(path, parentLayerIndex));
      }
    }
    for (int layerIndex = parentLayerIndex - 1; layerIndex >= 0; layerIndex--) {
      if (pathsOnLayer[layerIndex].size() > 0) {
        lowestRoutingNode->children.push_back(
            std::make_shared<sca::GRTreeNode>(layerIndex, node->x, node->y));
        lowestRoutingNode = lowestRoutingNode->children.back();
        for (auto &path : pathsOnLayer[layerIndex]) {
          lowestRoutingNode->children.push_back(
              getRoutingTree(path, layerIndex));
        }
      }
    }
    for (int layerIndex = parentLayerIndex + 1;
         layerIndex < gridGraph.getNumLayers(); layerIndex++) {
      if (pathsOnLayer[layerIndex].size() > 0) {
        highestRoutingNode->children.push_back(
            std::make_shared<sca::GRTreeNode>(layerIndex, node->x, node->y));
        highestRoutingNode = highestRoutingNode->children.back();
        for (auto &path : pathsOnLayer[layerIndex]) {
          highestRoutingNode->children.push_back(
              getRoutingTree(path, layerIndex));
        }
      }
    }
  }
  if (lowestRoutingNode->layerIdx > node->fixedLayers.low) {
    lowestRoutingNode->children.push_back(std::make_shared<sca::GRTreeNode>(
        node->fixedLayers.low, node->x, node->y));
  }
  if (highestRoutingNode->layerIdx < node->fixedLayers.high) {
    highestRoutingNode->children.push_back(std::make_shared<sca::GRTreeNode>(
        node->fixedLayers.high, node->x, node->y));
  }
  return routingNode;
}
} // namespace cugr2
