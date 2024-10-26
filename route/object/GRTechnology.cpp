#include "GRTechnology.hpp"
#include "../lefdef/LefDefDatabase.hpp"
#include "../util/log.hpp"
#include <algorithm>

GRTechnology::GRTechnology(const LefDefDatabase *db) {
  // dbu
  m_dbu = db->dbu;
  m_inv_dbu = 1.f / static_cast<float>(m_dbu);

  // layer
  m_layer_name.resize(db->layers.size());
  m_layer_direction.resize(db->layers.size());
  m_layer_cap.resize(db->layers.size());
  m_layer_res.resize(db->layers.size());
  for (size_t i = 0; i < db->layers.size(); i++) {
    m_layer_name[i] = db->layers[i].name;
    m_layer_direction[i] = db->layers[i].direction;
    m_layer_cap[i] = db->layers[i].sq_cap * db->layers[i].wire_width +
                     db->layers[i].edge_cap * 2.f;
    m_layer_res[i] = db->layers[i].sq_res * db->layers[i].wire_width;
    m_layer_idx_map.emplace(m_layer_name[i], static_cast<int>(i));
  }

  // cut layer
  m_cut_layer_name.resize(db->cut_layers.size());
  m_cut_layer_res.resize(db->cut_layers.size());
  for (size_t i = 0; i < db->cut_layers.size(); i++) {
    m_cut_layer_name[i] = db->cut_layers[i].name;
    m_cut_layer_res[i] = db->cut_layers[i].res;
    m_cut_layer_idx_map.emplace(m_cut_layer_name[i], static_cast<int>(i));
  }

  // grid
  m_grid_points_x.clear();
  m_grid_points_y.clear();
  for (const auto &def_grid : db->gcell_grids) {
    switch (def_grid.direction) {
    case LayerDirection::Horizontal:
      for (int i = 0; i < def_grid.count; i++) {
        m_grid_points_x.push_back(def_grid.start + i * def_grid.step);
      }
      break;
    case LayerDirection::Vertical:
      for (int i = 0; i < def_grid.count; i++) {
        m_grid_points_y.push_back(def_grid.start + i * def_grid.step);
      }
      break;
    }
  }
  m_grid_points_x.push_back(db->die_lx);
  m_grid_points_x.push_back(db->die_ux);
  m_grid_points_y.push_back(db->die_ly);
  m_grid_points_y.push_back(db->die_uy);
  std::sort(m_grid_points_x.begin(), m_grid_points_x.end());
  std::sort(m_grid_points_y.begin(), m_grid_points_y.end());
  m_grid_points_x.erase(
      std::unique(m_grid_points_x.begin(), m_grid_points_x.end()),
      m_grid_points_x.end());
  m_grid_points_y.erase(
      std::unique(m_grid_points_y.begin(), m_grid_points_y.end()),
      m_grid_points_y.end());

  // edge length
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
  size_t num_cell_y = m_grid_points_x.size() - 1;
  size_t num_layer = m_layer_name.size();
  m_edge_capcity.clear();
  m_edge_capcity.resize(num_layer,
                        std::vector<std::vector<float>>(
                            num_cell_x, std::vector<float>(num_cell_y, 0.f)));
  for (const auto &def_track : db->tracks) {
    int layer_idx = findLayer(def_track.layer_name);
    if (def_track.direction != layerDirection(layer_idx))
      continue;
    size_t l = static_cast<size_t>(layer_idx);
    // TODO: is it necessary to handle the offset defined in lef ?
    switch (def_track.direction) {
    case LayerDirection::Horizontal: {
      int y = def_track.start;
      size_t iy = 0;
      for (int c = 0; c < def_track.count; c++) {
        while (y >= m_grid_points_y[iy + 1]) {
          iy++;
        }
        for (size_t ix = 1; ix < num_cell_x; ix++) {
          m_edge_capcity[l][ix][iy] += 1;
        }
        y += def_track.step;
      }
    } break;
    case LayerDirection::Vertical: {
      int x = def_track.start;
      size_t ix = 0;
      for (int c = 0; c < def_track.count; c++) {
        while (x >= m_grid_points_y[ix + 1]) {
          ix++;
        }
        for (size_t iy = 1; iy < num_cell_y; iy++) {
          m_edge_capcity[l][ix][iy] += 1;
        }
        x += def_track.step;
      }
    } break;
    }
  }
}

std::vector<GRPoint>
GRTechnology::overlapGcells(const utils::BoxOnLayerT<int> &box) const {
  std::vector<GRPoint> pts;
  auto lx_it = std::upper_bound(m_grid_points_x.begin(), m_grid_points_x.end(),
                                box.lx());
  auto ly_it = std::upper_bound(m_grid_points_y.begin(), m_grid_points_y.end(),
                                box.ly());
  auto hx_it = std::lower_bound(m_grid_points_x.begin(), m_grid_points_x.end(),
                                box.hx());
  auto hy_it = std::lower_bound(m_grid_points_y.begin(), m_grid_points_y.end(),
                                box.hy());
  int lx = static_cast<int>(std::distance(m_grid_points_x.begin(), lx_it)) - 1;
  int ly = static_cast<int>(std::distance(m_grid_points_y.begin(), ly_it)) - 1;
  int hx = static_cast<int>(std::distance(m_grid_points_x.begin(), hx_it));
  int hy = static_cast<int>(std::distance(m_grid_points_y.begin(), hy_it));
  for (int x = lx; x < hx; x++) {
    for (int y = ly; y < hy; y++) {
      pts.emplace_back(box.layerIdx, x, y);
    }
  }
  // if (pts.size() == 0 || pts.size() > 10) {
  //   LOG_ERROR("0 gcell pin");
  //   std::printf("[%d, %d] x [%d, %d]\n", box.lx(), box.hx(), box.ly(),
  //               box.hy());
  //   std::printf("      ");
  //   for (auto x : m_grid_points_x) {
  //     std::printf("%5d ", x);
  //   }
  //   std::printf("\n");
  //   for (auto y : m_grid_points_y) {
  //     std::printf("%5d\n", y);
  //   }
  //   std::cout << "\n";
  // }
  return pts;
}

int GRTechnology::getWireLengthDbu(const GRPoint &p, const GRPoint &q) const {
  const int init_x = std::min(p.x, q.x);
  const int final_x = std::max(p.x, q.x);
  const int init_y = std::min(p.y, q.y);
  const int final_y = std::max(p.y, q.y);
  return m_edge_length_acc_x[static_cast<size_t>(final_x)] -
         m_edge_length_acc_x[static_cast<size_t>(init_x)] +
         m_edge_length_acc_y[static_cast<size_t>(final_y)] -
         m_edge_length_acc_y[static_cast<size_t>(init_y)];
}

GRPoint GRTechnology::dbuToGcell(const GRPoint &p) const {
  auto x_it =
      std::lower_bound(m_grid_points_x.begin(), m_grid_points_x.end(), p.x);
  auto y_it =
      std::lower_bound(m_grid_points_y.begin(), m_grid_points_y.end(), p.y);
  int x = static_cast<int>(std::distance(m_grid_points_x.begin(), x_it));
  int y = static_cast<int>(std::distance(m_grid_points_y.begin(), y_it));
  return GRPoint(p.layerIdx, x, y);
}
