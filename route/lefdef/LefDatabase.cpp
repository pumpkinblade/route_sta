#include "LefDatabase.hpp"
#include "../util/log.hpp"
#include <cstring>
#include <lefrReader.hpp>

static int lefLayerCbk(lefrCallbackType_e, lefiLayer *layer,
                       lefiUserData data) {
  LefDatabase *db = reinterpret_cast<LefDatabase *>(data);
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
  LefDatabase *db = reinterpret_cast<LefDatabase *>(data);
  LefVia lef_via;
  lef_via.name = via->name();
  for (int i = 0; i < via->numLayers(); i++)
    lef_via.layers.push_back(via->layerName(i));
  db->vias.push_back(lef_via);
  return 0;
}

static int lefLibCellBeginCbk(lefrCallbackType_e, const char *name,
                              lefiUserData data) {
  LefDatabase *db = reinterpret_cast<LefDatabase *>(data);
  LefLibCell lef_libcell;
  lef_libcell.name = name;
  db->libcells.push_back(lef_libcell);
  return 0;
}

static int lefLibCellCbk(lefrCallbackType_e, lefiMacro *macro,
                         lefiUserData data) {
  LefDatabase *db = reinterpret_cast<LefDatabase *>(data);
  LefLibCell &lef_libcell = db->libcells.back();
  lef_libcell.size_x = static_cast<float>(macro->sizeX());
  lef_libcell.size_y = static_cast<float>(macro->sizeY());
  return 0;
}

static int lefPinCbk(lefrCallbackType_e, lefiPin *pin, lefiUserData data) {
  LefDatabase *db = reinterpret_cast<LefDatabase *>(data);
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

bool LefDatabase::clear() {
  libcells.clear();
  layers.clear();
  cut_layers.clear();
  vias.clear();
  return true;
}

bool LefDatabase::read(const std::string &lef_file) {
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

  return true;
}
