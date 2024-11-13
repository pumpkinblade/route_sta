#include "Context.hpp"
#include "../cugr2/GlobalRouter.h"
#include "../lefdef/DefDatabase.hpp"
#include "../lefdef/LefDatabase.hpp"
#include "../util/log.hpp"
#include <fstream>
#include <iomanip>
#include <sta/Corner.hh>
#include <sta/Network.hh>
#include <sta/PortDirection.hh>
#include <sta/Report.hh>
#include <sta/Sta.hh>
#include <sta/Units.hh>
#include <unordered_map>

std::unique_ptr<Context> Context::s_ctx(new Context);

static sta::Instance *link(const char *top_cell_name, bool, sta::Report *,
                           sta::NetworkReader *) {
  return Context::context()->linkNetwork(top_cell_name);
}

bool Context::writeSlack(const std::string &file) {
  std::ofstream ostream(file);
  if (!ostream) {
    m_sta_report->error(70000, "can not open file %s", file.c_str());
    return false;
  }
  if (m_network == nullptr || m_sta_network->topInstance() == nullptr) {
    m_sta_report->error(70000,
                        "the Network has not been initialized or linked");
    return false;
  }

  m_parasitics_builder->clearParasitics();
  for (GRNet *net : m_network->nets()) {
    m_parasitics_builder->estimateParasitcs(net);
  }
  sta::Sta::sta()->delaysInvalid();

  for (GRNet *net : m_network->nets()) {
    float slack = m_parasitics_builder->getNetSlack(net);
    ostream << net->name() << " " << std::setprecision(2) << slack << '\n';
  }

  return true;
}

bool Context::readLef(const std::string &lef_file) {
  if (m_lef_db == nullptr) {
    m_lef_db = std::make_unique<LefDatabase>();
  }
  return m_lef_db->read(lef_file);
}

bool Context::readDef(const std::string &def_file, bool use_routing) {
  m_sta_network = sta::Sta::sta()->networkReader();
  m_sta_report = sta::Sta::sta()->report();
  sta::Sta::sta()->readNetlistBefore();
  m_sta_network->setLinkFunc(link);

  LOG_INFO("read def");
  auto def_db = std::make_unique<DefDatabase>();
  bool success = def_db->read(def_file, use_routing);
  if (!success)
    return false;

  LOG_INFO("init tech");
  m_tech = std::make_unique<GRTechnology>(m_lef_db.get(), def_db.get());
  LOG_INFO("init network");
  m_network =
      std::make_unique<GRNetwork>(m_lef_db.get(), def_db.get(), m_tech.get());
  LOG_INFO("init parasitics_builder");
  m_parasitics_builder =
      std::make_unique<MakeWireParasitics>(m_network.get(), m_tech.get());
  m_sta = sta::Sta::sta();

  m_sta_library = m_sta_network->findLibrary("lefdef");
  if (m_sta_library == nullptr)
    m_sta_library = m_sta_network->makeLibrary("lefdef", nullptr);

  // topcell in def file
  sta::Cell *sta_top_cell =
      m_sta_network->findCell(m_sta_library, m_network->designName().c_str());
  if (sta_top_cell) {
    m_sta_network->deleteCell(sta_top_cell);
  }
  sta_top_cell = m_sta_network->makeCell(
      m_sta_library, def_db->design_name.c_str(), false, def_file.c_str());
  for (const DefIoPin &iopin : def_db->iopins) {
    sta::Port *port = m_sta_network->makePort(sta_top_cell, iopin.name.c_str());
    switch (iopin.direction) {
    case PortDirection::Inout:
      m_sta_network->setDirection(port, sta::PortDirection::bidirect());
      break;
    case PortDirection::Input:
      m_sta_network->setDirection(port, sta::PortDirection::input());
      break;
    case PortDirection::Output:
      m_sta_network->setDirection(port, sta::PortDirection::output());
      break;
    default:
      m_sta_report->warn(70001, "Unknown direction of io PIN.%s",
                         iopin.name.c_str());
      break;
    }
  }

  return true;
}

