#include "App.hpp"
#include "../util/log.hpp"
#include <fstream>
#include <iomanip>
#include <sta/Network.hh>
#include <sta/NetworkClass.hh>
#include <sta/PortDirection.hh>
#include <sta/Sta.hh>

std::shared_ptr<App> App::s_app = std::make_shared<App>();

bool App::writeSlack(const std::string &file) {
  std::ofstream ostream(file);
  if (!ostream) {
    LOG_ERROR("can not open file %s", file.c_str());
    return false;
  }
  if (m_context == nullptr || m_sta_network == nullptr) {
    LOG_ERROR("the Network has not been initialize");
    return false;
  }
  if (m_sta_network->topInstance() == nullptr) {
    LOG_ERROR("the Network has not been linked");
    return false;
  }

  for (GRNet *net : m_context->getNets()) {
    sta::Net *sta_net = m_sta_network->findNet(m_sta_network->topInstance(),
                                               net->getName().c_str());
    if (sta_net == nullptr) {
      LOG_WARN("can not find net %s", net->getName().c_str());
    } else {
      float slack = sta::Sta::sta()->netSlack(sta_net, sta::MinMax::max());
      ostream << net->getName() << " " << std::setprecision(2) << slack << '\n';
    }
  }

  return true;
}

bool App::readLefDef(const std::string &lef_file, const std::string &def_file) {
  m_lef_file = lef_file;
  m_def_file = def_file;

  auto db = LefDefDatabase::read(m_lef_file, m_def_file);
  if (db == nullptr) {
    LOG_ERROR("error occur while read lef/def file");
    return false;
  }
  m_context = std::make_shared<GRContext>(db);

  m_sta_library = m_sta_network->findLibrary("lefdef");
  if (m_sta_library == nullptr)
    m_sta_library = m_sta_network->makeLibrary("lefdef", nullptr);

  // topcell in def file
  sta::Cell *sta_top_cell =
      m_sta_network->findCell(m_sta_library, m_context->getName().c_str());
  if (sta_top_cell) {
    m_sta_network->deleteCell(sta_top_cell);
  }
  sta_top_cell = m_sta_network->makeCell(
      m_sta_library, db->def_design_name.c_str(), false, m_def_file.c_str());
  for (const GRPin *pin : m_context->getPins()) {
    if (pin->getInstance() != nullptr)
      continue;
    sta::Port *port =
        m_sta_network->makePort(sta_top_cell, pin->getName().c_str());
    switch (pin->getDirection()) {
    case DIRECTION_INOUT:
      m_sta_network->setDirection(port, sta::PortDirection::bidirect());
      break;
    case DIRECTION_INPUT:
      m_sta_network->setDirection(port, sta::PortDirection::input());
      break;
    case DIRECTION_OUTPUT:
      m_sta_network->setDirection(port, sta::PortDirection::output());
      break;
    default:
      m_sta_report->warn(70001, "Unknown direction of io PIN.%s",
                         pin->getName().c_str());
      break;
    }
  }
  return true;
}

sta::Instance *App::linkNetwork(const std::string &top_cell_name) {
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
  for (const GRNet *net : m_context->getNets()) {
    sta::Net *sta_net =
        m_sta_network->makeNet(net->getName().c_str(), sta_top_inst);
    for (const GRPin *pin : net->getPins()) {
      if (pin->getInstance() == nullptr) { // IO pin
        sta::Port *sta_port =
            m_sta_network->findPort(sta_top_cell, pin->getName().c_str());
        if (sta_port == nullptr) {
          m_sta_report->error(70000, "io port name PIN.%s not found",
                              pin->getName().c_str());
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

        GRInstance *inst = pin->getInstance();
        sta::Instance *sta_inst = m_sta_network->findInstanceRelative(
            sta_top_inst, inst->getName().c_str());

        // if the instance not found, then create it
        if (sta_inst == nullptr) {
          sta::LibertyCell *sta_liberty_cell =
              m_sta_network->findLibertyCell(inst->getLibcellName().c_str());
          if (sta_liberty_cell == nullptr) {
            m_sta_report->error(70000, "%s is not a liberty libcell",
                                inst->getLibcellName().c_str());
            m_sta_network->deleteInstance(sta_top_inst);
            return nullptr;
          }
          sta_inst = m_sta_network->makeInstance(
              sta_liberty_cell, inst->getName().c_str(), sta_top_inst);
          sta_inst_cell_map.emplace(sta_inst, sta_liberty_cell);
        }

        // connect
        sta::LibertyCell *sta_liberty_cell = sta_inst_cell_map[sta_inst];
        sta::Cell *sta_cell = reinterpret_cast<sta::Cell *>(sta_liberty_cell);
        sta::Port *sta_port =
            m_sta_network->findPort(sta_cell, pin->getName().c_str());
        if (sta_port == nullptr) {
          m_sta_report->error(70000, "port name %s.%s not found",
                              m_sta_network->name(sta_cell),
                              pin->getName().c_str());
          m_sta_network->deleteInstance(sta_top_inst);
          return nullptr;
        }
        m_sta_network->connect(sta_inst, sta_port, sta_net);
      }
    }
  }
  return sta_top_inst;
}

void App::setStaNetwork(sta::NetworkReader *sta_network) {
  m_sta_network = sta_network;
  m_sta_report = sta::Sta::sta()->report();
}
