#pragma once

#include "Grid.hpp"
#include "Route.hpp"
#include "Technology.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace sca {

class Instance;
class Pin;
class Net;

class Pin {
public:
  Pin(const std::string &name) : m_name(name) {}
  const std::string &name() const { return m_name; }

  void setInstance(Instance *inst) { m_instance = inst; }
  void setNet(Net *net) { m_net = net; }
  void setPosition(const PointOnLayerT<int> &pos) { m_position = pos; }

  Instance *instance() const { return m_instance; }
  Net *net() const { return m_net; }
  const PointOnLayerT<int> &position() const { return m_position; }

private:
  std::string m_name;
  Net *m_net;
  Instance *m_instance;
  PointOnLayerT<int> m_position; // on-grid
};

class Instance {
public:
  Instance(const std::string &name, Libcell *libcell)
      : m_name(name), m_libcell(libcell) {}
  const std::string &name() const { return m_name; }
  Libcell *libcell() const { return m_libcell; }

  void setStaName(const std::string &sta_name) { m_sta_name = sta_name; }
  void setLx(DBU lx) { m_lx = lx; }
  void setLy(DBU ly) { m_ly = ly; }
  void setOrientation(Orientation ori) { m_ori = ori; }

  const std::string &staName() const { return m_sta_name; }
  DBU lx() const { return m_lx; }
  DBU ly() const { return m_ly; }
  Orientation orientation() const { return m_ori; }

  Pin *makePin(const std::string &pin_name);
  Pin *findPin(const std::string &pin_name) const;

private:
  std::string m_name;
  Libcell *m_libcell;

  std::string m_sta_name;
  DBU m_lx, m_ly;
  Orientation m_ori;

  std::vector<std::unique_ptr<Pin>> m_pins;
  std::unordered_map<std::string, Pin *> m_pin_name_map;
};

class Net {
public:
  Net(const std::string &name) : m_name(name) {}
  const std::string &name() const { return m_name; }

  void setStaName(const std::string &sta_name) { m_sta_name = sta_name; }
  const std::string &staName() const { return m_sta_name; }

  void connect(Pin *pin);
  int numPins() const { return static_cast<int>(m_pins.size()); }
  Pin *pin(int idx) const { return m_pins[idx]; }

  std::shared_ptr<GRTreeNode> routingTree() const { return m_tree; }
  void setRoutingTree(std::shared_ptr<GRTreeNode> tree) { m_tree = tree; }

private:
  std::string m_name;
  std::string m_sta_name;
  std::vector<Pin *> m_pins;
  std::shared_ptr<GRTreeNode> m_tree;
};

class Design {
public:
  void setTechnology(Technology *tech) { m_tech = tech; }
  void setDbu(double dbu) { m_dbu = dbu; }
  void setDieBox(const BoxT<DBU> &box);
  void addTrackConfig(const TrackConfig &tc) { m_tcs.push_back(tc); }
  void addGridConfig(const GridConfig &gc) { m_gcs.push_back(gc); }
  Grid *makeGrid();

  Technology *technology() const { return m_tech; }
  DBU dbu() const { return m_dbu; }
  const BoxT<DBU> &dieBox() const { return m_die_box; }
  const std::vector<TrackConfig> &trackConfigs() const { return m_tcs; }
  const std::vector<GridConfig> &gridConfigs() const { return m_gcs; }
  Grid *grid() const { return m_grid.get(); }

  Instance *makeTopInstance(const std::string &design_name);
  Instance *topInstance() const { return m_top_instance.get(); }
  const std::string &name() const { return m_top_instance->name(); }

  Instance *makeInstance(const std::string &inst_name, Libcell *libcell);
  Net *makeNet(const std::string &net_name);
  Instance *findInstance(const std::string &inst_name) const;
  Net *findNet(const std::string &net_name) const;
  int numInstances() const { return static_cast<int>(m_instances.size()); }
  int numNets() const { return static_cast<int>(m_nets.size()); }
  Instance *instance(int idx) { return m_instances[idx].get(); }
  Net *net(int idx) { return m_nets[idx].get(); }

  const std::vector<int> &makeNetIndicesToRoute();
  const std::vector<int> &netIndicesToRoute() const { return m_net_indices; }

private:
  Technology *m_tech;
  double m_dbu; // m_dbu * DBU == 1um
  BoxT<DBU> m_die_box;
  std::vector<TrackConfig> m_tcs;
  std::vector<GridConfig> m_gcs;
  std::unique_ptr<Grid> m_grid;

  std::unique_ptr<Instance> m_top_instance;
  std::vector<std::unique_ptr<Instance>> m_instances;
  std::unordered_map<std::string, Instance *> m_instance_name_map;
  std::vector<std::unique_ptr<Net>> m_nets;
  std::unordered_map<std::string, Net *> m_net_name_map;

  std::vector<int> m_net_indices;
};

} // namespace sca
