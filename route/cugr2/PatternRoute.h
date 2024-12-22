#pragma once
#include "GridGraph.h"

namespace cugr2 {

class SteinerTreeNode : public sca::PointT<int> {
public:
  std::vector<std::shared_ptr<SteinerTreeNode>> children;
  sca::IntervalT<int> fixedLayers;

  SteinerTreeNode(sca::PointT<int> point) : sca::PointT<int>(point) {}
  SteinerTreeNode(sca::PointT<int> point, sca::IntervalT<int> _fixedLayers)
      : sca::PointT<int>(point), fixedLayers(_fixedLayers) {}

  static void
  preorder(std::shared_ptr<SteinerTreeNode> node,
           std::function<void(std::shared_ptr<SteinerTreeNode>)> visit);
};

class PatternRoutingNode : public sca::PointT<int> {
public:
  const int index;
  // int x
  // int y
  std::vector<std::shared_ptr<PatternRoutingNode>> children;
  std::vector<std::vector<std::shared_ptr<PatternRoutingNode>>> paths;
  // childIndex -> pathIndex -> path
  sca::IntervalT<int> fixedLayers;
  // layers that must be visited in order to connect all the pins
  std::vector<CostT> costs; // layerIndex -> cost
  std::vector<std::vector<std::pair<int, int>>> bestPaths;
  // best path for each child; layerIndex -> childIndex -> (pathIndex,
  // layerIndex)
  bool optional;

  PatternRoutingNode(sca::PointT<int> point, int _index, bool _optional = false)
      : sca::PointT<int>(point), index(_index), optional(_optional) {}
  PatternRoutingNode(sca::PointT<int> point, sca::IntervalT<int> _fixedLayers,
                     int _index = 0)
      : sca::PointT<int>(point), fixedLayers(_fixedLayers), index(_index),
        optional(false) {}
};

class PatternRoute {
public:
  PatternRoute(sca::Net *_net, const GridGraph &graph, const Parameters &param)
      : net(_net), gridGraph(graph), parameters(param), numDagNodes(0) {}
  void constructSteinerTree();
  void constructRoutingDAG();
  void constructDetours(GridGraphView<bool> &congestionView);
  void run();
  void setSteinerTree(std::shared_ptr<SteinerTreeNode> tree) {
    steinerTree = tree;
  }
  std::shared_ptr<SteinerTreeNode> getSteinerTree() { return steinerTree; }
  void setRoutingDAG(std::shared_ptr<PatternRoutingNode> dag, int numNodes) {
    routingDag = dag, numDagNodes = numNodes;
  }

  std::shared_ptr<SteinerTreeNode> getsT() { return steinerTree; }
  std::shared_ptr<PatternRoutingNode> getrT() { return routingDag; }

  sca::Net *getScaNet() { return net; }

private:
  const Parameters &parameters;
  const GridGraph &gridGraph;
  sca::Net *net;
  int numDagNodes;
  std::shared_ptr<SteinerTreeNode> steinerTree;
  std::shared_ptr<PatternRoutingNode> routingDag;

  void constructPaths(std::shared_ptr<PatternRoutingNode> &start,
                      std::shared_ptr<PatternRoutingNode> &end,
                      int childIndex = -1);
  void calculateRoutingCosts(std::shared_ptr<PatternRoutingNode> &node);
  std::shared_ptr<sca::GRTreeNode>
  getRoutingTree(std::shared_ptr<PatternRoutingNode> &node,
                 int parentLayerIndex = -1);
};

} // namespace cugr2
