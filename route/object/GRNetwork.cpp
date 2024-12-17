#include "GRNetwork.hpp"
#include "../lefdef/DefDatabase.hpp"
#include "../lefdef/LefDatabase.hpp"
#include "../util/log.hpp"
#include "GRTechnology.hpp"
#include "GRTree.hpp"
#include <unordered_set>

static utils::BoxT<int> getInternalPinBox(Orientation cell_orient,
                                          const utils::BoxT<int> &cell_box,
                                          const utils::BoxT<int> &pin_box) {
  int lx = cell_box.lx(), ly = cell_box.ly();
  int sx = cell_box.width(), sy = cell_box.height();
  int dx = pin_box.lx(), dy = pin_box.ly();
  int bx = pin_box.width(), by = pin_box.height();
  utils::BoxT<int> box;
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

static utils::BoxT<int> getIoPinBox(Orientation pin_orient,
                                    const utils::PointT<int> &pin_loc,
                                    const utils::BoxT<int> &pin_box) {
  int x = pin_loc.x, y = pin_loc.y;
  int lx = pin_box.lx(), ly = pin_box.ly();
  int hx = pin_box.hx(), hy = pin_box.hy();
  utils::BoxT<int> box;
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

GRNetwork::GRNetwork(const LefDatabase *lef_db, const DefDatabase *def_db,
                     const GRTechnology *tech) {
  m_design_name = def_db->design_name;

  std::unordered_set<std::string> interest_iopin;
  std::unordered_set<std::string> interest_inst;
  for (const auto &def_net : def_db->nets) {
    for (const auto &[inst_name, pin_name] : def_net.internal_pins) {
      interest_inst.insert(inst_name);
    }
    for (const auto &pin_name : def_net.iopins) {
      interest_iopin.insert(pin_name);
    }
  }

  std::unordered_map<std::string, size_t> iopin_name_idx_map;
  std::unordered_map<std::string, size_t> inst_name_idx_map;
  std::unordered_map<std::string, size_t> libcell_name_idx_map;
  std::vector<std::unordered_map<std::string, size_t>>
      libpin_name_idx_map_per_libcell;
  for (size_t i = 0; i < def_db->iopins.size(); i++) {
    if (interest_iopin.find(def_db->iopins[i].name) != interest_iopin.end())
      iopin_name_idx_map.emplace(def_db->iopins[i].name, i);
  }
  for (size_t i = 0; i < def_db->cells.size(); i++) {
    if (interest_inst.find(def_db->cells[i].name) != interest_inst.end())
      inst_name_idx_map.emplace(def_db->cells[i].name, i);
  }
  for (size_t i = 0; i < lef_db->libcells.size(); i++) {
    libcell_name_idx_map.emplace(lef_db->libcells[i].name, i);
    libpin_name_idx_map_per_libcell.emplace_back();
    auto &map = libpin_name_idx_map_per_libcell.back();
    for (size_t j = 0; j < lef_db->libcells[i].pins.size(); j++)
      map.emplace(lef_db->libcells[i].pins[j].name, j);
  }

  std::unordered_map<std::string, GRInstance *> inst_name_map;
  for (const auto &def_net : def_db->nets) {
    GRNet *net = createNet(def_net.name);

    // handle internal pins
    for (const auto &[inst_name, pin_name] : def_net.internal_pins) {
      if (inst_name_map.find(inst_name) == inst_name_map.end()) {
        // create the instance
        GRInstance *inst = createInstance(inst_name);
        size_t idx = inst_name_idx_map.at(inst_name);
        inst->setLibcellName(def_db->cells[idx].libcell_name);
        inst_name_map.emplace(inst_name, inst);
      }
      // create the pin
      GRPin *pin = createPin(pin_name);
      GRInstance *inst = inst_name_map.at(inst_name);
      pin->setInstance(inst);
      pin->setNet(net);
      // add pin to instance & net
      inst->addPin(pin);
      net->addPin(pin);
      // compute pin's access points
      size_t inst_idx = inst_name_idx_map.at(inst_name);
      const auto &def_inst = def_db->cells[inst_idx];
      size_t libcell_idx = libcell_name_idx_map.at(def_inst.libcell_name);
      const auto &lef_libcell = lef_db->libcells[libcell_idx];
      size_t libpin_idx =
          libpin_name_idx_map_per_libcell[libcell_idx].at(pin_name);
      const auto &lef_libpin = lef_libcell.pins[libpin_idx];
      pin->setDirection(lef_libpin.direction);
      utils::BoxT<int> cell_box(
          def_inst.lx, def_inst.ly,
          def_inst.lx + tech->microToDbu(lef_libcell.size_x),
          def_inst.ly + tech->microToDbu(lef_libcell.size_y));
      std::vector<GRPoint> access_points;
      std::vector<utils::BoxOnLayerT<int>> boxes;
      for (size_t j = 0; j < lef_libpin.layers.size(); j++) {
        utils::BoxT<int> pin_box(tech->microToDbu(lef_libpin.boxes[j].lx()),
                                 tech->microToDbu(lef_libpin.boxes[j].ly()),
                                 tech->microToDbu(lef_libpin.boxes[j].hx()),
                                 tech->microToDbu(lef_libpin.boxes[j].hy()));
        utils::BoxOnLayerT<int> box(
            tech->findLayer(lef_libpin.layers[j]),
            getInternalPinBox(def_inst.orient, cell_box, pin_box));
        std::vector<GRPoint> pts = tech->overlapGcells(box);
        access_points.insert(access_points.end(), pts.begin(), pts.end());
        boxes.push_back(box);
      }
      std::sort(access_points.begin(), access_points.end());
      access_points.erase(
          std::unique(access_points.begin(), access_points.end()),
          access_points.end());
      pin->setAccessPoints(access_points);
      pin->setBoxexOnLayerDbu(boxes);
    }
    // handle io pins
    for (const auto &pin_name : def_net.iopins) {
      // create the pin
      GRPin *pin = createPin(pin_name);
      pin->setInstance(nullptr);
      pin->setNet(net);
      // add pin to net
      net->addPin(pin);
      // compute pin's access points
      size_t idx = iopin_name_idx_map.at(pin_name);
      const auto &def_iopin = def_db->iopins[idx];
      pin->setDirection(def_iopin.direction);
      std::vector<GRPoint> access_points;
      std::vector<utils::BoxOnLayerT<int>> boxes;
      for (size_t j = 0; j < def_iopin.layers.size(); j++) {
        utils::BoxOnLayerT<int> box(tech->findLayer(def_iopin.layers[j]),
                                    getIoPinBox(def_iopin.orients[j],
                                                def_iopin.pts[j],
                                                def_iopin.boxes[j]));
        std::vector<GRPoint> pts = tech->overlapGcells(box);
        access_points.insert(access_points.end(), pts.begin(), pts.end());
        boxes.push_back(box);
      }
      std::sort(access_points.begin(), access_points.end());
      access_points.erase(
          std::unique(access_points.begin(), access_points.end()),
          access_points.end());
      pin->setAccessPoints(access_points);
      pin->setBoxexOnLayerDbu(boxes);
    }
  }

  std::unordered_map<std::string, int> via_layer_map;
  for (const auto &lef_via : lef_db->vias) {
    int cut_layer_idx = -1;
    for (const auto &layer_name : lef_via.layers) {
      cut_layer_idx = tech->tryFindCutLayer(layer_name);
      if (cut_layer_idx >= 0)
        break;
    }
    via_layer_map.emplace(lef_via.name, cut_layer_idx);
  }

  for (size_t i = 0; i < def_db->nets.size(); i++) {
    std::vector<std::pair<GRPoint, GRPoint>> segs;
    for (const DefSegment &def_seg : def_db->route_per_net[i]) {
      GRPoint p, q;
      if (def_seg.x1 == def_seg.x2 && def_seg.y1 == def_seg.y2) { // via
        int cut_layer_idx = via_layer_map.at(def_seg.via_name);
        int layer1_idx = cut_layer_idx;
        int layer2_idx = cut_layer_idx + 1;
        p = GRPoint(layer1_idx, def_seg.x1, def_seg.y1);
        q = GRPoint(layer2_idx, def_seg.x1, def_seg.y1);
      } else { // wire
        int layer_idx = tech->findLayer(def_seg.layer_name);
        auto [init_x, final_x] = std::minmax(def_seg.x1, def_seg.x2);
        auto [init_y, final_y] = std::minmax(def_seg.y1, def_seg.y2);
        p = GRPoint(layer_idx, init_x, init_y);
        q = GRPoint(layer_idx, final_x, final_y);
      }
      p = tech->dbuToGcell(p);
      q = tech->dbuToGcell(q);
      segs.emplace_back(p, q);
    }
    // build routing tree using segs
    m_nets[i]->setRoutingTree(buildTree(segs, tech));
  }
}

GRInstance *GRNetwork::createInstance(const std::string &name) {
  if (m_instance_storage.size() == 0 ||
      m_instance_storage.back().size() == 1024) {
    m_instance_storage.emplace_back();
    m_instance_storage.back().reserve(1024);
  }
  m_instance_storage.back().emplace_back(name);
  m_instances.push_back(&m_instance_storage.back().back());
  return m_instances.back();
}

GRPin *GRNetwork::createPin(const std::string &name) {
  if (m_pin_storage.size() == 0 || m_pin_storage.back().size() == 1024) {
    m_pin_storage.emplace_back();
    m_pin_storage.back().reserve(1024);
  }
  m_pin_storage.back().emplace_back(name);
  m_pins.push_back(&m_pin_storage.back().back());
  return m_pins.back();
}

GRNet *GRNetwork::createNet(const std::string &name) {
  if (m_net_storage.size() == 0 || m_net_storage.back().size() == 1024) {
    m_net_storage.emplace_back();
    m_net_storage.back().reserve(1024);
  }
  m_net_storage.back().emplace_back(name);
  m_nets.push_back(&m_net_storage.back().back());
  return m_nets.back();
}
