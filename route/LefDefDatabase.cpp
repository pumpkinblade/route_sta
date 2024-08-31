#include "LefDefDatabase.hpp"
#include "utils.hpp"
#include <cstring>
#include <defrReader.hpp>
#include <lefrReader.hpp>

static int lefMacroBeginCbk(lefrCallbackType_e, const char *name,
                            lefiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  db->currentLibcell = LefLibcell(name);
  return 0;
}

static int lefMacroEndCbk(lefrCallbackType_e, const char *name,
                          lefiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  db->libcellNameMap.emplace(std::string(db->currentLibcell.name),
                             db->currentLibcell);
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
    } else if (std::strcmp(pin->direction(), "INOUT") == 0) {
      dir = DIRECTION_INOUT;
    } else {
      LOG_WARN("could recognize the direction of `%s.%s` ",
               db->currentLibcell.name.c_str(), pin->name());
    }
  } else {
    LOG_WARN("`%s.%s` does not have direction", db->currentLibcell.name.c_str(),
             pin->name());
  }
  db->currentLibcell.libpinNameMap.emplace(std::string(pin->name()),
                                           LefLibpin(pin->name()));
  return 0;
}

static int defComponentCbk(defrCallbackType_e, defiComponent *comp,
                           defiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  db->cellNameMap.emplace(std::string(comp->id()),
                          DefCell(comp->name(), comp->macroName()));
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
    } else if (std::strcmp(pin->direction(), "INOUT") == 0) {
      dir = DIRECTION_INOUT;
    } else {
      LOG_WARN("could recognize the direction of `PIN.%s` ", pin->pinName());
    }
  } else {
    LOG_WARN("`PIN.%s` does not have direction", pin->pinName());
  }
  db->ioPinNameMap.emplace(std::string(pin->pinName()),
                           DefIOPin(pin->pinName(), dir));
  return 0;
}

static int defNetCbk(defrCallbackType_e, defiNet *net, defiUserData data) {
  LefDefDatabase *db = reinterpret_cast<LefDefDatabase *>(data);
  DefNet iNet(net->name());
  for (int i = 0; i < net->numConnections(); i++) {
    if (std::strcmp(net->instance(i), "PIN") == 0) { // io pin
      iNet.ioPins.emplace_back(std::string(net->pin(i)));
    } else {
      iNet.internalPins.emplace_back(std::string(net->instance(i)),
                                     std::string(net->pin(i)));
    }
  }
  db->netNameMap.emplace(std::string(net->name()), iNet);
  return 0;
}

void LefDefDatabase::readLef(const char *file) {
  lefrInit();
  lefrSetUserData(this);
  lefrSetMacroBeginCbk(lefMacroBeginCbk);
  lefrSetMacroEndCbk(lefMacroEndCbk);
  lefrSetPinCbk(lefPinCbk);

  FILE *stream = std::fopen(file, "r");
  ASSERT(stream, "can't open %s", file);
  int res = lefrRead(stream, file, this);
  std::fclose(stream);
  ASSERT(res == 0, "lefapi exit with %d", res);
}

void LefDefDatabase::readDef(const char *file) {
  defrInit();
  defrSetUserData(this);
  defrSetComponentCbk(defComponentCbk);
  defrSetPinCbk(defPinCbk);
  defrSetNetCbk(defNetCbk);

  FILE *stream = std::fopen(file, "r");
  ASSERT(stream, "can't open %s", file);
  int res = defrRead(stream, file, this, 1);
  std::fclose(stream);
  ASSERT(res == 0, "lefapi exit with %d", res);
}
