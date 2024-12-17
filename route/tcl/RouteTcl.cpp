#include "RouteTcl.hpp"
#include "../context/Context.hpp"
#include <math.h>
#include <sta/Sta.hh>
#include <tcl.h>

static int test_cmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *CONST objv[]) {
  if (objc != 1) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::test");
    return TCL_ERROR;
  }
  return Context::ctx()->test() ? TCL_OK : TCL_ERROR;
}

static int read_lef_cmd(ClientData, Tcl_Interp *interp, int objc,
                        Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::read_lef lef_file");
    return TCL_ERROR;
  }
  const char *lef_file = Tcl_GetStringFromObj(objv[1], nullptr);
  return Context::ctx()->readLef(lef_file) ? TCL_OK : TCL_ERROR;
}

static int read_def_cmd(ClientData, Tcl_Interp *interp, int objc,
                        Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::read_def def_file");
    return TCL_ERROR;
  }
  const char *def_file = Tcl_GetStringFromObj(objv[1], nullptr);
  return Context::ctx()->readDef(def_file) ? TCL_OK : TCL_ERROR;
}

static int read_guide_cmd(ClientData, Tcl_Interp *interp, int objc,
                          Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::read_guide guide_file");
    return TCL_ERROR;
  }
  const char *guide_file = Tcl_GetStringFromObj(objv[1], nullptr);
  return Context::ctx()->readGuide(guide_file) ? TCL_OK : TCL_ERROR;
}

static int write_guide_cmd(ClientData, Tcl_Interp *interp, int objc,
                           Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::write_guide guide_file");
    return TCL_ERROR;
  }
  const char *guide_file = Tcl_GetStringFromObj(objv[1], nullptr);
  return Context::ctx()->writeGuide(guide_file) ? TCL_OK : TCL_ERROR;
}

static int write_slack_cmd(ClientData, Tcl_Interp *interp, int objc,
                           Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::write_slack slack_file");
    return TCL_ERROR;
  }
  const char *slack_file = Tcl_GetStringFromObj(objv[1], nullptr);
  return Context::ctx()->writeSlack(slack_file) ? TCL_OK : TCL_ERROR;
}

static int link_design_cmd(ClientData, Tcl_Interp *interp, int objc,
                           Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv,
                     "Usage : sca::link_design design_name");
    return TCL_ERROR;
  }
  const char *design_name = Tcl_GetStringFromObj(objv[1], nullptr);
  return Context::ctx()->linkDesign(design_name) ? TCL_OK : TCL_ERROR;
}

static int run_cugr2_cmd(ClientData, Tcl_Interp *interp, int objc,
                         Tcl_Obj *CONST objv[]) {
  if (objc != 1) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::run_cugr2");
    return TCL_ERROR;
  }
  return Context::ctx()->runCugr2() ? TCL_OK : TCL_ERROR;
}

static int estimate_parasitics_cmd(ClientData, Tcl_Interp *interp, int objc,
                                   Tcl_Obj *CONST objv[]) {
  if (objc != 1) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : sca::estimate_parasitics");
    return TCL_ERROR;
  }
  return Context::ctx()->estimateParasitcs() ? TCL_OK : TCL_ERROR;
}

int Route_Init(Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, "sca::test", test_cmd, nullptr, nullptr);
  Tcl_CreateObjCommand(interp, "sca::read_lef", read_lef_cmd, nullptr, nullptr);
  Tcl_CreateObjCommand(interp, "sca::read_def", read_def_cmd, nullptr, nullptr);
  Tcl_CreateObjCommand(interp, "sca::read_guide", read_guide_cmd, nullptr,
                       nullptr);
  Tcl_CreateObjCommand(interp, "sca::write_guide", write_guide_cmd, nullptr,
                       nullptr);
  Tcl_CreateObjCommand(interp, "sca::write_slack", write_slack_cmd, nullptr,
                       nullptr);
  Tcl_CreateObjCommand(interp, "sca::link_design", link_design_cmd, nullptr,
                       nullptr);
  Tcl_CreateObjCommand(interp, "sca::run_cugr2", run_cugr2_cmd, nullptr,
                       nullptr);
  Tcl_CreateObjCommand(interp, "sca::estimate_parasitics",
                       estimate_parasitics_cmd, nullptr, nullptr);
  return TCL_OK;
}
