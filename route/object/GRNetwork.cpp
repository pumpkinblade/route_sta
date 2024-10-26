#include "GRNetwork.hpp"
#include "../lefdef/LefDefDatabase.hpp"
#include "../util/log.hpp"
#include "GRTechnology.hpp"
#include "GRTree.hpp"
#include <unordered_set>

GRNetwork::GRNetwork(const LefDefDatabase *db, const GRTechnology *tech) {
  m_design_name = db->design_name;

  LOG_TRACE("interest iopin and def_cell");
  std::unordered_set<std::string> interest_iopin;
  std::unordered_set<std::string> interest_inst;
  for (const auto &def_net : db->nets) {
    for (const auto &[inst_name, pin_name] : def_net.internal_pins) {
      interest_inst.insert(inst_name);
    }
    for (const auto &pin_name : def_net.iopins) {
      interest_iopin.insert(pin_name);
    }
  }

  LOG_TRACE("iopin/inst/libcell name -> idx map");
  std::unordered_map<std::string, size_t> iopin_name_idx_map;
  std::unordered_map<std::string, size_t> inst_name_idx_map;
  std::unordered_map<std::string, size_t> libcell_name_idx_map;
  std::vector<std::unordered_map<std::string, size_t>>
      libpin_name_idx_map_per_libcell;
  for (size_t i = 0; i < db->iopins.size(); i++) {
    if (interest_iopin.find(db->iopins[i].name) != interest_iopin.end())
      iopin_name_idx_map.emplace(db->iopins[i].name, i);
  }
  for (size_t i = 0; i < db->cells.size(); i++) {
    if (interest_inst.find(db->cells[i].name) != interest_inst.end())
      inst_name_idx_map.emplace(db->cells[i].name, i);
  }
  for (size_t i = 0; i < db->libcells.size(); i++) {
    libcell_name_idx_map.emplace(db->libcells[i].name, i);
    libpin_name_idx_map_per_libcell.emplace_back();
    auto &map = libpin_name_idx_map_per_libcell.back();
    for (size_t j = 0; j < db->libcells[i].pins.size(); j++)
      map.emplace(db->libcells[i].pins[j].name, j);
  }

  LOG_TRACE("create nets");
  std::unordered_map<std::string, GRInstance *> inst_name_map;
  for (const auto &def_net : db->nets) {
    GRNet *net = createNet(def_net.name);

    // handle internal pins
    for (const auto &[inst_name, pin_name] : def_net.internal_pins) {
      if (inst_name_map.find(inst_name) == inst_name_map.end()) {
        // create the instance
        GRInstance *inst = createInstance(inst_name);
        size_t idx = inst_name_idx_map.at(inst_name);
        inst->setLibcellName(db->cells[idx].libcell_name);
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
      const auto &def_inst = db->cells[inst_idx];
      size_t libcell_idx = libcell_name_idx_map.at(def_inst.libcell_name);
      const auto &lef_libcell = db->libcells[libcell_idx];
      size_t libpin_idx =
          libpin_name_idx_map_per_libcell[libcell_idx].at(pin_name);
      const auto &lef_libpin = lef_libcell.pins[libpin_idx];
      int lx = def_inst.lx, ly = def_inst.ly;
      int sx = tech->microToDbu(lef_libcell.size_x),
          sy = tech->microToDbu(lef_libcell.size_y);
      std::vector<GRPoint> access_points;
      for (size_t j = 0; j < lef_libpin.layers.size(); j++) {
        utils::BoxT<float> box = lef_libpin.boxes[j];
        int dx = tech->microToDbu(box.lx());
        int dy = tech->microToDbu(box.ly());
        int bx = tech->microToDbu(box.width());
        int by = tech->microToDbu(box.height());
        utils::BoxOnLayerT<int> box2;
        box2.layerIdx = tech->findLayer(lef_libpin.layers[j]);
        switch (def_inst.orient) {
        case Orientation::N:
          box2.Set(lx + dx, ly + dy, lx + dx + bx, ly + dy + by);
          break;
        case Orientation::S:
          box2.Set(lx + sx - dx - bx, ly + sy - dy - by, lx + sx - dx,
                   ly + sy - dy);
          break;
        case Orientation::FN:
          box2.Set(lx + sx - dx - bx, ly + dy, lx + sx - dx, ly + dy + by);
          break;
        case Orientation::FS:
          box2.Set(lx + dx, ly + sy - dy - by, lx + dx + bx, ly + sy - dy);
          break;
        default:
          LOG_ERROR("Could not handle Orientation(%u)",
                    static_cast<unsigned>(def_inst.orient));
          box2.Set(lx + dx, ly + dy, lx + dx + bx, ly + dy + by);
          break;
        }
        std::vector<GRPoint> pts = tech->overlapGcells(box2);
        access_points.insert(access_points.end(), access_points.begin(),
                             access_points.end());
      }
      std::sort(access_points.begin(), access_points.end());
      access_points.erase(
          std::unique(access_points.begin(), access_points.end()),
          access_points.end());
      pin->setAccessPoints(access_points);
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
      const auto &def_iopin = db->iopins[idx];
      std::vector<GRPoint> access_points;
      for (size_t j = 0; j < def_iopin.layers.size(); j++) {
        utils::BoxT<int> box = def_iopin.boxes[j];
        int layerIdx = tech->findLayer(def_iopin.layers[j]);
        utils::BoxOnLayerT<int> box2(layerIdx, box);
        std::vector<GRPoint> pts = tech->overlapGcells(box2);
        access_points.insert(access_points.end(), pts.begin(), pts.end());
      }
      std::sort(access_points.begin(), access_points.end());
      access_points.erase(
          std::unique(access_points.begin(), access_points.end()),
          access_points.end());
      pin->setAccessPoints(access_points);
    }
  }

  LOG_TRACE("map via to cut_layer");
  std::unordered_map<std::string, int> via_layer_map;
  for (const auto &lef_via : db->lef_vias) {
    int cut_layer_idx = -1;
    for (const auto &layer_name : lef_via.layers) {
      cut_layer_idx = tech->tryFindCutLayer(layer_name);
      if (cut_layer_idx >= 0)
        break;
    }
    via_layer_map.emplace(lef_via.name, cut_layer_idx);
  }

  LOG_TRACE("create routing");
  for (size_t i = 0; i < db->nets.size(); i++) {
    std::vector<std::pair<GRPoint, GRPoint>> segs;
    for (const DefSegment &def_seg : db->route_per_net[i]) {
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
