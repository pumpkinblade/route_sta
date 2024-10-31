#ifndef __GR_TECHNOLOGY_HPP__
#define __GR_TECHNOLOGY_HPP__

#include "../util/common.hpp"
#include "../util/geo.hpp"
#include "GRTree.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class LefDatabase;
class DefDatabase;

class GRTechnology {
public:
  GRTechnology(const LefDatabase *lef_db, const DefDatabase *def_db);

  // dbu
  void setDbu(int u) { m_dbu = u; }
  int dbu() const { return m_dbu; }
  double dbuToMicro(int u) const { return m_inv_dbu * static_cast<double>(u); }
  double dbuToMeter(int u) const { return dbuToMicro(u) * 1e-6; }
  int microToDbu(double m) const {
    return static_cast<int>(static_cast<double>(m_dbu) * m);
  }

  // layer
  int findLayer(const std::string &name) const {
    return m_layer_idx_map.at(name);
  }
  int tryFindLayer(const std::string &name) const {
    return m_layer_idx_map.find(name) == m_layer_idx_map.end()
               ? -1
               : m_layer_idx_map.at(name);
  }
  int numLayers() const { return static_cast<int>(m_layer_name.size()); }
  const std::string &layerName(int idx) const {
    return m_layer_name[static_cast<size_t>(idx)];
  }
  LayerDirection layerDirection(int idx) const {
    return m_layer_direction[static_cast<size_t>(idx)];
  }
  double layerRes(int idx) const {
    return m_layer_res[static_cast<size_t>(idx)];
  }
  double layerCap(int idx) const {
    return m_layer_cap[static_cast<size_t>(idx)];
  }
  int minRoutingLayer() const { return 1; }
  // cut layer
  int findCutLayer(const std::string &name) const {
    return m_cut_layer_idx_map.at(name);
  }
  int tryFindCutLayer(const std::string &name) const {
    return m_cut_layer_idx_map.find(name) == m_cut_layer_idx_map.end()
               ? -1
               : m_cut_layer_idx_map.at(name);
  }
  const std::string &cutLayerName(int idx) const {
    return m_cut_layer_name[static_cast<size_t>(idx)];
  }
  double cutLayerRes(int idx) const {
    return m_cut_layer_res[static_cast<size_t>(idx)];
  }

  // grid
  std::vector<GRPoint> overlapGcells(const utils::BoxOnLayerT<int> &box) const;
  GRPoint dbuToGcell(const GRPoint &p) const;
  int numGcellX() const { return static_cast<int>(m_grid_points_x.size() - 1); }
  int numGcellY() const { return static_cast<int>(m_grid_points_y.size() - 1); }
  int edgeLengthDbuX(int x) const {
    return m_edge_length_x[static_cast<size_t>(x)];
  }
  int edgeLengthDbuY(int y) const {
    return m_edge_length_y[static_cast<size_t>(y)];
  }
  float edgeCapacity(int l, int x, int y) const {
    return m_edge_capcity[l][x][y];
  }
  int layerMinLengthDbu(int idx) const {
    return layerDirection(idx) == LayerDirection::Horizontal
               ? *std::min_element(m_edge_length_x.begin(),
                                   m_edge_length_x.end())
               : *std::min_element(m_edge_length_y.begin(),
                                   m_edge_length_y.end());
  }
  int getWireLengthDbu(const GRPoint &p, const GRPoint &q) const;

private:
  // dbu
  int m_dbu;
  double m_inv_dbu;

  // layer
  std::vector<std::string> m_layer_name;
  std::vector<LayerDirection> m_layer_direction;
  std::vector<double> m_layer_cap; // pF/um
  std::vector<double> m_layer_res; // Ohm/um
  std::unordered_map<std::string, int> m_layer_idx_map;
  // cut layer
  std::vector<std::string> m_cut_layer_name;
  std::vector<double> m_cut_layer_res; // Ohm
  std::unordered_map<std::string, int> m_cut_layer_idx_map;

  // grid
  std::vector<int> m_grid_points_x;     // dbu
  std::vector<int> m_grid_points_y;     // dbu
  std::vector<int> m_edge_length_x;     // dbu
  std::vector<int> m_edge_length_y;     // dbu
  std::vector<int> m_edge_length_acc_x; // dbu
  std::vector<int> m_edge_length_acc_y; // dbu
  std::vector<std::vector<std::vector<float>>> m_edge_capcity;
};

#endif
