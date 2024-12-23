#pragma once

#include "../util/geo.hpp"
#include "Helper.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace sca {

class Layer {
public:
  Layer(const std::string &name, int idx) : m_name(name), m_idx(idx) {}
  const std::string &name() const { return m_name; }
  int idx() const { return m_idx; }

  void setDirection(LayerDirection dir) { m_direction = dir; }
  void setWireWidth(double w) { m_wire_width = w; }
  void setSqRes(double r) { m_sq_res = r; }
  void setSqCap(double c) { m_sq_cap = c; }
  void setEdgeCap(double c) { m_edge_cap = c; }

  LayerDirection direction() const { return m_direction; }
  double wireWidth() const { return m_wire_width; }
  double sqRes() const { return m_sq_res; }
  double sqCap() const { return m_sq_cap; }
  double edgeCap() const { return m_edge_cap; }

private:
  std::string m_name;
  int m_idx;
  LayerDirection m_direction;
  double m_wire_width; // um
  double m_sq_res;     // Ohm/um2
  double m_sq_cap;     // pF/um2
  double m_edge_cap;   // pF/um2
};

class CutLayer {
public:
  CutLayer(const std::string &name, int idx) : m_name(name), m_idx(idx) {}
  const std::string &name() const { return m_name; }
  int idx() const { return m_idx; }

  void setRes(double r) { m_res = r; }
  double res() const { return m_res; }

private:
  std::string m_name;
  int m_idx;
  double m_res;
};

class Port {
public:
  Port(const std::string &name) : m_name(name) {}
  const std::string &name() const { return m_name; }

  void setDirection(PortDirection dir) { m_direction = dir; }
  PortDirection direction() const { return m_direction; }

  int numShapes() const { return static_cast<int>(m_shapes.size()); }
  void addShape(Layer *layer, const BoxT<double> &box);
  const std::pair<Layer *, BoxT<double>> &shape(size_t idx) const {
    return m_shapes[idx];
  }

private:
  std::string m_name;
  PortDirection m_direction;
  std::vector<std::pair<Layer *, BoxT<double>>> m_shapes;
};

class Libcell {
public:
  Libcell(const std::string &name) : m_name(name) {}
  const std::string &name() const { return m_name; }

  int numPorts() const { return static_cast<int>(m_ports.size()); }
  Port *port(size_t idx) { return m_ports[idx].get(); }
  Port *makePort(const std::string &port_name);
  Port *findPort(const std::string &port_name) const;

  void setWidth(double w) { m_width = w; }
  void setHeight(double h) { m_height = h; }
  double width() const { return m_width; }
  double height() const { return m_height; }

private:
  std::string m_name;
  std::vector<std::unique_ptr<Port>> m_ports;
  std::unordered_map<std::string, Port *> m_port_name_map;
  double m_width, m_height;
};

class Technology {
public:
  void reset();

  int numLayers() const { return static_cast<int>(m_layers.size()); }
  int numCutLayers() const { return static_cast<int>(m_cut_layers.size()); }
  int numLibcells() const { return static_cast<int>(m_libcells.size()); }

  Layer *layer (int idx) const { return m_layers[idx].get(); }
  CutLayer *cutLayer(int idx) { return m_cut_layers[idx].get(); }
  Libcell *libcell(int idx) { return m_libcells[idx].get(); }

  Layer *makeLayer(const std::string &layer_name);
  CutLayer *makeCutLayer(const std::string &cut_layer_name);
  Libcell *makeLibcell(const std::string &libcell_name);

  Layer *findLayer(const std::string &layer_name) const;
  CutLayer *findCutLayer(const std::string &cut_layer_name) const;
  Libcell *findLibcell(const std::string &libcell_name) const;

  double layerRes(int idx) const;
  double layerCap(int idx) const;
  double cutLayerRes(int idx) const;

  // void makeVia(CutLayer *layer, const std::string &via_name);
  // size_t findViaCutLayerIdx(const std::string &via_name) const;

private:
  // layer
  std::vector<std::unique_ptr<Layer>> m_layers;
  std::unordered_map<std::string, Layer *> m_layer_name_map;

  // cut layer
  std::vector<std::unique_ptr<CutLayer>> m_cut_layers;
  std::unordered_map<std::string, CutLayer *> m_cut_layer_name_map;

  // libcell
  std::vector<std::unique_ptr<Libcell>> m_libcells;
  std::unordered_map<std::string, Libcell *> m_libcell_name_map;

  // // via
  // std::unordered_map<std::string, size_t> m_via_cut_layer_map;
};

}; // namespace sca
