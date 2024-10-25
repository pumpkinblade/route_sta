#ifndef __GR_TECHNOLOGY_HPP__
#define __GR_TECHNOLOGY_HPP__

#include "../util/common.hpp"
#include "../util/geo.hpp"
#include "GRTree.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class LefDefDatabase;

class GRTechnology {
public:
  GRTechnology(const LefDefDatabase *db);

  // dbu
  void setDbu(int u) { m_dbu = u; }
  int dbu() const { return m_dbu; }
  float dbuToMicro(int u) const { return m_inv_dbu * static_cast<float>(u); }
  float dbuToMeter(int u) const { return dbuToMicro(u) * 1e-6f; }
  int microToDbu(float m) const {
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
  const std::string &layerName(int idx) const {
    return m_layer_name[static_cast<size_t>(idx)];
  }
  LayerDirection layerDirection(int idx) const {
    return m_layer_direction[static_cast<size_t>(idx)];
  }
  float layerRes(int idx) const {
    return m_layer_res[static_cast<size_t>(idx)];
  }
  float layerCap(int idx) const {
    return m_layer_cap[static_cast<size_t>(idx)];
  }
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
  float cutLayerRes(int idx) const {
    return m_cut_layer_res[static_cast<size_t>(idx)];
  }

  // grid
  std::vector<GRPoint> overlapGcells(const utils::BoxOnLayerT<int> &box) const;
  int getWireLengthDbu(const GRPoint &p, const GRPoint &q) const;
  GRPoint dbuToGcell(const GRPoint &p) const;

private:
  // dbu
  int m_dbu;
  float m_inv_dbu;

  // layer
  std::vector<std::string> m_layer_name;
  std::vector<LayerDirection> m_layer_direction;
  std::vector<float> m_layer_cap;
  std::vector<float> m_layer_res;
  std::unordered_map<std::string, int> m_layer_idx_map;
  // cut layer
  std::vector<std::string> m_cut_layer_name;
  std::vector<float> m_cut_layer_res;
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
