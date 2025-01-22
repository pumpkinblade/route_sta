#include "RouteTcl.hpp"
#include "../context/Context.hpp"
#include <math.h>
#include <sta/Sta.hh>
#include <tcl.h>

namespace sca {

/*irislin*/
static int net_sort_enable_cmd(ClientData, Tcl_Interp *interp, int objc,
                        Tcl_Obj *CONST objv[]) {
  if (objc != 1) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : net_sort_enable");
    return TCL_ERROR;
  }
  sca::Context::ctx()->setNetSort();
  return 0;
}
/*irislin*/

static int read_lef_cmd(ClientData, Tcl_Interp *interp, int objc,
                        Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::read_lef lef_file");
    return TCL_ERROR;
  }
  const char *lef_file = Tcl_GetStringFromObj(objv[1], nullptr);
  return sca::Context::ctx()->readLef(lef_file);
}

static int read_def_cmd(ClientData, Tcl_Interp *interp, int objc,
                        Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::read_def def_file");
    return TCL_ERROR;
  }
  const char *def_file = Tcl_GetStringFromObj(objv[1], nullptr);
  return sca::Context::ctx()->readDef(def_file);
}

static int link_design_cmd(ClientData, Tcl_Interp *interp, int objc,
                           Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv,
                     "Usage : sca::link_design design_name");
    return TCL_ERROR;
  }
  const char *design_name = Tcl_GetStringFromObj(objv[1], nullptr);
  return sca::Context::ctx()->linkDesign(design_name);
}

static int run_cugr2_cmd(ClientData, Tcl_Interp *interp, int objc,
                         Tcl_Obj *CONST objv[]) {
  if (objc != 1) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::run_cugr2");
    return TCL_ERROR;
  }
  return sca::Context::ctx()->runCugr2() ? TCL_OK : TCL_ERROR;
}

static int estimate_parasitics_cmd(ClientData, Tcl_Interp *interp, int objc,
                                   Tcl_Obj *CONST objv[]) {
  if (objc != 1) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::estimate_parasitics");
    return TCL_ERROR;
  }
  return sca::Context::ctx()->estimateParasitcs() ? TCL_OK : TCL_ERROR;
}

static int read_guide_cmd(ClientData, Tcl_Interp *interp, int objc,
                          Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::read_guide guide_file"); 
    return TCL_ERROR;
  }
  const char *guide_file = Tcl_GetStringFromObj(objv[1], nullptr);
  return sca::Context::ctx()->readGuide(guide_file) ? TCL_OK : TCL_ERROR;
}

static int write_guide_cmd(ClientData, Tcl_Interp *interp, int objc,
                           Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::write_guide guide_file"); return TCL_ERROR;
  }
  const char *guide_file = Tcl_GetStringFromObj(objv[1], nullptr);
  return sca::Context::ctx()->writeGuide(guide_file) ? TCL_OK : TCL_ERROR;
}

static int write_slack_cmd(ClientData, Tcl_Interp *interp, int objc,
                           Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::write_slack slack_file"); return TCL_ERROR;
  }
  const char *slack_file = Tcl_GetStringFromObj(objv[1], nullptr);
  return sca::Context::ctx()->writeSlack(slack_file) ? TCL_OK : TCL_ERROR;
}

static int set_layer_rc_cmd(ClientData, Tcl_Interp *interp, int objc,
                          Tcl_Obj *CONST objv[]) {
  if (objc != 7) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : set_layer_rc_cmd"); 
    return TCL_ERROR;
  }
  // get layer
  const char *layer = Tcl_GetString(objv[2]);
  // get resistance
  double resistance = atof(Tcl_GetString(objv[4]));
  // get capacitance
  double capacitance = atof(Tcl_GetString(objv[6]));

  return sca::Context::ctx()->setLayerRc(layer, resistance, capacitance) ? TCL_OK : TCL_ERROR;
}

static int set_wire_rc_cmd(ClientData, Tcl_Interp *interp, int objc,
                          Tcl_Obj *CONST objv[]) {
  if (objc != 4) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : set_wire_rc_cmd"); 
    return TCL_ERROR;
  }

  return TCL_OK;
}

int Route_Init(Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, "net_sort_enable", net_sort_enable_cmd, nullptr, nullptr);
  Tcl_CreateObjCommand(interp, "sca::read_lef", read_lef_cmd, nullptr, nullptr);
  Tcl_CreateObjCommand(interp, "sca::read_def", read_def_cmd, nullptr, nullptr);
  Tcl_CreateObjCommand(interp, "sca::link_design", link_design_cmd, nullptr,
                       nullptr);

  Tcl_CreateObjCommand(interp, "sca::run_cugr2", run_cugr2_cmd, nullptr,
                       nullptr);
  Tcl_CreateObjCommand(interp, "sca::estimate_parasitics",
                       estimate_parasitics_cmd, nullptr, nullptr);

  Tcl_CreateObjCommand(interp, "sca::read_guide", read_guide_cmd, nullptr,
                       nullptr);
  Tcl_CreateObjCommand(interp, "sca::write_guide", write_guide_cmd, nullptr,
                       nullptr);
  Tcl_CreateObjCommand(interp, "sca::write_slack", write_slack_cmd, nullptr,
                       nullptr);
  Tcl_CreateObjCommand(interp, "sca::link_design", link_design_cmd, nullptr,
                       nullptr);
  Tcl_CreateObjCommand(interp, "set_layer_rc", set_layer_rc_cmd, nullptr,
                       nullptr);
  Tcl_CreateObjCommand(interp, "set_wire_rc", set_wire_rc_cmd, nullptr,
                       nullptr);
  return TCL_OK;
}

}
