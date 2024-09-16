#include "GridGraph.h"
#include "GRNet.h"
#include "../utils/utils.h"
#include <fstream>

using std::max;
using std::min;
using std::vector;

GridGraph::GridGraph(const ISPD24Parser &parser, const Parameters &params)
    : parameters(params)
{
    nLayers = parser.n_layers;
    xSize = parser.size_x;
    ySize = parser.size_y;

    layerNames = parser.layer_names;
    layerDirections = parser.layer_directions;
    layerMinLengths = parser.layer_min_lengths;

    unit_length_wire_cost = parser.unit_length_wire_cost;
    unit_via_cost = parser.unit_via_cost;
    // unit_length_short_costs = parser.unit_length_short_costs;
    unit_overflow_costs = parser.unit_overflow_costs;

    // horizontal gridlines
    edgeLengths.resize(2);
    edgeLengths[MetalLayer::H] = parser.horizontal_gcell_edge_lengths;
    edgeLengths[MetalLayer::V] = parser.vertical_gcell_edge_lengths;

    graphEdges.assign(nLayers, vector<vector<GraphEdge>>(xSize, vector<GraphEdge>(ySize)));
    for (int layerIdx = 0; layerIdx < nLayers; layerIdx++)
    {
        constexpr unsigned int xBlock = 32;
        constexpr unsigned int yBlock = 32;
        for (int x = 0; x < xSize; x += xBlock)
            for (int y = 0; y < ySize; y += yBlock)
                for (int xx = x; xx < std::min(xSize, x + xBlock); xx++)
                    for (int yy = y; yy < std::min(ySize, y + yBlock); yy++)
                        graphEdges[layerIdx][xx][yy].capacity = parser.gcell_edge_capaicty[layerIdx][yy][xx];
    }
    congestionView.assign(2, vector<vector<bool>>(xSize, vector<bool>(ySize, false)));
    flag.assign(nLayers, vector<vector<bool>>(xSize, vector<bool>(ySize)));
}

inline double GridGraph::logistic(const CapacityT &input, const double slope) const
{
    return 1.0 / (1.0 + exp(input * slope));
}

CostT GridGraph::getWireCost(const int layerIndex, const utils::PointT<int> lower, const CapacityT demand) const
{
    // ----- legacy cost -----
    // unsigned direction = layerDirections[layerIndex];
    // DBU edgeLength = getEdgeLength(direction, lower[direction]);
    // DBU demandLength = demand * edgeLength;
    // const auto &edge = graphEdges[layerIndex][lower.x][lower.y];
    // CostT cost = demandLength * unit_length_wire_cost;
    // cost += demandLength * unit_overflow_costs[layerIndex] * (edge.capacity < 1.0 ? 1.0 : logistic(edge.capacity - edge.demand, parameters.cost_logistic_slope));
    // return cost;

    // ----- new cost -----
    unsigned direction = layerDirections[layerIndex];
    DBU edgeLength = getEdgeLength(direction, lower[direction]);
    const auto &edge = graphEdges[layerIndex][lower.x][lower.y];
    CostT slope = edge.capacity > 0.f ? 0.5f : 1.5f;
    CostT cost = edgeLength * unit_length_wire_cost +
                 unit_overflow_costs[layerIndex] * exp(slope * (edge.demand - edge.capacity)) * (exp(slope) - 1);
    return cost;
}

CostT GridGraph::getWireCost(const int layerIndex, const utils::PointT<int> u, const utils::PointT<int> v) const
{
    unsigned direction = layerDirections[layerIndex];
    assert(u[1 - direction] == v[1 - direction]);
    CostT cost = 0;
    if (direction == MetalLayer::H)
    {
        int l = min(u.x, v.x), h = max(u.x, v.x);
        for (int x = l; x < h; x++)
            cost += getWireCost(layerIndex, {x, u.y});
    }
    else
    {
        int l = min(u.y, v.y), h = max(u.y, v.y);
        for (int y = l; y < h; y++)
            cost += getWireCost(layerIndex, {u.x, y});
    }
    return cost;
}

