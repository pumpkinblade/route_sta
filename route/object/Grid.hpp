#pragma once

#include "Technology.hpp"

namespace sca {

class Design;
class Instance;
class Pin;
class Net;

struct TrackConfig {
  Layer *layer;
  DBU start, step;
  int count;
};

struct GridConfig {
  LayerDirection direction;
  DBU start, step;
  int count;
};

class Grid {
public:
  Grid(Design *design);

  int sizeX() const { return static_cast<int>(m_grid_points_x.size() - 1); }
  int sizeY() const { return static_cast<int>(m_grid_points_y.size() - 1); }
  DBU edgeLengthX(int x) const {
    return m_edge_length_x[static_cast<size_t>(x)];
  }
  DBU edgeLengthY(int y) const {
    return m_edge_length_y[static_cast<size_t>(y)];
  }
  DBU wireLength(const PointT<int> &p, const PointT<int> &q) const;
  double edgeCapacity(int l, int x, int y) const {
    return m_edge_capcity[l][x][y];
  }

  void computeAccessPoints(Pin *pin, std::vector<PointOnLayerT<int>> &pts);

private:
  Design *m_design;

  std::vector<DBU> m_grid_points_x;     // dbu
  std::vector<DBU> m_grid_points_y;     // dbu
  std::vector<DBU> m_edge_length_x;     // dbu
  std::vector<DBU> m_edge_length_y;     // dbu
  std::vector<DBU> m_edge_length_acc_x; // dbu
  std::vector<DBU> m_edge_length_acc_y; // dbu
  std::vector<std::vector<std::vector<double>>> m_edge_capcity;
};

} // namespace sca
