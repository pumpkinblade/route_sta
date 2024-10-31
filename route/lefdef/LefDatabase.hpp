#ifndef __LEF_DATABASE_HPP__
#define __LEF_DATABASE_HPP__

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
  float res; // Ohm
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

struct LefDatabase {
  // libcell
  std::vector<LefLibCell> libcells;
  // layers
  std::vector<LefLayer> layers;
  std::vector<LefCutLayer> cut_layers;
  // via
  std::vector<LefVia> vias;

  bool clear();
  bool read(const std::string &lef_file);
};

#endif