sta::Instance *Context::linkNetwork(const std::string &top_cell_name) {
  if (m_sta_library == nullptr) {
    m_sta_report->error(70000, "libray has not been initialized");
    return nullptr;
  }
  sta::Cell *sta_top_cell =
      m_sta_network->findCell(m_sta_library, top_cell_name.c_str());
  if (sta_top_cell == nullptr) {
    m_sta_report->error(70000, "%s is not a def design.",
                        top_cell_name.c_str());
    return nullptr;
  }

  sta::Instance *sta_top_inst =
      m_sta_network->makeInstance(sta_top_cell, "", nullptr);

  std::unordered_map<sta::Instance *, sta::LibertyCell *> sta_inst_cell_map;
  for (const GRNet *net : m_network->nets()) {
    sta::Net *sta_net =
        m_sta_network->makeNet(net->name().c_str(), sta_top_inst);
    for (const GRPin *pin : net->pins()) {
      if (pin->instance() == nullptr) { // IO pin
        sta::Port *sta_port =
            m_sta_network->findPort(sta_top_cell, pin->name().c_str());
        if (sta_port == nullptr) {
          m_sta_report->error(70000, "io port name PIN.%s not found",
                              pin->name().c_str());
          m_sta_network->deleteInstance(sta_top_inst);
          return nullptr;
        }
        if (m_sta_network->findPin(sta_top_inst, sta_port) == nullptr) {
          sta::Pin *sta_pin =
              m_sta_network->makePin(sta_top_inst, sta_port, nullptr);
          m_sta_network->makeTerm(sta_pin, sta_net);
        }
        // m_sta_network->connect(sta_top_inst, sta_port, sta_net);
      } else { // internal pin
        GRInstance *inst = pin->instance();
        sta::Instance *sta_inst = m_sta_network->findInstanceRelative(
            sta_top_inst, inst->name().c_str());
        // if the instance not found, then create it
        if (sta_inst == nullptr) {
          sta::LibertyCell *sta_liberty_cell =
              m_sta_network->findLibertyCell(inst->libcellName().c_str());
          if (sta_liberty_cell == nullptr) {
            m_sta_report->error(70000, "%s is not a liberty libcell",
                                inst->libcellName().c_str());
            m_sta_network->deleteInstance(sta_top_inst);
            return nullptr;
          }
          sta_inst = m_sta_network->makeInstance(
              sta_liberty_cell, inst->name().c_str(), sta_top_inst);
          sta_inst_cell_map.emplace(sta_inst, sta_liberty_cell);
        }

        // connect
        sta::LibertyCell *sta_liberty_cell = sta_inst_cell_map[sta_inst];
        sta::Cell *sta_cell = reinterpret_cast<sta::Cell *>(sta_liberty_cell);
        sta::Port *sta_port =
            m_sta_network->findPort(sta_cell, pin->name().c_str());
        if (sta_port == nullptr) {
          m_sta_report->error(70000, "port name %s.%s not found",
                              m_sta_network->name(sta_cell),
                              pin->name().c_str());
          m_sta_network->deleteInstance(sta_top_inst);
          return nullptr;
        }
        m_sta_network->makePin(sta_inst, sta_port, sta_net);
        // m_sta_network->connect(sta_inst, sta_port, sta_net);
      }
    }
  }
  return sta_top_inst;
}

bool Context::runCugr2() {
  cugr2::Parameters params;
  params.threads = 1;
  params.unit_length_wire_cost = 0.00131579;
  params.unit_via_cost = 4.;
  params.unit_overflow_costs =
      std::vector<double>(static_cast<size_t>(m_tech->numLayers()), 5.);
  params.min_routing_layer = m_tech->minRoutingLayer();
  params.cost_logistic_slope = 1.;
  params.maze_logistic_slope = .5;
  params.via_multiplier = 2.;
  params.target_detour_count = 20;
  params.max_detour_ratio = 0.25;
  cugr2::GlobalRouter globalRouter(m_network.get(), m_tech.get(), params);
  globalRouter.route();
  return true;
}

