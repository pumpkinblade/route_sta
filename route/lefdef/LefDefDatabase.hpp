#ifndef __LEF_DEF_DATABASE_HPP__
#define __LEF_DEF_DATABASE_HPP__

#include <memory>
#include <string>
#include <vector>

enum { DIRECTION_INOUT, DIRECTION_INPUT, DIRECTION_OUTPUT };

struct DefCell {
  std::string name;
  std::string libcell_name;

  DefCell() = default;
  DefCell(const std::string &name, const std::string &libcell_name)
      : name(name), libcell_name(libcell_name) {}
};

struct DefIOPin {
  std::string name;
  int direction;

  DefIOPin() = default;
  DefIOPin(const std::string &name) : name(name), direction(DIRECTION_INOUT) {}
  DefIOPin(const std::string &name, int dir) : name(name), direction(dir) {}
};

struct DefNet {
  std::string name;
  std::vector<std::pair<std::string, std::string>> internal_pins;
  std::vector<std::string> io_pins;

  DefNet() = default;
  DefNet(const std::string &name) : name(name) {}
};

struct LefDefDatabase {
  std::string def_design_name;
  std::vector<DefIOPin> def_io_pins;
  std::vector<DefCell> def_cells;
  std::vector<DefNet> def_nets;

  static std::shared_ptr<LefDefDatabase> read(const std::string &lef_file,
                                              const std::string &def_file);
};

#endif
