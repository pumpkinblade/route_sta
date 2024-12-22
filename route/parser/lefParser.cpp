#include "../object/Technology.hpp"
#include "../util/log.hpp"
#include <cstring>
#include <lefrReader.hpp>

namespace sca {

struct TechWrap {
  Technology *tech;
  Libcell *current_libcell;
};

static int layerCbk(lefrCallbackType_e, lefiLayer *lef_layer, void *data) {
  TechWrap *tw = reinterpret_cast<TechWrap *>(data);
  if (std::strcmp(lef_layer->type(), "ROUTING") == 0) {
    Layer *layer = tw->tech->makeLayer(lef_layer->name());
    layer->setDirection(std::strcmp(lef_layer->direction(), "HORIZONTAL") == 0
                            ? LayerDirection::Horizontal
                            : LayerDirection::Vertical);
    layer->setWireWidth(lef_layer->width());
    layer->setSqRes(lef_layer->resistance());
    layer->setSqCap(lef_layer->capacitance());
    layer->setEdgeCap(lef_layer->edgeCap());
  } else if (std::strcmp(lef_layer->type(), "CUT") == 0) {
    CutLayer *cut_layer = tw->tech->makeCutLayer(lef_layer->name());
    cut_layer->setRes(lef_layer->resistancePerCut());
  }
  return 0;
}

static int macroBeginCbk(lefrCallbackType_e, const char *name, void *data) {
  TechWrap *tw = reinterpret_cast<TechWrap *>(data);
  Libcell *libcell = tw->tech->makeLibcell(name);
  tw->current_libcell = libcell;
  return 0;
}

static int macroCbk(lefrCallbackType_e, lefiMacro *lef_macro, void *data) {
  TechWrap *tw = reinterpret_cast<TechWrap *>(data);
  tw->current_libcell->setWidth(lef_macro->sizeX());
  tw->current_libcell->setHeight(lef_macro->sizeY());
  return 0;
}

static int pinCbk(lefrCallbackType_e, lefiPin *lef_pin, void *data) {
  TechWrap *tw = reinterpret_cast<TechWrap *>(data);
  Port *port = tw->current_libcell->makePort(lef_pin->name());
  port->setDirection(PortDirection::Unknown);
  if (lef_pin->hasDirection()) {
    if (std::strcmp(lef_pin->direction(), "OUTPUT") == 0)
      port->setDirection(PortDirection::Output);
    else if (std::strcmp(lef_pin->direction(), "INPUT") == 0)
      port->setDirection(PortDirection::Input);
    else if (std::strcmp(lef_pin->direction(), "INOUT"))
      port->setDirection(PortDirection::Inout);
  }
  for (int i = 0; i < lef_pin->numPorts(); i++) {
    Layer *layer = nullptr;
    auto geos = lef_pin->port(i);
    for (int j = 0; j < geos->numItems(); j++) {
      switch (geos->itemType(j)) {
      case lefiGeomRectE: {
        lefiGeomRect *rect = geos->getRect(j);
        BoxT<double> box(rect->xl, rect->yl, rect->xh, rect->yh);
        port->addShape(layer, box);
      } break;
      case lefiGeomLayerE: {
        const char *layer_name = geos->getLayer(j);
        layer = tw->tech->findLayer(layer_name);
      } break;
      default:
        break;
      }
    }
  }
  return 0;
}

int readLefImpl(const std::string &lef_file_path, Technology *tech) {
  TechWrap tw;
  tw.tech = tech;
  tw.current_libcell = nullptr;

  lefrInit();
  lefrSetUserData(&tw);
  lefrSetLayerCbk(layerCbk);
  lefrSetMacroBeginCbk(macroBeginCbk);
  lefrSetMacroCbk(macroCbk);
  lefrSetPinCbk(pinCbk);

  FILE *lef_stream = std::fopen(lef_file_path.c_str(), "r");
  if (lef_stream == nullptr) {
    return 1;
  }
  int res = lefrRead(lef_stream, lef_file_path.c_str(), &tw);
  std::fclose(lef_stream);
  if (res) {
    LOG_ERROR("Error occurred when parsing LEF file. Exit Code: %i", res);
  }
  return res;
}

} // namespace sca
