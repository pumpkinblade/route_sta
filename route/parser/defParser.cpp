#include "../object/Design.hpp"
#include "../util/log.hpp"
#include <cstring>
#include <defrReader.hpp>

namespace sca {

static int designCbk(defrCallbackType_e, const char *name, void *data) {
  Design *design = reinterpret_cast<Design *>(data);
  design->makeTopInstance(name);
  return 0;
}

static int dbuCbk(defrCallbackType_e, double dbu, void *data) {
  Design *design = reinterpret_cast<Design *>(data);
  design->setDbu(dbu);
  return 0;
}

static int dieAreaCbk(defrCallbackType_e, defiBox *box, void *data) {
  Design *design = reinterpret_cast<Design *>(data);
  design->setDieBox(BoxT<DBU>(box->xl(), box->yl(), box->xh(), box->yh()));
  return 0;
}

static int pinCbk(defrCallbackType_e, defiPin *def_pin, void *data) {
  Design *design = reinterpret_cast<Design *>(data);
  Libcell *libcell = design->topInstance()->libcell();
  Port *port = libcell->makePort(def_pin->pinName());
  port->setDirection(PortDirection::Unknown);
  if (def_pin->hasDirection()) {
    if (std::strcmp(def_pin->direction(), "INPUT") == 0) {
      port->setDirection(PortDirection::Input);
    } else if (std::strcmp(def_pin->direction(), "OUTPUT") == 0) {
      port->setDirection(PortDirection::Output);
    } else if (std::strcmp(def_pin->direction(), "INOUT")) {
      port->setDirection(PortDirection::Inout);
    }
  }
  if (def_pin->numPorts() == 0) {
    int x = def_pin->placementX();
    int y = def_pin->placementY();
    for (int i = 0; i < def_pin->numLayer(); i++) {
      int xl, yl, xh, yh;
      def_pin->bounds(i, &xl, &yl, &xh, &yh);
      Orientation ori = static_cast<Orientation>(def_pin->orient());
      BoxT<DBU> box =
          getExternalPinBox(ori, PointT<DBU>(x, y), BoxT<DBU>(xl, yl, xh, yh));
      double dbu = design->dbu();
      BoxT<double> dbox(box.lx() / dbu, box.ly() / dbu, box.hx() / dbu,
                        box.hy() / dbu);
      const char *layer_name = def_pin->layer(i);
      Layer *layer = design->technology()->findLayer(layer_name);
      port->addShape(layer, dbox);
    }
  } else {
    for (int i = 0; i < def_pin->numPorts(); i++) {
      defiPinPort *def_port = def_pin->pinPort(i);
      int x = def_port->placementX();
      int y = def_port->placementY();
      for (int j = 0; j < def_port->numLayer(); j++) {
        int xl, yl, xh, yh;
        def_port->bounds(j, &xl, &yl, &xh, &yh);
        Orientation ori = static_cast<Orientation>(def_pin->orient());
        BoxT<DBU> box = getExternalPinBox(ori, PointT<DBU>(x, y),
                                          BoxT<DBU>(xl, yl, xh, yh));
        double dbu = design->dbu();
        BoxT<double> dbox(box.lx() / dbu, box.ly() / dbu, box.hx() / dbu,
                          box.hy() / dbu);
        const char *layer_name = def_port->layer(j);
        Layer *layer = design->technology()->findLayer(layer_name);
        port->addShape(layer, dbox);
      }
    }
  }
  return 0;
}

static int componentCbk(defrCallbackType_e, defiComponent *def_comp,
                        void *data) {
  Design *design = reinterpret_cast<Design *>(data);
  Libcell *libcell = design->technology()->findLibcell(def_comp->name());
  Instance *inst = design->makeInstance(def_comp->id(), libcell);
  inst->setOrientation(static_cast<Orientation>(def_comp->placementOrient()));
  inst->setLx(def_comp->placementX());
  inst->setLy(def_comp->placementY());
  return 0;
}

static int netCbk(defrCallbackType_e, defiNet *def_net, void *data) {
  Design *design = reinterpret_cast<Design *>(data);
  Net *net = design->makeNet(def_net->name());
  for (int i = 0; i < def_net->numConnections(); i++) {
    if (std::strcmp(def_net->instance(i), "PIN") == 0) { // io pin
      Pin *pin = design->topInstance()->makePin(def_net->pin(i));
      net->connect(pin);
    } else {
      Instance *inst = design->findInstance(def_net->instance(i));
      Pin *pin = inst->makePin(def_net->pin(i));
      net->connect(pin);
    }
  }
  return 0;
}

static int trackCbk(defrCallbackType_e, defiTrack *def_track, void *data) {
  Design *design = reinterpret_cast<Design *>(data);
  // check layer and direction
  LayerDirection dir = std::strcmp(def_track->macro(), "X") == 0
                           ? LayerDirection::Vertical
                           : LayerDirection::Horizontal;
  Layer *layer = design->technology()->findLayer(def_track->layer(0));
  if (layer->direction() == dir) {
    TrackConfig tc;
    tc.count = static_cast<int>(def_track->xNum());
    tc.step = static_cast<DBU>(def_track->xStep());
    tc.start = static_cast<int>(def_track->x());
    tc.layer = layer;
    design->addTrackConfig(tc);
  }
  return 0;
}

static int gcellGridCbk(defrCallbackType_e, defiGcellGrid *def_grid,
                        void *data) {
  Design *design = reinterpret_cast<Design *>(data);
  GridConfig gc;
  gc.direction = std::strcmp(def_grid->macro(), "X") == 0
                     ? LayerDirection::Vertical
                     : LayerDirection::Horizontal;
  gc.count = static_cast<int>(def_grid->xNum());
  gc.step = static_cast<DBU>(def_grid->xStep());
  gc.start = static_cast<DBU>(def_grid->x());
  design->addGridConfig(gc);
  return 0;
}

int readDefImpl(const std::string &def_file_path, Design *design) {
  defrInit();
  defrSetUserData(design);
  defrSetUnitsCbk(dbuCbk);
  defrSetDieAreaCbk(dieAreaCbk);
  defrSetDesignCbk(designCbk);
  defrSetComponentCbk(componentCbk);
  defrSetPinCbk(pinCbk);
  defrSetNetCbk(netCbk);
  defrSetTrackCbk(trackCbk);
  // defrSetGcellGridCbk(gcellGridCbk);
  FILE *def_stream = std::fopen(def_file_path.c_str(), "r");
  if (def_stream == nullptr) {
    return 1;
  }
  int res = defrRead(def_stream, def_file_path.c_str(), design, 1);
  std::fclose(def_stream);
  if (res != 0) {
    LOG_ERROR("Error occurred when parsing LEF file. Exit Code: %i", res);
  }
  return res;
}
} // namespace sca
