#include "GRContext.hpp"
#include <unordered_map>
#include <unordered_set>

GRContext::GRContext(const std::shared_ptr<LefDefDatabase> &db) {
  m_name = db->def_design_name;

  // reserve space for instance, pin, net
  size_t max_num_instance = 0, max_num_pin = 0, max_num_net = 0;
  std::unordered_set<std::string> inst_name_set;
  for (const auto &def_net : db->def_nets) {
    max_num_net++;
    max_num_pin += def_net.internal_pins.size() + def_net.io_pins.size();
    for (const auto &[inst_name, pin_name] : def_net.internal_pins) {
      inst_name_set.insert(inst_name);
    }
  }
  max_num_instance = inst_name_set.size();
  m_instance_storage.reserve(max_num_instance);
  m_instances.reserve(max_num_instance);
  m_pin_storage.reserve(max_num_pin);
  m_pins.reserve(max_num_pin);
  m_net_storage.reserve(max_num_net);
  m_nets.reserve(max_num_net);

  // record instance - libcell map
  std::unordered_map<std::string, std::string> inst_cell_map;
  for (const auto &def_inst : db->def_cells) {
    inst_cell_map.emplace(def_inst.name, def_inst.libcell_name);
  }

  // create nets
  std::unordered_map<std::string, GRInstance *> inst_name_map;
  for (const auto &def_net : db->def_nets) {
    GRNet *net = createNet(def_net.name);

    // handle internal pins
    for (const auto &[inst_name, pin_name] : def_net.internal_pins) {
      if (inst_name_map.find(inst_name) == inst_name_map.end()) {
        // create the instance
        GRInstance *inst = createInstance(inst_name);
        inst->setLibcellName(inst_cell_map[inst_name]);
        inst_name_map.emplace(inst_name, inst);
      }
      // create the pin
      GRPin *pin = createPin(pin_name);
      GRInstance *inst = inst_name_map.at(inst_name);
      pin->setInstance(inst);
      pin->setNet(net);
      // add pin to instance & net
      inst->addPin(pin);
      net->addPin(pin);
    }
    // handle io pins
    for (const auto &pin_name : def_net.io_pins) {
      // create the pin
      GRPin *pin = createPin(pin_name);
      pin->setInstance(nullptr);
      pin->setNet(net);
      // add pin to net
      net->addPin(pin);
    }
  }
}

GRInstance *GRContext::createInstance(const std::string &name) {
  m_instance_storage.emplace_back(name);
  m_instances.push_back(&m_instance_storage.back());
  return m_instances.back();
}

GRPin *GRContext::createPin(const std::string &name) {
  m_pin_storage.emplace_back(name);
  m_pins.push_back(&m_pin_storage.back());
  return m_pins.back();
}

GRNet *GRContext::createNet(const std::string &name) {
  m_net_storage.emplace_back(name);
  m_nets.push_back(&m_net_storage.back());
  return m_nets.back();
}
