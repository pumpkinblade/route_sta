#include "Context.hpp"
#include "../cugr2/GlobalRouter.h"
#include "../lefdef/LefDefDatabase.hpp"
#include "../util/log.hpp"
#include <fstream>
#include <iomanip>
#include <sta/Network.hh>
#include <sta/PortDirection.hh>
#include <sta/Report.hh>
#include <sta/Sta.hh>
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

  for (GRNet *net : m_network->nets()) {
    float slack = m_parasitics_builder->getNetSlack(net);
    ostream << net->name() << " " << std::setprecision(2) << slack << '\n';
  }

  return true;
}

bool Context::readLefDef(const std::string &lef_file,
                         const std::string &def_file, bool use_routing) {
  m_sta_network = sta::Sta::sta()->networkReader();
  m_sta_report = sta::Sta::sta()->report();
  m_sta_network->setLinkFunc(link);

  m_lef_file = lef_file;
  m_def_file = def_file;

  LefDefDatabase db;
  bool success = db.read(m_lef_file, m_def_file, use_routing);
  if (!success)
    return false;

  LOG_INFO("read lef/def done");
  m_tech = std::make_unique<GRTechnology>(&db);
  LOG_INFO("init tech done");
  m_network = std::make_unique<GRNetwork>(&db, m_tech.get());
  LOG_INFO("init network done");
  m_parasitics_builder =
      std::make_unique<MakeWireParasitics>(m_network.get(), m_tech.get());
  LOG_INFO("init parasitics_builder done");

  m_sta_library = m_sta_network->findLibrary("lefdef");
  if (m_sta_library == nullptr)
    m_sta_library = m_sta_network->makeLibrary("lefdef", nullptr);

  // topcell in def file
  sta::Cell *sta_top_cell =
      m_sta_network->findCell(m_sta_library, m_network->designName().c_str());
  if (sta_top_cell) {
    m_sta_network->deleteCell(sta_top_cell);
  }
  sta_top_cell = m_sta_network->makeCell(m_sta_library, db.design_name.c_str(),
                                         false, m_def_file.c_str());
  for (const DefIoPin &iopin : db.iopins) {
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
              m_sta_network->makePin(sta_top_inst, sta_port, sta_net);
          m_sta_network->makeTerm(sta_pin, sta_net);
        }
        m_sta_network->connect(sta_top_inst, sta_port, sta_net);
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
        m_sta_network->connect(sta_inst, sta_port, sta_net);
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
