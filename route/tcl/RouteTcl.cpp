#include "RouteTcl.hpp"
#include "../context/Context.hpp"
#include <math.h>
#include <sta/Sta.hh>
#include <tcl.h>

#define CMD_PREFIX "r_"

static int read_lefdef_cmd(ClientData, Tcl_Interp *interp, int objc,
                           Tcl_Obj *CONST objv[]) {
  if (objc != 3) {
    Tcl_WrongNumArgs(interp, 2, objv, "Usage : read_lefdef lef_file def_file");
    return TCL_ERROR;
  }
  const char *lef_file = Tcl_GetStringFromObj(objv[1], nullptr);
  const char *def_file = Tcl_GetStringFromObj(objv[2], nullptr);

  bool ok = Context::context()->readLefDef(lef_file, def_file);
  if (ok)
    return TCL_OK;
  else
    return TCL_ERROR;
}

static int write_slack_cmd(ClientData, Tcl_Interp *interp, int objc,
                           Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, 2, objv,
                     "Usage : " CMD_PREFIX "write_slack slack_file");
    return TCL_ERROR;
  }
  const char *slack_file = Tcl_GetStringFromObj(objv[1], nullptr);

  bool ok = Context::context()->writeSlack(slack_file);
  if (ok)
    return TCL_OK;
  else
    return TCL_ERROR;
}

int Route_Init(Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, CMD_PREFIX "read_lefdef", read_lefdef_cmd,
                       nullptr, nullptr);
  Tcl_CreateObjCommand(interp, CMD_PREFIX "write_slack", write_slack_cmd,
                       nullptr, nullptr);
  return TCL_OK;
}
