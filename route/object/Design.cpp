#include "Design.hpp"
#include "Helper.hpp"

namespace sca {

Pin *Instance::makePin(const std::string &pin_name) {
  Pin *pin = makeHelper(pin_name, m_pin_name_map, m_pins, pin_name);
  pin->setInstance(this);
  return pin;
}

Pin *Instance::findPin(const std::string &pin_name) const {
  return findHelper(pin_name, m_pin_name_map);
}

void Net::connect(Pin *pin) {
  m_pins.push_back(pin);
  pin->setNet(this);
}

void Design::setDieBox(const BoxT<DBU> &box) {
  m_die_box = box;
  if (m_top_instance) {
    Libcell *libcell = m_top_instance->libcell();
    libcell->setWidth(box.width());
    libcell->setHeight(box.height());
  }
}

Instance *Design::makeTopInstance(const std::string &design_name) {
  Libcell *libcell = m_tech->makeLibcell(design_name);
  m_top_instance = std::make_unique<Instance>(design_name, libcell);
  m_top_instance->setLx(0);
  m_top_instance->setLy(0);
  m_top_instance->setOrientation(Orientation::N);
  if (m_die_box.IsValid()) {
    libcell->setWidth(m_die_box.width() / m_dbu);
    libcell->setHeight(m_die_box.height() / m_dbu);
  }
  return m_top_instance.get();
}

Instance *Design::makeInstance(const std::string &inst_name, Libcell *libcell) {
  return makeHelper(inst_name, m_instance_name_map, m_instances, inst_name,
                    libcell);
}

Net *Design::makeNet(const std::string &net_name) {
  return makeHelper(net_name, m_net_name_map, m_nets, net_name);
}

Instance *Design::findInstance(const std::string &inst_name) const {
  return findHelper(inst_name, m_instance_name_map);
}

Net *Design::findNet(const std::string &net_name) const {
  return findHelper(net_name, m_net_name_map);
}

} // namespace sca
