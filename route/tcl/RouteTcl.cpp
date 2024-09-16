#include "../app/App.hpp"
#include <math.h>
#include <sta/Sta.hh>
#include <tcl.h>

#define CMD_PREFIX "r_"

sta::Instance *linkNetwork(const char *top_cell_name, bool make_black_boxes,
                           sta::Report *report, sta::NetworkReader *network) {
  return App::app()->linkNetwork(top_cell_name);
}

int read_lefdef_cmd(ClientData clientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *CONST objv[]) {
  if (objc != 3) {
    Tcl_WrongNumArgs(interp, 2, objv, "Usage : read_lefdef lef_file def_file");
    return TCL_ERROR;
  }
  const char *lef_file = Tcl_GetStringFromObj(objv[1], nullptr);
  const char *def_file = Tcl_GetStringFromObj(objv[2], nullptr);

  sta::NetworkReader *network = sta::Sta::sta()->networkReader();
  App::app()->setStaNetwork(network);
  network->setLinkFunc(linkNetwork);
  bool ok = App::app()->readLefDef(lef_file, def_file);
  if (ok)
    return TCL_OK;
  else
    return TCL_ERROR;
}

int write_slack_cmd(ClientData clientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *CONST objv[]) {
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, 2, objv,
                     "Usage : " CMD_PREFIX "write_slack slack_file");
    return TCL_ERROR;
  }
  const char *slack_file = Tcl_GetStringFromObj(objv[1], nullptr);

  bool ok = App::app()->writeSlack(slack_file);
  if (ok)
    return TCL_OK;
  else
    return TCL_ERROR;
}

int Route_Init(Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, CMD_PREFIX "read_lefdef", read_lefdef_cmd,
                       (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
  Tcl_CreateObjCommand(interp, CMD_PREFIX "write_slack", write_slack_cmd,
                       (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
  return TCL_OK;
}
