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

  clearParasitics();
}

void MakeWireParasitics::estimateParasitcs(GRNet *net) {
  sta::Net *sta_net =
      m_sta_network->findNet(m_sta_network->topInstance(), net->name().c_str());
  ASSERT(sta_net != nullptr, "Could not find net `%s`", net->name().c_str());

  sta::Corner *corner = m_sta->corners()->corners()[0];
  sta::MinMax *min_max = sta::MinMax::max();
  sta::ParasiticAnalysisPt *ap = corner->findParasiticAnalysisPt(min_max);
  m_parasitics->deleteReducedParasitics(sta_net, ap);
  sta::Parasitic *parasitic =
      m_parasitics->makeParasiticNetwork(sta_net, false, ap);

  m_node_map.clear();
  m_resistor_id = 1;
  makeRouteParasitics(net, sta_net, parasitic);
  makeParasiticsToPins(net, sta_net, parasitic);

  m_arc_delay_calc->reduceParasitic(parasitic, sta_net, corner,
                                    sta::MinMaxAll::all());
  m_parasitics->deleteParasiticNetworks(sta_net);
}

void MakeWireParasitics::clearParasitics() {
  // Remove any existing parasitics.
  m_sta->deleteParasitics();
  // Make separate parasitics for each corner.
  m_sta->setParasiticAnalysisPts(false);
}

float MakeWireParasitics::getNetSlack(GRNet *net) {
  sta::Net *sta_net =
      m_sta_network->findNet(m_sta_network->topInstance(), net->name().c_str());
  float slack = m_sta->netSlack(sta_net, sta::MinMax::max());
  return slack;
}

void MakeWireParasitics::makeRouteParasitics(GRNet *net, sta::Net *sta_net,
                                             sta::Parasitic *parasitic) {
  if (net->routingTree() == nullptr)
    return;
  GRTreeNode::preorder(
      net->routingTree(), [&](std::shared_ptr<GRTreeNode> tree) {
        const int min_routing_layer = m_tech->minRoutingLayer();
        for (const auto &child : tree->children) {
          const auto [init_layer, final_layer] =
              std::minmax(tree->layerIdx, child->layerIdx);
          const auto [init_x, final_x] = std::minmax(tree->x, child->x);
          const auto [init_y, final_y] = std::minmax(tree->y, child->y);
          GRPoint pt1(init_layer, init_x, init_y);
          GRPoint pt2(final_layer, final_x, final_y);
          // <net>:<sub_node>
          sta::ParasiticNode *n1 = ensureParasiticNode(pt1, parasitic, sta_net);
          sta::ParasiticNode *n2 = ensureParasiticNode(pt2, parasitic, sta_net);
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
          m_parasitics->makeResistor(parasitic, m_resistor_id++, res, n1, n2);
          m_parasitics->incrCap(n2, 0.5f * cap);
        }
      });
}

void MakeWireParasitics::makeParasiticsToPins(GRNet *net, sta::Net *sta_net,
                                              sta::Parasitic *parasitic) {
  for (GRPin *pin : net->pins()) {
    GRInstance *inst = pin->instance();
    sta::ParasiticNode *n1 = nullptr, *n2 = nullptr;
    if (inst != nullptr) {
      // <instance>:<port>
      sta::Instance *sta_inst = m_sta_network->findInstanceRelative(
          m_sta_network->topInstance(), inst->name().c_str());
      sta::Pin *sta_pin = m_sta_network->findPin(sta_inst, pin->name().c_str());
      n1 = m_parasitics->ensureParasiticNode(parasitic, sta_pin, m_sta_network);
      n2 = ensureParasiticNode(pin->accessPoint(), parasitic, sta_net);
    } else {
      // <io_port>
      sta::Pin *sta_pin = m_sta_network->findPin(m_sta_network->topInstance(),
                                                 pin->name().c_str());
      n1 = m_parasitics->ensureParasiticNode(parasitic, sta_pin, m_sta_network);
      n2 = ensureParasiticNode(pin->accessPoint(), parasitic, sta_net);
    }
    if (n1 && n2) {
      m_parasitics->makeResistor(parasitic, m_resistor_id++, 1., n1, n2);
    }
  }
}

sta::ParasiticNode *MakeWireParasitics::ensureParasiticNode(
    const GRPoint &pt, sta::Parasitic *parasitic, sta::Net *net) {
  if (m_node_map.find(pt) == m_node_map.end()) {
    int id = static_cast<int>(m_node_map.size());
    sta::ParasiticNode *node =
        m_parasitics->ensureParasiticNode(parasitic, net, id, m_sta_network);
    m_node_map.emplace(pt, node);
  }
  return m_node_map.at(pt);
}
