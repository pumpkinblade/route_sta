#ifndef __GR_CONTEXT_HPP__
#define __GR_CONTEXT_HPP__

#include "../lefdef/LefDefDatabase.hpp"
#include "GRInstance.hpp"
#include "GRNet.hpp"
#include "GRPin.hpp"
#include <vector>

class GRContext {
public:
  GRContext(const std::shared_ptr<LefDefDatabase> &db);

  const std::string &getName() const { return m_name; }
  const std::vector<GRInstance *> &getInstances() const { return m_instances; }
  const std::vector<GRNet *> &getNets() const { return m_nets; }
  const std::vector<GRPin *> &getPins() const { return m_pins; }

private:
  GRInstance *createInstance(const std::string &name);
  GRPin *createPin(const std::string &name);
  GRNet *createNet(const std::string &name);

private:
  std::string m_name;
  std::vector<GRNet> m_net_storage;
  std::vector<GRPin> m_pin_storage;
  std::vector<GRInstance> m_instance_storage;
  std::vector<GRNet *> m_nets;
  std::vector<GRInstance *> m_instances;
  std::vector<GRPin *> m_pins;
};

#endif
