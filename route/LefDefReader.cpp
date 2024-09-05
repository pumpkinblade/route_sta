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

  m_db.readLef(lef_file, m_report);
  m_db.readDef(def_file, m_report);

  m_library = m_network->findLibrary("lefdef");
  if (m_library == nullptr)
    m_library = m_network->makeLibrary("lefdef", nullptr);

  // // libcells defined in lef file
  // for (const auto &libcell : m_db.lef_libcells) {
  //   Cell *cell = m_network->findCell(m_library, libcell.name.c_str());
  //   if (cell) {
  //     m_network->deleteCell(cell);
  //   }
  //   cell = m_network->makeCell(m_library, libcell.name.c_str(), true,
  //                              m_lef_file.c_str());
  //   for (const auto &libpin : libcell.libpins) {
  //     Port *port = m_network->makePort(cell, libpin.name.c_str());
  //     switch (libpin.direction) {
  //     case DIRECTION_INOUT:
  //       m_network->setDirection(port, PortDirection::bidirect());
  //       break;
  //     case DIRECTION_INPUT:
  //       m_network->setDirection(port, PortDirection::input());
  //       break;
  //     case DIRECTION_OUTPUT:
  //       m_network->setDirection(port, PortDirection::output());
  //       break;
  //     default:
  //       m_report->warn(70001, "Unknown direction of pin %s.%s",
  //                      libcell.name.c_str(), libpin.name.c_str());
  //       break;
  //     }
  //   }
  // }

  // topcell in def file
  Cell *top_cell = m_network->findCell(m_library, m_db.def_design_name.c_str());
  if (top_cell) {
    m_network->deleteCell(top_cell);
  }
  top_cell = m_network->makeCell(m_library, m_db.def_design_name.c_str(), false,
                                 m_def_file.c_str());
  for (const auto &io_pin : m_db.def_io_pins) {
    Port *port = m_network->makePort(top_cell, io_pin.name.c_str());
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
  return true;
}

Instance *LefDefReader::linkNetwork(const char *top_cell_name) {
  Cell *top_cell = nullptr;
  if (m_library == nullptr ||
      (top_cell = m_network->findCell(m_library, top_cell_name)) == nullptr) {
    m_report->error(70000, "%s is not a def design.", top_cell_name);
    return nullptr;
  }

  m_report->reportLine("making top instance");
  Instance *top_inst = m_network->makeInstance(top_cell, "", nullptr);

  // m_report->reportLine("making instances");
  // std::unordered_map<Instance *, Cell *> inst_cell_map;
  // for (const auto &def_cell : m_db.def_cells) {
  //   Cell *cell =
  //       m_network->findCell(liberty_library, def_cell.libcell_name.c_str());
  //   if (cell == nullptr) {
  //     m_report->error(70000, "%s is not a lef/liberty libcell.",
  //                     def_cell.libcell_name.c_str());
  //     m_network->deleteInstance(top_inst);
  //     return nullptr;
  //   }
  //   Instance *inst =
  //       m_network->makeInstance(cell, def_cell.name.c_str(), top_inst);
  //   inst_cell_map.emplace(inst, cell);
  // }

  m_report->reportLine("making nets");
  std::unordered_map<std::string, std::string> defcell_leflibcell_map;
  std::unordered_map<Instance *, LibertyCell *> inst_libertycell_map;
  for (const auto &def_cell : m_db.def_cells) {
    defcell_leflibcell_map.emplace(def_cell.name, def_cell.libcell_name);
  }
  for (const auto &def_net : m_db.def_nets) {
    Net *net = m_network->makeNet(def_net.name.c_str(), top_inst);
    for (const auto &[def_cell_name, lef_libpin_name] : def_net.internal_pins) {
      Instance *inst =
          m_network->findInstanceRelative(top_inst, def_cell_name.c_str());

      // if the instance not found, create the def instance
      if (inst == nullptr) {
        if (defcell_leflibcell_map.find(def_cell_name) ==
            defcell_leflibcell_map.end()) {
          m_report->error(70000, "%s is not a def cell", def_cell_name.c_str());
          m_network->deleteInstance(top_inst);
          return nullptr;
        }
        const std::string &lef_libcell_name =
            defcell_leflibcell_map[def_cell_name];
        LibertyCell *liberty_cell =
            m_network->findLibertyCell(lef_libcell_name.c_str());
        if (liberty_cell == nullptr) {
          m_report->error(70000, "%s is not a liberty libcell",
                          lef_libcell_name.c_str());
          m_network->deleteInstance(top_inst);
          return nullptr;
        }
        inst = m_network->makeInstance(liberty_cell, def_cell_name.c_str(),
                                       top_inst);
        inst_libertycell_map.emplace(inst, liberty_cell);
      }

      LibertyCell *liberty_cell = inst_libertycell_map[inst];
      Cell *cell = reinterpret_cast<Cell *>(liberty_cell);
      Port *port = m_network->findPort(cell, lef_libpin_name.c_str());
      if (port == nullptr) {
        m_report->error(70000, "port name %s.%s not found",
                        m_network->name(cell), lef_libpin_name.c_str());
        m_network->deleteInstance(top_inst);
        return nullptr;
      }
      m_network->connect(inst, port, net);
    }

    for (const auto &def_io_pin_name : def_net.io_pins) {
      Port *port = m_network->findPort(top_cell, def_io_pin_name.c_str());
      if (port == nullptr) {
        m_report->error(70000, "io port name PIN.%s not found",
                        def_io_pin_name.c_str());
        m_network->deleteInstance(top_inst);
        return nullptr;
      }
      if (m_network->findPin(top_inst, port) == nullptr) {
        Pin *pin = m_network->makePin(top_inst, port, net);
        m_network->makeTerm(pin, net);
      }
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
    report->error(70000, "the lefdef has not been initialized");
  return lefdef_reader->linkNetwork(top_cell_name);
}
