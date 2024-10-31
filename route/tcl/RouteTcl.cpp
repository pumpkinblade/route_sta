#include "RouteTcl.hpp"
#include "../context/Context.hpp"
#include <math.h>
#include <sta/Sta.hh>
#include <tcl.h>

static int read_lef_cmd(ClientData, Tcl_Interp *interp, int objc,
                        Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : r_read_lef lef_file");
    return TCL_ERROR;
  }
  const char *lef_file = Tcl_GetStringFromObj(objv[1], nullptr);
  bool ok = Context::context()->readLef(lef_file);
  return ok ? TCL_OK : TCL_ERROR;
}

static int read_def_cmd(ClientData, Tcl_Interp *interp, int objc,
                        Tcl_Obj *CONST objv[]) {
  if (objc != 3) {
    Tcl_WrongNumArgs(interp, objc, objv,
                     "Usage : r_read_def def_file use_routing");
    return TCL_ERROR;
  }
  const char *def_file = Tcl_GetStringFromObj(objv[1], nullptr);
  int use_routing = 0;
  Tcl_GetIntFromObj(interp, objv[2], &use_routing);
  bool ok = Context::context()->readDef(def_file, !!use_routing);
  return ok ? TCL_OK : TCL_ERROR;
}

static int write_slack_cmd(ClientData, Tcl_Interp *interp, int objc,
                           Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : r_write_slack slack_file");
    return TCL_ERROR;
  }
  const char *slack_file = Tcl_GetStringFromObj(objv[1], nullptr);

  bool ok = Context::context()->writeSlack(slack_file);
  if (ok)
    return TCL_OK;
  else
    return TCL_ERROR;
}

static int run_cugr2_cmd(ClientData, Tcl_Interp *interp, int objc,
                         Tcl_Obj *CONST objv[]) {
  if (objc != 1) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : r_run_cugr2");
    return TCL_ERROR;
  }
  bool ok = Context::context()->runCugr2();
  if (ok)
    return TCL_OK;
  else
    return TCL_ERROR;
}

static int write_guide_cmd(ClientData, Tcl_Interp *interp, int objc,
                           Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, objc, objv, "Usage : r_write_guide guide_file");
    return TCL_ERROR;
  }
  const char *guide_file = Tcl_GetStringFromObj(objv[1], nullptr);
  bool ok = Context::context()->writeGuide(guide_file);
  if (ok)
    return TCL_OK;
  else
    return TCL_ERROR;
}

int Route_Init(Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, "r_read_lef", read_lef_cmd, nullptr, nullptr);
  Tcl_CreateObjCommand(interp, "r_read_def", read_def_cmd, nullptr, nullptr);
  Tcl_CreateObjCommand(interp, "r_write_slack", write_slack_cmd, nullptr,
                       nullptr);
  Tcl_CreateObjCommand(interp, "r_run_cugr2", run_cugr2_cmd, nullptr, nullptr);
  Tcl_CreateObjCommand(interp, "r_write_guide", write_guide_cmd, nullptr,
                       nullptr);
  return TCL_OK;
}
