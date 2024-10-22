#include "LefDefDatabase.hpp"
#include "../util/log.hpp"
#include <cstring>
#include <defrReader.hpp>
#include <lefrReader.hpp>
#include <sta/Report.hh>

static int lefLayerCbk(lefrCallbackType_e, lefiLayer *layer,
                       lefiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  if (strcmp(layer->type(), "ROUTING") == 0) {
    db->layers.emplace_back();
    db->layers.back().name = layer->name();
    db->layers.back().direction = (strcmp(layer->direction(), "HORIZONTAL") == 0
                                       ? LayerDirection::Horizontal
                                       : LayerDirection::Vertical);
    db->layers.back().sq_res = static_cast<float>(layer->resistance());
    db->layers.back().sq_cap = static_cast<float>(layer->capacitance());
    db->layers.back().edge_cap = static_cast<float>(layer->edgeCap());
    db->layers.back().wire_width = static_cast<float>(layer->width());
    if (layer->hasXYOffset()) {
      db->layers.back().track_offset_x = static_cast<float>(layer->offsetX());
      db->layers.back().track_offset_y = static_cast<float>(layer->offsetY());
    } else if (layer->hasOffset()) {
      db->layers.back().track_offset_x = static_cast<float>(layer->offset());
      db->layers.back().track_offset_y = static_cast<float>(layer->offset());
    } else {
      db->layers.back().track_offset_x = db->layers.back().track_offset_y = 0.f;
    }
  } else if (strcmp(layer->type(), "CUT") == 0) {
    db->cut_layers.emplace_back();
    db->cut_layers.back().name = layer->name();
    db->cut_layers.back().res = static_cast<float>(layer->resistancePerCut());
  }
  return 0;
}

static int lefViaCbk(lefrCallbackType_e, lefiVia *via, lefiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  LefVia lef_via;
  lef_via.name = via->name();
  for (int i = 0; i < via->numLayers(); i++)
    lef_via.layers.push_back(via->layerName(i));
  db->lef_vias.push_back(lef_via);
  return 0;
}

static int lefLibCellBeginCbk(lefrCallbackType_e, const char *name,
                              lefiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  LefLibCell lef_libcell;
  lef_libcell.name = name;
  db->libcells.push_back(lef_libcell);
  return 0;
}

static int lefLibCellCbk(lefrCallbackType_e, lefiMacro *macro,
                         lefiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  LefLibCell &lef_libcell = db->libcells.back();
  lef_libcell.size_x = static_cast<float>(macro->sizeX());
  lef_libcell.size_y = static_cast<float>(macro->sizeY());
  return 0;
}

static int lefPinCbk(lefrCallbackType_e, lefiPin *pin, lefiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  LefPin lef_pin;
  lef_pin.name = pin->name();
  lef_pin.direction = PortDirection::Inout;
  if (pin->hasDirection()) {
    if (std::strcmp(pin->direction(), "OUTPUT") == 0) {
      lef_pin.direction = PortDirection::Output;
    } else if (std::strcmp(pin->direction(), "INPUT") == 0) {
      lef_pin.direction = PortDirection::Input;
    } else {
      lef_pin.direction = PortDirection::Inout;
    }
  }
  for (int i = 0; i < pin->numPorts(); i++) {
    lefiGeometries *geos = pin->port(i);
    std::string layer;
    for (int j = 0; j < geos->numItems(); j++) {
      switch (geos->itemType(j)) {
      case lefiGeomRectE: {
        lefiGeomRect *rect = geos->getRect(j);
        float xl = static_cast<float>(rect->xl);
        float yl = static_cast<float>(rect->yl);
        float xh = static_cast<float>(rect->xh);
        float yh = static_cast<float>(rect->yh);
        lef_pin.layers.push_back(layer);
        lef_pin.boxes.emplace_back(xl, yl, xh, yh);
      } break;
      case lefiGeomLayerE:
        layer = geos->getLayer(j);
        break;
      default:
        LOG_WARN("cant not handle geometry shape except rect");
        break;
      }
    }
  }
  LefLibCell &lef_libcell = db->libcells.back();
  lef_libcell.pins.push_back(lef_pin);
  return 0;
}

static int defDbuCbk(defrCallbackType_e, double dbu, defiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  db->dbu = static_cast<int>(dbu);
  return 0;
}

static int defDieAreaCbk(defrCallbackType_e, defiBox *box, defiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  db->die_lx = box->xl();
  db->die_ly = box->yl();
  db->die_ux = box->xh();
  db->die_uy = box->yh();
  return 0;
}

static int defDesignCbk(defrCallbackType_e, const char *name,
                        defiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  db->design_name = name;
  return 0;
}

static int defComponentCbk(defrCallbackType_e, defiComponent *comp,
                           defiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  DefCell def_cell;
  def_cell.name = comp->id();
  def_cell.libcell_name = comp->name();
  def_cell.orient = static_cast<Orientation>(comp->placementOrient());
  def_cell.lx = comp->placementX();
  def_cell.ly = comp->placementY();
  db->cells.push_back(def_cell);
  return 0;
}

