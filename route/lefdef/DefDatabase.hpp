#ifndef __DEF_DATABASE_HPP__
#define __DEF_DATABASE_HPP__

#include "../util/common.hpp"
#include "../util/geo.hpp"
#include <string>
#include <vector>

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

struct DefDatabase {
  // network
  std::string design_name;
  std::vector<DefIoPin> iopins;
  std::vector<DefCell> cells;
  std::vector<DefNet> nets;

  // hierachy divisor i.e. / or .
  char divisor;

  // dbu
  int dbu;

  // die size
  int die_lx, die_ly, die_ux, die_uy;

  // routing
  std::vector<std::vector<DefSegment>> route_per_net;

  // grid
  std::vector<DefTrack> tracks;
  std::vector<DefGcellGrid> gcell_grids;

  DefDatabase();
  bool clear();
  // bool read(const char *def_file, bool use_routing);
  bool read(const char *def_file, bool use_routing);
};
#endif