bool Context::writeGuide(const std::string &file) {
  LOG_TRACE("writing guide");
  std::ofstream fout(file);
  std::vector<std::array<int, 6>> guide;
  for (GRNet *net : m_network->nets()) {
    treeToGuide(guide, net->routingTree(), m_tech.get());
    fout << net->name() << std::endl;
    fout << "(\n";
    for (const auto &[lx, ly, lz, hx, hy, hz] : guide)
      fout << lx << " " << ly << " " << lz << " " << hx << " " << hy << " "
           << hz << "\n";
    fout << ")\n";
  }
  fout.close();
  return true;
}

bool Context::test() {
  m_sta->setParasiticAnalysisPts(false);
  const sta::MinMax *ap_min_max = sta::MinMax::max();
  const sta::Corner *ap_corner = m_sta->corners()->corners()[0];
  sta::ParasiticAnalysisPt *ap = ap_corner->findParasiticAnalysisPt(ap_min_max);
  sta::Parasitics *parasitics = m_sta->parasitics();

  float res1 = 40.f;
  float cap1 = 0.243 * 1e-12;
  float cap2 = 0.032 * 1e-12;

  // net in1
  {
    sta::Net *sta_net =
        m_sta_network->findNet(m_sta_network->topInstance(), "in1");
    parasitics->deleteReducedParasitics(sta_net, ap);
    sta::Parasitic *parasitic =
        parasitics->makeParasiticNetwork(sta_net, false, ap);
    // n1{ in1 }, n2{ r1:D }
    sta::Pin *pin1 =
        m_sta_network->findPin(m_sta_network->topInstance(), "in1");
    sta::ParasiticNode *n1 =
        parasitics->ensureParasiticNode(parasitic, pin1, m_sta_network);
    sta::Instance *r1 =
        m_sta_network->findInstanceRelative(m_sta_network->topInstance(), "r1");
    sta::Pin *pin2 = m_sta_network->findPin(r1, "D");
    sta::ParasiticNode *n2 =
        parasitics->ensureParasiticNode(parasitic, pin2, m_sta_network);
    parasitics->incrCap(n1, cap1);
    parasitics->makeResistor(parasitic, 1, res1, n1, n2);
    parasitics->incrCap(n2, cap2);
  }

  // net in2
  {
    sta::Net *sta_net =
        m_sta_network->findNet(m_sta_network->topInstance(), "in2");
    parasitics->deleteReducedParasitics(sta_net, ap);
    sta::Parasitic *parasitic =
        parasitics->makeParasiticNetwork(sta_net, false, ap);
    // n1{ in2 }, n2{ r2:D }
    sta::Pin *pin1 =
        m_sta_network->findPin(m_sta_network->topInstance(), "in2");
    sta::ParasiticNode *n1 =
        parasitics->ensureParasiticNode(parasitic, pin1, m_sta_network);
    sta::Instance *r2 =
        m_sta_network->findInstanceRelative(m_sta_network->topInstance(), "r2");
    sta::Pin *pin2 = m_sta_network->findPin(r2, "D");
    sta::ParasiticNode *n2 =
        parasitics->ensureParasiticNode(parasitic, pin2, m_sta_network);
    parasitics->incrCap(n1, cap1);
    parasitics->makeResistor(parasitic, 1, res1, n1, n2);
    parasitics->incrCap(n2, cap2);
  }

  // net clk1
  {
    sta::Net *sta_net =
        m_sta_network->findNet(m_sta_network->topInstance(), "clk1");
    parasitics->deleteReducedParasitics(sta_net, ap);
    sta::Parasitic *parasitic =
        parasitics->makeParasiticNetwork(sta_net, false, ap);
    // n1{ clk1 }, n2{ r1:CK }
    sta::Pin *pin1 =
        m_sta_network->findPin(m_sta_network->topInstance(), "clk1");
    sta::ParasiticNode *n1 =
        parasitics->ensureParasiticNode(parasitic, pin1, m_sta_network);
    sta::Instance *r1 =
        m_sta_network->findInstanceRelative(m_sta_network->topInstance(), "r1");
    sta::Pin *pin2 = m_sta_network->findPin(r1, "CK");
    sta::ParasiticNode *n2 =
        parasitics->ensureParasiticNode(parasitic, pin2, m_sta_network);
    parasitics->incrCap(n1, cap1);
    parasitics->makeResistor(parasitic, 1, res1, n1, n2);
    parasitics->incrCap(n2, cap2);
  }

  // net clk2
  {
    sta::Net *sta_net =
        m_sta_network->findNet(m_sta_network->topInstance(), "clk2");
    parasitics->deleteReducedParasitics(sta_net, ap);
    sta::Parasitic *parasitic =
        parasitics->makeParasiticNetwork(sta_net, false, ap);
    // n1{ clk2 }, n2{ r2:CK }
    sta::Pin *pin1 =
        m_sta_network->findPin(m_sta_network->topInstance(), "clk2");
    sta::ParasiticNode *n1 =
        parasitics->ensureParasiticNode(parasitic, pin1, m_sta_network);
    sta::Instance *r2 =
        m_sta_network->findInstanceRelative(m_sta_network->topInstance(), "r2");
    sta::Pin *pin2 = m_sta_network->findPin(r2, "CK");
    sta::ParasiticNode *n2 =
        parasitics->ensureParasiticNode(parasitic, pin2, m_sta_network);
    parasitics->incrCap(n1, cap1);
    parasitics->makeResistor(parasitic, 1, res1, n1, n2);
    parasitics->incrCap(n2, cap2);
  }

  // net clk3
  {
    sta::Net *sta_net =
        m_sta_network->findNet(m_sta_network->topInstance(), "clk3");
    parasitics->deleteReducedParasitics(sta_net, ap);
    sta::Parasitic *parasitic =
        parasitics->makeParasiticNetwork(sta_net, false, ap);
    // n1{ clk3 }, n2{ r3:CK }
    sta::Pin *pin1 =
        m_sta_network->findPin(m_sta_network->topInstance(), "clk3");
    sta::ParasiticNode *n1 =
        parasitics->ensureParasiticNode(parasitic, pin1, m_sta_network);
    sta::Instance *r3 =
        m_sta_network->findInstanceRelative(m_sta_network->topInstance(), "r3");
    sta::Pin *pin2 = m_sta_network->findPin(r3, "CK");
    sta::ParasiticNode *n2 =
        parasitics->ensureParasiticNode(parasitic, pin2, m_sta_network);
    parasitics->incrCap(n1, cap1);
    parasitics->makeResistor(parasitic, 1, res1, n1, n2);
    parasitics->incrCap(n2, cap2);
  }

  // net r1q
  {
    sta::Net *sta_net =
        m_sta_network->findNet(m_sta_network->topInstance(), "r1q");
    parasitics->deleteReducedParasitics(sta_net, ap);
    sta::Parasitic *parasitic =
        parasitics->makeParasiticNetwork(sta_net, false, ap);
    // n1{ r1:Q }, n2{ u2:A1 }
    sta::Instance *r1 =
        m_sta_network->findInstanceRelative(m_sta_network->topInstance(), "r1");
    sta::Pin *pin1 = m_sta_network->findPin(r1, "Q");
    sta::ParasiticNode *n1 =
        parasitics->ensureParasiticNode(parasitic, pin1, m_sta_network);
    sta::Instance *u2 =
        m_sta_network->findInstanceRelative(m_sta_network->topInstance(), "u2");
    sta::Pin *pin2 = m_sta_network->findPin(u2, "A1");
    sta::ParasiticNode *n2 =
        parasitics->ensureParasiticNode(parasitic, pin2, m_sta_network);
    parasitics->incrCap(n1, cap1);
    parasitics->makeResistor(parasitic, 1, res1, n1, n2);
    parasitics->incrCap(n2, cap2);
  }

  // net r2q
  {
    sta::Net *sta_net =
        m_sta_network->findNet(m_sta_network->topInstance(), "r2q");
    parasitics->deleteReducedParasitics(sta_net, ap);
    sta::Parasitic *parasitic =
        parasitics->makeParasiticNetwork(sta_net, false, ap);
    // n1{ r2:Q }, n2{ u1:A }
    sta::Instance *r2 =
        m_sta_network->findInstanceRelative(m_sta_network->topInstance(), "r2");
    sta::Pin *pin1 = m_sta_network->findPin(r2, "Q");
    sta::ParasiticNode *n1 =
        parasitics->ensureParasiticNode(parasitic, pin1, m_sta_network);
    sta::Instance *u1 =
        m_sta_network->findInstanceRelative(m_sta_network->topInstance(), "u1");
    sta::Pin *pin2 = m_sta_network->findPin(u1, "A");
    sta::ParasiticNode *n2 =
        parasitics->ensureParasiticNode(parasitic, pin2, m_sta_network);
    parasitics->incrCap(n1, cap1);
    parasitics->makeResistor(parasitic, 1, res1, n1, n2);
    parasitics->incrCap(n2, cap2);
  }

  // net u1z
  {
    sta::Net *sta_net =
        m_sta_network->findNet(m_sta_network->topInstance(), "u1z");
    parasitics->deleteReducedParasitics(sta_net, ap);
    sta::Parasitic *parasitic =
        parasitics->makeParasiticNetwork(sta_net, false, ap);
    // n1{ u1:Z }, n2{ u2:A2 }
    sta::Instance *u1 =
        m_sta_network->findInstanceRelative(m_sta_network->topInstance(), "u1");
    sta::Pin *pin1 = m_sta_network->findPin(u1, "Z");
    sta::ParasiticNode *n1 =
        parasitics->ensureParasiticNode(parasitic, pin1, m_sta_network);
    sta::Instance *u2 =
        m_sta_network->findInstanceRelative(m_sta_network->topInstance(), "u2");
    sta::Pin *pin2 = m_sta_network->findPin(u2, "A2");
    sta::ParasiticNode *n2 =
        parasitics->ensureParasiticNode(parasitic, pin2, m_sta_network);
    parasitics->incrCap(n1, cap1);
    parasitics->makeResistor(parasitic, 1, res1, n1, n2);
    parasitics->incrCap(n2, cap2);
  }

  // net u2z
  {
    sta::Net *sta_net =
        m_sta_network->findNet(m_sta_network->topInstance(), "u2z");
    parasitics->deleteReducedParasitics(sta_net, ap);
    sta::Parasitic *parasitic =
        parasitics->makeParasiticNetwork(sta_net, false, ap);
    // n1{ u2:ZN }, n2{ r3:D }
    sta::Instance *u1 =
        m_sta_network->findInstanceRelative(m_sta_network->topInstance(), "u2");
    sta::Pin *pin1 = m_sta_network->findPin(u1, "ZN");
    sta::ParasiticNode *n1 =
        parasitics->ensureParasiticNode(parasitic, pin1, m_sta_network);
    sta::Instance *u2 =
        m_sta_network->findInstanceRelative(m_sta_network->topInstance(), "r3");
    sta::Pin *pin2 = m_sta_network->findPin(u2, "D");
    sta::ParasiticNode *n2 =
        parasitics->ensureParasiticNode(parasitic, pin2, m_sta_network);
    parasitics->incrCap(n1, cap1);
    parasitics->makeResistor(parasitic, 1, res1, n1, n2);
    parasitics->incrCap(n2, cap2);
  }

  // net out
  {
    sta::Net *sta_net =
        m_sta_network->findNet(m_sta_network->topInstance(), "out");
    parasitics->deleteReducedParasitics(sta_net, ap);
    sta::Parasitic *parasitic =
        parasitics->makeParasiticNetwork(sta_net, false, ap);
    // n1{ r3:Q }, n2{ out }
    sta::Instance *r3 =
        m_sta_network->findInstanceRelative(m_sta_network->topInstance(), "r3");
    sta::Pin *pin1 = m_sta_network->findPin(r3, "Q");
    sta::ParasiticNode *n1 =
        parasitics->ensureParasiticNode(parasitic, pin1, m_sta_network);
    sta::Pin *pin2 =
        m_sta_network->findPin(m_sta_network->topInstance(), "out");
    sta::ParasiticNode *n2 =
        parasitics->ensureParasiticNode(parasitic, pin2, m_sta_network);
    parasitics->incrCap(n1, cap1);
    parasitics->makeResistor(parasitic, 1, res1, n1, n2);
    parasitics->incrCap(n2, cap2);
  }

  m_sta->delaysInvalid();

  return true;
}
