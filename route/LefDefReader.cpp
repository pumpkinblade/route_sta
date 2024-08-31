#include "LefDefReader.hpp"
#include "LefDefDatabase.hpp"
#include "utils.hpp"
#include <cstring>
#include <sta/PortDirection.hh>

using namespace sta;

LefDefReader *lefdef_reader;

LefDefReader::LefDefReader(NetworkReader *network)
    : m_network(network), m_library(nullptr) {
  network->setLinkFunc(linkLefDefNetwork);
}

bool LefDefReader::read(const char *lef_file, const char *def_file) {
  m_lef_file = lef_file;
  m_def_file = def_file;

  LefDefDatabase db;
  db.readLef(lef_file);
  db.readDef(def_file);

  m_library = m_network->findLibrary("lefdef");
  if (m_library == nullptr)
    m_library = m_network->makeLibrary("lefdef", nullptr);

  // libcells defined in lef file
  for (const auto &[name, libcell] : db.libcellNameMap) {
    Cell *cell = m_network->findCell(m_library, libcell.name.c_str());
    if (cell) {
      m_network->deleteCell(cell);
    }
    cell = m_network->makeCell(m_library, libcell.name.c_str(), true,
                               m_lef_file.c_str());
    for (const auto &[libpin_name, libpin] : libcell.libpinNameMap) {
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
        LOG_WARN("Unknown direction of pin %s.%s", libcell.name.c_str(),
                 libpin.name.c_str());
        break;
      }
    }
  }

  // topcell in def file
  for (const auto &[io_pin_name, io_pin] : db.ioPinNameMap) {
    Cell *cell = m_network->findCell(m_library, "top");
    if (cell) {
      m_network->deleteCell(cell);
    }
    cell = m_network->makeCell(m_library, "top", true, m_def_file.c_str());
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
      LOG_WARN("Unknown direction of pin %s.%s", io_pin.name.c_str(),
               io_pin.name.c_str());
      break;
    }
  }
  return true;
}

bool readLefDefFile(const char *lef_file, const char *def_file,
                    NetworkReader *network) {
  if (lefdef_reader == nullptr)
    lefdef_reader = new LefDefReader(network);
  return lefdef_reader->read(lef_file, def_file);
}

Instance *linkLefDefNetwork(const char *top_cell_name, bool make_black_boxes,
                            Report *report, NetworkReader *network) {
  return nullptr;
}
