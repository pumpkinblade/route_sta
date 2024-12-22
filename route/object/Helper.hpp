#pragma once

#include "../util/geo.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace sca {

using DBU = int;

enum class PortDirection { Unknown, Input, Output, Inout };
enum class LayerDirection { Horizontal, Vertical };
enum class NetUsage { Signal, Clock, Power, Ground };
enum class Orientation { N, W, S, E, FN, FW, FS, FE };

template <class K, class T, class... Args>
T *makeHelper(const K &obj_name, std::unordered_map<K, T *> &obj_map,
              std::vector<std::unique_ptr<T>> &objs, Args &&...args) {
  auto obj = std::make_unique<T>(std::forward<Args>(args)...);
  auto _obj = obj.get();
  obj_map.emplace(obj_name, _obj);
  objs.push_back(std::move(obj));
  return _obj;
}

template <class K, class T>
T *findHelper(const K &obj_name, const std::unordered_map<K, T *> &obj_map) {
  if (obj_map.find(obj_name) == obj_map.end())
    return nullptr;
  return obj_map.at(obj_name);
}

template <class T>
BoxT<T> getExternalPinBox(Orientation pin_orient, PointT<T> pin_loc,
                          BoxT<T> pin_box) {
  int x = pin_loc.x, y = pin_loc.y;
  int lx = pin_box.lx(), ly = pin_box.ly();
  int hx = pin_box.hx(), hy = pin_box.hy();
  BoxT<int> box;
  switch (pin_orient) {
  case Orientation::N:
    box.Set(x + lx, y + ly, x + hx, y + hy);
    break;
  case Orientation::W:
    box.Set(x - hy, y + lx, x - ly, y + hx);
    break;
  case Orientation::S:
    box.Set(x - hx, y - hy, x - lx, y - ly);
    break;
  case Orientation::E:
    box.Set(x + ly, y - hx, x + hy, y - lx);
    break;
  case Orientation::FN:
    box.Set(x - hx, y + ly, x - lx, y + hy);
    break;
  case Orientation::FW:
    box.Set(x + ly, y + lx, x + hy, y + hx);
    break;
  case Orientation::FS:
    box.Set(x + lx, y - hy, x + hx, y - ly);
    break;
  case Orientation::FE:
    box.Set(x - hy, y - hx, x - ly, y - lx);
    break;
  };
  return box;
}

template <class T>
BoxT<T> getInternalPinBox(Orientation cell_orient, BoxT<T> cell_box,
                          BoxT<T> pin_box) {
  int lx = cell_box.lx(), ly = cell_box.ly();
  int sx = cell_box.width(), sy = cell_box.height();
  int dx = pin_box.lx(), dy = pin_box.ly();
  int bx = pin_box.width(), by = pin_box.height();
  BoxT<int> box;
  switch (cell_orient) {
  case Orientation::N:
    box.Set(lx + dx, ly + dy, lx + dx + bx, ly + dy + by);
    break;
  case Orientation::W:
    box.Set(lx + sy - dy - by, ly + dx, lx + sy - dy, ly + dx + bx);
    break;
  case Orientation::S:
    box.Set(lx + sx - dx - bx, ly + sy - dy - by, lx + sx - dx, ly + sy - dy);
    break;
  case Orientation::E:
    box.Set(lx + dy, ly + sx - dx - bx, lx + dy + by, ly + sx - dx);
    break;
  case Orientation::FN:
    box.Set(lx + sx - dx - bx, ly + dy, lx + sx - dx, ly + dy + by);
    break;
  case Orientation::FW:
    box.Set(lx + dy, ly + dx, lx + dy + by, ly + dx + bx);
    break;
  case Orientation::FS:
    box.Set(lx + dx, ly + sy - dy - by, lx + dx + bx, ly + sy - dy);
    break;
  case Orientation::FE:
    box.Set(lx + sy - dy - by, ly + sx - dx - bx, lx + sy - dy, ly + sx - dx);
    break;
  }
  return box;
}
} // namespace sca
