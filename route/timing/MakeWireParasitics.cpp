#include "MakeWireParasitics.hpp"
#include "../util/log.hpp"
#include <sta/Corner.hh>
#include <sta/Network.hh>
#include <sta/Sta.hh>

MakeWireParasitics::MakeWireParasitics(const GRNetwork *network,
                                       const GRTechnology *tech) {
  m_network = network;
  m_tech = tech;
  m_sta = sta::Sta::sta();
  m_sta_network = m_sta->network();
  m_parasitics = m_sta->parasitics();
  m_arc_delay_calc = m_sta->arcDelayCalc();
  m_resistor_id = 0;
}

void MakeWireParasitics::estimateParasitcs() {
  for (GRNet *net : m_network->nets()) {
    estimateParasitcs(net);
  }
}

void MakeWireParasitics::estimateParasitcs(GRNet *net) {
  sta::Net *sta_net =
      m_sta_network->findNet(m_sta_network->topInstance(), net->name().c_str());
  ASSERT(sta_net != nullptr, "Could not find net `%s`", net->name().c_str());
  for (sta::Corner *corner : *m_sta->corners()) {
    NodeRoutePtMap node_map;

    sta::ParasiticAnalysisPt *analysis_point =
        corner->findParasiticAnalysisPt(sta::MinMax::max());
    sta::Parasitic *parasitic =
        m_parasitics->makeParasiticNetwork(sta_net, false, analysis_point);
    makeRouteParasitics(net, sta_net, corner, analysis_point, parasitic,
                        node_map);
    m_arc_delay_calc->reduceParasitic(parasitic, sta_net, corner,
                                      sta::MinMaxAll::all());
  }
  m_parasitics->deleteParasiticNetworks(sta_net);
}

void MakeWireParasitics::clearParasitics() {
  // Remove any existing parasitics.
  m_sta->deleteParasitics();
  // Make separate parasitics for each corner.
  m_sta->setParasiticAnalysisPts(true);
}

float MakeWireParasitics::getNetSlack(GRNet *net) {
  sta::Net *sta_net =
      m_sta_network->findNet(m_sta_network->topInstance(), net->name().c_str());
  float slack = m_sta->netSlack(sta_net, sta::MinMax::max());
  return slack;
}

void MakeWireParasitics::makeRouteParasitics(GRNet *net, sta::Net *sta_net,
                                             sta::Corner *, // only one corner
                                             sta::ParasiticAnalysisPt *,
                                             sta::Parasitic *parasitic,
                                             NodeRoutePtMap &node_map) {
  if (net->routingTree() == nullptr)
    return;
  GRTreeNode::preorder(
      net->routingTree(), [&](std::shared_ptr<GRTreeNode> tree) {
        const int min_routing_layer = 1;
        for (const auto &child : tree->children) {
          const auto [init_layer, final_layer] =
              std::minmax(tree->layerIdx, child->layerIdx);
          const auto [init_x, final_x] = std::minmax(tree->x, child->x);
          const auto [init_y, final_y] = std::minmax(tree->y, child->y);
          sta::ParasiticNode *n1 =
              (init_layer >= min_routing_layer)
                  ? ensureParasiticNode(init_x, init_y, init_layer, node_map,
                                        parasitic, sta_net)
                  : nullptr;
          sta::ParasiticNode *n2 =
              (init_layer >= min_routing_layer)
                  ? ensureParasiticNode(final_x, final_y, final_layer, node_map,
                                        parasitic, sta_net)
                  : nullptr;
          if (!n1 || !n2)
            continue;

          const int wire_length_dbu = m_tech->getWireLengthDbu(*tree, *child);
          float res = 0.f, cap = 0.f;
          if (wire_length_dbu == 0) { // via
            for (int l = init_layer; l < final_layer; l++)
              res += m_tech->cutLayerRes(init_layer);
          } else {
            cap += m_tech->layerCap(init_layer) *
                   m_tech->dbuToMicro(wire_length_dbu);
            res += m_tech->layerRes(init_layer) *
                   m_tech->dbuToMicro(wire_length_dbu);
          }
          m_parasitics->incrCap(n1, 0.5f * cap);
          m_parasitics->makeResistor(parasitic, m_resistor_id++, res, n1, n2);
          m_parasitics->incrCap(n2, 0.5f * cap);
        }
      });
}

sta::ParasiticNode *MakeWireParasitics::ensureParasiticNode(
    int x, int y, int layer, NodeRoutePtMap &node_map,
    sta::Parasitic *parasitic, sta::Net *net) const {
  auto *parasitics = sta::Sta::sta()->parasitics();
  GRPoint pin_loc(layer, x, y);
  if (node_map.find(pin_loc) == node_map.end()) {
    int id = static_cast<int>(node_map.size());
    sta::ParasiticNode *node =
        parasitics->ensureParasiticNode(parasitic, net, id, m_sta_network);
    node_map.emplace(pin_loc, node);
  }
  return node_map.at(pin_loc);
}