CostT GridGraph::getViaCost(const int layerIndex, const utils::PointT<int> loc) const
{
    assert(layerIndex + 1 < nLayers);

    // ----- legacy cost -----
    // CostT cost = unit_via_cost;
    // // Estimated wire cost to satisfy min-area
    // for (int l = layerIndex; l <= layerIndex + 1; l++)
    // {
    //     unsigned direction = layerDirections[l];
    //     utils::PointT<int> lowerLoc = loc;
    //     lowerLoc[direction] -= 1;
    //     DBU lowerEdgeLength = loc[direction] > 0 ? getEdgeLength(direction, lowerLoc[direction]) : 0;
    //     DBU higherEdgeLength = loc[direction] < getSize(direction) - 1 ? getEdgeLength(direction, loc[direction]) : 0;
    //     CapacityT demand = (CapacityT)layerMinLengths[l] / (lowerEdgeLength + higherEdgeLength) * parameters.via_multiplier;
    //     if (lowerEdgeLength > 0)
    //         cost += getWireCost(l, lowerLoc, demand);
    //     if (higherEdgeLength > 0)
    //         cost += getWireCost(l, loc, demand);
    // }
    // return cost;

    // ----- new cost -----
    return unit_via_cost;
}

CostT GridGraph::getNonStackViaCost(const int layerIndex, const utils::PointT<int> loc) const
{
    // ----- legacy cost -----
    // return 0;

    // ----- new cost -----
    auto [x, y] = loc;
    bool isHorizontal = (layerDirections[layerIndex] == MetalLayer::H);
    if (isHorizontal ? (x == 0) : (y == 0))
    {
        const auto &rightEdge = graphEdges[layerIndex][x][y];
        CostT rightSlope = rightEdge.capacity > 0.f ? 0.5f : 1.5f;
        return unit_overflow_costs[layerIndex] * std::exp(rightSlope * (rightEdge.demand - rightEdge.capacity)) * (std::exp(rightSlope) - 1);
    }
    else if (isHorizontal ? (x == xSize - 1) : (y == ySize - 1))
    {
        const auto &leftEdge = graphEdges[layerIndex][x - isHorizontal][y - !isHorizontal];
        CostT leftSlope = leftEdge.capacity > 0.f ? 0.5f : 1.5f;
        return unit_overflow_costs[layerIndex] * std::exp(leftSlope * (leftEdge.demand - leftEdge.capacity)) * (std::exp(leftSlope) - 1);
    }
    else
    {
        const auto &rightEdge = graphEdges[layerIndex][x][y];
        CostT rightSlope = rightEdge.capacity > 0.f ? 0.5f : 1.5f;
        const auto &leftEdge = graphEdges[layerIndex][x - isHorizontal][y - !isHorizontal];
        CostT leftSlope = leftEdge.capacity > 0.f ? 0.5f : 1.5f;
        return unit_overflow_costs[layerIndex] * std::exp(rightSlope * (rightEdge.demand - rightEdge.capacity)) * (std::exp(0.5f * rightSlope) - 1) +
               unit_overflow_costs[layerIndex] * std::exp(leftSlope * (leftEdge.demand - leftEdge.capacity)) * (std::exp(0.5f * leftSlope) - 1);
    }
}

