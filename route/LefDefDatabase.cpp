#include "LefDefDatabase.hpp"
#include <cstring>
#include <defrReader.hpp>
#include <lefrReader.hpp>

static int lefMacroBeginCbk(lefrCallbackType_e, const char *name,
                            lefiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  db->current_lef_libcell = LefLibcell(name);
  return 0;
}

static int lefMacroEndCbk(lefrCallbackType_e, const char *name,
                          lefiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  db->lef_libcells.push_back(db->current_lef_libcell);
  return 0;
}

static int lefPinCbk(lefrCallbackType_e, lefiPin *pin, lefiUserData data) {
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
  db->current_lef_libcell.libpins.emplace_back(pin->name());
  return 0;
}

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
  db->def_nets.push_back(n);
  return 0;
}

void LefDefDatabase::readLef(const char *file, sta::Report *report) {
  lefrInit();
  lefrSetUserData(this);
  lefrSetMacroBeginCbk(lefMacroBeginCbk);
  lefrSetMacroEndCbk(lefMacroEndCbk);
  lefrSetPinCbk(lefPinCbk);

  FILE *stream = std::fopen(file, "r");
  if (stream == nullptr) {
    report->error(70000, "can not open %s", file);
    return;
  }
  int res = lefrRead(stream, file, this);
  std::fclose(stream);
  if (res != 0) {
    report->error(70000, "lefapi exit with %d", res);
    return;
  }
}

void LefDefDatabase::readDef(const char *file, sta::Report *report) {
  defrInit();
  defrSetUserData(this);
  defrSetDesignCbk(defDesignCbk);
  defrSetComponentCbk(defComponentCbk);
  defrSetPinCbk(defPinCbk);
  defrSetNetCbk(defNetCbk);

  FILE *stream = std::fopen(file, "r");
  if (stream == nullptr) {
    report->error(70000, "can not open %s", file);
    return;
  }
  int res = defrRead(stream, file, this, 1);
  std::fclose(stream);
  if (res != 0) {
    report->error(70000, "defapi exit with %d", res);
    return;
  }
}
