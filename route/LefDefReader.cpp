#include "LefDefReader.hpp"
#include "LefDefDatabase.hpp"
#include <cstring>
#include <sta/PortDirection.hh>
#include <sta/Report.hh>
#include <sta/Sta.hh>

using namespace sta;

LefDefReader *lefdef_reader;

LefDefReader::LefDefReader(NetworkReader *network)
    : m_report(network->report()), m_debug(network->debug()),
      m_network(network), m_library(nullptr) {
  network->setLinkFunc(linkLefDefNetwork);
}

bool LefDefReader::read(const char *lef_file, const char *def_file) {
  m_db = {};
  m_lef_file = lef_file;
  m_def_file = def_file;

  m_db.readLef(lef_file);
  m_db.readDef(def_file);

  m_library = m_network->findLibrary("lefdef");
  if (m_library == nullptr)
    m_library = m_network->makeLibrary("lefdef", nullptr);

  // libcells defined in lef file
  for (const auto &libcell : m_db.lef_libcells) {
    Cell *cell = m_network->findCell(m_library, libcell.name.c_str());
    if (cell) {
      m_network->deleteCell(cell);
    }
    cell = m_network->makeCell(m_library, libcell.name.c_str(), true,
                               m_lef_file.c_str());
    for (const auto &libpin : libcell.libpins) {
      Port *port = m_network->makePort(cell, libpin.name.c_str());
      switch (libpin.direction) {
      case DIRECTION_INOUT:
        m_network->setDirection(port, PortDirection::bidirect());
        break;
      case DIRECTION_INPUT:
        m_network->setDirection(port, PortDirection::input());
        break;
      case DIRECTION_OUTPUT:
        m_network->setDirection(port, PortDirection::output());
        break;
      default:
        m_report->warn(70001, "Unknown direction of pin %s.%s",
                       libcell.name.c_str(), libpin.name.c_str());
        break;
      }
    }
  }

  // topcell in def file
  for (const auto &io_pin : m_db.def_io_pins) {
    Cell *cell = m_network->findCell(m_library, "__def_top");
    if (cell) {
      m_network->deleteCell(cell);
    }
    cell =
        m_network->makeCell(m_library, "__def_top", false, m_def_file.c_str());
    Port *port = m_network->makePort(cell, io_pin.name.c_str());
    switch (io_pin.direction) {
    case DIRECTION_INOUT:
      m_network->setDirection(port, PortDirection::bidirect());
      break;
    case DIRECTION_INPUT:
      m_network->setDirection(port, PortDirection::input());
      break;
    case DIRECTION_OUTPUT:
      m_network->setDirection(port, PortDirection::output());
      break;
    default:
      m_report->warn(70001, "Unknown direction of io PIN.%s",
                     io_pin.name.c_str());
      break;
    }
  }
  for (const auto &io_pin : m_db.def_io_pins) {
    m_report->reportLine("%s", io_pin.name.c_str());
  }
  m_report->reportLine("================================");
  for (const auto &net : m_db.def_nets) {
    for (const auto &io_pin_name : net.io_pins) {
      m_report->reportLine("%s", io_pin_name.c_str());
    }
  }
  return true;
}

Instance *LefDefReader::linkNetwork() {
  std::unordered_map<std::string, Instance *> inst_name_map;
  std::unordered_map<std::string, Cell *> cell_inst_name_map;
  Cell *top_cell = m_network->findCell(m_library, "__def_top");
  if (top_cell == nullptr) {
    m_report->error(70004, "the lefdef library has not been read");
    return nullptr;
  }
  m_report->reportLine("make top instance");
  Instance *top_inst = m_network->makeInstance(top_cell, "", nullptr);
  for (const auto &def_cell : m_db.def_cells) {
    Cell *cell = m_network->findCell(m_library, def_cell.libcell_name.c_str());
    Instance *inst =
        m_network->makeInstance(cell, def_cell.name.c_str(), top_inst);
    inst_name_map.emplace(def_cell.name, inst);
    cell_inst_name_map.emplace(def_cell.name, cell);
  }
  m_report->reportLine("make nets");
  for (const auto &def_net : m_db.def_nets) {
    Net *net = m_network->makeNet(def_net.name.c_str(), top_inst);
    for (const auto &[def_cell_name, lef_libpin_name] : def_net.internal_pins) {
      if (inst_name_map.count(def_cell_name) == 0) {
        m_report->error(70006, "instance name not found");
      }
      Instance *inst = inst_name_map[def_cell_name];
      Cell *cell = cell_inst_name_map[def_cell_name];
      Port *port = m_network->findPort(cell, lef_libpin_name.c_str());
      if (port == nullptr)
        m_report->error(70006, "port name not found");
      m_network->connect(inst, port, net);
    }
    for (const auto &def_io_pin_name : def_net.io_pins) {
      Port *port = m_network->findPort(top_cell, def_io_pin_name.c_str());
      if (port == nullptr) {
        m_report->reportLine("%s", def_io_pin_name.c_str());
        m_report->error(70006, "io name not found");
      }
      if (m_network->findPin(top_inst, port) == nullptr) {
        Pin *pin = m_network->makePin(top_inst, port, net);
        m_network->makeTerm(pin, net);
      }
      Pin *pin = m_network->findPin(top_inst, port);
      m_network->connect(top_inst, port, net);
    }
  }
  return top_inst;
}

bool readLefDefFile(const char *lef_file, const char *def_file,
                    NetworkReader *network) {
  if (lefdef_reader == nullptr)
    lefdef_reader = new LefDefReader(network);
  return lefdef_reader->read(lef_file, def_file);
}

Instance *linkLefDefNetwork(const char *top_cell_name, bool make_black_boxes,
                            Report *report, NetworkReader *network) {
  if (lefdef_reader == nullptr)
    report->error(70005, "the lefdef has not been initialized");
  return lefdef_reader->linkNetwork();
}
