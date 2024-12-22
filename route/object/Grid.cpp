#include "Grid.hpp"
#include "../util/log.hpp"
#include "Design.hpp"
#include <algorithm>

namespace sca {

Grid::Grid(Design *design) {
  m_design = design;

  // init grid point
  for (const auto &gc : m_design->gridConfigs()) {
    switch (gc.direction) {
    case LayerDirection::Horizontal:
      for (int i = 0; i < gc.count; i++) {
        m_grid_points_y.push_back(gc.start + i * gc.step);
      }
      break;
    case LayerDirection::Vertical:
      for (int i = 0; i < gc.count; i++) {
        m_grid_points_x.push_back(gc.start + i * gc.step);
      }
      break;
    }
  }
  m_grid_points_x.push_back(m_design->dieBox().lx());
  m_grid_points_x.push_back(m_design->dieBox().hx());
  m_grid_points_y.push_back(m_design->dieBox().ly());
  m_grid_points_y.push_back(m_design->dieBox().hy());
  std::sort(m_grid_points_x.begin(), m_grid_points_x.end());
  std::sort(m_grid_points_y.begin(), m_grid_points_y.end());
  m_grid_points_x.erase(
      std::unique(m_grid_points_x.begin(), m_grid_points_x.end()),
      m_grid_points_x.end());
  m_grid_points_y.erase(
      std::unique(m_grid_points_y.begin(), m_grid_points_y.end()),
      m_grid_points_y.end());

  // compute edge length
  m_edge_length_acc_x.push_back(0);
  for (size_t i = 0; i + 2 < m_grid_points_x.size(); i++) {
    int x1 = (m_grid_points_x[i] + m_grid_points_x[i + 1]) / 2;
    int x2 = (m_grid_points_x[i + 1] + m_grid_points_x[i + 2]) / 2;
    m_edge_length_x.push_back(x2 - x1);
    m_edge_length_acc_x.push_back(m_edge_length_acc_x.back() + x2 - x1);
  }
  m_edge_length_acc_y.push_back(0);
  for (size_t i = 0; i + 2 < m_grid_points_y.size(); i++) {
    int y1 = (m_grid_points_y[i] + m_grid_points_y[i + 1]) / 2;
    int y2 = (m_grid_points_y[i + 1] + m_grid_points_y[i + 2]) / 2;
    m_edge_length_y.push_back(y2 - y1);
    m_edge_length_acc_y.push_back(m_edge_length_acc_y.back() + y2 - y1);
  }

  // compute capcity
  size_t num_cell_x = m_grid_points_x.size() - 1;
  size_t num_cell_y = m_grid_points_y.size() - 1;
  size_t num_layer = m_design->technology()->numLayers();
  m_edge_capcity.clear();
  m_edge_capcity.resize(num_layer,
                        std::vector<std::vector<double>>(
                            num_cell_x, std::vector<double>(num_cell_y, 0.f)));
  for (const auto &tc : m_design->trackConfigs()) {
    size_t l = static_cast<size_t>(tc.layer->idx());
    switch (tc.layer->direction()) {
    case LayerDirection::Horizontal: {
      int y = tc.start;
      size_t iy = 0;
      for (int c = 0; c < tc.count; c++) {
        while (y >= m_grid_points_y[iy + 1]) {
          iy++;
        }
        for (size_t ix = 0; ix < num_cell_x - 1; ix++) {
          m_edge_capcity[l][ix][iy] += 1;
        }
        y += tc.step;
      }
    } break;
    case LayerDirection::Vertical: {
      int x = tc.start;
      size_t ix = 0;
      for (int c = 0; c < tc.count; c++) {
        while (x >= m_grid_points_x[ix + 1]) {
          ix++;
        }
        for (size_t iy = 0; iy < num_cell_y - 1; iy++) {
          m_edge_capcity[l][ix][iy] += 1;
        }
        x += tc.step;
      }
    } break;
    }
  }
}

DBU Grid::wireLength(const PointT<int> &p, const PointT<int> &q) const {
  const int init_x = std::min(p.x, q.x);
  const int final_x = std::max(p.x, q.x);
  const int init_y = std::min(p.y, q.y);
  const int final_y = std::max(p.y, q.y);
  return m_edge_length_acc_x[static_cast<size_t>(final_x)] -
         m_edge_length_acc_x[static_cast<size_t>(init_x)] +
         m_edge_length_acc_y[static_cast<size_t>(final_y)] -
         m_edge_length_acc_y[static_cast<size_t>(init_y)];
}

void Grid::computeAccessPoints(Pin *pin, std::vector<PointOnLayerT<int>> &pts) {
  Instance *inst = pin->instance();
  Libcell *libcell = inst->libcell();
  BoxT<DBU> inst_box(inst->lx(), inst->ly(),
                     inst->lx() + static_cast<DBU>(libcell->width()),
                     inst->ly() + static_cast<DBU>(libcell->height()));
  Port *port = libcell->findPort(pin->name());
  double dbu = m_design->dbu();
  ASSERT(port, "null port");

  for (int i = 0; i < port->numShapes(); i++) {
    auto [layer, box] = port->shape(i);
    int z = layer->idx();
    BoxT<DBU> pin_box(
        static_cast<DBU>(box.lx() * dbu), static_cast<DBU>(box.ly() * dbu),
        static_cast<DBU>(box.hx() * dbu), static_cast<DBU>(box.hy() * dbu));
    pin_box = getInternalPinBox(inst->orientation(), inst_box, pin_box);

    auto lx_it = std::upper_bound(m_grid_points_x.begin(),
                                  m_grid_points_x.end(), pin_box.lx());
    auto ly_it = std::upper_bound(m_grid_points_y.begin(),
                                  m_grid_points_y.end(), pin_box.ly());
    auto hx_it = std::lower_bound(m_grid_points_x.begin(),
                                  m_grid_points_x.end(), pin_box.hx());
    auto hy_it = std::lower_bound(m_grid_points_y.begin(),
                                  m_grid_points_y.end(), pin_box.hy());
    int lx =
        static_cast<int>(std::distance(m_grid_points_x.begin(), lx_it)) - 1;
    int ly =
        static_cast<int>(std::distance(m_grid_points_y.begin(), ly_it)) - 1;
    int hx = static_cast<int>(std::distance(m_grid_points_x.begin(), hx_it));
    int hy = static_cast<int>(std::distance(m_grid_points_y.begin(), hy_it));
    for (int x = lx; x < hx; x++) {
      for (int y = ly; y < hy; y++) {
        pts.emplace_back(z, x, y);
      }
    }
  }

  std::sort(pts.begin(), pts.end());
  pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
}

} // namespace sca
