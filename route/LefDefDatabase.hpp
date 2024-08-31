#ifndef __LEF_DEF_DATABASE_HPP__
#define __LEF_DEF_DATABASE_HPP__

#include <string>
#include <unordered_map>
#include <vector>

enum { DIRECTION_INOUT, DIRECTION_INPUT, DIRECTION_OUTPUT };

struct LefLibpin {
  std::string name;
  int direction;

  LefLibpin() = default;
  LefLibpin(const std::string &name) : name(name), direction(DIRECTION_INOUT) {}
  LefLibpin(const std::string &name, int dir) : name(name), direction(dir) {}
};

struct LefLibcell {
  std::string name;
  std::unordered_map<std::string, LefLibpin> libpinNameMap;

  LefLibcell() = default;
  LefLibcell(const std::string &name) : name(name) {}
};

struct DefCell {
  std::string name;
  std::string libcellName;

  DefCell() = default;
  DefCell(const std::string &name, const std::string &libcellName)
      : name(name), libcellName(libcellName) {}
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
  std::vector<std::pair<std::string, std::string>> internalPins;
  std::vector<std::string> ioPins;

  DefNet() = default;
  DefNet(const std::string &name) : name(name) {}
};

struct LefDefDatabase {
  LefLibcell currentLibcell;
  std::unordered_map<std::string, LefLibcell> libcellNameMap;
  std::unordered_map<std::string, DefCell> cellNameMap;
  std::unordered_map<std::string, DefNet> netNameMap;
  std::unordered_map<std::string, DefIOPin> ioPinNameMap;

  void readLef(const char *file);
  void readDef(const char *file);
};

#endif
