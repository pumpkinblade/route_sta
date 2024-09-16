#include "LefDefDatabase.hpp"
#include <cstring>
#include <defrReader.hpp>
#include <lefrReader.hpp>
#include <sta/Report.hh>

static int defDesignCbk(defrCallbackType_e, const char *name,
                        defiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  db->def_design_name = name;
  return 0;
}

static int defComponentCbk(defrCallbackType_e, defiComponent *comp,
                           defiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  db->def_cells.emplace_back(comp->id(), comp->name());
  return 0;
}

static int defPinCbk(defrCallbackType_e, defiPin *pin, defiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  int dir = DIRECTION_INOUT;
  if (pin->hasDirection()) {
    if (std::strcmp(pin->direction(), "OUTPUT") == 0) {
      dir = DIRECTION_OUTPUT;
    } else if (std::strcmp(pin->direction(), "INPUT") == 0) {
      dir = DIRECTION_INPUT;
    } else {
      dir = DIRECTION_INOUT;
    }
  }
  db->def_io_pins.emplace_back(pin->pinName(), dir);
  return 0;
}

static int defNetCbk(defrCallbackType_e, defiNet *net, defiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  DefNet n(net->name());
  for (int i = 0; i < net->numConnections(); i++) {
    if (std::strcmp(net->instance(i), "PIN") == 0) { // io pin
      n.io_pins.emplace_back(std::string(net->pin(i)));
    } else {
      n.internal_pins.emplace_back(std::string(net->instance(i)),
                                   std::string(net->pin(i)));
    }
  }
  for (int i = 0; i < net->numWires() && (db->def_nets.size() == 0); i++) {
    defiWire *wire = net->wire(i);
    for (int j = 0; j < wire->numPaths(); j++) {
      defiPath *path = wire->path(j);
      path->initTraverse();
      for (int e; (e = path->next()) != DEFIPATH_DONE;) {
        switch (e) {
        case DEFIPATH_LAYER:
          break;
        case DEFIPATH_VIA:
          break;
        case DEFIPATH_POINT: {
          int x, y;
          path->getPoint(&x, &y);
        } break;
        default:
          break;
        }
      }
    }
  }
  db->def_nets.push_back(n);
  return 0;
}

std::shared_ptr<LefDefDatabase>
LefDefDatabase::read(const std::string &lef_file, const std::string &def_file) {
  auto db = std::make_shared<LefDefDatabase>();

  // currently we dont need to read lef_file
  defrInit();
  defrSetAddPathToNet();
  defrSetUserData(db.get());
  defrSetDesignCbk(defDesignCbk);
  defrSetComponentCbk(defComponentCbk);
  defrSetPinCbk(defPinCbk);
  defrSetNetCbk(defNetCbk);
  FILE *stream = std::fopen(def_file.c_str(), "r");
  if (stream == nullptr) {
    return nullptr;
  }
  int res = defrRead(stream, def_file.c_str(), db.get(), 1);
  std::fclose(stream);
  if (res != 0) {
    return nullptr;
  }

  return db;
}