void GridGraph::selectAccessPoints(GRNet &net, robin_hood::unordered_map<uint64_t, std::pair<utils::PointT<int>, utils::IntervalT<int>>> &selectedAccessPoints) const
{
    selectedAccessPoints.clear();
    // cell hash (2d) -> access point, fixed layer interval
    selectedAccessPoints.reserve(net.getNumPins());
    const auto &boundingBox = net.getBoundingBox();
    utils::PointT<int> netCenter(boundingBox.cx(), boundingBox.cy());
    for (const auto &accessPoints : net.getPinAccessPoints())
    {
        std::pair<int, int> bestAccessDist = {0, std::numeric_limits<int>::max()};
        int bestIndex = -1;
        for (int index = 0; index < accessPoints.size(); index++)
        {
            const auto &point = accessPoints[index];
            int accessibility = 0;
            if (point.layerIdx >= parameters.min_routing_layer)
            {
                unsigned direction = getLayerDirection(point.layerIdx);
                accessibility += getEdge(point.layerIdx, point.x, point.y).capacity >= 1;
                if (point[direction] > 0)
                {
                    auto lower = point;
                    lower[direction] -= 1;
                    accessibility += getEdge(lower.layerIdx, lower.x, lower.y).capacity >= 1;
                }
            }
            else
            {
                accessibility = 1;
            }
            int distance = abs(netCenter.x - point.x) + abs(netCenter.y - point.y);
            if (accessibility > bestAccessDist.first || (accessibility == bestAccessDist.first && distance < bestAccessDist.second))
            {
                bestIndex = index;
                bestAccessDist = {accessibility, distance};
            }
        }
        if (bestAccessDist.first == 0)
        {
            log() << "Warning: the pin is hard to access." << std::endl;
        }
        const utils::PointT<int> selectedPoint = accessPoints[bestIndex];
        const uint64_t hash = hashCell(selectedPoint.x, selectedPoint.y);
        if (selectedAccessPoints.find(hash) == selectedAccessPoints.end())
        {
            selectedAccessPoints.emplace(hash, std::make_pair(selectedPoint, utils::IntervalT<int>()));
        }
        utils::IntervalT<int> &fixedLayerInterval = selectedAccessPoints[hash].second;
        for (const auto &point : accessPoints)
        {
            if (point.x == selectedPoint.x && point.y == selectedPoint.y)
            {
                fixedLayerInterval.Update(point.layerIdx);
            }
        }
    }
    // // Extend the fixed layers to 2 layers higher to facilitate track switching
    // for (auto &accessPoint : selectedAccessPoints)
    // {
    //     utils::IntervalT<int> &fixedLayers = accessPoint.second.second;
    //     fixedLayers.high = min(fixedLayers.high + 2, (int)getNumLayers() - 1);
    // }
}

void GridGraph::commitWire(const int layerIndex, const utils::PointT<int> lower, const bool reverse)
{
    graphEdges[layerIndex][lower.x][lower.y].demand += (reverse ? -1.f : 1.f);
    unsigned direction = getLayerDirection(layerIndex);
#ifdef CONGESTION_UPDATE
    if(checkOverflow(layerIndex, lower.x, lower.y)){
        congestionView[direction][lower.x][lower.y] = true;
    }
    // else{
    //     congestionView[direction][lower.x][lower.y] = false;
    // }
#endif
    DBU edgeLength = getEdgeLength(direction, lower[direction]);
    totalLength += (reverse ? -edgeLength : edgeLength);
}

void GridGraph::commitVia(const int layerIndex, const utils::PointT<int> loc, const bool reverse)
{
    assert(layerIndex + 1 < nLayers);
    totalNumVias += (reverse ? -1 : 1);
}

void GridGraph::commitNonStackVia(const int layerIndex, const utils::PointT<int> loc, const bool reverse)
{
    auto isHorizontal = getLayerDirection(layerIndex) == MetalLayer::H;
    auto [x, y] = loc;
    if(isHorizontal ? (x == 0) : (y == 0))
        graphEdges[layerIndex][x][y].demand += (reverse ? -1.f : 1.f);
    else if(isHorizontal? (x == xSize - 1) : (y == ySize - 1))
        graphEdges[layerIndex][x - isHorizontal][y - !isHorizontal].demand += (reverse ? -1.f : 1.f);
    else
    {
        graphEdges[layerIndex][x][y].demand += reverse ? -.5f : .5f;
        graphEdges[layerIndex][x - isHorizontal][y - !isHorizontal].demand += (reverse ? -.5f : .5f);
    }
}

