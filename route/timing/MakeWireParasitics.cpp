#include "MakeWireParasitics.hpp"
#include "../util/log.hpp"
#include <sta/Corner.hh>
#include <sta/Network.hh>
#include <sta/Sta.hh>
#include <sta/Units.hh>

MakeWireParasitics::MakeWireParasitics(const GRNetwork *network,
                                       const GRTechnology *tech) {
  m_network = network;
  m_tech = tech;
  m_sta = sta::Sta::sta();
  m_sta_network = m_sta->network();
  m_parasitics = m_sta->parasitics();
  m_arc_delay_calc = m_sta->arcDelayCalc();
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
  size_t resistor_id = 1;
  GRTreeNode::preorder(
      net->routingTree(), [&](std::shared_ptr<GRTreeNode> tree) {
        const int min_routing_layer = m_tech->minRoutingLayer();
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

          float res = 0.f, cap = 0.f;
          if (init_layer == final_layer) { // wire
            int wire_length_dbu = m_tech->getWireLengthDbu(*tree, *child);
            double wire_length = m_tech->dbuToMeter(wire_length_dbu);
            cap += m_tech->layerCap(init_layer) * wire_length;
            res += m_tech->layerRes(init_layer) * wire_length;
          } else { // via
            for (int l = init_layer; l < final_layer; l++)
              res += m_tech->cutLayerRes(l);
          }
          m_parasitics->incrCap(n1, 0.5f * cap);
          m_parasitics->makeResistor(parasitic, resistor_id++, res, n1, n2);
          m_parasitics->incrCap(n2, 0.5f * cap);
          // sta::Unit *res_unit = m_sta->units()->resistanceUnit();
          // sta::Unit *cap_unit = m_sta->units()->capacitanceUnit();
          // LOG_DEBUG("res: %s %s, cap: %s %s", res_unit->asString(res),
          //           res_unit->scaledSuffix(), cap_unit->asString(cap),
          //           cap_unit->scaledSuffix());
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
