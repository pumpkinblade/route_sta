#include "GRNetwork.hpp"
#include "../lefdef/LefDefDatabase.hpp"
#include "GRTechnology.hpp"
#include "GRTree.hpp"

GRNetwork::GRNetwork(const LefDefDatabase *db, const GRTechnology *tech) {
  m_design_name = db->design_name;

  struct PPin {
    std::string name;
    std::vector<GRPoint> access_points;
  };
  struct PCell {
    std::string name;
    std::string libcell_name;
    std::unordered_map<std::string, PPin> pin_map;
  };

  // map iopin_name to ppin
  std::unordered_map<std::string, PPin> ppin_map;
  for (const auto &def_iopin : db->iopins) {
    PPin ppin;
    ppin.name = def_iopin.name;
    for (size_t j = 0; j < def_iopin.layers.size(); j++) {
      utils::BoxT<int> box = def_iopin.boxes[j];
      int layerIdx = tech->findLayer(def_iopin.layers[j]);
      utils::BoxOnLayerT<int> box2(layerIdx, box);
      std::vector<GRPoint> pts = tech->overlapGcells(box2);
      ppin.access_points.insert(ppin.access_points.end(), pts.begin(),
                                pts.end());
    }
    std::sort(ppin.access_points.begin(), ppin.access_points.end());
    ppin.access_points.erase(
        std::unique(ppin.access_points.begin(), ppin.access_points.end()),
        ppin.access_points.end());
    ppin_map.emplace(ppin.name, ppin);
  }

  // map def_cell to pcell
  std::unordered_map<std::string, PCell> pcell_map;
  for (const auto &cell : db->cells) {
    PCell pcell;
    pcell.name = cell.name;
    pcell.libcell_name = cell.libcell_name;
    // find lef_libcell
    auto libcell = std::find_if(db->libcells.begin(), db->libcells.end(),
                                [&](const LefLibCell &lef_cell) {
                                  return lef_cell.name == pcell.libcell_name;
                                });
    int lx = cell.lx, ly = cell.ly;
    int sx = tech->microToDbu(libcell->size_x),
        sy = tech->microToDbu(libcell->size_y);
    for (const auto &libpin : libcell->pins) {
      // create PPin
      PPin ppin;
      ppin.name = libpin.name;
      for (size_t j = 0; j < libpin.layers.size(); j++) {
        utils::BoxT<float> box = libpin.boxes[j];
        int box_lx = tech->microToDbu(box.lx());
        int box_ly = tech->microToDbu(box.ly());
        int box_hx = tech->microToDbu(box.hx());
        int box_hy = tech->microToDbu(box.hy());
        utils::BoxOnLayerT<int> box2;
        box2.layerIdx = tech->findLayer(libpin.layers[j]);
        switch (cell.orient) {
        case Orientation::N:
          // (x, y) |-> (x, y)
          box2.Set(lx + box_lx, ly + box_ly, lx + box_hx, ly + box_hy);
          break;
        case Orientation::W:
          // (x, y) |-> (sy - y, x)
          box2.Set(lx + sy - box_hy, ly + box_lx, lx + sy - box_ly,
                   ly + box_hx);
          break;
        case Orientation::S:
          // (x, y) |-> (sx - x, sy - y)
          box2.Set(lx + sx - box_hx, ly + sy - box_hy, lx + sx - box_lx,
                   ly + sy - box_ly);
          break;
        case Orientation::E:
          // (x, y) |-> (y, sx - x)
          box2.Set(lx + box_ly, ly + sx - box_hx, lx + box_hy,
                   ly + sx - box_lx);
          break;
        case Orientation::FN:
          // (x, y) |-> (sx - x, y)
          box2.Set(lx + sx - box_hx, ly + box_ly, lx + sx - box_lx,
                   ly + box_hy);
          break;
        case Orientation::FW:
          // (x, y) |-> (y, x)
          box2.Set(lx + box_ly, ly + box_lx, lx + box_hy, ly + box_hx);
          break;
        case Orientation::FS:
          // (x, y) |-> (x, sy - y)
          box2.Set(lx + box_lx, ly + sy - box_ly, lx + box_hx,
                   ly + sy - box_hy);
          break;
        case Orientation::FE:
          // (x, y) |-> (sy - y, sx - x)
          box2.Set(lx + sy - box_hy, ly + sx - box_hx, lx + sy - box_ly,
                   ly + sx - box_lx);
          break;
        }
        std::vector<GRPoint> pts = tech->overlapGcells(box2);
        ppin.access_points.insert(ppin.access_points.end(),
                                  ppin.access_points.begin(),
                                  ppin.access_points.end());
      }
      // map pin_name to ppin
      std::sort(ppin.access_points.begin(), ppin.access_points.end());
      ppin.access_points.erase(
          std::unique(ppin.access_points.begin(), ppin.access_points.end()),
          ppin.access_points.end());
      pcell.pin_map.emplace(ppin.name, ppin);
    }
    pcell_map.emplace(pcell.name, pcell);
  }

  // create nets
  std::unordered_map<std::string, GRInstance *> inst_name_map;
  for (const auto &def_net : db->nets) {
    GRNet *net = createNet(def_net.name);

    // handle internal pins
    for (const auto &[inst_name, pin_name] : def_net.internal_pins) {
      if (inst_name_map.find(inst_name) == inst_name_map.end()) {
        // create the instance
        GRInstance *inst = createInstance(inst_name);
        inst->setLibcellName(pcell_map.at(inst_name).libcell_name);
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
      const PCell &pcell = pcell_map.at(inst_name);
      pin->setAccessPoints(pcell.pin_map.at(pin_name).access_points);
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
      pin->setAccessPoints(ppin_map.at(pin_name).access_points);
    }
  }

  /* std::unordered_map<std::string, int> via_layer_map;
  for (const auto &lef_via : db->lef_vias) {
    int cut_layer_idx = -1;
    for (const auto &layer_name : lef_via.layers) {
      cut_layer_idx = tech->tryFindCutLayer(layer_name);
      if (cut_layer_idx >= 0)
        break;
    }
    via_layer_map.emplace(lef_via.name, cut_layer_idx);
  }

  for (size_t i = 0; i < db->nets.size(); i++) {
    std::vector<std::pair<GRPoint, GRPoint>> segs;
    for (const DefSegment &def_seg : db->route_per_net[i]) {
      GRPoint start, end;
      if (def_seg.x1 == def_seg.x2 && def_seg.y1 == def_seg.y2) { // via
        int cut_layer_idx = via_layer_map.at(def_seg.via_name);
        int layer1_idx = cut_layer_idx;
        int layer2_idx = cut_layer_idx + 1;
        start = GRPoint(layer1_idx, def_seg.x1, def_seg.y1);
        end = GRPoint(layer2_idx, def_seg.x1, def_seg.y1);
      } else { // wire
        int layer_idx = tech->findCutLayer(def_seg.layer_name);
        auto [init_x, final_x] = std::minmax(def_seg.x1, def_seg.x2);
        auto [init_y, final_y] = std::minmax(def_seg.y1, def_seg.y2);
        start = GRPoint(layer_idx, init_x, init_y);
        end = GRPoint(layer_idx, final_x, final_y);
      }
      segs.emplace_back(start, end);
    }
    // build routing tree using segs
    m_nets[i]->setRoutingTree(buildTree(segs));
  } */
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