// void GridGraph::commitTree(const std::shared_ptr<GRTreeNode> &tree, const bool reverse)
// {
//     GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node)
//                          {
//         for (const auto& child : node->children) {
//             if (node->layerIdx == child->layerIdx) {
//                 unsigned direction = layerDirections[node->layerIdx];
//                 if (direction == MetalLayer::H) {
//                     assert(node->y == child->y);
//                     int l = min(node->x, child->x), h = max(node->x, child->x);
//                     for (int x = l; x < h; x++) {
//                         commitWire(node->layerIdx, {x, node->y}, reverse);
//                     }
//                 } else {
//                     assert(node->x == child->x);
//                     int l = min(node->y, child->y), h = max(node->y, child->y);
//                     for (int y = l; y < h; y++) {
//                         commitWire(node->layerIdx, {node->x, y}, reverse);
//                     }
//                 }
//             } else {
//                 int maxLayerIndex = max(node->layerIdx, child->layerIdx);
//                 for (int layerIdx = min(node->layerIdx, child->layerIdx); layerIdx < maxLayerIndex; layerIdx++) {
//                     if(layerIdx > min(node->layerIdx, child->layerIdx) && layerIdx < maxLayerIndex-1){
//                         // non_stack_via
//                         commitNonStackVia(layerIdx,{node->x, node->y}, reverse);
//                     }
//                     commitVia(layerIdx, {node->x, node->y}, reverse);
//                 }
//             }
//         } });
// }

void GridGraph::commitTree(const std::shared_ptr<GRTreeNode>& tree, const bool reverse)
{
    // reset flag
    GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
        for (const auto& child : node->children) {
            if(node->layerIdx == child->layerIdx) { // wires
                if(getLayerDirection(node->layerIdx) == MetalLayer::H) {
                    for(int x = min(node->x, child->x), xe = max(node->x, child->x); x <= xe; x++)
                        flag[node->layerIdx][x][node->y] = false;
                }
                else {
                    for(int y = min(node->y, child->y), ye = max(node->y, child->y); y <= ye; y++)
                        flag[node->layerIdx][node->x][y] = false;
                }
            } else { // vias
                for(int z = min(node->layerIdx, child->layerIdx), ze = max(node->layerIdx, child->layerIdx); z <= ze; z++)
                    flag[z][node->x][node->y] = false;
            }
        }
    });
    // commit wire
    GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
        for (const auto& child : node->children) {
            if(node->layerIdx == child->layerIdx) { // wires
                if(getLayerDirection(node->layerIdx) == MetalLayer::H) {
                    for(int x = min(node->x, child->x), xe = max(node->x, child->x); x <= xe; x++)
                    {
                        if(x < xe)
                            commitWire(node->layerIdx, { x, node->y }, reverse);
                        flag[node->layerIdx][x][node->y] = true;
                    }
                }
                else {
                    for(int y = min(node->y, child->y), ye = max(node->y, child->y); y <= ye; y++)
                    {
                        if(y < ye)
                            commitWire(node->layerIdx, { node->x, y }, reverse);
                        flag[node->layerIdx][node->x][y] = true;
                    }
                }
            }
        }
    });
    // commit (nonstack) via
    GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
        for (const auto& child : node->children) {
            if(node->layerIdx != child->layerIdx) {
                for(int z = min(node->layerIdx, child->layerIdx), ze = max(node->layerIdx, child->layerIdx); z < ze; z++)
                {
                    if(!flag[z][node->x][node->y])
                    {
                        flag[z][node->x][node->y] = true;
                        commitNonStackVia(z, { node->x, node->y }, reverse);
                    }
                    commitVia(z, {node->x, node->y}, reverse);
                }
            }
        }
    });
}

int GridGraph::checkOverflow(const int layerIndex, const utils::PointT<int> u, const utils::PointT<int> v) const
{
    int num = 0;
    unsigned direction = layerDirections[layerIndex];
    if (direction == MetalLayer::H)
    {
        assert(u.y == v.y);
        int l = min(u.x, v.x), h = max(u.x, v.x);
        for (int x = l; x < h; x++)
        {
            if (checkOverflow(layerIndex, x, u.y))
                num++;
        }
    }
    else
    {
        assert(u.x == v.x);
        int l = min(u.y, v.y), h = max(u.y, v.y);
        for (int y = l; y < h; y++)
        {
            if (checkOverflow(layerIndex, u.x, y))
                num++;
        }
    }
    return num;
}

int GridGraph::checkOverflow(const std::shared_ptr<GRTreeNode> &tree) const
{
    if (!tree)
        return 0;
    int num = 0;
    GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node)
                         {
        for (auto& child : node->children) {
            // Only check wires
            if (node->layerIdx == child->layerIdx) {
                num += checkOverflow(node->layerIdx, (utils::PointT<int>)*node, (utils::PointT<int>)*child);
            }
        } });
    return num;
}

