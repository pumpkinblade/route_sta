#include "DefDatabase.hpp"
#include <cstring>
#include <defrReader.hpp>
#include <sta/Report.hh>

static int defDivisorCbk(defrCallbackType_e, const char *divisor,
                         defiUserData data) {
  DefDatabase *db = reinterpret_cast<DefDatabase *>(data);
  db->divisor = divisor[0];
  return 0;
}

static int defDbuCbk(defrCallbackType_e, double dbu, defiUserData data) {
  DefDatabase *db = reinterpret_cast<DefDatabase *>(data);
  db->dbu = static_cast<int>(dbu);
  return 0;
}

static int defDieAreaCbk(defrCallbackType_e, defiBox *box, defiUserData data) {
  DefDatabase *db = reinterpret_cast<DefDatabase *>(data);
  db->die_lx = box->xl();
  db->die_ly = box->yl();
  db->die_ux = box->xh();
  db->die_uy = box->yh();
  return 0;
}

static int defDesignCbk(defrCallbackType_e, const char *name,
                        defiUserData data) {
  DefDatabase *db = reinterpret_cast<DefDatabase *>(data);
  db->design_name = name;
  return 0;
}

static int defComponentCbk(defrCallbackType_e, defiComponent *comp,
                           defiUserData data) {
  DefDatabase *db = reinterpret_cast<DefDatabase *>(data);
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
  DefDatabase *db = reinterpret_cast<DefDatabase *>(data);
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
  if (pin->numPorts() == 0) {
    int x = pin->placementX();
    int y = pin->placementY();
    for (int i = 0; i < pin->numLayer(); i++) {
      int xl, yl, xh, yh;
      pin->bounds(i, &xl, &yl, &xh, &yh);
      def_iopin.orients.push_back(static_cast<Orientation>(pin->orient()));
      def_iopin.boxes.emplace_back(xl, yl, xh, yh);
      def_iopin.pts.emplace_back(x, y);
      def_iopin.layers.push_back(pin->layer(i));
    }
  } else {
    for (int i = 0; i < pin->numPorts(); i++) {
      defiPinPort *port = pin->pinPort(i);
      int x = port->placementX();
      int y = port->placementY();
      for (int j = 0; j < port->numLayer(); j++) {
        int xl, yl, xh, yh;
        port->bounds(j, &xl, &yl, &xh, &yh);
        def_iopin.orients.push_back(static_cast<Orientation>(port->orient()));
        def_iopin.boxes.emplace_back(xl, yl, xh, yh);
        def_iopin.pts.emplace_back(x, y);
        def_iopin.layers.push_back(port->layer(j));
      }
    }
  }
  db->iopins.push_back(def_iopin);
  return 0;
}

static int defNetCbk(defrCallbackType_e, defiNet *net, defiUserData data) {
  DefDatabase *db = reinterpret_cast<DefDatabase *>(data);
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
  for (int i = 0; i < net->numWires(); i++) {
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
        case DEFIPATH_FLUSHPOINT: {
          int ext;
          if (is_first_point) {
            path->getFlushPoint(&seg.x1, &seg.y1, &ext);
            seg.x2 = seg.x1;
            seg.y2 = seg.y1;
            is_first_point = false;
          } else {
            path->getFlushPoint(&seg.x2, &seg.y2, &ext);
          }
          (void)ext;
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
  DefDatabase *db = reinterpret_cast<DefDatabase *>(data);
  DefTrack def_track;
  def_track.count = static_cast<int>(track->xNum());
  def_track.step = static_cast<int>(track->xStep());
  def_track.start = static_cast<int>(track->x());
  def_track.direction = std::strcmp(track->macro(), "X") == 0
                            ? LayerDirection::Vertical
                            : LayerDirection::Horizontal;
  def_track.layer_name = track->layer(0);
  db->tracks.push_back(def_track);
  return 0;
}

static int defGridCbk(defrCallbackType_e, defiGcellGrid *grid,
                      defiUserData data) {
  DefDatabase *db = reinterpret_cast<DefDatabase *>(data);
  DefGcellGrid def_grid;
  def_grid.direction = std::strcmp(grid->macro(), "X") == 0
                           ? LayerDirection::Vertical
                           : LayerDirection::Horizontal;
  def_grid.start = grid->x();
  def_grid.count = grid->xNum();
  def_grid.step = static_cast<int>(grid->xStep());
  db->gcell_grids.push_back(def_grid);
  return 0;
}

DefDatabase::DefDatabase() { clear(); }

bool DefDatabase::clear() {
  design_name = "";
  iopins.clear();
  cells.clear();
  nets.clear();
  dbu = 2000;
  die_lx = die_ly = die_ux = die_uy = 0;
  route_per_net.clear();
  tracks.clear();
  gcell_grids.clear();
  divisor = '/';
  return true;
}

bool DefDatabase::read(const char *def_file, bool use_routing) {
  int res = 0;
  defrInit();
  defrSetUserData(this);
  if (use_routing) {
    defrSetAddPathToNet();
  }
  defrSetDividerCbk(defDivisorCbk);
  defrSetUnitsCbk(defDbuCbk);
  defrSetDieAreaCbk(defDieAreaCbk);
  defrSetDesignCbk(defDesignCbk);
  defrSetComponentCbk(defComponentCbk);
  defrSetPinCbk(defPinCbk);
  defrSetNetCbk(defNetCbk);
  defrSetTrackCbk(defTrackCbk);
  defrSetGcellGridCbk(defGridCbk);
  FILE *def_stream = std::fopen(def_file, "r");
  if (def_stream == nullptr) {
    return false;
  }
  res = defrRead(def_stream, def_file, this, 1);
  std::fclose(def_stream);
  if (res != 0) {
    return false;
  }

  return true;
}
