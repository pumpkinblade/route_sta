#ifndef __LEF_DEF_DATABASE_HPP__
#define __LEF_DEF_DATABASE_HPP__

#include "../util/common.hpp"
#include "../util/geo.hpp"
#include <string>
#include <vector>

struct LefVia {
  std::string name;
  std::vector<std::string> layers;
};

struct LefLayer {
  std::string name;
  LayerDirection direction;
  float wire_width;     // um
  float sq_res;         // Ohm / um2 // square resistance
  float sq_cap;         // pF / um2 // square capacitance
  float edge_cap;       // pF / um // edge capacitance
  float track_offset_x; // track offset
  float track_offset_y; // track offset
};

struct LefCutLayer {
  std::string name;
  float res;
};

struct LefPin {
  std::string name;
  PortDirection direction;
  std::vector<utils::BoxT<float>> boxes;
  std::vector<std::string> layers;
};

struct LefLibCell {
  std::string name;
  std::vector<LefPin> pins;
  float size_x, size_y;
};

struct DefCell {
  std::string name;
  std::string libcell_name;
  Orientation orient;
  int lx, ly;

  DefCell() = default;
  DefCell(const std::string &name, const std::string &libcell_name)
      : name(name), libcell_name(libcell_name) {}
};

struct DefIoPin {
  std::string name;
  PortDirection direction;
  std::vector<Orientation> orients;
  std::vector<utils::BoxT<int>> boxes;
  std::vector<utils::PointT<int>> pts;
  std::vector<std::string> layers;
};

struct DefNet {
  std::string name;
  std::vector<std::pair<std::string, std::string>> internal_pins;
  std::vector<std::string> iopins;
};

struct DefSegment {
  std::string layer_name;
  std::string via_name;
  int x1, y1, x2, y2; // dbu
};

struct DefTrack {
  std::string layer_name;
  LayerDirection direction;
  int start, count, step;
};

struct DefGcellGrid {
  LayerDirection direction;
  int start, count, step;
};

struct LefDefDatabase {
  // libcell
  std::vector<LefLibCell> libcells;

  // network
  std::string design_name;
  std::vector<DefIoPin> iopins;
  std::vector<DefCell> cells;
  std::vector<DefNet> nets;

  // tech
  int dbu;
  int die_lx, die_ly, die_ux, die_uy;
  std::vector<LefLayer> layers;
  std::vector<LefCutLayer> cut_layers;

  // routing
  std::vector<LefVia> lef_vias;
  std::vector<std::vector<DefSegment>> route_per_net;

  // grid
  std::vector<DefTrack> tracks;
  std::vector<DefGcellGrid> gcell_grids;

  bool read(const std::string &lef_file, const std::string &def_file,
            bool use_routing);
};

#endif