std::string GridGraph::getPythonString(const std::shared_ptr<GRTreeNode> &routingTree) const
{
    vector<std::tuple<utils::PointT<int>, utils::PointT<int>, bool>> edges;
    GRTreeNode::preorder(routingTree, [&](std::shared_ptr<GRTreeNode> node)
                         {
        for (auto& child : node->children) {
            if (node->layerIdx == child->layerIdx) {
                unsigned direction = getLayerDirection(node->layerIdx);
                int r = (*node)[1 - direction];
                const int l = min((*node)[direction], (*child)[direction]);
                const int h = max((*node)[direction], (*child)[direction]);
                if (l == h) continue;
                utils::PointT<int> lpoint = (direction == MetalLayer::H ? utils::PointT<int>(l, r) : utils::PointT<int>(r, l));
                utils::PointT<int> hpoint = (direction == MetalLayer::H ? utils::PointT<int>(h, r) : utils::PointT<int>(r, h));
                bool congested = false;
                for (int c = l; c < h; c++) {
                    utils::PointT<int> cpoint = (direction == MetalLayer::H ? utils::PointT<int>(c, r) : utils::PointT<int>(r, c));
                    if (checkOverflow(node->layerIdx, cpoint.x, cpoint.y) != congested) {
                        if (lpoint != cpoint) {
                            edges.emplace_back(lpoint, cpoint, congested);
                            lpoint = cpoint;
                        }
                        congested = !congested;
                    }
                }
                if (lpoint != hpoint) edges.emplace_back(lpoint, hpoint, congested);
            }
        } });
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < edges.size(); i++)
    {
        auto &edge = edges[i];
        ss << "[" << std::get<0>(edge) << ", " << std::get<1>(edge) << ", " << (std::get<2>(edge) ? 1 : 0) << "]";
        ss << (i < edges.size() - 1 ? ", " : "]");
    }
    return ss.str();
}

void GridGraph::extractBlockageView(GridGraphView<bool> &view) const
{
    view.assign(2, vector<vector<bool>>(xSize, vector<bool>(ySize, true)));
    for (int layerIndex = parameters.min_routing_layer; layerIndex < nLayers; layerIndex++)
    {
        unsigned direction = getLayerDirection(layerIndex);
        for (int x = 0; x < xSize; x++)
        {
            for (int y = 0; y < ySize; y++)
            {
                if (getEdge(layerIndex, x, y).capacity >= 1.0)
                {
                    view[direction][x][y] = false;
                }
            }
        }
    }
}

void GridGraph::extractCongestionView(GridGraphView<bool> &view) const
{
    view.assign(2, vector<vector<bool>>(xSize, vector<bool>(ySize, false)));
    for (int layerIndex = parameters.min_routing_layer; layerIndex < nLayers; layerIndex++)
    {
        unsigned direction = getLayerDirection(layerIndex);
        for (int x = 0; x < xSize; x++)
        {
            for (int y = 0; y < ySize; y++)
            {
                if (checkOverflow(layerIndex, x, y))
                {
                    view[direction][x][y] = true;
                }
            }
        }
    }
}

// void GridGraph::extractCongestionView() const
// {
//     // view.assign(2, vector<vector<bool>>(xSize, vector<bool>(ySize, false)));
//     for (int layerIndex = parameters.min_routing_layer; layerIndex < nLayers; layerIndex++)
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