static int defPinCbk(defrCallbackType_e, defiPin *pin, defiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  DefIoPin def_iopin;
  def_iopin.name = pin->pinName();
  def_iopin.direction = PortDirection::Inout;
  if (pin->hasDirection()) {
    if (std::strcmp(pin->direction(), "OUTPUT") == 0) {
      def_iopin.direction = PortDirection::Output;
    } else if (std::strcmp(pin->direction(), "INPUT") == 0) {
      def_iopin.direction = PortDirection::Input;
    } else {
      def_iopin.direction = PortDirection::Inout;
    }
  }
  for (int i = 0; i < pin->numPorts(); i++) {
    defiPinPort *port = pin->pinPort(i);
    int x = port->placementX();
    int y = port->placementX();
    for (int j = 0; j < port->numLayer(); j++) {
      def_iopin.layers.push_back(port->layer(j));
      int xl, yl, xh, yh;
      port->bounds(j, &xl, &yl, &xh, &yh);
      def_iopin.layers.push_back(port->layer(j));
      def_iopin.boxes.emplace_back(x + xl, y + yl, x + xh, y + yh);
    }
  }
  db->iopins.push_back(def_iopin);
  return 0;
}

static int defNetCbk(defrCallbackType_e, defiNet *net, defiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  DefNet def_net;
  def_net.name = net->name();
  for (int i = 0; i < net->numConnections(); i++) {
    if (std::strcmp(net->instance(i), "PIN") == 0) { // io pin
      def_net.iopins.push_back(net->pin(i));
    } else {
      def_net.internal_pins.emplace_back(std::string(net->instance(i)),
                                         std::string(net->pin(i)));
    }
  }
  std::vector<DefSegment> route;
  for (int i = 0; i < net->numWires() && (db->nets.size() == 0); i++) {
    defiWire *wire = net->wire(i);
    for (int j = 0; j < wire->numPaths(); j++) {
      // assume a path only has two point
      DefSegment seg;
      bool is_first_point = true;
      defiPath *path = wire->path(j);
      path->initTraverse();
      for (int e; (e = path->next()) != DEFIPATH_DONE;) {
        switch (e) {
        case DEFIPATH_LAYER: {
          seg.layer_name = path->getLayer();
        } break;
        case DEFIPATH_VIA: {
          seg.via_name = path->getVia();
        } break;
        case DEFIPATH_POINT: {
          if (is_first_point) {
            path->getPoint(&seg.x1, &seg.y1);
            seg.x2 = seg.x1;
            seg.y2 = seg.y1;
            is_first_point = false;
          } else {
            path->getPoint(&seg.x2, &seg.y2);
          }
        } break;
        default:
          break;
        }
      }
      route.push_back(seg);
    }
  }
  db->nets.push_back(def_net);
  db->route_per_net.push_back(std::move(route));
  return 0;
}

static int defTrackCbk(defrCallbackType_e, defiTrack *track,
                       defiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  DefTrack def_track;
  def_track.count = static_cast<int>(track->xNum());
  def_track.step = static_cast<int>(track->xStep());
  def_track.start = static_cast<int>(track->x());
  def_track.direction = std::strcmp(track->macro(), "X") == 0
                            ? LayerDirection::Horizontal
                            : LayerDirection::Vertical;
  def_track.layer_name = track->layer(0);
  db->tracks.push_back(def_track);
  return 0;
}

static int defGridCbk(defrCallbackType_e, defiGcellGrid *grid,
                      defiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  DefGcellGrid def_grid;
  def_grid.direction = std::strcmp(grid->macro(), "X") == 0
                           ? LayerDirection::Horizontal
                           : LayerDirection::Vertical;
  def_grid.start = grid->x();
  def_grid.count = grid->xNum();
  def_grid.step = static_cast<int>(grid->xStep());
  db->gcell_grids.push_back(def_grid);
  return 0;
}

bool LefDefDatabase::read(const std::string &lef_file,
                          const std::string &def_file) {
  int res = 0;
  lefrInit();
  lefrSetUserData(this);
  lefrSetLayerCbk(lefLayerCbk);
  lefrSetViaCbk(lefViaCbk);
  lefrSetMacroBeginCbk(lefLibCellBeginCbk);
  lefrSetMacroCbk(lefLibCellCbk);
  lefrSetPinCbk(lefPinCbk);
  FILE *lef_stream = std::fopen(lef_file.c_str(), "r");
  if (lef_stream == nullptr) {
    return false;
  }
  res = lefrRead(lef_stream, lef_file.c_str(), this);
  std::fclose(lef_stream);
  if (res != 0) {
    return false;
  }

  defrInit();
  defrSetAddPathToNet();
  defrSetUserData(this);
  defrSetUnitsCbk(defDbuCbk);
  defrSetDieAreaCbk(defDieAreaCbk);
  defrSetDesignCbk(defDesignCbk);
  defrSetComponentCbk(defComponentCbk);
  defrSetPinCbk(defPinCbk);
  defrSetNetCbk(defNetCbk);
  defrSetTrackCbk(defTrackCbk);
  defrSetGcellGridCbk(defGridCbk);
  FILE *def_stream = std::fopen(def_file.c_str(), "r");
  if (def_stream == nullptr) {
    return false;
  }
  res = defrRead(def_stream, def_file.c_str(), this, 1);
  std::fclose(def_stream);
  if (res != 0) {
    return false;
  }

  return true;
}
