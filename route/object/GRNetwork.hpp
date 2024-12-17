#ifndef __GR_NETWORK_HPP__
#define __GR_NETWORK_HPP__

#include "GRInstance.hpp"
#include "GRNet.hpp"
#include "GRPin.hpp"
#include <vector>

class LefDatabase;
class DefDatabase;
class GRTechnology;

class GRNetwork {
public:
  GRNetwork(const LefDatabase *lef_db, const DefDatabase *def_db,
            const GRTechnology *tech);

  const std::string &designName() const { return m_design_name; }
  const std::vector<GRInstance *> &instances() const { return m_instances; }
  const std::vector<GRNet *> &nets() const { return m_nets; }
  const std::vector<GRPin *> &pins() const { return m_pins; }

private:
  GRInstance *createInstance(const std::string &name);
  GRPin *createPin(const std::string &name);
  GRNet *createNet(const std::string &name);

private:
  // network
  std::string m_design_name;
  std::vector<std::vector<GRInstance>> m_instance_storage;
  std::vector<std::vector<GRPin>> m_pin_storage;
  std::vector<std::vector<GRNet>> m_net_storage;
  std::vector<GRNet *> m_nets;
  std::vector<GRInstance *> m_instances;
  std::vector<GRPin *> m_pins;
  std::vector<GRNet *> m_snets; // power/ground
};

#endif