void GridGraph::extractWireCostView(GridGraphView<CostT> &view) const
{
    view.assign(2, vector<vector<CostT>>(xSize, vector<CostT>(ySize, std::numeric_limits<CostT>::max())));
    for (unsigned direction = 0; direction < 2; direction++)
    {
        vector<int> layerIndices;
        CostT unitOverflowCost = std::numeric_limits<CostT>::max();
        for (int layerIndex = parameters.min_routing_layer; layerIndex < getNumLayers(); layerIndex++)
        {
            if (getLayerDirection(layerIndex) == direction)
            {
                layerIndices.emplace_back(layerIndex);
                unitOverflowCost = min(unitOverflowCost, getUnitOverflowCost(layerIndex));
            }
        }
        for (int x = 0; x < xSize; x++)
        {
            for (int y = 0; y < ySize; y++)
            {
                int edgeIndex = direction == MetalLayer::H ? x : y;
                if (edgeIndex >= getSize(direction) - 1)
                    continue;
                CapacityT capacity = 0;
                CapacityT demand = 0;
                for (int layerIndex : layerIndices)
                {
                    const auto &edge = getEdge(layerIndex, x, y);
                    capacity += edge.capacity;
                    demand += edge.demand;
                }
                DBU length = getEdgeLength(direction, edgeIndex);
                // ------ legacy cost ------
                // view[direction][x][y] = length * (unit_length_wire_cost + unitOverflowCost * (capacity < 1.0 ? 1.0 : logistic(capacity - demand, parameters.maze_logistic_slope)));

                // ------ new cost ------
                CostT slope = capacity > 0.f ? 0.5f : 1.5f;
                view[direction][x][y] = length * unit_length_wire_cost + 50*unitOverflowCost * exp(slope * (demand - capacity)) * (exp(slope) - 1);
            }
        }
    }
}

void GridGraph::updateWireCostView(GridGraphView<CostT> &view, std::shared_ptr<GRTreeNode> routingTree) const
{
    vector<vector<int>> sameDirectionLayers(2);
    vector<CostT> unitOverflowCost(2, std::numeric_limits<CostT>::max());
    for (int layerIndex = parameters.min_routing_layer; layerIndex < getNumLayers(); layerIndex++)
    {
        unsigned direction = getLayerDirection(layerIndex);
        sameDirectionLayers[direction].emplace_back(layerIndex);
        unitOverflowCost[direction] = min(unitOverflowCost[direction], getUnitOverflowCost(layerIndex));
    }
    auto update = [&](unsigned direction, int x, int y)
    {
        int edgeIndex = direction == MetalLayer::H ? x : y;
        if (edgeIndex >= getSize(direction) - 1)
            return;
        CapacityT capacity = 0;
        CapacityT demand = 0;
        for (int layerIndex : sameDirectionLayers[direction])
        {
            if (getLayerDirection(layerIndex) != direction)
                continue;
            const auto &edge = getEdge(layerIndex, x, y);
            capacity += edge.capacity;
            demand += edge.demand;
        }
        DBU length = getEdgeLength(direction, edgeIndex);
        // ------ legacy cost ------
        // view[direction][x][y] = length * (unit_length_wire_cost + unitOverflowCost[direction] * (capacity < 1.0 ? 1.0 : logistic(capacity - demand, parameters.maze_logistic_slope)));

        // ------ new cost ------
        CostT slope = capacity > 0.f ? 0.5f : 1.5f;
        view[direction][x][y] = length * unit_length_wire_cost + 50*unitOverflowCost[direction] * exp(slope * (demand - capacity)) * (exp(slope) - 1);
    };
    GRTreeNode::preorder(routingTree, [&](std::shared_ptr<GRTreeNode> node)
                         {
        for (const auto& child : node->children) {
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
                for (int layerIdx = min(node->layerIdx, child->layerIdx); layerIdx < maxLayerIndex; layerIdx++) {
                    unsigned direction = getLayerDirection(layerIdx);
                    update(direction, node->x, node->y);
                    if ((*node)[direction] > 0) update(direction, node->x - 1 + direction, node->y - direction);
                }
            }
        } });
}

void GridGraph::write(const std::string heatmap_file) const
{
    log() << "writing heatmap to file..." << std::endl;
    std::stringstream ss;

    ss << nLayers << " " << xSize << " " << ySize << " " << std::endl;
    for (int layerIndex = 0; layerIndex < nLayers; layerIndex++)
    {
        ss << layerNames[layerIndex] << std::endl;
        for (int y = 0; y < ySize; y++)
        {
            for (int x = 0; x < xSize; x++)
            {
                ss << (graphEdges[layerIndex][x][y].capacity - graphEdges[layerIndex][x][y].demand)
                   << (x == xSize - 1 ? "" : " ");
            }
            ss << std::endl;
        }
    }
    std::ofstream fout(heatmap_file);
    fout << ss.str();
    fout.close();
}
