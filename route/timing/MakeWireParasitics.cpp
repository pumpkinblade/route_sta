#include "MakeWireParasitics.hpp"
#include "../object/Route.hpp"
#include "../util/log.hpp"
#include <sta/Corner.hh>
#include <sta/Network.hh>
#include <sta/PortDirection.hh>
#include <sta/Sta.hh>
#include <sta/Units.hh>

namespace sca {

static sta::PortDirection *toStaDirection(PortDirection dir) {
  switch (dir) {
  case PortDirection::Input:
    return sta::PortDirection::input();
  case PortDirection::Output:
    return sta::PortDirection::output();
  case PortDirection::Inout:
    return sta::PortDirection::bidirect();
  default:
    break;
  };
  return sta::PortDirection::unknown();
}

MakeWireParasitics::MakeWireParasitics(Design *design) {
  m_design = design;

  m_sta = sta::Sta::sta();
  m_sta->readNetlistBefore();

  // create top cell
  LOG_TRACE("create sta top cell");
  m_sta_network = m_sta->networkReader();
  m_sta_library = m_sta_network->makeLibrary(m_design->name().c_str(), nullptr);
  m_sta_top_cell = m_sta_network->makeCell(
      m_sta_library, m_design->name().c_str(), false, nullptr);
  Instance *top_inst = m_design->topInstance();
  Libcell *top_libcell = top_inst->libcell();
  for (int i = 0; i < top_libcell->numPorts(); i++) {
    Port *port = top_libcell->port(i);
    sta::Port *sta_port =
        m_sta_network->makePort(m_sta_top_cell, port->name().c_str());
    m_sta_network->setDirection(sta_port, toStaDirection(port->direction()));
  }

  // create top instance
  LOG_TRACE("create sta top inst");
  m_sta->readNetlistBefore();
  m_sta_top_inst = m_sta_network->makeInstance(
      m_sta_top_cell, m_design->name().c_str(), nullptr);

  // create instances
  LOG_TRACE("create sta insts");
  std::set<Instance *> net_insts;
  for (int i = 0; i < m_design->numNets(); i++) {
    Net *net = m_design->net(i);
    for (int j = 0; j < net->numPins(); j++) {
      Pin *pin = net->pin(j);
      if (pin->instance() != design->topInstance())
        net_insts.insert(pin->instance());
    }
  }
  std::map<Instance *, std::pair<sta::Instance *, sta::LibertyCell *>> inst_map;
  {
    int i = 0;
    for (Instance *inst : net_insts) {
      inst->setStaName("_" + std::to_string(i) + "_");
      Libcell *libcell = inst->libcell();
      sta::LibertyCell *sta_liberty_cell =
          m_sta_network->findLibertyCell(libcell->name().c_str());
      sta::Instance *sta_inst = m_sta_network->makeInstance(
          sta_liberty_cell, inst->staName().c_str(), m_sta_top_inst);
      inst_map.emplace(inst, std::make_pair(sta_inst, sta_liberty_cell));
      i++;
    }
  }

  // create nets
  LOG_TRACE("create sta nets");
  for (int i = 0; i < m_design->numNets(); i++) {
    Net *net = m_design->net(i);
    net->setStaName("_" + std::to_string(i) + "_");
    sta::Net *sta_net =
        m_sta_network->makeNet(net->staName().c_str(), m_sta_top_inst);
    for (int j = 0; j < net->numPins(); j++) {
      Pin *pin = net->pin(j);
      if (pin->instance() == m_design->topInstance()) {
        sta::Port *sta_port =
            m_sta_network->findPort(m_sta_top_cell, pin->name().c_str());
        ASSERT(sta_port, "");
        sta::Pin *sta_pin =
            m_sta_network->makePin(m_sta_top_inst, sta_port, nullptr);
        m_sta_network->makeTerm(sta_pin, sta_net);
      } else {
        Instance *inst = pin->instance();
        auto [sta_inst, sta_liberty_cell] = inst_map.at(inst);
        sta::Cell *sta_cell = reinterpret_cast<sta::Cell *>(sta_liberty_cell);
        sta::Port *sta_port =
            m_sta_network->findPort(sta_cell, pin->name().c_str());
        m_sta_network->makePin(sta_inst, sta_port, sta_net);
      }
    }
  }

  LOG_TRACE("acquire arcDelayCalc and parasitics");
  m_parasitics = m_sta->parasitics();
  m_arc_delay_calc = m_sta->arcDelayCalc();
  clearParasitics();
}

MakeWireParasitics::~MakeWireParasitics() { m_sta_network->clear(); }

void MakeWireParasitics::estimateParasitcs(Net *net) {
  sta::Net *sta_net = m_sta_network->findNet(m_sta_network->topInstance(),
                                             net->staName().c_str());

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

float MakeWireParasitics::getNetSlack(Net *net) {
  sta::Net *sta_net = m_sta_network->findNet(m_sta_network->topInstance(),
                                             net->staName().c_str());
  float slack = m_sta->netSlack(sta_net, sta::MinMax::max());
  return slack;
}

void MakeWireParasitics::makeRouteParasitics(Net *net, sta::Net *sta_net,
                                             sta::Parasitic *parasitic) {
  if (net->routingTree() == nullptr || net->routingTree()->children.size() == 0)
    return;
  GRTreeNode::preorder(
      net->routingTree(), [&](std::shared_ptr<GRTreeNode> tree) {
        for (const auto &child : tree->children) {
          const auto [init_layer, final_layer] =
              std::minmax(tree->layerIdx, child->layerIdx);
          const auto [init_x, final_x] = std::minmax(tree->x, child->x);
          const auto [init_y, final_y] = std::minmax(tree->y, child->y);
          PointOnLayerT<int> pt1(init_layer, init_x, init_y);
          PointOnLayerT<int> pt2(final_layer, final_x, final_y);
          // <net>:<sub_node>
          sta::ParasiticNode *n1 = ensureParasiticNode(pt1, parasitic, sta_net);
          sta::ParasiticNode *n2 = ensureParasiticNode(pt2, parasitic, sta_net);

          float res = 0.f, cap = 0.f;
          Technology *tech = m_design->technology();
          Grid *grid = m_design->grid();
          if (init_layer == final_layer) { // wire
            double dbu = m_design->dbu();
            DBU wire_length_dbu = grid->wireLength(*tree, *child);
            double wire_length = (wire_length_dbu / dbu) * 1e-6;
            cap += tech->layerCap(init_layer) * wire_length;
            res += tech->layerRes(init_layer) * wire_length;
          } else { // via
            for (int l = init_layer; l < final_layer; l++)
              res += tech->cutLayerRes(l);
          }
          m_parasitics->incrCap(n1, 0.5f * cap);
          m_parasitics->makeResistor(parasitic, m_resistor_id++, res, n1, n2);
          m_parasitics->incrCap(n2, 0.5f * cap);
        }
      });
}

void MakeWireParasitics::makeParasiticsToPins(Net *net, sta::Net *sta_net,
                                              sta::Parasitic *parasitic) {
  for (int i = 0; i < net->numPins(); i++) {
    Pin *pin = net->pin(i);
    Instance *inst = pin->instance();
    sta::Instance *sta_inst =
        inst == m_design->topInstance()
            ? m_sta_top_inst
            : m_sta_network->findInstanceRelative(m_sta_top_inst,
                                                  inst->staName().c_str());
    sta::Pin *sta_pin = m_sta_network->findPin(sta_inst, pin->name().c_str());
    sta::ParasiticNode *n1 =
        m_parasitics->ensureParasiticNode(parasitic, sta_pin, m_sta_network);
    sta::ParasiticNode *n2 =
        ensureParasiticNode(pin->position(), parasitic, sta_net);
    m_parasitics->makeResistor(parasitic, m_resistor_id++, 0.0, n1, n2);
  }
}

sta::ParasiticNode *MakeWireParasitics::ensureParasiticNode(
    const PointOnLayerT<int> &pt, sta::Parasitic *parasitic, sta::Net *net) {
  if (m_node_map.find(pt) == m_node_map.end()) {
    int id = static_cast<int>(m_node_map.size());
    sta::ParasiticNode *node =
        m_parasitics->ensureParasiticNode(parasitic, net, id, m_sta_network);
    m_node_map.emplace(pt, node);
  }
  return m_node_map.at(pt);
}

} // namespace sca
